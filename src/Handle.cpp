#include <curl-multi-asio/Handle.h>

using cma::Handle;

std::atomic_size_t Handle::s_instanceCount = 0;
std::unique_ptr<cma::Detail::Lifetime> Handle::s_lifetime;

Handle::Handle(asio::any_io_executor executor) noexcept
	: m_executor(std::move(executor)), m_nativeHandle(
		curl_multi_init(), curl_multi_cleanup)
{
#ifdef CMA_MANAGE_CURL
	if (s_instanceCount++ == 0)
		s_lifetime = std::make_unique<Detail::Lifetime>();
#endif
}

Handle::~Handle() noexcept
{
#ifdef CMA_MANAGE_CURL
	if (--s_instanceCount == 0)
		s_lifetime.reset();
#endif
}