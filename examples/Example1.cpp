/*
 *	Example1 shows a simple synchronous GET call
 *	to www.example.com. It shows the bare minimum
 *	required to make a CURL request.
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
	easy.SetURL("http://www.example.com/");
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