#include <curl-multi-asio/Detail/Lifetime.h>

#include <exception>

using cma::Detail::Lifetime;

std::atomic_size_t Lifetime::s_refCount = 0;

Lifetime::Lifetime() noexcept
{
	if (s_refCount++ == 0)
		curl_global_init(CURL_GLOBAL_ALL);
}

Lifetime::~Lifetime() noexcept
{
	if (--s_refCount == 0)
		curl_global_cleanup();
}

Lifetime::Lifetime(const Lifetime&) noexcept
{
	++s_refCount;
}

Lifetime::Lifetime(Lifetime&&) noexcept
{
	++s_refCount;
}