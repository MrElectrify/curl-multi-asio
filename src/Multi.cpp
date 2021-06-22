#include <curl-multi-asio/Multi.h>

using cma::Multi;

Multi::Multi(const asio::any_io_executor& executor) noexcept
	: m_executor(executor), m_nativeHandle(
		curl_multi_init(), curl_multi_cleanup),
	m_timer(executor) {}

size_t Multi::Cancel(asio::error_code& ec) noexcept
{
	// if there are no operations, there is no need for a timer
	m_timer.cancel(ec);
	// call each handler with asio::error::operation_aborted
	const size_t numHandlers = m_easyHandlerMap.size();
	// simply clear the maps, the destructors will untrack the handles
	m_nativeHandlerMap.clear();
	m_easyHandlerMap.clear();
	return numHandlers;
}