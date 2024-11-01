
/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#define BUFFERLENGTH 256  // Define the buffer length for messages

/* displays error messages from system calls */
void error(char *msg) {
    perror(msg);  // Print error message from system
    exit(1);  // Exit with status 1
}

int main(int argc, char *argv[]) {
    socklen_t clilen;  // Length of client address
    int sockfd, newsockfd, portno;  // Socket file descriptors and port number
    char buffer[BUFFERLENGTH];  // Buffer to store messages
    struct sockaddr_in6 serv_addr, cli_addr;  // Server and client address structs
    int n;  // Variable to track read/write status

    if (argc < 2) {  // Check for port argument
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
  
    /* Create a new IPv6 socket (AF_INET6) for TCP (SOCK_STREAM) */
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));  // Clear serv_addr
    portno = atoi(argv[1]);  // Convert port number from string to int
    serv_addr.sin6_family = AF_INET6;  // Use IPv6
    serv_addr.sin6_addr = in6addr_any;  // Accept connections from any address
    serv_addr.sin6_port = htons(portno);  // Convert port number to network byte order

    /* Bind socket to the address */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    /* Set socket to listen for incoming connections, with a maximum backlog of 5 */
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);  // Set client address length
  
    /* Enter infinite loop to handle connections */
    while (1) {
        /* Wait for a connection from a client */
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            error("ERROR on accept");
        }
        bzero(buffer, BUFFERLENGTH);  // Clear buffer

        /* Read message from client */
        n = read(newsockfd, buffer, BUFFERLENGTH - 1);
        if (n < 0) {
            error("ERROR reading from socket");
        }

        printf("Here is the message: %s\n", buffer);  // Display client message

        /* Send a reply to the client */
        n = write(newsockfd, "I got your message", 18);
        if (n < 0) {
            error("ERROR writing to socket");
        }

        close(newsockfd);  // Close client connection to free memory
    }

    return 0; 
}
