/*
 *	Example1 shows a simple synchronous GET call
 *	to www.example.com and storing the result in
 *	an ostream. It also shows getting information
 *	such as response code
 */
 
#include <curl-multi-asio/Easy.h>
#include <curl-multi-asio/Multi.h>

#include <iostream>

int main()
{
	asio::io_context ctx;
	cma::Easy easy;
	cma::Multi multi(ctx);
	easy.SetURL("http://www.example.com/");
	// this is where the real magic happens. the request is
	// performed synchronously, and the result is returned to
	// us. we handle this one just for completion's sake, and
	// to show how easy it is to get error information
	multi.AsyncPerform(easy, [](const asio::error_code& ec, const cma::Error& e)
		{
			std::cout << "EASY\n";
		});
	ctx.run();
	return 0;
}