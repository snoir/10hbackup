all: debug

debug:
	clang -g -Wall -Wextra -Werror -o 10hbackup 10hbackup.c -lcurl -ljson-c

debug-allow-warn:
	clang -g -Wall -Wextra -o 10hbackup 10hbackup.c -lcurl -ljson-c

release:
	clang -Wall -Wextra -Werror -o 10hbackup 10hbackup.c -lcurl -ljson-c
