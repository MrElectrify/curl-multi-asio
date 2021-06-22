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
	easy.SetURL("https://www.example.com/");
	// at least in the case of windows, this is required for SSL
	// to properly validate certificates
	easy.SetOption(CURLoption::CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
	if (auto res = easy.Perform(); res)
	{
		std::cerr << "Error: " << res.ToString() << " (" << res.GetValue() << ")\n";
		return 1;
	}
	return 0;
}