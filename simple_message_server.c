/*
 * server.c
 *
 *  Created on: 28.11.2016
 *      Author: thomas
 */


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


#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define BACKLOG 10
#define SERVER_LOGIC_PATH "/usr/local/bin/simple_message_server_logic"
#define SERVER_LOGIC_FILE "simple_message_server_logic"

const char * programName;
const char * usageText = 	"usage: simple_message_server options\n"
							"options:\n"
							"\t-p, --port <port>	well-known port of the server [0..65535]\n"
							"\t-h, --help\n";

void PrintError(char * funcName, bool evalErrno, const char * message);
void SpecifyAddrInfo(struct addrinfo * hints);
// Adopted from file simple_message_client_commandline_handling.c from (R) Prof.
int smc_parsecommandline(
    int argc,
    const char * const argv[],
    const char **port
    );

int smc_parsecommandline(
    int argc,
    const char * const argv[],
    const char **port
    )
{
    int c;

    *port = NULL;

    struct option long_options[] =
    {
        {"port", 1, NULL, 'p'},
        {0, 0, 0, 0}
    };

    while (
        (c = getopt_long(
             argc,
             (char ** const) argv,
             "p:h",
             long_options,
             NULL
             )
            ) != -1
        )
    {
        switch (c)
        {
            case 'p':
                *port = optarg;
                break;

            case 'h':
            	fprintf(stdout, usageText);
            	return EXIT_SUCCESS;
                break;

            case '?':
            default:
            	fprintf(stderr, usageText);
                return EXIT_FAILURE;
                break;
        }
    }

    if ( (optind != argc) || (*port == NULL) )
    {
    	fprintf(stderr, usageText);
    	return EXIT_FAILURE;
    }
}

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

void SpecifyAddrInfo(struct addrinfo * hints)
{
	memset(hints, 0, sizeof(struct addrinfo));
	hints->ai_family = AF_INET;    		/* IPv4 */
	hints->ai_socktype = SOCK_STREAM;	/* TCP */
	hints->ai_flags = AI_PASSIVE;   	/* Server is passive */
}

int CloseConnection(int socketDescriptor)
{
	if(socketDescriptor != -1)
	{
		if(close(socketDescriptor) == -1)
		{
			PrintError("main() -> close()", true, NULL);
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}
	else
	{
		return EXIT_FAILURE;
	}
}

// Test Source File
int main(int argc, const char * const argv[])
{
	struct addrinfo addrInfoSettings;
	struct addrinfo * addrInfoResults, * addrInfoResultsPtr;
	int socketDescriptor = 0, acceptedSocketDescriptor = 0;
	const struct sockaddr sockAddr;
	const char * port;
	int r=0;
	pid_t pid = -1;

	programName = argv[0];

	// ParseArguments
	//ParseArguments();

	if(smc_parsecommandline(argc, argv, &port) == EXIT_FAILURE)
	{
		return EXIT_FAILURE; 	//  Fallunterscheidung zw. failure und ok return der Func fehlt!!
	}

	// Open passive connection

	SpecifyAddrInfo(&addrInfoSettings);


	r = getaddrinfo(NULL, port, &addrInfoSettings, &addrInfoResults);
	if(r != 0)
	{
		PrintError("main() -> getaddrinfo()", false, gai_strerror(r));
		return EXIT_FAILURE;
	}

	for(addrInfoResultsPtr = addrInfoResults;
		addrInfoResultsPtr != NULL;
		addrInfoResultsPtr = addrInfoResultsPtr->ai_next)
	{


		socketDescriptor = socket(addrInfoResultsPtr->ai_family,
									addrInfoResultsPtr->ai_socktype,
									addrInfoResultsPtr->ai_protocol);
		if(socketDescriptor == -1)
		{
			continue;	// Try next
		}

		if(bind(socketDescriptor, addrInfoResultsPtr->ai_addr, addrInfoResultsPtr->ai_addrlen) == 0)
		{
			break;	// Success
		}

		// bind() failed
		if(CloseConnection(socketDescriptor) == EXIT_FAILURE)
		{
			PrintError("main() -> CloseConnection()", true, NULL);
			return EXIT_FAILURE;
		}
	}

	if(addrInfoResultsPtr == NULL)
	{
		// Wasn't able to bind any address
		PrintError("main()", true, "Could not bind to any address");
		return EXIT_FAILURE;
	}

	freeaddrinfo(addrInfoResults);


	if(listen(socketDescriptor, BACKLOG) == -1)
	{
		PrintError("main() -> listen()", true, NULL);
		return EXIT_FAILURE;
	}

	// accept in schleife
	for(;;)
	{
		acceptedSocketDescriptor = accept(socketDescriptor, addrInfoResultsPtr->ai_addr, &(addrInfoResultsPtr->ai_addrlen));
		if(acceptedSocketDescriptor == -1)
		{
			PrintError("main() -> accept()", true, NULL);
			return EXIT_FAILURE;
		}



		// spawn()
		pid = fork();

		if(pid == -1)
		{
			PrintError("main() -> fork()", true, NULL);
			CloseConnection(socketDescriptor);
			return EXIT_FAILURE;
		}

		if(pid == 0) 	// Child process
		{
			// Close unneeded descriptor
			if (close(acceptedSocketDescriptor) == -1)
			{
				PrintError("Child process: main() -> close()", true, NULL);
				CloseConnection(socketDescriptor);
				_Exit(EXIT_FAILURE);
			}

			// Redirect stdin
			if (dup2(socketDescriptor, STDIN_FILENO) == -1)
			{
				PrintError("Child process: main() -> dup2()", true, NULL);
				CloseConnection(socketDescriptor);
				_Exit(EXIT_FAILURE);
			}

			// Redirect stdout
			if (dup2(socketDescriptor, STDOUT_FILENO) == -1)
			{
				PrintError("Child process: main() -> dup2()", true, NULL);
				CloseConnection(socketDescriptor);
				close(STDIN_FILENO); // return Wert checken!
				_Exit(EXIT_FAILURE);
			}

			// Close unneeded descriptor
			if (close(socketDescriptor) == -1)
			{
				PrintError("Child process: main() -> dup2()", true, NULL);
				CloseConnection(socketDescriptor);
				close(STDIN_FILENO); // return Wert checken!
				close(STDOUT_FILENO); // return Wert checken!
				_Exit(EXIT_FAILURE);
			}

			// Execute server logic program
			execl(SERVER_LOGIC_PATH, SERVER_LOGIC_FILE, (char *) NULL);
			// Child process is not allowed to reach this point
			PrintError("main()", true, "Child process reached unallowed source region");
			_Exit(EXIT_FAILURE);
		}

		// No error and not child process -> parent process

		// Close connection

		if(CloseConnection(socketDescriptor) == EXIT_FAILURE)
		{
			return EXIT_FAILURE;
		}

	}

	// Close unneeded descriptor
	if (close(acceptedSocketDescriptor) == -1)
	{
		PrintError("main() -> close()", true, NULL);
		return EXIT_FAILURE;
	}

	printf("ServerFunc main() exits now\n");
	return EXIT_SUCCESS;
}
