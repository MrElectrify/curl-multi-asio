#ifndef CURLMULTIASIO_DETAIL_LIFETIME_H_
#define CURLMULTIASIO_DETAIL_LIFETIME_H_

/// @file
/// cURL Lifetime Manager
/// 6/21/21 13:21

// curl-multi-asio includes
#include <curl-multi-asio/Common.h>

// STL includes
#ifdef _DEBUG
#include <atomic>
#endif

namespace cma
{
	namespace Detail
	{
		/// @brief An RAII wrapper around cURL's global instance
		/// variables. No more than one can exist at a time
		class Lifetime
		{
		public:
			/// @brief Calls curl_global_init. In debug builds, this
			/// will throw std::runtime_error if it is not the only instance
			Lifetime() NOEXCEPT_RELEASE;
			/// @brief Calls curl_global_cleanup
			~Lifetime() noexcept;
		private:
#ifdef _DEBUG
			static std::atomic_size_t s_refCount;
#endif
		};
	}
}

#endif