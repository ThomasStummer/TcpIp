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


#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

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
	memset(&hints, 0, sizeof(struct addrinfo));
	hints->ai_family = AF_INET;    		/* IPv4 */
	hints->ai_socktype = SOCK_STREAM;	/* TCP */
	hints->ai_flags = AI_PASSIVE;   	/* Server is passive */
	hints->ai_protocol = 0;         	/* Any protocol */
	hints->ai_canonname = NULL;
	hints->ai_addr = NULL;
	hints->ai_next = NULL;
}

// Test Source File
int main(int argc, const char * const argv[])
{
	struct addrinfo addrInfoSettings;
	struct addrinfo * addrInfoResults, * addrInfoResultsPtr;
	int socketDescriptor = 0;
	const struct sockaddr sockAddr;
	const char * port;
	int r=0;

	programName = argv[0];

	// ParseArguments
	//ParseArguments();

	if(smc_parsecommandline(argc, argv, &port) == EXIT_FAILURE)
	{
		return EXIT_FAILURE; 	//  Fallunterscheidung zw. failure und ok return der Func fehlt!!
	}
printf("%s", port);
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


		socketDescriptor = socket(addrInfoSettings.ai_family,
								  addrInfoSettings.ai_socktype,
								  addrInfoSettings.ai_protocol);
		if(socketDescriptor == -1)
		{
			continue;	// Try next
		}

		if(bind(socketDescriptor, &sockAddr, sizeof(addrInfoSettings)) == 0)
		{
			break;	// Success
		}

		close(socketDescriptor); 	// bind() failed
	}

	if(addrInfoResultsPtr == NULL)
	{
		// Wasn't able to bind any address
		PrintError("main()", true, "Could not bind to any address");
		return EXIT_FAILURE;
	}

	freeaddrinfo(addrInfoResults);

	/*
	listen();

	accept();

	bind();

	// spawn()

	fork();

	execute();


	read();

	write();


	read();
	// Close connection
	close();

*/

	close(socketDescriptor);	// for testing

	printf("ServerFunc main() exits now");
}
