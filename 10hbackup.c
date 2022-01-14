#include <curl/curl.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

#define BASE_DEEZER_URI "https://api.deezer.com"

struct http_data {
	size_t size;
	char **data;
};

size_t
deezer_callback(char *data, size_t size, size_t nmemb, struct http_data *userdata);

int
http_request(CURL *handler, char *uri, char **buffer);

int
get_playlists(CURL *handler, char *token, struct json_object *playlists_array);

int
main(int argc, char *argv[])
{
	char token[255];
	json_object *playlists_array = json_object_new_array();

	if (argc < 2) {
		fprintf(stderr, "Missing deezer token as argument\n");
		return EXIT_FAILURE;
	} else {
		strncpy(token, argv[1], sizeof(token));
	}

	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();

	get_playlists(curl, token, playlists_array);

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	return EXIT_SUCCESS;
}

int
http_request(CURL *handler, char *uri, char **buffer)
{
	CURLcode res;
	char err_buff[CURL_ERROR_SIZE];
	struct http_data data = { .size = 0, .data = buffer };

	curl_easy_setopt(handler, CURLOPT_ERRORBUFFER, err_buff);
	curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, &deezer_callback);
	curl_easy_setopt(handler, CURLOPT_WRITEDATA, &data);

	curl_easy_setopt(handler, CURLOPT_URL, uri);

	res = curl_easy_perform(handler);

	return res;
}

int
get_playlists(CURL *handler, char *token, struct json_object *playlists_array)
{
	int uri_size = strlen(BASE_DEEZER_URI) + strlen("/user/me/playlists") +
		strlen("?access_token=") + strlen(token);
	char uri[uri_size];
	size_t nb_playlists;
	struct json_object *parsed_playlists;
	struct json_object *uri_next_obj;
	struct json_object *data_array;
	char *buffer = malloc(1);

	strncpy(uri, BASE_DEEZER_URI, strlen(BASE_DEEZER_URI));
	strncat(uri, "/user/me/playlists", strlen("/user/me/playlists"));
	strncat(uri, "?access_token=", strlen("?access_token="));
	strncat(uri, token, strlen(token));
	http_request(handler, uri, &buffer);
	parsed_playlists = json_tokener_parse(buffer);

	if (parsed_playlists == NULL) {
		fprintf(stderr, "Error occured while parsing playlists' JSON\n");
		return 1;
	}

	json_object_object_get_ex(parsed_playlists, "data", &data_array);

	json_object_object_get_ex(parsed_playlists, "next", &uri_next_obj);
	const char *uri_next = json_object_get_string(uri_next_obj);

	nb_playlists = json_object_array_length(data_array);

	for (size_t i=0; i < nb_playlists; i++) {
		json_object *item = json_object_array_get_idx(data_array, i);
		json_object_get(item);
		json_object_array_add(playlists_array, item);
	}

	free(buffer);
	buffer = malloc(1);

	while (uri_next != NULL) {
		http_request(handler, (char *)uri_next, &buffer);
		parsed_playlists = json_tokener_parse(buffer);

		json_object_object_get_ex(parsed_playlists, "next", &uri_next_obj);
		json_object_object_get_ex(parsed_playlists, "data", &data_array);

		nb_playlists = json_object_array_length(data_array);
		uri_next = json_object_get_string(uri_next_obj);

		for (size_t i=0; i < nb_playlists; i++) {
			json_object *item = json_object_array_get_idx(data_array, i);
			json_object_get(item);
			json_object_array_add(playlists_array, item);
		}

		free(buffer);
		buffer = malloc(1);
	}

	return 0;
}


size_t
deezer_callback(char *data, size_t size, size_t nmemb, struct http_data *userdata) {
	int realsize = size * nmemb;

	char *data_ptr = realloc(*userdata->data, userdata->size + realsize);
	*userdata->data = data_ptr;

	memcpy(&data_ptr[userdata->size], data, realsize);
	userdata->size += realsize;

	return realsize;
}
