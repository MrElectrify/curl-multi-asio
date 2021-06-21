#ifndef CURLMULTIASIO_EASY_H_
#define CURLMULTIASIO_EASY_H_

/// @file
/// cURL Easy Handle
/// 6/21/21 15:26

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>

// STL includes
#include <memory>

namespace cma
{
	class Multi;

	/// @brief Easy is a wrapper around an easy CURL handle
	class Easy
	{
	public:
		/// @brief Creates an easy CURL handle by curl_easy_init
		Easy() noexcept;
		/// @brief Destroys the easy CURL handle by curl_easy_cleanup
		~Easy() = default;

		/// @return The native handle
		inline CURL* GetNativeHandle() const noexcept { return m_nativeHandle.get(); }
	private:
		friend Multi;

		std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> m_nativeHandle;
	};
}

#endif