#ifndef CURLMULTIASIO_HANDLE_H_
#define CURLMULTIASIO_HANDLE_H_

/// @file
/// cURL multi handle
/// 6/21/21 11:45

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>
#include <curl-multi-asio/Detail/Lifetime.h>

// STL includes
#include <atomic>
#include <memory>

namespace cma
{
	/// @brief Handle is a multi handle, which tracks and executes
	/// all curl_multi calls
	class Handle
	{
	public:
		/// @brief Creates the handle and if necessary, initializes cURL.
		/// If CMA_MANAGE_CURL is specified when the library is built,
		/// cURL's lifetime is managed by the total instances of Handle,
		/// and curl_global_init will be called by the library.
		/// If you would rather manage the lifetime yourself, an interface
		/// is provided in cma::Detail::Lifetime.
		/// @param executor The executor type
		Handle(asio::any_io_executor executor) noexcept;
		/// @brief Creates the handle and if necessary, initializes cURL.
		/// If CMA_MANAGE_CURL is specified when the library is built,
		/// cURL's lifetime is managed by the total instances of Handle,
		/// and curl_global_init will be called by the library.
		/// If you would rather manage the lifetime yourself, an interface
		/// is provided in cma::Detail::Lifetime.
		/// @tparam ExecutionContext The execution context type
		/// @param ctx The execution context
		template<typename ExecutionContext>
		Handle(ExecutionContext& ctx) noexcept
			: Handle(ctx.get_executor()) {}
		/// @brief Cancels any outstanding operations, and destroys handles.
		/// If CMA_MANAGE_CURL is specified when the library is built and
		/// this is the only existing Handle, curl_global_cleanup will be called
		~Handle() noexcept;
		/// @return The native handle
		inline CURLM* GetNativeHandle() const noexcept { return m_nativeHandle.get(); }
	private:
		asio::any_io_executor m_executor;
#ifdef CMA_MANAGE_CURL
		static std::atomic_size_t s_instanceCount;
		static std::unique_ptr<Detail::Lifetime> s_lifetime;
#endif
		std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)> m_nativeHandle;
	};
}

#endif