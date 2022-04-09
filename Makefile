PROG= 10hbackup
OBJS= 10hbackup.o 10hbackup_config.o
CFLAGS= -Wall -Wextra -Werror
LDFLAGS= -lcurl -ljson-c -l git2
CC_ARGS= -o ${PROG} ${OBJS} ${CFLAGS} ${LDFLAGS}
DEBUG_ARGS= -g -fsanitize=address
CC= clang

${PROG}: ${OBJS}
	${CC} ${CC_ARGS}

debug: ${OBJS}
	${CC} ${DEBUG_ARGS} ${CC_ARGS}

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	rm -f ${PROG} *.o
