#!/usr/bin/make -f

TARGET=tracie.so
OBJS=tracie.o

CFLAGS=-fPIC
LDFLAGS=-lc -ldl -Wl,-soname,${TARGET}

all: ${TARGET}

clean:
	rm -rf *.o ${TARGET}

${TARGET}: ${OBJS}
	${CC} -shared $< ${LDFLAGS} -o $@
