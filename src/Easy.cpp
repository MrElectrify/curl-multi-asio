#include <curl-multi-asio/Easy.h>

using cma::Easy;

Easy::Easy() noexcept : 
	m_nativeHandle(curl_easy_init(), curl_easy_cleanup) {}