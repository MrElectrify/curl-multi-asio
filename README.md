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
token signature also being `void(asio::error_code, cma::Error)`, which is a nice wrapper around both `CURLcode` and `CURLMcode`.

Speaking of the completion token signature for `cma::Multi`'s `AsyncPerform` function, an `asio::error_code` is passed as required, and only
has an error in the event of `asio::error::operation_aborted`. It is possible to cancel all asynchronous operations just as with many other
`asio` structures by simply calling `Cancel`. This will immediately call handlers with `asio::error::operation_aborted` and cease any operations.
Otherwise, errors will usually be stored in `cma::Error`.

Many examples are provided in `examples/` which show synchronous usage (whose building can be disabled with the CMake option `CMA_BUILD_EXAMPLES`), 
asynchronous usage with different types of buffers, and asynchronous futures. Everything is extensively commented in doxygen format, and the `docs`
target in make/ninja/whatever flavor will generate docs for every bit of code.

## Thread Safety
Although I haven't extensively tested this with multiple threads, the design should allow operation to be thread safe with a single exception.
Calls to `cma::Multi::AsyncPerform` must not be done concurrently, and should be wrapped in an `asio::strand`, either implicit or explicit.
Otherwise, `curl-multi-asio` can have many threads concurrently processing work in the executor. General `asio` guarantees should apply. Just
use your brain and don't do obvious don'ts like adding an easy handle while it's already processing, or for some reason change some of the cURL
callbacks like `CURLMOPT_SOCKETCALLBACK`. `cma::Multi` is not singleton, it can be instantiated many times in different places with different
executors.

## Notes
I wrote this all in less than a day so it's not gonna perfect. Don't come with pitchforks when you find weird bugs, do a little bit of debugging,
or simply open up an issue and help better the project for everybody. No such project exists with proper modern `asio` initiation in my small amount
of looking, so hopefully we can make this useful for everybody. Though in my limited testing everything seems to work just fine. A lot of the code is
based on the `libev` and `boost::asio` examples included with `cURL`, but much more C++ than C.
