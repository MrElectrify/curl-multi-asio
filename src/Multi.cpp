#include <curl-multi-asio/Multi.h>

#include <chrono>
#include <functional>

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

size_t Multi::Cancel(cma::error_code& ec, CURLMcode error) noexcept
{
	// if there are no operations, there is no need for a timer.
	m_timer.cancel(ec);
	for (auto& handler : m_easyHandlerMap)
	{
		// post each completion in case the handler tries to cancel itself
		asio::post(m_executor, [handler = std::move(handler.second)]
			{
				handler->Complete(asio::error::operation_aborted);
			});
	}
	m_easyHandlerMap.clear();
	return m_easyHandlerMap.size();
}

bool Multi::Cancel(const Easy& easy, CURLMcode error) noexcept
{
	// ensure that the easy handle is already being handled
	auto handlerIt = m_easyHandlerMap.find(easy.GetNativeHandle());
	if (handlerIt == m_easyHandlerMap.end())
		return false;
	// post each completion in case the handler tries to cancel itself
	asio::post(m_executor, [handler = std::move(handlerIt->second)]
		{
			handler->Complete(asio::error::operation_aborted);
		});
	// delete the handler
	m_easyHandlerMap.erase(handlerIt);
	// if there are no more operations, there is no need for a timer
	if (m_easyHandlerMap.empty() == true)
	{
		cma::error_code ignored;
		m_timer.cancel(ignored);
	}
	return true;
}

int Multi::CloseSocketCb(Multi* userp, curl_socket_t item) noexcept
{
	auto socketIt = userp->m_easySocketMap.find(item);
	cma::error_code ec;
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
	Multi* userp, int* socketp) noexcept
{
	// delete our last action
	if (what == CURL_POLL_REMOVE)
	{
		delete socketp;
		return 0;
	}
	if (socketp == nullptr)
	{
		// allocate our last action
		socketp = new int(0);
		curl_multi_assign(userp->GetNativeHandle(), s, socketp);
	}
	// find the socket
	auto socketIt = userp->m_easySocketMap.find(s);
	if (socketIt == userp->m_easySocketMap.end())
		return 0;
	int last = *socketp;
	*socketp = what;
	// do what cURL wants, only if it changed
	if ((what == CURL_POLL_IN || what == CURL_POLL_INOUT) &&
		(last != CURL_POLL_IN && last != CURL_POLL_INOUT))
		socketIt->second.async_read_some(asio::null_buffers(),
			asio::bind_executor(userp->m_strand, std::bind(&Multi::EventCallback,
				userp, std::placeholders::_1, s, CURL_POLL_IN, socketp)));
	if ((what == CURL_POLL_OUT || what == CURL_POLL_INOUT) &&
		(last != CURL_POLL_OUT && last != CURL_POLL_INOUT))
		socketIt->second.async_write_some(asio::null_buffers(),
			asio::bind_executor(userp->m_strand, std::bind(&Multi::EventCallback,
				userp, std::placeholders::_1, s, CURL_POLL_OUT, socketp)));
	return 0;
}

int Multi::TimerCallback(CURLM* multi, long timeout_ms, Multi* userp) noexcept
{
	if (timeout_ms == -1)
	{
		// delete the timer, per cURL docs
		cma::error_code ignored;
		userp->m_timer.cancel(ignored);
	}
	else
	{
		// start the timer
		userp->m_timer.expires_from_now(std::chrono::milliseconds(timeout_ms));
		userp->m_timer.async_wait(asio::bind_executor(
			userp->m_strand, [userp] (const cma::error_code& ec)
			{
				if (ec)
					return;
				int still_running = 0;
				cma::error_code ignored;
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
		auto handlerIt = m_easyHandlerMap.find(msg->easy_handle);
		if (handlerIt == m_easyHandlerMap.end())
			continue;
		// move the handler out and erase it in case 
		// it tries to cancel itself
		auto handler = std::move(handlerIt->second);
		// remove it from the handler map. the deleter
		// will also remove the handle from multi
		m_easyHandlerMap.erase(handlerIt);
		// a descriptor is done. call its handler
		handler->Complete(msg->data.result);
	}
}

void Multi::EventCallback(const cma::error_code& ec, curl_socket_t s,
	int what, int* last) noexcept
{
	// make sure it's a socket that hasn't bene closed
	if (m_easySocketMap.find(s) == m_easySocketMap.end())
		return;
	// if the action changed, let curl handle it
	if (what != *last && *last != CURL_POLL_INOUT)
		return;
	int still_running = 0;
	cma::error_code ignored;
	if (ec)
		what = CURL_CSELECT_ERR;
	if (auto err = curl_multi_socket_action(GetNativeHandle(), s,
		what, &still_running); err != CURLMcode::CURLM_OK)
	{
		Cancel(ignored, err);
		return;
	}
	// check for completed transfers
	CheckTransfers();
	// we have no reason to continue if there are none running
	if (still_running == 0)
		m_timer.cancel(ignored);
	// if the socket still exists and the mission remainds
	// unchanged, keep it up
	auto socketIt = m_easySocketMap.find(s);	
	if (!ec && socketIt != m_easySocketMap.end() &&
		(what == *last || *last == CURL_POLL_INOUT))
	{
		if (what == CURL_POLL_IN)
			socketIt->second.async_read_some(asio::null_buffers(),
				asio::bind_executor(m_strand, std::bind(&Multi::EventCallback,
					this, std::placeholders::_1, s, what, last)));
		else if (what == CURL_POLL_OUT)
			socketIt->second.async_write_some(asio::null_buffers(),
				asio::bind_executor(m_strand, std::bind(&Multi::EventCallback,
					this, std::placeholders::_1, s, what, last)));
	}
}