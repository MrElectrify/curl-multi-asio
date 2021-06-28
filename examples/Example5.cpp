/*
 *	Example5 is the first truly asynchronous example. It
 *	shows two asynchronous GET requests to
 *	www.example.com and www.google.com, and storing
 *	the results in std::strings
 */
 
#include <curl-multi-asio/Easy.h>
#include <curl-multi-asio/Multi.h>

#include <iostream>
#include <string>

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
	cma::Easy example;
	example.SetURL("http://www.example.com/");
	std::string exampleBuf;
	example.SetBuffer(exampleBuf);
	cma::Easy google;
	google.SetURL("http://www.google.com/");
	std::string googleBuf;
	google.SetBuffer(googleBuf);
	// this is is where the even more real magic happens. this call
	// will queue the asynchronus operation on the executor. we could
	// even have another easy handle doing stuff in parallel! just set
	// it up like this one, set your options, and call AsyncPerform on 
	// that one, either before the executor starts working, or while it
	// is working during a completion handler to start another one.
	multi.AsyncPerform(example, [&exampleBuf](const asio::error_code& ec)
		{
			if (ec)
				std::cerr << "Error: " << ec.message() << " (" << ec << ")\n";
			else
				std::cout << "Completed easy perform for example with " << exampleBuf.size() << " bytes\n";
		});
	multi.AsyncPerform(google, [&googleBuf](const asio::error_code& ec)
		{
			if (ec)
				std::cerr << "Error: " << ec.message() << " (" << ec << ")\n";
			else
				std::cout << "Completed easy perform for google with " << googleBuf.size() << " bytes\n";
		});
	// all of the processing happens right here
	ctx.run();
	return 0;
}