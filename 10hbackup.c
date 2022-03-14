#include <curl/curl.h>
#include <git2.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include "10hbackup_config.h"

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

char *
uri_add_token(char *uri, char *token);

int
write_json_to_file(json_object *json_data, char *filename);

int
get_category(char* category, CURL *curl, char *token, char *output_dir, struct request_count *requests_counting);

int
git_add_and_commit(char *output_dir);

int
main(int argc, char *argv[])
{
	char *token = NULL, *output_dir = NULL, *config_file = NULL;
	char *categories[] = {"albums", "playlists"};
	int res = EXIT_SUCCESS, ch, config_size;
	struct request_count requests_counting;
	config_key_value *config;

	while ((ch = getopt(argc, argv, "c:")) != -1) {
		switch (ch) {
		case 'c':
			config_file = malloc(sizeof(char) * strlen(optarg) + 1);
			strncpy(config_file, optarg, strlen(optarg) + 1);
			break;
		case '?':
		default:
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	config_size = read_config(config_file, &config);
	output_dir = get_conf(config, config_size, "output_dir");
	token = get_conf(config, config_size, "token");

	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();

	requests_counting.nb = 0;
	requests_counting.ts = (int)time(NULL);

	for (int i = 0; i < (int)(sizeof(categories) / sizeof(categories[0])); i++) {
		res = get_category(categories[i], curl, token, output_dir, &requests_counting);
	}

	res = git_add_and_commit(output_dir);

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
	char *buffer;
	const char *uri_next = uri;

	while (uri_next != NULL) {
		buffer = malloc(1);
		if (requests_counting->nb == 50) {
			ts_diff = difftime((int)time(NULL), requests_counting->ts);
			if (ts_diff < 6)
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

			if (strcmp(json_object_get_string(error_type), "DataException") == 0) {
				free(buffer);
				return (1);
			} else {
				free(buffer);
				return (-1);
			}
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

	json_object_put(parsed_json);

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
	int uri_size = strlen(BASE_DEEZER_URI) + strlen(path);
	char *uri = malloc(sizeof(char) * (uri_size + 1));
	char *full_uri;

	strncpy(uri, BASE_DEEZER_URI, strlen(BASE_DEEZER_URI) + 1);
	strncat(uri, path, strlen(path) + 1);
	full_uri = uri_add_token(uri, token);

	free(uri);

	return (full_uri);
}

char *
uri_add_token(char *uri, char *token) {
	int full_uri_size = strlen(uri) + strlen(token) +
		strlen("?access_token=");
	char *full_uri = malloc(sizeof(char) * (full_uri_size +1));

	strncpy(full_uri, uri, strlen(uri) + 1);
	strncat(full_uri, "?access_token=", strlen("?access_token=") + 1);
	strncat(full_uri, token, strlen(token) + 1);

	return (full_uri);
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
get_category(char* category, CURL *curl, char *token, char *output_dir, struct request_count *requests_counting)
{
	json_object *category_item_list_array = json_object_new_array();
	json_object *category_item_array;
	json_object *item, *item_uri_obj, *item_id_obj;
	char *file_path, *category_uri, *item_id_str, *item_full_uri;
	char category_output_dir[strlen(output_dir) + strlen(category) + 2];
	char category_path[strlen("/user/me/") + strlen(category) + 1];
	const char *item_uri;
	int res, nb_items, item_id_str_len, file_path_len;
	unsigned long int item_id;

	strncpy(category_path, "/user/me/", strlen("/user/me/") + 1);
	strncat(category_path, category, strlen(category) + 1);
	category_uri = uri_concat(category_path, token);
	snprintf(category_output_dir, strlen(output_dir) + strlen(category) + 2,
			"%s/%s", output_dir, category);
	res = get_json_data_array(curl, category_uri, category_item_list_array, requests_counting);
	if (res == -1) {
		return (-1);
	}

	res = mkdir(category_output_dir, 0755);
	free(category_uri);

	nb_items = json_object_array_length(category_item_list_array);

	for (int i = 0; i < nb_items; i++) {
		category_item_array = json_object_new_array();
		item = json_object_array_get_idx(category_item_list_array, i);
		json_object_object_get_ex(item, "id", &item_id_obj);
		json_object_object_get_ex(item, "tracklist", &item_uri_obj);
		item_id = json_object_get_uint64(item_id_obj);
		item_uri = json_object_get_string(item_uri_obj);
		item_full_uri = uri_add_token((char *)item_uri, token);
		item_id_str_len = snprintf(NULL, 0, "%lu", item_id) + 1;
		item_id_str = malloc(item_id_str_len + 1);
		snprintf(item_id_str, item_id_str_len, "%lu", item_id);
		file_path_len = strlen(category_output_dir) + item_id_str_len +
			strlen(".json") + 2;
		file_path = malloc(file_path_len);
		snprintf(file_path, file_path_len, "%s/%s%s", category_output_dir, item_id_str, ".json");

		fprintf(stdout, "[%s] Fetching item '%s'\n", category, item_id_str);
		res = get_json_data_array(curl, item_full_uri, category_item_array, requests_counting);
		if (res == -1) {
			return (-1);
		}

		/* No data */
		/* TODO: remove code duplication */
		if (res == 1) {
			json_object_put(category_item_array);
			free(item_full_uri);
			free(item_id_str);
			free(file_path);
			continue;
		}

		res = write_json_to_file(category_item_array, file_path);
		if (res == -1) {
			return (-1);
		}

		json_object_put(category_item_array);
		free(item_full_uri);
		free(item_id_str);
		free(file_path);
	}

	file_path = malloc(strlen(output_dir) + strlen(category) +
			strlen(".json") + 2);
	snprintf(file_path, strlen(output_dir) + strlen(category) + strlen("json") + 3,
			"%s/%s.%s", output_dir, category, "json");

	res = write_json_to_file(category_item_list_array, file_path);

	free(file_path);
	json_object_put(category_item_list_array);
	if (res == -1) {
		return (-1);
	}

	return 0;
}

int
git_add_and_commit(char *output_dir) {
	int res = 0, git_res = 0;
	char commit_message[50], date_rfc2822[32];
	char *git_path[] = {"."};
	time_t timestamp;
	git_repository *repo = NULL;
	git_index *idx = NULL;
	git_oid new_tree_id, new_commit_id;
	git_tree *tree = NULL;
	git_strarray git_arr = {git_path, 1};
	git_signature *me = NULL;
	git_object *parent = NULL;
	git_reference *ref = NULL;

	git_libgit2_init();

	if (git_repository_open_ext(
	    NULL, output_dir, GIT_REPOSITORY_OPEN_NO_SEARCH, NULL) == 0) {
		git_res = git_repository_open(&repo, output_dir);
	} else {
		git_res = git_repository_init(&repo, output_dir, false);
	}

	if (git_res != 0) {
		goto cleanup;
	}

	git_res = git_signature_now(&me, "Me", "me@example.com");
	if (git_res != 0) {
		goto cleanup;
	}

	git_res = git_revparse_ext(&parent, &ref, repo, "HEAD");
	if (git_res == GIT_ENOTFOUND) {
		git_res = 0;
	} else if (git_res != 0) {
		goto cleanup;
	}

	git_res = git_repository_index(&idx, repo);
	if (git_res != 0) {
		goto cleanup;
	}

	git_res = git_index_add_all(idx, &git_arr, 0, NULL, 0);
	if (git_res != 0) {
		goto cleanup;
	}

	git_res = git_index_write(idx);
	if (git_res != 0) {
		goto cleanup;
	}

	git_res = git_index_write_tree(&new_tree_id, idx);
	if (git_res != 0) {
		goto cleanup;
	}

	git_res = git_tree_lookup(&tree, repo, &new_tree_id);
	if (git_res != 0) {
		goto cleanup;
	}
	timestamp = time(NULL);
	strftime(date_rfc2822, 32, "%a, %d %b %Y %T %z", localtime(&timestamp));
	snprintf(commit_message, 50, "Deezer export at %s", date_rfc2822);

	git_res = git_commit_create_v(
			&new_commit_id,
			repo,
			"HEAD",
			me,
			me,
			"UTF-8",
			commit_message,
			tree,
			parent ? 1 : 0, parent);

cleanup:
	if (git_res != 0) {
		const git_error *e = git_error_last();
		fprintf(stderr, "Git error %d/%d:\n", res, e->klass);
		fprintf(stderr, "  %s\n", e->message);
		res = -1;
	}

	git_signature_free(me);
	git_object_free(parent);
	git_reference_free(ref);
	git_tree_free(tree);
	git_index_free(idx);
	git_repository_free(repo);
	git_libgit2_shutdown();

	return (res);
}
