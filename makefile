LDFLAGS += -L/usr/local/lib

CFLAGS: -Wall -Werror -Wextra -Wstrict-prototypes -pedantic -fno-common -O3 -g -std=gnu11

all: simple_message_client simple_message_server 

clean:
	rm simple_message_client.o simple_message_client simple_message_server.o simple_message_server

simple_message_client: simple_message_client.o 
	gcc -g -o simple_message_client simple_message_client.o -L/usr/local/lib -lsimple_message_client_commandline_handling

simple_message_client.o:
	gcc -c -g simple_message_client.c
	
simple_message_server: simple_message_server.o 
	gcc -g -o simple_message_server simple_message_server.o

simple_message_server.o:
	gcc -c -g simple_message_server.c
