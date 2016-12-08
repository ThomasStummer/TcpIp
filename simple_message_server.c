/*
 * simple_message_server.c
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
#define BACKLOG 10
#define SERVER_LOGIC_PATH "/usr/local/bin/simple_message_server_logic"
#define SERVER_LOGIC_FILE "simple_message_server_logic"

// Global variables
const char * programName;
const char * usageText = 	"usage: simple_message_server options\n"
							"options:\n"
							"\t-p, --port <port>	port of the server [0..65535]\n"
							"\t-h, --help\n";

// Prototypes
void PrintError(char * funcName, bool evalErrno, const char * message);
int ParseCommandLine(int argc, const char * const argv[], const char **port);
int CreateSignalHandler(void);
void SignalHandler(int signal);
int SpecifyAddrInfo(struct addrinfo * hints);
int CloseSocketDescriptor(int socketDescriptor);
int CreateAndBindListeningSocket(const char * port, int * socketDescriptor, struct addrinfo ** addrInfoResultsPtr);
int AcceptIncomingConnections(int socketDescriptor , struct addrinfo ** addrInfoResultsPtr);
int Spawn(int socketDescriptor, int acceptedSocketDescriptor);

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

int ParseCommandLine(int argc, const char * const argv[], const char **port)
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
            	return EXIT_FAILURE;
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
    return EXIT_SUCCESS;
}

int CreateSignalHandler()
{
	struct sigaction signalAction;

	signalAction.sa_handler = SignalHandler;
	sigemptyset(&signalAction.sa_mask);	// Clear all signals
	signalAction.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &signalAction, NULL) == -1)
	{
		PrintError("CreateSignalHandler() -> sigaction()", true, NULL);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void SignalHandler(int signal)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
	// -1 ... any child process
	// NULL ... we don't need detailed information from waitpid
	// WNNOHANG ... waitpid returns immediately if no child has exited
}

int SpecifyAddrInfo(struct addrinfo * hints)
{
	memset(hints, 0, sizeof(struct addrinfo));
	hints->ai_family = AF_INET;    		/* IPv4 */
	hints->ai_socktype = SOCK_STREAM;	/* TCP */
	hints->ai_flags = AI_PASSIVE;   	/* Server is passive */
	return EXIT_SUCCESS;
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

int CreateAndBindListeningSocket(const char * port, int * socketDescriptor, struct addrinfo ** addrInfoResultsPtr)
{
	struct addrinfo addrInfoSettings;
	struct addrinfo * addrInfoResults;
	int r=0;

	if(SpecifyAddrInfo(&addrInfoSettings) == EXIT_FAILURE)
	{
		PrintError("CreateAndBindSocket() -> SpecifyAddrInfo()", true, NULL);
		return EXIT_FAILURE;
	}

	r = getaddrinfo(NULL, port, &addrInfoSettings, &addrInfoResults);
	if(r != 0)
	{
		PrintError("CreateAndBindSocket() -> getaddrinfo()", false, gai_strerror(r));
		return EXIT_FAILURE;
	}

	for(*addrInfoResultsPtr = addrInfoResults;
		*addrInfoResultsPtr != NULL;
		*addrInfoResultsPtr = (*addrInfoResultsPtr)->ai_next)
	{
		*socketDescriptor = socket((*addrInfoResultsPtr)->ai_family,
									(*addrInfoResultsPtr)->ai_socktype,
									(*addrInfoResultsPtr)->ai_protocol);
		if(*socketDescriptor == -1)
		{
			continue;	// Try next
		}

		if(bind(*socketDescriptor,
				(*addrInfoResultsPtr)->ai_addr,
				(*addrInfoResultsPtr)->ai_addrlen) == 0)
		{
			break;	// Success
		}

		// bind() failed
		if(CloseSocketDescriptor(*socketDescriptor) == EXIT_FAILURE)
		{
			PrintError("CreateAndBindSocket() -> CloseSocketDescriptor()", true, NULL);
			return EXIT_FAILURE;
		}
	}

	if(*addrInfoResultsPtr == NULL)
	{
		// Wasn't able to bind any address
		PrintError("CreateAndBindSocket()", true, "Could not bind to any address");
		return EXIT_FAILURE;
	}

	freeaddrinfo(addrInfoResults);
	addrInfoResults = NULL;

	if(listen(*socketDescriptor, BACKLOG) == -1)
	{
		PrintError("CreateAndBindSocket() -> listen()", true, NULL);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int AcceptIncomingConnections(int socketDescriptor , struct addrinfo ** addrInfoResultsPtr)
{
	int acceptedSocketDescriptor = 0;

	for(;;)
	{
		// Accept incoming connection
		acceptedSocketDescriptor = accept(socketDescriptor, (*addrInfoResultsPtr)->ai_addr, &((*addrInfoResultsPtr)->ai_addrlen));
		if(acceptedSocketDescriptor == -1)
		{
			PrintError("AcceptIncomingConnections() -> accept()", true, NULL);
			return EXIT_FAILURE;
		}

		// Fork new process and execute business logic
		if(Spawn(socketDescriptor, acceptedSocketDescriptor) == EXIT_FAILURE)
		{
			PrintError("AcceptIncomingConnections() -> Spawn()", false, NULL);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int Spawn(int socketDescriptor, int acceptedSocketDescriptor)
{
	pid_t pid = -1;
	pid = fork();

	if(pid == -1)
	{
		PrintError("main() -> fork()", true, NULL);
		CloseSocketDescriptor(acceptedSocketDescriptor);
		CloseSocketDescriptor(socketDescriptor);
		return EXIT_FAILURE;
	}

	if(pid == 0) 	// Child process
	{
		// Close unneeded descriptor
		if (CloseSocketDescriptor(socketDescriptor) == EXIT_FAILURE)
		{
			PrintError("Child process: Spawn() -> CloseSocketDescriptor(socketDescriptor)", true, NULL);
			CloseSocketDescriptor(acceptedSocketDescriptor);
			_Exit(EXIT_FAILURE);
		}

		// Redirect stdin
		if (dup2(acceptedSocketDescriptor, STDIN_FILENO) == -1)
		{
			PrintError("Child process: Spawn() -> dup2()", true, NULL);
			CloseSocketDescriptor(acceptedSocketDescriptor);
			CloseSocketDescriptor(socketDescriptor);
			_Exit(EXIT_FAILURE);
		}

		// Redirect stdout
		if (dup2(acceptedSocketDescriptor, STDOUT_FILENO) == -1)
		{
			PrintError("Child process: Spawn() -> dup2()", true, NULL);
			CloseSocketDescriptor(acceptedSocketDescriptor);
			CloseSocketDescriptor(socketDescriptor);
			close(STDIN_FILENO);
			_Exit(EXIT_FAILURE);
		}

		// Close unneeded descriptor
		if (CloseSocketDescriptor(acceptedSocketDescriptor) == EXIT_FAILURE)
		{
			PrintError("Child process: Spawn() -> CloseSocketDescriptor(acceptedSocketDescriptor)", true, NULL);
			CloseSocketDescriptor(socketDescriptor);
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			_Exit(EXIT_FAILURE);
		}

		// Execute server logic program
		execl(SERVER_LOGIC_PATH, SERVER_LOGIC_FILE, (char *) NULL);

		// Child process is not allowed to reach this point
		PrintError("Spawn()", true, "Child process reached unallowed code region");
		_Exit(EXIT_FAILURE);
	}

	// No error and not child process -> parent process -> close unneeded descriptor
	if (close(acceptedSocketDescriptor) == -1)
	{
		PrintError("main() -> close()", true, NULL);
		return EXIT_FAILURE;
	}
}

int main(int argc, const char * const argv[])
{
	struct addrinfo * addrInfoResultsPtr;
	int socketDescriptor = 0;
	const struct sockaddr sockAddr;
	const char * port;

	programName = argv[0];

	// ParseArguments
	if(ParseCommandLine(argc, argv, &port) == EXIT_FAILURE)
	{
		PrintError("main() -> ParseCommandLine()", false, NULL);
		return EXIT_FAILURE;
	}

	// Create signal handler to prevent child processes from becoming zombie processes
	if(CreateSignalHandler() == EXIT_FAILURE)
	{
		PrintError("main() -> CreateSignalHandler()", false, NULL);
		return EXIT_FAILURE;
	}

	// Set up and bind socket
	if(CreateAndBindListeningSocket(port, &socketDescriptor, &addrInfoResultsPtr) == EXIT_FAILURE)
	{
		PrintError("main() -> CreateAndBindSocket()", false, NULL);
		return EXIT_FAILURE;
	}

	// Accept incoming connections
	if(AcceptIncomingConnections(socketDescriptor, &addrInfoResultsPtr) == EXIT_FAILURE)
	{
		PrintError("main() -> AcceptIncomingConnections()", false, NULL);
		return EXIT_FAILURE;
	}

	// Close socket descriptor
	if(CloseSocketDescriptor(socketDescriptor) == EXIT_FAILURE)
	{
		PrintError("main() -> CloseSocketDescriptor(socketDescriptor)", true, NULL);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
