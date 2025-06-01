all:
	mkdir -p bin
	gcc -Wall -c common.c -o common.o
	gcc -Wall client.c common.o -o bin/client
	gcc -Wall server.c common.o -o bin/server
	gcc -Wall server-mt.c common.o -lpthread -o bin/server-mt

clean:
	rm -f common.o bin/client bin/server bin/server-mt
	rmdir bin || true