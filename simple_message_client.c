/*
 * simple_message_client.c
 *
 *  VCS - TCP/IP
 *  BIC
 *  3A1
 *
 *  Authors: ic15b008 Matula Patrick
 *    		 ic15b079 Stummer Thomas
 */

// Includes
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>

// Defines
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

// Global variables
const char * programName;

// Prototypes
void PrintError(char * funcName, bool evalErrno, const char * message);
int CloseSocketDescriptor(int socketDescriptor);

// Functions

void PrintError(char * funcName, bool evalErrno, const char * message)
{
	fprintf(stderr, "Error in program ");
	fprintf(stderr, programName);
	fprintf(stderr, ": function ");
	fprintf(stderr, funcName);

	if(message != NULL)
	{
		fprintf(stderr, ": ");
		fprintf(stderr, message);
	}

	if(evalErrno)
	{
		fprintf(stderr, ": ");
		fprintf(stderr, strerror(errno));
	}

	fprintf(stderr, "\n");
}

int CloseSocketDescriptor(int socketDescriptor)
{
	if(socketDescriptor != -1)
	{
		if(close(socketDescriptor) == -1)
		{
			PrintError("CloseSocketDescriptor() -> close()", true, NULL);
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int main(int argc, char * argv[])
{
	printf("Hi I'm a client!");
}
