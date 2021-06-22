#ifndef CURLMULTIASIO_MULTI_H_
#define CURLMULTIASIO_MULTI_H_

/// @file
/// cURL multi handle
/// 6/21/21 11:45

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>
#include <curl-multi-asio/Detail/Lifetime.h>

// STL includes
#include <atomic>
#include <unordered_set>

namespace cma
{
	class Easy;

	/// @brief Multi is a multi handle, which tracks and executes
	/// all curl_multi calls
	class Multi
	{
	public:
		/// @brief Creates the handle and if necessary, initializes cURL.
		/// If CMA_MANAGE_CURL is specified when the library is built,
		/// cURL's lifetime is managed by the total instances of Multi,
		/// and curl_global_init will be called by the library.
		/// If you would rather manage the lifetime yourself, an interface
		/// is provided in cma::Detail::Lifetime.
		/// @param executor The executor type
		Multi(asio::any_io_executor executor) noexcept;
		/// @brief Creates the handle and if necessary, initializes cURL.
		/// If CMA_MANAGE_CURL is specified when the library is built,
		/// cURL's lifetime is managed by the total instances of Multi,
		/// and curl_global_init will be called by the library.
		/// If you would rather manage the lifetime yourself, an interface
		/// is provided in cma::Detail::Lifetime.
		/// @tparam ExecutionContext The execution context type
		/// @param ctx The execution context
		template<typename ExecutionContext>
		Multi(ExecutionContext& ctx) noexcept
			: Multi(ctx.get_executor()) {}
		/// @brief Cancels any outstanding operations, and destroys handles.
		/// If CMA_MANAGE_CURL is specified when the library is built and
		/// this is the only instance of Multi, curl_global_cleanup will be called
		~Multi() = default;
		Multi(const Multi&) = delete;
		Multi& operator=(const Multi&) = delete;
		/// @brief The other multi instance ends up in an invalid state
		Multi(Multi&& other) = default;
		/// @brief The other multi instance ends up in an invalid state
		/// @return This multi handle
		Multi& operator=(Multi&& other) = default;

		/// @return The native handle
		inline CURLM* GetNativeHandle() const noexcept { return m_nativeHandle.get(); }
	private:
		asio::any_io_executor m_executor;
#ifdef CMA_MANAGE_CURL
		Detail::Lifetime s_lifetime;
#endif
		std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)> m_nativeHandle;
		std::unordered_set<CURL*> m_easyNativeHandles;
	};
}

#endif