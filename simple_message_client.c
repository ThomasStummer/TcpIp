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
#define size 8

/*
 * --------------------------------------------------------------- globals --
 */

const char * programName;

/*
 * ------------------------------------------------------------- prototypes --
 */

int CloseSocketDescriptor(int socketDescriptor);
void printError(char * funcName, bool evalErrno, const char * message);
void usagefunc(FILE *outputStream, const char *programName, int exitCode);

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
 * \brief Function for closing a socket descriptor
 *
 * \param socketDescriptor the descriptor of the socket that shall be closed
 *
 * \return EXIT_SUCCESS in case of success
 * \return EXIT_FAILURE in case of failure
 *
 */
int CloseSocketDescriptor(int socketDescriptor)
{
    if(socketDescriptor != -1)
    {
        if(close(socketDescriptor) == -1)
        {
            printError("CloseSocketDescriptor() -> close()", true, NULL);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

void initSocketAndConnect(const char *server, const char *port, int *sfd)
{
    struct addrinfo hints, *res, *rp;

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
        *sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (*sfd == -1)
            continue;

        if (connect(*sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(*sfd);
    }

    freeaddrinfo(res);
    freeaddrinfo(rp);
}

void sendMessage(int sfd, const char* user, const char* message, const char* img_url)
{
    int sdw = dup(sfd);
    if(sdw == -1)
    {
        printError("sendMessage()", true, "error with dup(sfd)");
        return;
    }

    FILE *fpw = fdopen(sdw, "w");
    if (fpw == NULL)
    {
        printError("sendMessage()", true, "fdopen with w doesn't work");
	    return;
    }

    /* request with image url */
    if (!(img_url == NULL))
    {
        if(fprintf(fpw, "user=%s\nimg=%s\n%s\n", user, img_url, message) < 0)
        {
            printError("sendMessage()", true, "error fprintf(fpw, \"user=%s\\nimg=%s\\n%s\\n\", user, img_url, message)");
        }
    }
    else
    {
        if(fprintf(fpw, "user=%s\n%s\n", user, message) < 0)
        {
            printError("sendMessage()", true, "error fprintf(fpw, \"user=%s\\n%s\\n\", user, message)");
        }
    }

    /*close write */
    fflush(fpw);
    if(shutdown(sdw, SHUT_WR) != 0)
    {
	    printError("sendMessage()", true, "error shutdown(sdw, SHUT_WR)");
    }
    if(fclose(fpw) != 0)
    {
	    printError("sendMessage()", true, "error fclose(fpw)");
    }
}

int readResponse(int sfd)
{
    /* open read */
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
    /* read first 3 lines */
    for (i = 0; i < 3; i++) {
        read = getline(&line, &len, fpr);
        if (i == 0)
        {
                sscanf(line, "status=%d", &status);
        }
        if (i == 1)
        {
                filenameHtml = (char*)realloc(filenameHtml,read);
                if(filenameHtml == NULL)
                {
                        printError("readResponse()", true, "no memory for realloc filenameHtml");
                }
                sscanf(line, "file=%s", filenameHtml);
        }
        if (i == 2)
        {
                sscanf(line, "len=%d", &lengthHtml);
        }
    }
        /* now we have all 3 parameters for the first response html thing */
        FILE *fpResponseHtmlFile = fopen(filenameHtml, "w");

        char buffer[lengthHtml];

        fread(buffer, 1, lengthHtml, fpr);
        fwrite(buffer, lengthHtml, 1, fpResponseHtmlFile);

        /* now it is time for the png */
        for (i = 0; i < 3; i++) {
            read = getline(&line, &len, fpr);
            if (i == 0)
            {
                filenamePng = (char*)realloc(filenamePng, read);
                if(filenamePng == NULL)
                {
                        printError("readResponse()", true, "no memory for realloc filenamePng");
                }
                sscanf(line, "file=%s", filenamePng);
            }
            if (i == 1)
            {
                sscanf(line, "len=%d", &lengthPng);
            }
        }


        FILE *fpResponseHtmlPng = fopen(filenamePng, "w");
        char bufferPng[lengthPng];
        fread(bufferPng, 1, lengthPng, fpr);
        fwrite(bufferPng, lengthPng, 1, fpResponseHtmlPng);

    return status;
}


int main(int argc, const char **argv) {

    int sfd;
    programName = argv[0];

    /* check parameter */
    const char *server = NULL;
    const char *port = NULL;
    const char *user = NULL;
    const char *message = NULL;
    const char *img_url = NULL;
    int verbose = -1;

    smc_parsecommandline(argc, argv, &usagefunc, &server, &port, &user, &message, &img_url, &verbose);

    initSocketAndConnect(server, port, &sfd);
    sendMessage(sfd, user, message, img_url);
    return readResponse(sfd);
}

/*
 * =================================================================== eof ==
 */
