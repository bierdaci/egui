CC = gcc

CFLAGS = -Wall -g -I../../
LDFLAGS = -L../.libs -lelib -lpthread
ALL: test

load: load.c
	$(CC) $(CFLAGS) -o load $^ $(LDFLAGS)

xml: xml.c
	$(CC) $(CFLAGS) -o xml $^ $(LDFLAGS)

test: test.c
	$(CC) $(CFLAGS) -o test $^ $(LDFLAGS)

clean:
	rm test
