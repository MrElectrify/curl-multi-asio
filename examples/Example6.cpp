/*
 *	Example6 shows a simple asynchronous GET call
 *	to www.example.com with a future
 */

#include <curl-multi-asio/Easy.h>
#include <curl-multi-asio/Multi.h>

#include <iostream>

int main()
{
	// the io_context is our choice of executor here. it will
	// run until it is out of work, or in this case, the request
	// finishes either with error or success
	asio::io_context ctx;
	// this is new here. it's a multi handle, and it is what will
	// allow us to make asynchronous calls to perform. it takes any
	// executor (io_context in our case) and executes all of the
	// request work on that executor
	cma::Multi multi(ctx);
	// now we use the easy handle as usual. set all of our regular
	// options just as if we were making a synchronous call
	cma::Easy easy;
	easy.SetURL("http://www.example.com/");
	// this is is where the even more real magic happens. this call
	// will queue the asynchronus operation on the executor. we could
	// even have another easy handle doing stuff in parallel! just set
	// it up like this one, set your options, and call AsyncPerform on 
	// that one, either before the executor starts working, or while it
	// is working during a completion handler to start another one.
	auto error = multi.AsyncPerform(easy, asio::use_future);
	// all of the processing happens right here
	ctx.run();
	// here we get the result from the future
	auto e = error.get();
	if (e)
	{
		std::cerr << "Error: " << e.ToString() << '\n';
		return 1;
	}
	std::cout << "Success!\n";
	return 0;
}