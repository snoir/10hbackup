#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#define BASE_DEEZER_URI "https://api.deezer.com"

struct http_data {
	size_t size;
	char *data;
};

size_t
deezer_callback(char *data, size_t size, size_t nmemb, struct http_data *userdata);

int
http_request(CURL *handler, char *uri, struct http_data *data);

int
get_playlists(CURL *handler, char *token, struct http_data *data);

int
main(int argc, char *argv[])
{
	char token[255];

	if (argc < 2) {
		fprintf(stderr, "Missing deezer token as argument\n");
		return EXIT_FAILURE;
	} else {
		strncpy(token, argv[1], sizeof(token));
	}

	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();

	struct http_data deezer_data;
	deezer_data.data = malloc(1);
	deezer_data.size = 0;

	get_playlists(curl, token, &deezer_data);

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	free(deezer_data.data);

	return EXIT_SUCCESS;
}

int
http_request(CURL *handler, char *uri, struct http_data *data)
{
	CURLcode res;
	char err_buff[CURL_ERROR_SIZE];

	curl_easy_setopt(handler, CURLOPT_ERRORBUFFER, err_buff);
	curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, &deezer_callback);
	curl_easy_setopt(handler, CURLOPT_WRITEDATA, data);

	curl_easy_setopt(handler, CURLOPT_URL, uri);

	res = curl_easy_perform(handler);

	if (res != CURLE_OK) {
		fprintf(stderr, "Error occured:\n");
		fprintf(stderr, "  %s\n", err_buff);
	} else {
		printf("%.*s", (int)data->size, data->data);
	}

	return res;
}

int
get_playlists(CURL *handler, char *token, struct http_data *data)
{
	int uri_size = strlen(BASE_DEEZER_URI) + strlen("/user/me/playlists") +
		strlen("?access_token=") + strlen(token);
	char uri[uri_size];

	strncpy(uri, BASE_DEEZER_URI, strlen(BASE_DEEZER_URI));
	strncat(uri, "/user/me/playlists", strlen("/user/me/playlists"));
	strncat(uri, "?access_token=", strlen("?access_token="));
	strncat(uri, token, strlen(token));

	return http_request(handler, uri, data);
}


size_t
deezer_callback(char *data, size_t size, size_t nmemb, struct http_data *userdata) {
	int realsize = size * nmemb;

	userdata->data = realloc(userdata->data, userdata->size + realsize);

	memcpy(&userdata->data[userdata->size], data, realsize);
	userdata->size += realsize;

	return realsize;
}
