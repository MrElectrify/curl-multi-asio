#include <curl-multi-asio/Multi.h>

using cma::Multi;

Multi::Multi(asio::any_io_executor executor) noexcept
	: m_executor(std::move(executor)), m_nativeHandle(
		curl_multi_init(), curl_multi_cleanup) {}
