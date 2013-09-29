CC = gcc

CFLAGS = -Wall -g -I ../
LDFLAGS = -L ../elib/.libs -L ../egal/.libs -L ../egui/.libs -legal -lelib -legui -lfreetype -lXrender -lpthread
all:server client register msg

msg:logmsg.o share.o usock.o msg-client.o msg.o
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS}

client:client.o share.o logmsg.o thread.o usock.o
	${CC} $^ -o $@ -lpthread

server:server.c share.o logmsg.o thread.o xml.o
	${CC} $^ -o $@ -lpthread

register:register.c share.o logmsg.o thread.o usock.o
	${CC} $^ -o $@ -lpthread

share.o:share.c share.h logmsg.h
	${CC} ${CFLAGS} $< -c

usock.o:usock.c usock.h share.h logmsg.h
	${CC} ${CFLAGS} $< -c

logmsg.o:logmsg.c logmsg.h
	${CC} ${CFLAGS} $< -c

msg-client.o:msg-client.c common.h
	${CC} ${CFLAGS} $< -c

client.o:client.c common.h
	${CC} ${CFLAGS} $< -c

thread.o:thread.c thread.h
	${CC} ${CFLAGS} $< -c

xml.o:xml.c xml.h list.h
	${CC} ${CFLAGS} $< -c

msg.o:msg.c
	${CC} ${CFLAGS} $< -c

clean:
	rm -f *.o msg server client register

