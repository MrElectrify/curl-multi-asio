add_library(curl-multi-asio Handle.cpp Detail/Lifetime.cpp)

target_include_directories(curl-multi-asio
	PUBLIC ../include)

target_link_libraries(curl-multi-asio
	PUBLIC asio
	PUBLIC CURL::libcurl)

if (CMA_CURL_OPENSSL)
	find_package(OpenSSL REQUIRED)
	target_link_libraries(curl-multi-asio
		PUBLIC OpenSSL::Crypto
		PUBLIC OpenSSL::SSL)
	if (UNIX)
		# openSSL uses pthreads
		find_package(Threads REQUIRED)
		target_link_libraries(curl-multi-asio
			PUBLIC Threads::Threads)
	endif()
endif()

if (CMA_CURL_ARES)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};../cmake/)
	find_package(CARES REQUIRED)
	target_link_libraries(curl-multi-asio
		PUBLIC ${CARES_LIBRARY})
endif()

if (CMA_CURL_GZIP)
	find_package(ZLIB REQUIRED)
	target_link_libraries(curl-multi-asio
		PUBLIC z)
endif()

if (CMA_USE_BOOST)
	target_compile_options(curl-multi-asio
		PUBLIC -DCMA_USE_BOOST=1)
endif()

if (CMA_MANAGE_CURL)
	target_compile_options(curl-multi-asio
		PUBLIC -DCMA_MANAGE_CURL=1)
endif()