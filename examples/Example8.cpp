/*
 *	Example8 shows a simple synchronous GET call
 *	with custom headers and urlencoded data
 */

#include <curl-multi-asio/Easy.h>

#include <iostream>

int main()
{
	// this example is rather simple. we don't even require
	// any sort of io_context or anything because this request
	// will be completed synchronously. in the real world we
	// would want to check if easy is valid, the result of
	// setting each option, and fail if those don't work. for
	// simplicity's sake we won't do that here
	cma::Easy easy;
	// there's an overload for SetURL that accepts key-value pairs
	// of url=encoded parameters. this is simply a convenience to
	// make some code look neater and more understandable when
	// working with url-encoded parameters
	easy.SetURL("http://www.postman-echo.com/get", 
		{ { "key", "value" }, { "another", "keyvalue" } });
	// this also accepts key-value pairs, and sets custom headers.
	// if we want to clear custom headers, simply use ClearHeaders.
	// the lifetime of the headers is managed by the easy handle
	// and will be automatically freed otherwise once the handle
	// goes out of scope
	easy.AddHeader({ "cma", "true" });
	easy.AddHeader({ "another", "header" });
	// this is where the real magic happens. the request is
	// performed synchronously, and the result is returned to
	// us. we handle this one just for completion's sake, and
	// to show how easy it is to get error information
	if (auto res = easy.Perform(); res)
	{
		std::cerr << "Error: " << res.message() << " (" << res << ")\n";
		return 1;
	}
	return 0;
}