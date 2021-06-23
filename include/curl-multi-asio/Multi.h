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
#include <unordered_map>
#include <utility>

namespace cma
{
	/// @brief Multi is a multi handle, which tracks and executes
	/// all curl_multi calls. Multi is generally thread-safe as
	/// everything is executed in strands, however calls to AsyncPerform
	/// must not happen at the same time.
	class Multi
	{
	private:
		/// @brief These handlers store all of the handler data including
		/// the raw socket, and the handler itself. They also handle
		/// unregistration
		class PerformHandlerBase
		{
		public:
			PerformHandlerBase(CURL* easyHandle) noexcept :
				m_easyHandle(easyHandle) {}
			virtual ~PerformHandlerBase() = default;

			/// @brief Completes the perform, and calls the handler. Must
			/// set handled status
			/// @param ec The error code
			/// @param e The curl error
			virtual void Complete(asio::error_code ec, Error e) noexcept = 0;

			/// @return The underlying easy handle
			inline CURL* GetEasyHandle() const noexcept { return m_easyHandle; }
			/// @return If the handler was considered handled
			inline bool Handled() const noexcept { return m_handled; }
		protected:
			/// @param handled If the handle was considered handled
			inline void SetHandled(bool handled) noexcept { m_handled = handled; }
		private:
			CURL* m_easyHandle;
			bool m_handled = false;
		};
		template<typename Handler>
		class PerformHandler : public PerformHandlerBase
		{
		public:
			PerformHandler(const asio::any_io_executor& executor,
				CURL* easyHandle, Handler& handler) noexcept : 
				PerformHandlerBase(easyHandle), m_executor(executor),
				m_handler(std::move(handler)) {}
			~PerformHandler() noexcept
			{
				// abort if we haven't been handled
				if (Handled() == false)
					Complete(asio::error::make_error_code(
						asio::error::operation_aborted), Error{});
			}

			void Complete(asio::error_code ec, Error e) noexcept
			{
				auto executor = asio::get_associated_executor(
					m_handler, m_executor);
				// post it just in case the Async function calls this directly
				asio::post(asio::bind_executor(executor,
					std::bind(m_handler, std::move(ec), std::move(e))));
				SetHandled(true);
			}
		private:
			asio::any_io_executor m_executor;
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
		/// the completion token either on error or success. No calls should
		/// be made concurrently, but can be made on different threads. In
		/// a multithreaded environment, post the call in a strand.
		/// @tparam CompletionToken The completion token type
		/// @param easy The easy handle to perform the action on
		/// @param token The completion token
		/// @return DEDUCED
		template<typename CompletionToken>
		auto AsyncPerform(Easy& easy, CompletionToken&& token) noexcept
		{
			auto initiation = [this](auto&& handler, Easy& easy)
			{
				// set the open and close socket functions. this allows
				// us to make them asio sockets for async functionality
				easy.SetOption(CURLoption::CURLOPT_OPENSOCKETFUNCTION, &Multi::OpenSocketCb);
				easy.SetOption(CURLoption::CURLOPT_OPENSOCKETDATA, this);
				easy.SetOption(CURLoption::CURLOPT_CLOSESOCKETFUNCTION, &Multi::CloseSocketCb);
				easy.SetOption(CURLoption::CURLOPT_CLOSESOCKETDATA, this);
				// store the handler
				std::shared_ptr<PerformHandlerBase> performHandler(
					new PerformHandler<typename std::decay_t<
					decltype(handler)>>(m_executor, easy.GetNativeHandle(), handler), 
					std::bind(&Multi::FreeHandler, this, std::placeholders::_1));
				// track the socket and initiate the transfer. if this fails
				if (auto res = curl_multi_add_handle(GetNativeHandle(),
					easy.GetNativeHandle()); res != CURLM_OK)
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
		/// @param error The error to send to all open handlers
		/// @return The number of asynchronous operations canceled
		size_t Cancel(asio::error_code& ec, 
			CURLMcode error = CURLMcode::CURLM_OK) noexcept;

		/// @brief Sets a multi option
		/// @tparam T The option value type
		/// @param option The option
		/// @param val The value
		/// @return The resulting error
		template<typename T>
		inline Error SetOption(CURLMoption option, T&& val) noexcept
		{
			// weird GCC bug where forward thinks its return value is ignored
			return curl_multi_setopt(GetNativeHandle(), option, static_cast<T&&>(val));
		}
	private:
		/// @brief Closes a socket, and then we can free the socket. For a
		/// description of arguments, check cURL documentation for
		/// CURLOPT_CLOSESOCKETFUNCTION
		/// @return 0 on success, CURL_BADSOCKET on failure
		static int CloseSocketCb(Multi* clientp, curl_socket_t item) noexcept;
		/// @brief Opens an asio socket for an address. For a description
		/// of arguments, check cURL documentation for CURLOPT_OPENSOCKETFUNCTION
		/// @return The socket
		static curl_socket_t OpenSocketCb(Multi* clientp, curlsocktype purpose,
			curl_sockaddr* address) noexcept;
		/// @brief The socket callback called by cURL when a socket should
		/// read, write, or be destroyed. For a description of arguments,
		/// check cURL documentation for CURLMOPT_SOCKETFUNCTION
		/// @return 0 on success
		static int SocketCallback(CURL* easy, curl_socket_t s, int what,
			Multi* userp, int* socketp) noexcept;
		/// @brief The timer callback called by cURL when a timer should be set.
		/// For a description on arguments, check cURL documentation for
		/// CURLMOPT_TIMERFUNCTION
		/// @return 0 on success, 1 on failure
		static int TimerCallback(CURLM* multi, long timeout_ms, Multi* userp) noexcept;

		/// @brief Checks the handle for completed handles and calls any
		/// completion handlers for finished transfers, before removing them
		void CheckTransfers() noexcept;
		/// @brief Handles socket events for reads and writes
		/// @param ec The error code
		/// @param s The socket
		/// @param what The type of event
		void EventCallback(const asio::error_code& ec, curl_socket_t s,
			int what, int* last) noexcept;
		/// @brief Untracks and frees a handler
		/// @param handler The handler to untrack
		inline void FreeHandler(PerformHandlerBase* handler) noexcept
		{
			curl_multi_remove_handle(GetNativeHandle(), handler->GetEasyHandle());
			// actually free the handler too
			std::default_delete<PerformHandlerBase>()(handler);
		}
		asio::any_io_executor m_executor;
#ifdef CMA_MANAGE_CURL
		Detail::Lifetime s_lifetime;
#endif
		// when the handlers are destructed, their curl handle must be untracked
		std::unordered_map<CURL*, std::shared_ptr<PerformHandlerBase>> m_easyHandlerMap;
		std::unordered_map<curl_socket_t, asio::ip::tcp::socket> m_easySocketMap;
		asio::system_timer m_timer;
		asio::strand<asio::any_io_executor> m_strand;
		std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)> m_nativeHandle;
	};
}

#endif