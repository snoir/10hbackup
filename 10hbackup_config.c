#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char *key;
	char *value;
} config_key_value;

int
read_config(char *path, config_key_value **config);

char *
get_conf(config_key_value *config, int config_size, char *key);

int
main() {
	int config_size;
	char *output_dir, *value;
	config_key_value *config;

	config_size = read_config("config", &config);

	value = get_conf(config, config_size, "output_dir");
}

int
read_config(char *path, config_key_value **config) {
	char line[1024];
	char *key, *value;
	FILE *fd;
	int nb_elem = 0, value_size, key_size;
	config_key_value *config_ptr;

	fd = fopen(path, "r");
	*config = malloc(0);

	while(fgets(line, sizeof(line), fd)) {
		config_ptr = realloc(*config, (nb_elem + 1) * sizeof(config_key_value));
		*config = config_ptr;
		key = strtok(line, "=");
		value = strtok(NULL, "=");
		value[strlen(value) - 1] = '\0';
		value_size = strlen(value);
		key_size = strlen(key);

		config_ptr[nb_elem].key = malloc(key_size);
		config_ptr[nb_elem].value = malloc(value_size);
		strncpy(config_ptr[nb_elem].key, key, key_size);
		strncpy(config_ptr[nb_elem].value, value, value_size);

		nb_elem++;
	}

	return (nb_elem);
}

char *
get_conf(config_key_value *config, int config_size, char *key) {
	int config_key_size = strlen(key);
	char *res = NULL;

	for (int i = 0; i < config_size; i++) {
		if (strncmp(config[i].key, key, config_key_size) == 0) {
			res = config[i].value;
		}
	}

	return res;
}
