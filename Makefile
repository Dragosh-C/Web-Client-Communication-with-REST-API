CC=gcc
CFLAGS=-I.

client: client.c include/requests.c include/helpers.c include/buffer.c include/parson.c
	$(CC) -o client client.c include/requests.c include/helpers.c include/buffer.c include/parson.c -Wall

run: client
	./client

clean:
	rm -f *.o client
