#ifndef CURLMULTIASIO_EASY_H_
#define CURLMULTIASIO_EASY_H_

/// @file
/// cURL Easy Handle
/// 6/21/21 15:26

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>
#include <curl-multi-asio/Detail/Lifetime.h>

// STL includes
#include <memory>

namespace cma
{
	class Multi;

	/// @brief Easy is a wrapper around an easy CURL handle
	class Easy
	{
	public:
		/// @brief Creates an easy CURL handle by curl_easy_init.
		Easy() noexcept;
		/// @brief Destroys the easy CURL handle by curl_easy_cleanup
		~Easy() = default;
		/// @brief Duplicates the easy handle
		/// @param other The handle to duplicate from
		Easy(const Easy& other) noexcept;
		/// @brief Diplicates the easy handle
		/// @param other The handle to duplicate from
		/// @return This handle
		Easy& operator=(const Easy& other) noexcept;
		/// @brief Moves the easy handle
		/// @param other The other easy handle
		Easy(Easy&& other) = default;
		/// @brief Moves the easy handle
		/// @param other The other easy handle
		/// @return This handle
		Easy& operator=(Easy&& other) = default;

		/// @return The native handle
		inline CURL* GetNativeHandle() const noexcept { return m_nativeHandle.get(); }
	private:
#ifdef CMA_MANAGE_CURL
		Detail::Lifetime m_lifeTime;
#endif
		std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> m_nativeHandle;
	};
}

#endif