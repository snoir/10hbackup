PROG= 10hbackup
OBJS= 10hbackup.o 10hbackup_config.o
LDFLAGS= -lcurl -ljson-c -lgit2
CC= clang
OS!= uname

.ifdef DEBUG
CFLAGS= -g -fsanitize=address -Wall -Wextra -Werror
.elifdef DEBUG_ALLOW_WARN
CFLAGS= -g
.else
CFLAGS= -Wall -Wextra -Werror
.endif

.if ${OS} == "FreeBSD"
CFLAGS+= -I/usr/local/include
LDFLAGS+= -L/usr/local/lib
.endif

${PROG}: ${OBJS}
	${CC} -o ${PROG} ${OBJS} ${CFLAGS} ${LDFLAGS}

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	rm -f ${PROG} *.o
