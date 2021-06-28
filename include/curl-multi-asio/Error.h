#ifndef CURLMULTIASIO_ERROR_H_
#define CURLMULTIASIO_ERROR_H_

/// @file
/// cURL Error
/// 6/21/21 23:10

// curl-multi-asio includes
#include <curl-multi-asio/Detail/ErrorCategory.h>

// STL includes
#include <string_view>

// register the error codes
#ifdef CMA_USE_BOOST
namespace boost
{
	namespace system
	{
#else
namespace std
{
#endif
	template<>
	struct is_error_code_enum<CURLcode> : std::true_type {};
	template<>
	struct is_error_code_enum<CURLMcode> : std::true_type {};
#ifdef CMA_USE_BOOST
	}
}
#else
}
#endif // 

namespace cma
{
#ifdef CMA_USE_BOOST
	using error_code = boost::system::error_code;
#else
	using error_code = asio::error_code;
#endif
}

/// @brief Makes an error code from a CURLcode
/// @param code The CURLcode
/// @return The error code
inline cma::error_code make_error_code(CURLcode code) noexcept
{
	if (code == CURLcode::CURLE_OK)
		return {};
	return { static_cast<int>(code), cma::Detail::CURLcodeErrCategory::Instance() };
}
/// @brief Makes an error code from a CURLMcode
/// @param code The CURLcode
/// @return The error code
inline cma::error_code make_error_code(CURLMcode code) noexcept
{
	if (code == CURLMcode::CURLM_OK)
		return {};
	return { static_cast<int>(code), cma::Detail::CURLMcodeErrCategory::Instance() };
}

#endif