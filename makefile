all: client

clean:
	rm client.o client

client: client.o
	gcc -g -o client client.o

client.o:
	gcc -c -g client.c