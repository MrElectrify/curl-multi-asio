#include <curl-multi-asio/Easy.h>

using cma::Easy;

Easy::Easy() noexcept : 
	m_nativeHandle(curl_easy_init(), curl_easy_cleanup),
	m_headerList(nullptr, curl_slist_free_all) {}

// just duplicate the raw handle
Easy::Easy(const Easy& other) noexcept :
	m_nativeHandle(curl_easy_duphandle(other.GetNativeHandle()), curl_easy_cleanup),
	m_headerList(nullptr, curl_slist_free_all) 
{
	// add each header manually
	for (auto node = other.m_headerList.get(); node != nullptr;
		node = node->next)
		AddHeaderStr(node->data);
}

Easy& Easy::operator=(const Easy& other) noexcept
{
	if (this == &other)
		return *this;
	m_nativeHandle.reset(curl_easy_duphandle(other.GetNativeHandle()));
	return *this;
}

bool Easy::AddHeaderStr(const char* headerStr) noexcept
{
	// add the header to the list
	const auto result = curl_slist_append(m_headerList.get(), headerStr);
	if (result == nullptr)
		return false;
	// release the unique_ptr so we don't destroy it
	m_headerList.release();
	m_headerList.reset(result);
	if (const auto res = SetOption(CURLoption::CURLOPT_HTTPHEADER,
		m_headerList.get()); res)
		return false;
	return true;
}

bool Easy::AddHeader(std::pair<std::string_view, std::string_view> header) noexcept
{
	std::string headerStr(header.first.data(), header.first.size());
	headerStr += ": ";
	headerStr += header.second;
	return AddHeaderStr(headerStr.c_str());
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