cmake_minimum_required(VERSION 3.15)

project(curl-multi-asio)

# allow it to search for FindCARES
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# we need curl of course
find_package(CURL REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CMA_BUILD_EXAMPLES "Build curl-multi-asio examples" ON)
option(CMA_USE_BOOST "Use boost::asio" OFF)
option(CMA_CURL_OPENSSL "cURL uses OpenSSL and needs OpenSSL to be linked" ON)
option(CMA_CURL_ARES "cURL uses c-ares and needs c-ares to be linked" OFF)
option(CMA_CURL_GZIP "cURL uses gzip and needs gzip to be linked" OFF)
option(CMA_MANAGE_CURL "The program is only using curl-multi-asio for cURL. It will manage cURL's global state" ON)
set(CMA_ASIO_INCLUDE_DIR "" CACHE FILEPATH "asio Include directory. If there is already an asio target, this is ignored")

if ((NOT TARGET asio) AND
	CMA_ASIO_INCLUDE_DIR STREQUAL "")
	message(FATAL_ERROR "asio must be provided as a target or in CMA_ASIO_INCLUDE_DIR")
endif()

# if an asio target has not been defined, define one
if (NOT TARGET asio)
	add_library(asio INTERFACE)
	target_include_directories(asio
		INTERFACE ${CMA_ASIO_INCLUDE_DIR})
endif()

# we want documentation generated
if (${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
	find_package(Doxygen)
	if (DOXYGEN_FOUND)
		add_subdirectory(docs)
	else()
		message(STATUS "Doxygen not found, skipping docs")
	endif()
endif()

# the main library is built in src
add_subdirectory(extern)
add_subdirectory(src)
if (CMA_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()