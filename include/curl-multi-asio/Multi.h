#ifndef CURLMULTIASIO_MULTI_H_
#define CURLMULTIASIO_MULTI_H_

/// @file
/// cURL multi handle
/// 6/21/21 11:45

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>
#include <curl-multi-asio/Detail/Lifetime.h>
#include <curl-multi-asio/Easy.h>
#include <curl-multi-asio/Error.h>

// STL includes
#include <atomic>
#include <functional>
#include <unordered_map>
#include <utility>

namespace cma
{
	/// @brief Multi is a multi handle, which tracks and executes
	/// all curl_multi calls
	class Multi
	{
	private:
		/// @brief These handlers store all of the handler data including
		/// the raw socket, and the handler itself. They also handle
		/// unregistration
		class PerformHandlerBase
		{
		public:
			PerformHandlerBase(const asio::any_io_executor& executor,
				CURL* easyHandle, CURL* multiHandle) noexcept :
				m_socket(executor), m_easyHandle(easyHandle),
				m_multiHandle(multiHandle) {}
			virtual ~PerformHandlerBase() noexcept { asio::error_code ignored; m_socket.close(ignored); }

				/// @brief Completes the perform, and calls the handler. Must
			/// set handled status
			/// @param ec The error code
			/// @param e The curl error
			virtual void Complete(asio::error_code ec, Error e) noexcept = 0;

			/// @return The underlying easy handle
			inline CURL* GetEasyHandle() const noexcept { return m_easyHandle; }
			/// @return The underlying multi handle
			inline CURLM* GetMultiHandle() const noexcept { return m_multiHandle; }
			/// @return The handler's raw socket
			inline asio::ip::tcp::socket& GetSocket() noexcept { return m_socket; }
			/// @return If the handler was considered handled
			inline bool Handled() const noexcept { return m_handled; }
		protected:
			/// @param handled If the handle was considered handled
			inline void SetHandled(bool handled) noexcept { m_handled = handled; }
		private:
			asio::ip::tcp::socket m_socket;
			CURL* m_easyHandle;
			CURLM* m_multiHandle;
			bool m_handled = false;
		};
		template<typename Handler>
		class PerformHandler : public PerformHandlerBase
		{
		public:
			PerformHandler(const asio::any_io_executor& executor, CURL* easyHandle,
				CURLM* multiHandle, Handler& handler) noexcept : 
				PerformHandlerBase(executor, easyHandle, multiHandle),
				m_handler(std::move(handler)) {}
			~PerformHandler() noexcept
			{
				// abort if we haven't been handled
				if (Handled() == false)
					Complete(asio::error::make_error_code(
						asio::error::operation_aborted), Error{});
				// untrack the handle. this is here rather than a deleter because
				// it seems cleaner to do it here and avoid all of that ugly
				// unique_ptr class template mess
				curl_multi_remove_handle(GetMultiHandle(), GetEasyHandle());
			}

			void Complete(asio::error_code ec, Error e) noexcept
			{
				auto executor = asio::get_associated_executor(
					m_handler, GetSocket().get_executor());
				// post it just in case the Async function calls this directly
				asio::post(asio::bind_executor(executor,
					std::bind(m_handler, std::move(ec), std::move(e))));
				SetHandled(true);
			}
		private:
			Handler m_handler;
		};
	public:
		/// @brief Creates the handle and if necessary, initializes cURL.
		/// If CMA_MANAGE_CURL is specified when the library is built,
		/// cURL's lifetime is managed by the total instances of Multi,
		/// and curl_global_init will be called by the library.
		/// If you would rather manage the lifetime yourself, an interface
		/// is provided in cma::Detail::Lifetime.
		/// @param executor The executor type
		Multi(const asio::any_io_executor& executor) noexcept;
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
		~Multi() noexcept { asio::error_code ignored; Cancel(ignored); }
		// we don't allow copies, because multi handles can't be duplicated.
		// there's not even a reason to do so, multi handles don't really hold
		// much of a state themselves besides stuff that shouldn't be duplicated
		Multi(const Multi&) = delete;
		Multi& operator=(const Multi&) = delete;
		/// @brief The other multi instance ends up in an invalid state
		Multi(Multi&& other) = default;
		/// @brief The other multi instance ends up in an invalid state
		/// @return This multi handle
		Multi& operator=(Multi&& other) = default;

		/// @return The native handle
		inline CURLM* GetNativeHandle() const noexcept { return m_nativeHandle.get(); }

		/// @return Whether or not the handle is valid
		inline operator bool() const noexcept { return m_nativeHandle != nullptr; }

		/// @brief Launches an asynchronous perform operation, and notifies
		/// the completion token either on error or success
		/// @tparam CompletionToken The completion token type
		/// @param easy The easy handle to perform the action on
		/// @param token The completion token
		/// @return DEDUCED
		template<typename CompletionToken>
		auto AsyncPerform(Easy& easy, CompletionToken&& token) noexcept
		{
			auto initiation = [this](auto&& handler, const Easy& easy)
			{
				// store the handler
				std::unique_ptr<PerformHandlerBase> performHandler(
					new PerformHandler<typename std::decay_t<decltype(handler)>>(m_executor,
						easy.GetNativeHandle(), GetNativeHandle(), handler));
				// track the socket and initiate the transfer. if this fails
				if (auto res = curl_multi_add_handle(GetNativeHandle(), easy.GetNativeHandle());
					res != CURLM_OK)
					return performHandler->Complete(asio::error_code{}, res);
				// track the handler
				m_easyHandlerMap.emplace(easy.GetNativeHandle(), std::move(performHandler));
			};
			return asio::async_initiate<CompletionToken, 
				void(asio::error_code, Error)>(
				initiation, token, easy);
		}
		/// @brief Cancels all outstanding asynchronous operations,
		/// and calls handlers with asio::error::operation_aborted
		/// @param ec The error code output
		/// @return The number of asynchronous operations canceled
		size_t Cancel(asio::error_code& ec) noexcept;
	private:
		/// @brief Notifies a handler of cancellation
		/// @param handler The handler
		inline static void NotifyCancel(PerformHandlerBase& handler) noexcept
		{
			handler.Complete(asio::error::make_error_code(
				asio::error::operation_aborted), Error{});
		}
		asio::any_io_executor m_executor;
#ifdef CMA_MANAGE_CURL
		Detail::Lifetime s_lifetime;
#endif
		std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)> m_nativeHandle;
		// when the handlers are destructed, their curl handle must be called
		std::unordered_map<CURL*, std::unique_ptr<PerformHandlerBase>> m_easyHandlerMap;
		std::unordered_map<curlsocktype, std::reference_wrapper<PerformHandlerBase>> m_nativeHandlerMap;
		asio::system_timer m_timer;
	};
}

#endif