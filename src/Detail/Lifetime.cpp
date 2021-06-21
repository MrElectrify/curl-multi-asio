#include <curl-multi-asio/Detail/Lifetime.h>

#include <exception>

using cma::Detail::Lifetime;

#ifdef _DEBUG
std::atomic_size_t Lifetime::s_refCount = 0;
#endif

Lifetime::Lifetime() NOEXCEPT_RELEASE
{
#ifdef _DEBUG
	// if there is already an instance, there is already
	// something managing lifetimes. throw std::runtime_error
	if (s_refCount++ != 0)
		throw std::runtime_error("More than one instance of cma::Detail::Lifetime was detected");
#endif
	curl_global_init(CURL_GLOBAL_ALL);
}

Lifetime::~Lifetime() noexcept
{
#ifdef _DEBUG
	--s_refCount;
#endif
	curl_global_cleanup();
}