# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy proxy_single_thread proxy_concurrent

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy_single_thread.o: proxy_single_thread.c csapp.h log.h
	$(CC) $(CFLAGS) -c proxy_single_thread.c

proxy_concurrent.o: proxy_concurrent.c csapp.h log.h
	$(CC) $(CFLAGS) -c proxy_concurrent.c

sbuf.o: sbuf.h
	$(CC) $(CFLAGS) -c sbuf.c
log.o: log.h
	$(CC) $(CFLAGS) -c log.c

proxy: proxy.o csapp.o sbuf.o log.o
	$(CC) $(CFLAGS) proxy.o csapp.o sbuf.o log.o -o proxy $(LDFLAGS)

proxy_single_thread: proxy_single_thread.o csapp.o log.o 
	$(CC) $(CFLAGS) proxy_single_thread.o csapp.o log.o -o proxy_single_thread $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

