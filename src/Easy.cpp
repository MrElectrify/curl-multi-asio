#include <curl-multi-asio/Easy.h>

using cma::Easy;

Easy::Easy() noexcept : 
	m_nativeHandle(curl_easy_init(), curl_easy_cleanup) {}

// just duplicate the raw handle
Easy::Easy(const Easy& other) noexcept :
	m_nativeHandle(curl_easy_duphandle(other.GetNativeHandle()), curl_easy_cleanup) {}

Easy& Easy::operator=(const Easy& other) noexcept
{
	if (this == &other)
		return *this;
	m_nativeHandle.reset(curl_easy_duphandle(other.GetNativeHandle()));
	return *this;
}

cma::error_code Easy::SetBuffer(DefaultBuffer) noexcept
{
	return SetOption(CURLoption::CURLOPT_WRITEFUNCTION, nullptr);
}
cma::error_code Easy::SetBuffer(NullBuffer) noexcept
{
	static NullBuffer s_nb;
	if (auto res = SetOption(CURLoption::CURLOPT_WRITEFUNCTION,
		Easy::WriteCb<NullBuffer>); res)
		return res;
	return SetOption(CURLoption::CURLOPT_WRITEDATA, &s_nb);
}