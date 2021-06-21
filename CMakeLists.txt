cmake_minimum_required(VERSION 3.15)

project(curl-multi-asio)

# we need curl of course
find_package(CURL REQUIRED)

option(CMA_USE_BOOST "Use boost::asio" OFF)
option(CMA_CURL_OPENSSL "cURL uses OpenSSL and needs OpenSSL to be linked" ON)
option(CMA_CURL_ARES "cURL uses c-ares and needs c-ares to be linked" OFF)
option(CMA_CURL_GZIP "cURL uses gzip and needs gzip to be linked" OFF)
option(CMA_MANAGE_CURL "The program is only using curl-multi-asio for cURL. It will manage cURL's global state" ON)
set(CMA_ASIO_INCLUDE_DIR "" CACHE FILEPATH "asio Include directory. If there is already an asio target, this is ignored")

# if an asio target has not been defined, define one
if (NOT TARGET asio)
	add_library(asio INTERFACE)
	target_include_directories(asio
		INTERFACE ${CMA_ASIO_INCLUDE_DIR})
endif()

# we want documentation generated
find_package(Doxygen)
if (DOXYGEN_FOUND)
	add_subdirectory(docs)
else()
	message(STATUS "Doxygen not found, skipping docs")
endif()

# the main library is built in src
add_subdirectory(src)
add_subdirectory(examples)