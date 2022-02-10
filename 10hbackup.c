#include <curl/curl.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BASE_DEEZER_URI "https://api.deezer.com"

struct http_data {
	size_t size;
	char **data;
};

struct request_count {
	int nb;
	int ts;
};

size_t
deezer_callback(char *data, size_t size, size_t nmemb, struct http_data *userdata);

int
http_request(CURL *handler, char *uri, char **buffer);

int
get_json_data_array(CURL *handler, char *token, json_object *item_list_array, struct request_count *requests_counting);

char *
uri_concat(char *path, char *token);

void
uri_add_token(char *uri, char *token);

int
write_json_to_file(json_object *json_data, char *filename);

int
get_playlists(CURL *curl, char *token, char *output_dir, struct request_count *requests_counting);

int
main(int argc, char *argv[])
{
	char *token = NULL, *output_dir = NULL;
	int res, ch;
	struct request_count requests_counting;

	res = EXIT_SUCCESS;

	while ((ch = getopt(argc, argv, "d:t:")) != -1) {
		switch (ch) {
		case 'd':
			output_dir = malloc(sizeof(char) * strlen(optarg) + 1);
			strncpy(output_dir, optarg, strlen(optarg) + 1);
			break;
		case 't':
			token = malloc(sizeof(char) * strlen(optarg) + 1);
			strncpy(token, optarg, strlen(optarg) + 1);
			break;
		case '?':
		default:
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	if (token == NULL || output_dir == NULL) {
		fprintf(stderr, "Both -t and -d options are mandatory\n");
		exit(EXIT_FAILURE);
	}

	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();

	requests_counting.nb = 0;
	requests_counting.ts = (int)time(NULL);

	res = get_playlists(curl, token, output_dir, &requests_counting);

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	free(token);
	free(output_dir);

	return (res);
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
		return (-1);
	}

	*(*buffer + data.size) = '\0';

	return (0);
}

int
get_json_data_array(CURL *handler, char *uri, json_object *item_list_array, struct request_count *requests_counting)
{
	json_object *parsed_json;
	json_object *uri_next_obj;
	json_object *current_data_array;
	json_object *deezer_error;
	int http_res, ts_diff;
	size_t nb_items;
	char *buffer = malloc(1);
	const char *uri_next = uri;

	while (uri_next != NULL) {
		buffer = malloc(1);
		if (requests_counting->nb == 50) {
			ts_diff = difftime((int)time(NULL), requests_counting->ts);
			sleep(6 - ts_diff);
			requests_counting->nb = 0;
			requests_counting->ts = (int)time(NULL);
		}
		http_res = http_request(handler, (char *)uri_next, &buffer);
		requests_counting->nb++;

		if (http_res == -1)
			return (-1);

		parsed_json = json_tokener_parse(buffer);

		if (parsed_json == NULL) {
			fprintf(stderr, "Error occured while parsing JSON\n");
			return (-1);
		}

		json_object_object_get_ex(parsed_json, "error", &deezer_error);

		if (deezer_error != NULL) {
			json_object *error_type = json_object_object_get(deezer_error, "type");
			json_object *error_message = json_object_object_get(deezer_error, "message");
			fprintf(stderr, "%s Deezer error:\n",
					json_object_get_string(error_type));
			fprintf(stderr, "  %s\n", json_object_get_string(error_message));
			return (-1);
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

	return (0);
}

size_t
deezer_callback(char *data, size_t size, size_t nmemb, struct http_data *userdata) {
	int realsize = size * nmemb;

	char *data_ptr = realloc(*userdata->data, userdata->size + realsize + 1);
	*userdata->data = data_ptr;

	memcpy(&data_ptr[userdata->size], data, realsize);
	userdata->size += realsize;

	return (realsize);
}

char *
uri_concat(char *path, char *token) {
	int uri_size = strlen(BASE_DEEZER_URI) + strlen(path) +
		strlen("?access_token=") + strlen(token);
	char *uri = malloc(sizeof(char) * (uri_size + 1));

	strncpy(uri, BASE_DEEZER_URI, strlen(BASE_DEEZER_URI) + 1);
	strncat(uri, path, strlen(path) + 1);
	uri_add_token(uri, token);

	return (uri);
}

void
uri_add_token(char *uri, char *token) {
	strncat(uri, "?access_token=", strlen("?access_token="));
	strncat(uri, token, strlen(token));
}

int
write_json_to_file(json_object *json_data, char *filename)
{
	FILE *json_file;
	const char *json_data_string;

	json_data_string = json_object_to_json_string_ext(json_data, JSON_C_TO_STRING_PRETTY);
	json_file = fopen(filename, "w+");

	if (json_file == NULL) {
		fprintf(stderr, "File '%s' could not be opened", filename);
		return (-1);
	}

	if (ferror(json_file)) {
		fprintf(stderr, "Error while writing to '%s'", filename);
		return (-1);
	}

	fprintf(json_file, "%s", json_data_string);
	fclose(json_file);

	return (0);
}

int
get_playlists(CURL *curl, char *token, char *output_dir, struct request_count *requests_counting)
{
	json_object *playlist_list_array = json_object_new_array();
	json_object *playlist_array = json_object_new_array();
	json_object *playlist_item, *playlist_uri_obj, *playlist_id_obj;
	char *file_path, *playlists_uri, *playlist_id_str, *playlist_full_uri;
	char playlists_output_dir[strlen(output_dir) + strlen("playlists") + 2];
	const char *playlist_uri;
	int res, nb_playlists, playlist_id_str_len;
	unsigned long int playlist_id;

	playlists_uri = uri_concat("/user/me/playlists", token);
	sprintf(playlists_output_dir, "%s/%s", output_dir, "playlists");
	res = get_json_data_array(curl, playlists_uri, playlist_list_array, requests_counting);
	if (res == -1) {
		return (-1);
	}

	res = mkdir(playlists_output_dir, 0755);

	nb_playlists = json_object_array_length(playlist_list_array);

	for (int i = 0; i < nb_playlists; i++) {
		playlist_item = json_object_array_get_idx(playlist_list_array, i);
		json_object_object_get_ex(playlist_item, "id", &playlist_id_obj);
		json_object_object_get_ex(playlist_item, "tracklist", &playlist_uri_obj);
		playlist_id = json_object_get_uint64(playlist_id_obj);
		playlist_uri = json_object_get_string(playlist_uri_obj);
		playlist_full_uri = malloc(strlen(playlist_uri) + strlen(token) + 1);
		strcpy(playlist_full_uri, playlist_uri);
		uri_add_token(playlist_full_uri, token);
		playlist_id_str_len = snprintf(NULL, 0, "%lu", playlist_id) + 1;
		playlist_id_str = malloc(playlist_id_str_len + 1);
		snprintf(playlist_id_str, playlist_id_str_len, "%lu", playlist_id);
		file_path = malloc(strlen(playlists_output_dir) + playlist_id_str_len +
				strlen(".json") + 2);
		sprintf(file_path, "%s/%s%s", playlists_output_dir, playlist_id_str, ".json");

		res = get_json_data_array(curl, playlist_full_uri, playlist_array, requests_counting);
		if (res == -1) {
			return (-1);
		}

		res = write_json_to_file(playlist_array, file_path);
		if (res == -1) {
			return (-1);
		}

		free(playlist_full_uri);
		free(file_path);
	}

	file_path = malloc(strlen(output_dir) +
			strlen("playlists.json") + 2);
	sprintf(file_path, "%s/%s", output_dir, "playlists.json");

	res = write_json_to_file(playlist_list_array, file_path);
	if (res == -1) {
		return (-1);
	}

	return 0;
}
