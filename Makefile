CC = gcc
INCLUDES =
CFLAGS = -Wall -ansi -pedantic ${INCLUDES}
LDFLAGS =
LIBS =

BIN = ledplay
OBJS = ledplay.o

all: ${BIN}

${BIN}: ${OBJS}
	${CC} -o $@ ${LDFLAGS} ${OBJS} ${LIBS}

.c.o:
	${CC} -c -o $@ ${CFLAGS} $<

clean:
	rm -f ${BIN} ${OBJS}
