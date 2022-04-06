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
	${CC} ${CC_ARGS} ${DEBUG_ARGS}

.c.o:
	${CC} -c $<

clean:
	rm -f ${PROG} *.o
