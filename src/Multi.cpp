#include <curl-multi-asio/Multi.h>

using cma::Multi;

std::atomic_size_t Multi::s_instanceCount = 0;
std::unique_ptr<cma::Detail::Lifetime> Multi::s_lifetime;

Multi::Multi(asio::any_io_executor executor) noexcept
	: m_executor(std::move(executor)), m_nativeHandle(
		curl_multi_init(), curl_multi_cleanup)
{
#ifdef CMA_MANAGE_CURL
	if (s_instanceCount++ == 0)
		s_lifetime = std::make_unique<Detail::Lifetime>();
#endif
}

Multi::~Multi() noexcept
{
#ifdef CMA_MANAGE_CURL
	if (--s_instanceCount == 0)
		s_lifetime.reset();
#endif
}