all: debug

debug:
	clang -g -Wall -Wextra -Werror -o 10hbackup 10hbackup.c -lcurl

debug-allow-warn:
	clang -g -Wall -Wextra -o 10hbackup 10hbackup.c -lcurl

release:
	clang -Wall -Wextra -Werror -o 10hbackup 10hbackup.c -lcurl
