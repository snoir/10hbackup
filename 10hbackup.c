#include <curl/curl.h>
#include <json-c/json.h>
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
get_playlists(CURL *handler, char *token);

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

	get_playlists(curl, token);

	curl_easy_cleanup(curl);
	curl_global_cleanup();

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
get_playlists(CURL *handler, char *token)
{
	int uri_size = strlen(BASE_DEEZER_URI) + strlen("/user/me/playlists") +
		strlen("?access_token=") + strlen(token);
	char uri[uri_size];
	size_t nb_playlists;
	struct json_object *parsed_playlists;
	struct json_object *uri_next_obj;
	struct json_object *data_array;
	struct http_data deezer_data;
	struct json_object *playlists_array;
	deezer_data.data = malloc(1);
	deezer_data.size = 0;

	strncpy(uri, BASE_DEEZER_URI, strlen(BASE_DEEZER_URI));
	strncat(uri, "/user/me/playlists", strlen("/user/me/playlists"));
	strncat(uri, "?access_token=", strlen("?access_token="));
	strncat(uri, token, strlen(token));

	http_request(handler, uri, &deezer_data);
	parsed_playlists = json_tokener_parse(deezer_data.data);

	if (parsed_playlists == NULL) {
		fprintf(stderr, "Error occured while parsing playlists' JSON\n");
		return 1;
	}

	playlists_array = json_object_new_array();
	json_object_object_get_ex(parsed_playlists, "data", &data_array);

	json_object_object_get_ex(parsed_playlists, "next", &uri_next_obj);
	const char *uri_next = json_object_get_string(uri_next_obj);

	nb_playlists = json_object_array_length(data_array);

	for (size_t i=0; i < nb_playlists; i++) {
		json_object_array_add(playlists_array, json_object_array_get_idx(data_array, i));
	}

	free(deezer_data.data);
	deezer_data.size = 0;
	deezer_data.data = malloc(1);

	while (uri_next != NULL) {
		http_request(handler, (char *)uri_next, &deezer_data);
		parsed_playlists = json_tokener_parse(deezer_data.data);

		json_object_object_get_ex(parsed_playlists, "next", &uri_next_obj);
		json_object_object_get_ex(parsed_playlists, "data", &data_array);

		nb_playlists = json_object_array_length(data_array);
		uri_next = json_object_get_string(uri_next_obj);

		for (size_t i=0; i < nb_playlists; i++) {
			json_object_array_add(playlists_array,
					json_object_array_get_idx(data_array, i));
		}

		free(deezer_data.data);
		deezer_data.size = 0;
		deezer_data.data = malloc(1);
	}

	return 0;
}


size_t
deezer_callback(char *data, size_t size, size_t nmemb, struct http_data *userdata) {
	int realsize = size * nmemb;

	userdata->data = realloc(userdata->data, userdata->size + realsize);

	memcpy(&userdata->data[userdata->size], data, realsize);
	userdata->size += realsize;

	return realsize;
}
