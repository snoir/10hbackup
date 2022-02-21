all: debug

debug:
	clang -g -Wall -Wextra -Werror -o 10hbackup 10hbackup.c -lcurl -ljson-c \
		-l git2

debug-allow-warn:
	clang -g -Wall -Wextra -o 10hbackup 10hbackup.c -lcurl -ljson-c \
		-l git2


release:
	clang -Wall -Wextra -Werror -o 10hbackup 10hbackup.c -lcurl -ljson-c \
		-l git2

