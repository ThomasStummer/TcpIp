/*
 * @file simple_message_client.c
 * Verteilte Systeme - TCP/IP
 * @author Thomas Stummer <ic15b079@technikum-wien.at>
 * @author Patrick Matula <ic15b008@technikum-wien.at>
 * @date 2016/12/10
 * @version 1.0
 */

/*
 * -------------------------------------------------------------- includes --
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "/usr/local/include/simple_message_client_commandline_handling.h"

/*
 * --------------------------------------------------------------- defines --
 */

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/*
 * --------------------------------------------------------------- globals --
 */

const char * programName;
int verbose = false;

/*
 * ------------------------------------------------------------- prototypes --
 */

void printError(char * funcName, bool evalErrno, const char * message);
void usagefunc(FILE *outputStream, const char *programName, int exitCode);
void initSocketAndConnect(const char *server, const char *port, int *sfd);
void sendMessage(int sfd, const char* user, const char* message, const char* img_url);
int readResponse(int sfd);
void verboseOutput(const char* text);

/*
 * ------------------------------------------------------------- functions --
 */

/**
 *
 * \brief Function for printing an error message
 *
 * \param funcName the name of the function that throws an error
 * \param evalErrno for specifying if the error number shall be printed out
 * \param message the message that shall be printed out
 *
 */
void printError(char * funcName, bool evalErrno, const char * message)
{
    fprintf(stderr, "Error in program ");
    fprintf(stderr, "%s", programName);
    fprintf(stderr, ": function ");
    fprintf(stderr, "%s", funcName);

    if(message != NULL)
    {
        fprintf(stderr, ": ");
        fprintf(stderr, "%s", message);
    }

    if(evalErrno)
    {
        fprintf(stderr, ": ");
        fprintf(stderr, "%s", strerror(errno));
    }

    fprintf(stderr, "\n");
}

/**
 *
 * \brief Function for printing usage of the client
 *
 * \param outputStream the stream the usage shall be printed to
 * \param programName the name of the client program
 * \param exitCode the exit code of the client program
 *
 */
void usagefunc(FILE *outputStream, const char *programName, int exitCode){
    fprintf(outputStream, "usage: %s\n", programName);
    fprintf(outputStream, "options:\n");
    fprintf(outputStream, "-s, --server \t <server> full qualified domain name or IP address of the server\n");
    fprintf(outputStream, "-p, --port \t <port> well-known port of the server [0..65535]\n");
    fprintf(outputStream, "-u, --user \t <name> name of the posting user\n");
    fprintf(outputStream, "-i, --image \t <URL> URL pointing to an image of the posting user\n");
    fprintf(outputStream, "-m, --message \t <message> message to be added to the bulletin board\n");
    fprintf(outputStream, "-v, --verbose \t verbose output\n");
    fprintf(outputStream, "-h, --help");

    exit(exitCode);
}

/**
 *
 * \brief Function for initalising socket and connecting to server
 *
 * \param server the name or address of the server the client shall connect with
 * \param port the port of the server the client shall connect with
 * \param sfd the descriptor of the connected socket
 *
 */
void initSocketAndConnect(const char *server, const char *port, int *sfd)
{
    verboseOutput("function initSocketAndConnect() :: init some addrinfo structs.");
    struct addrinfo hints, *res, *rp;

    verboseOutput("function initSocketAndConnect() :: reseting values for hints.");
    memset(&hints, 0, sizeof(hints));
    verboseOutput("function initSocketAndConnect() :: specifying some filters for hints.");
    hints.ai_family = AF_UNSPEC;
    /* only tcp */
    hints.ai_socktype = SOCK_STREAM;

    verboseOutput("function initSocketAndConnect() :: Get all available addresses (possible sockets).");
    if((getaddrinfo(server,port, &hints, &res)) != 0)
    {
        printError("initSocket", true, "Error getaddrinfo()");
    }

    verboseOutput("function initSocketAndConnect() :: Try to connect to a socket (loop).");
    /* copy & paste man page from getaddrinfo(3) */
    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        *sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (*sfd == -1)
            continue;

        if (connect(*sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(*sfd);
    }

    verboseOutput("function initSocketAndConnect() :: cleanup with structs.");
    freeaddrinfo(res);
}

/**
 *
 * \brief Function for initalising socket and connecting to server
 *
 * \param sfd the descriptor of the connected socket
 * \param user the name of the user that uses the client
 * \param message the text the user posts
 * \param img_url the URL of the image the user posts
 *
 */
void sendMessage(int sfd, const char* user, const char* message, const char* img_url)
{
    verboseOutput("function sendMessage() :: duplicate socket descriptor for write access.");
    int sdw = dup(sfd);
    if(sdw == -1)
    {
        printError("sendMessage()", true, "error with dup(sfd)");
        return;
    }
    verboseOutput("function sendMessage() :: dup was successfully.");

    verboseOutput("function sendMessage() :: try to fdopen with our new sdw(SocketDescirptorWrite).");
    FILE *fpw = fdopen(sdw, "w");
    if (fpw == NULL)
    {
        printError("sendMessage()", true, "fdopen with w doesn't work");
	    return;
    }
    verboseOutput("function sendMessage() :: fdopen was successful.");

    /* request with image url */
    if (!(img_url == NULL))
    {
        verboseOutput("function sendMessage() :: You call this program with img_url parameter.");
        verboseOutput("function sendMessage() :: Try to send message.");
        if(fprintf(fpw, "user=%s\nimg=%s\n%s\n", user, img_url, message) < 0)
        {
            printError("sendMessage()", true, "error fprintf(fpw, \"user=%s\\nimg=%s\\n%s\\n\", user, img_url, message)");
        }
        verboseOutput("function sendMessage() :: message sent successful.");
    }
    else
    {
        verboseOutput("function sendMessage() :: You call this program not with img_url parameter.");
        verboseOutput("function sendMessage() :: Try to send message.");
        if(fprintf(fpw, "user=%s\n%s\n", user, message) < 0)
        {
            printError("sendMessage()", true, "error fprintf(fpw, \"user=%s\\n%s\\n\", user, message)");
        }
        verboseOutput("function sendMessage() :: message sent successful.");
    }

    verboseOutput("function sendMessage() :: fflush fpw.");
    fflush(fpw);
    verboseOutput("function sendMessage() :: shutdown the sdw.");
    if(shutdown(sdw, SHUT_WR) != 0)
    {
	    printError("sendMessage()", true, "error shutdown(sdw, SHUT_WR)");
    }
    verboseOutput("function sendMessage() :: shutdown sdw successful.");
    verboseOutput("function sendMessage() :: fclose the fpw.");
    if(fclose(fpw) != 0)
    {
	    printError("sendMessage()", true, "error fclose(fpw)");
    }
    verboseOutput("function sendMessage() :: fclose fpw successful.");
}

/**
 *
 * \brief main function for creating a simple message client
 *
 * \param sdf the descriptor of the connected socket
 *
 * \return status
 *
 */
int readResponse(int sfd)
{
    verboseOutput("Function readResponse() :: Create file pointer with socketdescriptor in read.");
    FILE *fpr = fdopen(sfd, "r");
    if (fpr == NULL)
    {
        printError("readResponse()", true, "fdopen with r doesn't work");
    }

    char * line = NULL;
    size_t len = 0;
    int i = 0;
    int status = 0;
    char *filenameHtml = NULL;
    char *filenamePng = NULL;
    int lengthPng = 0;
    int lengthHtml = 0;
    int read = 0;

    verboseOutput("Function readResponse() :: Trying to read first 3 parameters (status, file, len)");
    /* read first 3 lines */
    for (i = 0; i < 3; i++)
    {
        read = getline(&line, &len, fpr);
        if(read == -1)
        {
        	printError("readResponse()", true, "getline() failed");
        }

        switch(i)
        {
        	case 0:
				if(sscanf(line, "status=%d", &status) == EOF)
				{
					printError("readResponse()", true, "sscanf() for status failed");
				}
                verboseOutput("Function readResponse() :: read successfully status.");
				break;
        	case 1:
				filenameHtml = (char*)realloc(filenameHtml, read);
				if(filenameHtml == NULL)
				{
					printError("readResponse()", true, "no memory for realloc filenameHtml");
				}
				if(sscanf(line, "file=%s", filenameHtml) == EOF)
				{
					printError("readResponse()", true, "sscanf() for filenameHtml failed");
				}
                verboseOutput("Function readResponse() :: read successfully filenameHtml.");
                break;
        	case 2:
				if(sscanf(line, "len=%d", &lengthHtml) == EOF)
				{
					printError("readResponse()", true, "sscanf() for lengthHtml failed");
				}
                verboseOutput("Function readResponse() :: read successfully lengthHtml.");
                break;
        }
    }
	verboseOutput("Function readResponse() :: 3 parameters read successully. Now I try to read the whole html file.");
    verboseOutput("Function readResponse() :: Create file pointer to the new HTML file.");
	FILE *fpResponseHtmlFile = fopen(filenameHtml, "w");
	if(fpResponseHtmlFile == NULL)
	{
		printError("readResponse()", true, "fopen(filenameHtml, 'w') failed");
	}

	char buffer[lengthHtml];

    verboseOutput("Function readResponse() :: Read from socket for HTML file.");
	if(fread(buffer, 1, lengthHtml, fpr) != 1)
	{
		printError("readResponse()", true, "fread() failed");
	}

    verboseOutput("Function readResponse() :: Write from buffer to file.");
	if(fwrite(buffer, lengthHtml, 1, fpResponseHtmlFile) != 1)
	{
		printError("readResponse()", true, "fwrite() failed");
	}

    verboseOutput("Function readResponse() :: HTML File completed.");
    verboseOutput("Function readResponse() :: PNG File starting.");
    verboseOutput("Function readResponse() :: Try to read 2 parameters (filenamePng and lengthPng).");
	for (i = 0; i < 2; i++) {
		read = getline(&line, &len, fpr);
		if (i == 0)
		{
			filenamePng = (char*)realloc(filenamePng, read);
			if(filenamePng == NULL)
			{
					printError("readResponse()", true, "no memory for realloc filenamePng");
                    return EXIT_FAILURE;
			}
			if(sscanf(line, "file=%s", filenamePng) == EOF)
            {
                printError("readResponse()", true, "sscanf() for filenamePng failed");
                return EXIT_FAILURE;
            }
            verboseOutput("Function readResponse() :: read successfully filenamePng.");
		}
		if (i == 1)
		{
			if(sscanf(line, "len=%d", &lengthPng) == EOF)
            {
                printError("readResponse()", true, "sscanf() for lengthPng failed");
                return EXIT_FAILURE;
            }
            verboseOutput("Function readResponse() :: read successfully lengthPng.");
        }
	}

    verboseOutput("Function readResponse() :: create File pointer for png file.");

    FILE *fpResponseHtmlPng = fopen(filenamePng, "w");
    if(fpResponseHtmlPng == NULL)
    {
        printError("readResponse()", true, "fopen(filenamePng, 'w') failed");
    }
	char bufferPng[lengthPng];

    verboseOutput("Function readResponse() :: read from socket to buffer (png).");
	if(fread(bufferPng, 1, lengthPng, fpr) != 1)
    {
        printError("readResponse()", true, "fread() for png failed");
    }
    verboseOutput("Function readResponse() :: write from buffer to file (png).");
    if(fwrite(bufferPng, lengthPng, 1, fpResponseHtmlPng) != 1)
    {
        printError("readResponse()", true, "fread() for png failed");
    }

    /* Free allocated memory */
    free(filenameHtml);
    free(filenamePng);
    verboseOutput("Function readResponse() :: Freed filenameHtml and filenamePng");
    
    /* Closing file pointers */
    if(fclose(fpResponseHtmlFile) != 0)
    {
    	printError("readResponse()", true, "error fclose(fpResponseHtmlFile)");
    }
    verboseOutput("Function readResponse() :: closed fpResponseHtmlFile");
    
    if(fclose(fpResponseHtmlPng) != 0)
    {
    	printError("readResponse()", true, "error fclose(fpResponseHtmlPng)");
    }
	verboseOutput("Function readResponse() :: closed fpResponseHtmlPng");

	if(fclose(fpr) != 0)
	{
		printError("readResponse()", true, "error fclose(fpr)");	
	}
	verboseOutput("Function readResponse() :: closed fpr");
	
    verboseOutput("Function readResponse() :: everything done. return with right exit value.");
    return status;
}

/**
 *
 * \brief function for printing verbose output
 *
 * \param text the text to print
 *
 */
void verboseOutput(const char *text)
{
    if(verbose)
    {
        fprintf(stdout, "%s\n", text);
    }
}

/**
 *
 * \brief main function for creating a simple message client
 *
 * \param argc the number of arguments
 * \param argv the arguments itselves (including the program name in argv[0])
 *
 * \return status of the readResponse function
 *
 */
int main(int argc, const char **argv) {

    int sfd;
    programName = argv[0];

    /* check parameter */
    const char *server = NULL;
    const char *port = NULL;
    const char *user = NULL;
    const char *message = NULL;
    const char *img_url = NULL;
    int verboseParam = -1;

    verboseOutput("Checking parameter...");
    smc_parsecommandline(argc, argv, &usagefunc, &server, &port, &user, &message, &img_url, &verboseParam);
    verboseOutput("parameter checking successfully.");

    verbose = verboseParam;

    verboseOutput("Entering function initSocketAndConnect().");
    initSocketAndConnect(server, port, &sfd);
    verboseOutput("Leaving function initSocketAndConnect().");
    verboseOutput("Entering function sendMessage().");
    sendMessage(sfd, user, message, img_url);
    verboseOutput("Leaving function sendMessage().");
    verboseOutput("Entering function readResponse(). return value of function readResponse() is the return value for main program.");
    return readResponse(sfd);
}

/*
 * =================================================================== eof ==
 */
