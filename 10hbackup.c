#include <curl/curl.h>
#include <stdlib.h>

int
main()
{
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();

	return EXIT_SUCCESS;
}
