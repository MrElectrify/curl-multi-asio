#ifndef CURLMULTIASIO_ERROR_H_
#define CURLMULTIASIO_ERROR_H_

/// @file
/// cURL Error
/// 6/21/21 23:10

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>

// STL includes
#include <string_view>
#include <variant>

namespace cma
{
	/// @brief Error is a wrapper around CURLcode/CURLMcode. It provides
	/// facilities for lazily getting string and value errors
	class Error
	{
	public:
		Error() = default;
		Error(CURLcode code) noexcept : m_code(code) {}
		Error(CURLMcode code) noexcept : m_code(code) {}

		/// @return The contained CURLcode, or OK
		inline CURLcode GetCURLcode() const noexcept { return (m_code.index() == 0) ? std::get<0>(m_code) : CURLE_OK; }
		/// @return The contained CURLMcode, or OK
		inline CURLMcode GetCURLMcode() const noexcept { return (m_code.index() == 1) ? std::get<1>(m_code) : CURLM_OK; }
		/// @return The raw code of the error
		inline auto GetValue() const noexcept { return m_code; }
		/// @return The string representation of the error
		inline std::string_view ToString() const noexcept 
		{ 
			return (m_code.index() == 0) ?
				curl_easy_strerror(std::get<0>(m_code)) :
				curl_multi_strerror(std::get<1>(m_code)); 
		}

		inline bool operator==(CURLcode code) const noexcept { return m_code.index() == 0 && std::get<0>(m_code) == code; }
		inline operator bool() const noexcept 
		{ 
			return (m_code.index() == 0) ? 
				std::get<0>(m_code) != CURLcode::CURLE_OK :
				std::get<1>(m_code) != CURLMcode::CURLM_OK;
		}
	private:
		std::variant<CURLcode, CURLMcode> m_code = CURLcode::CURLE_OK;
	};
}

#endif