/*
* hallo
 * @file simple_message_client.c
 * @author Thomas Stummer <thomas.stummer@technikum-wien.at>
 * @author Patrick Matula <patrick.matula@technikum-wien.at>
 * @version 1.0
 *
 * Verteilte Systeme - TCP/IP
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

// Defines
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

// Global variables
const char * programName;

// Prototypes
int CloseSocketDescriptor(int socketDescriptor);
void printError(char * funcName, bool evalErrno, const char * message);
void usagefunc(FILE *outputStream, const char *programName, int exitCode);

void printError(char * funcName, bool evalErrno, const char * message)
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

void initSocketAndConnect(const char *server, const char *port, int socket)
{
    struct addrinfo hints, *res;

    /* reset values */
    memset(&hints, 0, sizeof(hints));
    /* allow ipv4 and ipv6 */
    hints.ai_family = AF_UNSPEC;
    /* only tcp */
    hints.ai_socktype = SOCK_STREAM;

    if((getaddrinfo(server,port, &hints, &res)) != 0)
    {
        printError("initSocket", true, "Error getaddrinfo()");
    }

    /* copy & paste from man page from getaddrinfo(3) */
    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sfd);
    }

    freeaddrinfo(res); 
}

int main(int argc, const char **argv) {

    /* check parameter */
    const char *server = NULL;
    const char *port = NULL;
    const char *user = NULL;
    const char *message = NULL;
    const char *img_url = NULL;
    int verbose = -1;

    smc_parsecommandline(argc, argv, &usagefunc, &server, &port, &user, &message, &img_url, &verbose);

    initSocketAndConnect();

    return 0;
}

