#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#define BASE_DEEZER_URI "api.deezer.com"

struct http_data {
	size_t size;
	char *data;
};

size_t
deezer_callback(char *data, size_t size, size_t nmemb, struct http_data *userdata);

int
main()
{
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();

	CURLcode res;
	char err_buff[CURL_ERROR_SIZE];
	struct http_data deezer_data;
	deezer_data.data = malloc(1);
	deezer_data.size = 0;

	curl_easy_setopt(curl, CURLOPT_URL, BASE_DEEZER_URI);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_buff);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &deezer_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &deezer_data);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		fprintf(stderr, "Error occured:\n");
		fprintf(stderr, "  %s\n", err_buff);
		return EXIT_FAILURE;
	} else {
		printf("%s\n", deezer_data.data);
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	return EXIT_SUCCESS;
}

size_t
deezer_callback(char *data, size_t size, size_t nmemb, struct http_data *userdata) {
	int realsize = size * nmemb;

	userdata->data = realloc(userdata->data, userdata->size + realsize);

	memcpy(&userdata->data[userdata->size], data, realsize);
	userdata->size += realsize;

	return realsize;
}
