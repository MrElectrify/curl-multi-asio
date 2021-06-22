#include <curl-multi-asio/Multi.h>

#include <chrono>

using cma::Multi;

Multi::Multi(const asio::any_io_executor& executor) noexcept
	: m_executor(executor), m_timer(executor), m_strand(executor),
	m_nativeHandle(curl_multi_init(), curl_multi_cleanup)
{
	// set the timer function and data
	SetOption(CURLMoption::CURLMOPT_TIMERFUNCTION, &Multi::TimerCallback);
	SetOption(CURLMoption::CURLMOPT_TIMERDATA, this);
	// also set the socket function and data
	SetOption(CURLMoption::CURLMOPT_SOCKETFUNCTION, &Multi::SocketCallback);
	SetOption(CURLMoption::CURLMOPT_SOCKETDATA, this);
}

size_t Multi::Cancel(asio::error_code& ec, CURLMcode error) noexcept
{
	// if there are no operations, there is no need for a timer
	m_timer.cancel(ec);
	// call each handler with asio::error::operation_aborted
	const size_t numHandlers = m_easyHandlerMap.size();
	for (const auto& handler : m_easyHandlerMap)
		handler.second->Complete(asio::error::operation_aborted, error);
	// clear the map and free handler data
	m_easyHandlerMap.clear();
	return numHandlers;
}

int Multi::CloseSocketCb(Multi* userp, curl_socket_t item) noexcept
{
	auto socketIt = userp->m_easySocketMap.find(item);
	asio::error_code ec;
	// move the socket out so it doesn't get stuck if the close fails.
	// delete the old iterator
	auto socket = std::move(socketIt->second);
	userp->m_easySocketMap.erase(socketIt);
	socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
	// close the socket
	return (socket.close(ec)) ? 1 : 0;
}

curl_socket_t Multi::OpenSocketCb(Multi* userp, curlsocktype purpose,
	curl_sockaddr* address) noexcept
{
	if (purpose != curlsocktype::CURLSOCKTYPE_IPCXN ||
		address->family != AF_INET)
		return CURL_SOCKET_BAD;
	// open the socket
	auto sock = socket(address->family, address->socktype, address->protocol);
	if (sock == -1)
		return CURL_SOCKET_BAD;
	// create and save the socket
	userp->m_easySocketMap.emplace(sock, asio::ip::tcp::socket(
		userp->m_executor, asio::ip::tcp::v4(), sock));
	return sock;
}

int Multi::SocketCallback(CURL* easy, curl_socket_t s, int what,
	Multi* userp, void* socketp) noexcept
{
	// if it is remove, don't worry about it right now.
	if (what == CURL_POLL_REMOVE)
		return 0;
	// find the socket
	auto socketIt = userp->m_easySocketMap.find(s);
	if (socketIt == userp->m_easySocketMap.end())
		return 0;
	// do what cURL wants
	if (what == CURL_POLL_IN || what == CURL_POLL_INOUT)
		socketIt->second.async_read_some(asio::null_buffers(),
			asio::bind_executor(userp->m_strand, std::bind(&Multi::EventCallback,
				userp, std::placeholders::_1, std::placeholders::_2, s, what)));
	if (what == CURL_POLL_OUT || what == CURL_POLL_INOUT)
		socketIt->second.async_write_some(asio::null_buffers(),
			asio::bind_executor(userp->m_strand, std::bind(&Multi::EventCallback,
				userp, std::placeholders::_1, std::placeholders::_2, s, what)));
	return 0;
}

int Multi::TimerCallback(CURLM* multi, long timeout_ms, Multi* userp) noexcept
{
	if (timeout_ms == -1)
	{
		// delete the timer, per cURL docs
		asio::error_code ignored;
		userp->m_timer.cancel(ignored);
	}
	else
	{
		// start the timer
		userp->m_timer.expires_from_now(std::chrono::milliseconds(timeout_ms));
		userp->m_timer.async_wait(asio::bind_executor(
			userp->m_strand, [userp] (const asio::error_code& ec)
			{
				if (ec)
					return;
				int still_running = 0;
				asio::error_code ignored;
				if (auto err = curl_multi_socket_action(userp->GetNativeHandle(),
					CURL_SOCKET_TIMEOUT, 0, &still_running); err != CURLMcode::CURLM_OK)
				{
					userp->Cancel(ignored, err);
					return;
				}
				// we may have completed some transfers here. check
				userp->CheckTransfers();

			}));
	}
	return 0;
}

void Multi::CheckTransfers() noexcept
{
	int msgs_in_queue = 0;
	while (CURLMsg* msg = curl_multi_info_read(GetNativeHandle(), &msgs_in_queue))
	{
		// only pay attention to finished transfers
		if (msg->msg != CURLMSG::CURLMSG_DONE)
			continue;
		// a descriptor is done. call its handler
		m_easyHandlerMap.at(msg->easy_handle)->Complete(
			asio::error_code{}, msg->data.result);
		// remove it from the handler map. the deleter
		// will also remove the handle from multi
		m_easyHandlerMap.erase(msg->easy_handle);
	}
}

void Multi::EventCallback(const asio::error_code& ec, size_t, curl_socket_t s,
	int what) noexcept
{
	if (ec)
		return;
	int still_running = 0;
	asio::error_code ignored;
	if (auto err = curl_multi_socket_action(GetNativeHandle(), s,
		what, &still_running); err != CURLMcode::CURLM_OK)
	{
		Cancel(ignored, err);
		return;
	}
	// check for completed transfers
	CheckTransfers();
}