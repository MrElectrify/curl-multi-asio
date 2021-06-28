#ifndef CURLMULTIASIO_DETAIL_ERRORCATEGORY_H_
#define CURLMULTIASIO_DETAIL_ERRORCATEGORY_H_

/// @file
/// Error Category
/// 6/28/21

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>

namespace cma
{
	namespace Detail
	{
#ifdef CMA_USE_BOOST
		using error_category = boost::system::error_category;
#else
		using error_category = std::error_category;
#endif
		/// @brief A category for CURLcodes
		struct CURLcodeErrCategory : error_category
		{
			const char* name() const noexcept override
			{
				return "CURLcode";
			}
			std::string message(int ev) const override
			{
				return curl_easy_strerror(static_cast<CURLcode>(ev));
			}
			static const CURLcodeErrCategory& Instance() noexcept
			{
				static const CURLcodeErrCategory s_instance;
				return s_instance;
			}
		};
		/// @brief A category for CURLMcodes
		struct CURLMcodeErrCategory : error_category
		{
			const char* name() const noexcept override
			{
				return "CURLMcode";
			}
			std::string message(int ev) const override
			{
				return curl_multi_strerror(static_cast<CURLMcode>(ev));
			}
			static const CURLMcodeErrCategory& Instance() noexcept
			{
				static const CURLMcodeErrCategory s_instance;
				return s_instance;
			}
		};
	}
}

#endif