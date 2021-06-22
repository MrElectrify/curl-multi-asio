#ifndef CURLMULTIASIO_EASY_H_
#define CURLMULTIASIO_EASY_H_

/// @file
/// cURL Easy Handle
/// 6/21/21 15:26

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>
#include <curl-multi-asio/Detail/Lifetime.h>
#include <curl-multi-asio/Error.h>

// expected includes
#include <tl/expected.hpp>

// STL includes
#include <memory>
#include <utility>

namespace cma
{
	class Multi;

	/// @brief Easy is a wrapper around an easy CURL handle
	class Easy
	{
	public:
		enum class RequestType
		{
			GET,
			POST,
			PUT,
		};
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

		/// @brief Perform the request
		/// @return The resulting code
		inline Error Perform() noexcept
		{
			return curl_easy_perform(GetNativeHandle());
		}

		/// @brief Gets info from the easy handle
		/// @tparam T The data type
		/// @param info The info
		/// @param out The output instance
		/// @return The resulting error
		template<typename T>
		inline Error GetInfo(CURLINFO info, T& out) noexcept
		{
			return curl_easy_getinfo(GetNativeHandle(), info, &out);
		}
		/// @brief Gets info from the easy handle
		/// @tparam T The data type
		/// @param info The info
		/// @return An instance to the type, or the error
		template<typename T>
		inline tl::expected<T, Error> GetInfo(CURLINFO info) noexcept
		{
			T inst;
			if (auto res = GetInfo(info, inst); 
				res != CURLcode::CURLE_OK)
				return res;
			// no copy elision here. move it into the expected
			return std::move(inst);
		}
		/// @brief Sets an option on the easy handle
		/// @tparam T The value type
		/// @param option The option
		/// @param value The value
		/// @return The resulting error
		template<typename T>
		inline Error SetOption(CURLoption option, T&& value) noexcept
		{
			return curl_easy_setopt(GetNativeHandle(), option, std::forward<T>(value));
		}
		/// @brief Sets the URL to traverse to
		/// @param url The URL
		/// @return The resulting error
		inline Error SetURL(std::string_view url) noexcept
		{
			return SetOption(CURLoption::CURLOPT_URL, url.data());
		}

		/// @return Whether or not the handle is valid
		inline operator bool() const noexcept { return m_nativeHandle != nullptr; }
	private:
#ifdef CMA_MANAGE_CURL
		Detail::Lifetime m_lifeTime;
#endif
		std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> m_nativeHandle;
	};
}

#endif