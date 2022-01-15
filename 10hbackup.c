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
get_json_data_array(CURL *handler, char *token, json_object *item_list_array);

char *
uri_concat(char *path, char *token);

int
main(int argc, char *argv[])
{
	char token[255];
	json_object *playlist_list_array = json_object_new_array();

	if (argc < 2) {
		fprintf(stderr, "Missing deezer token as argument\n");
		return EXIT_FAILURE;
	} else {
		strncpy(token, argv[1], sizeof(token));
	}

	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();

	char *playlists_uri = uri_concat("/user/me/playlists", token);
	get_json_data_array(curl, playlists_uri, playlist_list_array);

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

	if (res != CURLE_OK) {
		fprintf(stderr, "Error occured while fetching data:\n");
		fprintf(stderr, "  %s\n", err_buff);
		return 1;
	}

	return 0;
}

int
get_json_data_array(CURL *handler, char *uri, json_object *item_list_array)
{
	struct json_object *parsed_json;
	struct json_object *uri_next_obj;
	struct json_object *current_data_array;
	int http_res;
	size_t nb_items;
	char *buffer = malloc(1);
	const char *uri_next = uri;

	while (uri_next != NULL) {
		buffer = malloc(1);
		http_res = http_request(handler, (char *)uri_next, &buffer);

		if (http_res)
			return 1;

		parsed_json = json_tokener_parse(buffer);

		if (parsed_json == NULL) {
			fprintf(stderr, "Error occured while parsing JSON\n");
			return 1;
		}

		json_object_object_get_ex(parsed_json, "next", &uri_next_obj);
		json_object_object_get_ex(parsed_json, "data", &current_data_array);

		nb_items = json_object_array_length(current_data_array);
		uri_next = json_object_get_string(uri_next_obj);

		for (size_t i=0; i < nb_items; i++) {
			json_object *item = json_object_array_get_idx(current_data_array, i);
			json_object_get(item);
			json_object_array_add(item_list_array, item);
		}

		free(buffer);
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

char *
uri_concat(char *path, char *token) {
	int uri_size = strlen(BASE_DEEZER_URI) + strlen(path) +
		strlen("?access_token=") + strlen(token);
	char *uri = malloc(sizeof(char) * (uri_size + 1));

	strncpy(uri, BASE_DEEZER_URI, strlen(BASE_DEEZER_URI));
	strncat(uri, path, strlen(path));
	strncat(uri, "?access_token=", strlen("?access_token="));
	strncat(uri, token, strlen(token));

	return uri;
}
