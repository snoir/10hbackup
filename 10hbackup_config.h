typedef struct {
	char *key;
	char *value;
} config_key_value;

int
read_config(char *path, config_key_value **config);

char *
get_conf(config_key_value *config, int config_size, char *key);

void
free_config(config_key_value *config, int config_size);
