#ifndef CURLMULTIASIO_ERROR_H_
#define CURLMULTIASIO_ERROR_H_

/// @file
/// cURL Error
/// 6/21/21 23:10

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>

// STL includes
#include <string_view>

namespace cma
{
	/// @brief Error is a wrapper around CURLcode. It provides
	/// facilities for lazily getting string and value errors
	class Error
	{
	public:
		Error() = default;
		Error(CURLcode code) noexcept : m_code(code) {}

		/// @return The raw code of the error
		inline CURLcode GetValue() const noexcept { return m_code; }
		/// @return The string representation of the error
		inline std::string_view ToString() const noexcept { return curl_easy_strerror(m_code); }

		inline bool operator==(CURLcode code) const noexcept { return m_code == code; }
		inline operator bool() const noexcept { return m_code != CURLcode::CURLE_OK; }
	private:
		CURLcode m_code = CURLcode::CURLE_OK;
	};
}

#endif