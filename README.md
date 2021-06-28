# curl-multi-asio
An asio/boost::asio interface for libcurl's multi functionality, utilizing composed operations that support futures, asynchronous, 
and synchronous operations on cURLeasy handles.

## Requirements
- A compliant C++20 compiler for the included concepts

## Tested Configurations
- MSVC 2019 16.10.1
- GCC 11.1.0
- chriskohlhoff/asio 1.18.2

## Usage
If your flavor of `asio` is `boost`, set the CMake option `CMA_USE_BOOST` to `ON`. I don't use boost so I haven't personally tested it
but I don't see a reason that any recent (1.70+) version of boost shouldn't support it. Either define your own `asio` target, or provide
the include path in `CMA_ASIO_INCLUDE_DIR` and it will define an `asio` target for you. Some other convenience options are provided to
hopefully match your cURL installaton, whether it includes `c-ares` (not really tested in async) or `OpenSSL`.

`curl-multi-asio` is designed with minimal obstruction in mind. All of the regular cURL options are available in both `easy` and `multi` form.

The lifetime of cURL's globals are managed using `curl-multi-asio`. If you are already using cURL or want to manage the globals yourself, 
set the CMake option `CMA_MANAGE_CURL` to off.

`CURL` easy handles are created as `cma::Easy`. They manage the handle themselves, and provide a few helper functions such as `SetBuffer`, 
which is defined for `std::ostream` and a concept that mimics some sort of STL contiguous memory container of chars.
Easy handles can be used on their own to perform synchronous requests with the `Perform` method.

`CURLM` multi handles are created as `cma::Multi`. They require some sort of executor to function, generally in the form of `asio::io_context`. 
They allow asynchronous performance of easy handles by using the `AsyncPerform` function, which accepts a set-up easy handle with all of the
desired options, as well as a completion token. Tokens such as callbacks and futures are supported, however coroutine support is not quite
ready. I'm not entirely sure why to be honest, I don't know enough about coroutines, but something leads me to believe it is due to the completion
token signature also being `void(error_code)`, which is a nice wrapper around both `CURLcode` and `CURLMcode`.

Many examples are provided in `examples/` which show synchronous usage (whose building can be disabled with the CMake option `CMA_BUILD_EXAMPLES`), 
asynchronous usage with different types of buffers, and asynchronous futures. Everything is extensively commented in doxygen format, and the `docs`
target in make/ninja/whatever flavor will generate docs for every bit of code.

## Errors
Error facilities are provided inside of the usual `asio::error_code` or `boost::system::error_code`, depending on your flavor. If there is a
cURL error, or `asio::error::operation_aborted`, it will be stored in the `error_code`.

## Thread Safety
Although I haven't extensively tested this with multiple threads, the design should allow operation to be thread safe with a single exception.
`curl-multi-asio` can have many threads concurrently processing work in the executor. General `asio` guarantees should apply. Just
use your brain and don't do obvious don'ts like adding an easy handle while it's already processing, or for some reason change some of the cURL
callbacks like `CURLMOPT_SOCKETCALLBACK`. `cma::Multi` is not singleton, it can be instantiated many times in different places with different
executors.

## Notes
This was written with the standalone `asio` by `chriskohlhoff`, and has not been tested with `boost::asio`, but generally it should be ready to
drop in with `boost`, or within 5 minutes of fixing class names. I also wrote the bulk of the code in less than a day so don't come with pitchforks
when your server crashes and burns. Instead, please try and figure out what's causing your issues and make an issue or a PR.