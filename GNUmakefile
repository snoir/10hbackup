PROG= 10hbackup
OBJS= 10hbackup.o 10hbackup_config.o
LDFLAGS= -lcurl -ljson-c -l git2
CC= clang

ifdef DEBUG
CFLAGS= -g -fsanitize=address -Wall -Wextra -Werror
else ifdef DEBUG_ALLOW_WARN
CFLAGS= -g
else
CFLAGS= -Wall -Wextra -Werror
endif

${PROG}: ${OBJS}
	${CC} -o ${PROG} ${OBJS} ${CFLAGS} ${LDFLAGS}

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	rm -f ${PROG} *.o
