/*
 *	Example1 shows a simple synchronous GET call
 *	to www.example.com. It shows the bare minimum
 *	required to make a CURL request.
 */
 
#include <curl-multi-asio/Handle.h>

int main()
{
	// asio::io_context is used here but anything that either
	// is asio::any_io_executor or contains an executor can be
	// used here
	asio::io_context ctx;
	// initialize the handle. this is what will allow us to use
	// anything in the curl multi facilities
	cma::Handle handle(ctx);
	return 0;
}