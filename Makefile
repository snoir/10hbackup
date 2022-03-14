all: debug

debug:
	clang -g -Wall -Wextra -Werror -fsanitize=address -o 10hbackup 10hbackup.c \
		-lcurl -ljson-c -l git2

debug-allow-warn:
	clang -g -Wall -fsanitize=address -Wextra -o 10hbackup 10hbackup.c 10hbackup_config.c \
		-lcurl -ljson-c -l git2

release:
	clang -Wall -Wextra -Werror -o 10hbackup 10hbackup.c 10hbackup_config.c \
		-lcurl -ljson-c -l git2

