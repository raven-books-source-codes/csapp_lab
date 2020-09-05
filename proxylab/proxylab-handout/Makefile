# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

cache.o: cache.h cache.c
	$(CC) $(CFLAGS) -c cache.c

sbuf.o: sbuf.h sbuf.c
	$(CC) $(CFLAGS) -c sbuf.c

log.o: log.h
	$(CC) $(CFLAGS) -c log.c

proxy: proxy.o csapp.o sbuf.o log.o cache.o
	$(CC) $(CFLAGS) proxy.o csapp.o sbuf.o cache.o log.o -o proxy $(LDFLAGS)

test: test.c log.o csapp.o cache.o
	$(CC) $(CFLAGS) log.o csapp.o cache.o test.c -o test $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

