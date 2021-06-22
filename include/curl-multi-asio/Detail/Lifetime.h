#ifndef CURLMULTIASIO_DETAIL_LIFETIME_H_
#define CURLMULTIASIO_DETAIL_LIFETIME_H_

/// @file
/// cURL Lifetime Manager
/// 6/21/21 13:21

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>

// STL includes
#include <atomic>

namespace cma
{
	namespace Detail
	{
		/// @brief An RAII wrapper around cURL's global instance
		/// variables. No more than one can exist at a time
		class Lifetime
		{
		public:
			/// @brief Calls curl_global_init if this is the first Lifetime
			/// instance created
			Lifetime() noexcept;
			/// @brief Calls curl_global_cleanup if this is the last Lifetime
			/// instance remaining
			~Lifetime() noexcept;
			Lifetime(const Lifetime&) noexcept;
			Lifetime& operator=(const Lifetime&) = default;
			Lifetime(Lifetime&&) noexcept;
			Lifetime& operator=(Lifetime&&) = default;
		private:
			static std::atomic_size_t s_refCount;
		};
	}
}

#endif