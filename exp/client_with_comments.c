
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFERLENGTH 256  // Define buffer length for messages

/* displays error messages from system calls */
void error(char *msg) {
    perror(msg);  // Print system error message
    exit(0);  // Exit with status 0
}

int main(int argc, char *argv[]) {
    int sockfd, n;  // Socket file descriptor and read/write status
    struct addrinfo hints;  // Hints structure for address setup
    struct addrinfo *result, *rp;  // Resulting address list and iterator
    int res;  // Result of getaddrinfo
    
    char buffer[BUFFERLENGTH];  // Buffer for client messages
    if (argc < 3) {  // Check if hostname and port are provided
       fprintf(stderr, "usage %s hostname port\n", argv[0]);
       exit(1);
    }

    /* Setup hints to allow either IPv4 or IPv6 (AF_UNSPEC) and TCP (SOCK_STREAM) */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // Specify TCP connection
    hints.ai_flags = 0;
    hints.ai_protocol = 0;  // Allow any protocol

    /* Retrieve matching address info */
    res = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (res != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
        exit(EXIT_FAILURE);
    }

    /* Try each address until a successful connection is made */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (sockfd == -1)
            continue;  // If failed, move to next address

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;  // Successful connection

        close(sockfd);  // Close unsuccessful socket
    }

    if (rp == NULL) {  // If no address succeeded
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);  // Free address info memory

    /* Request message input from user */
    printf("Please enter the message: ");
    bzero(buffer, BUFFERLENGTH);  // Clear buffer
    fgets(buffer, BUFFERLENGTH, stdin);  // Get user input

    /* Send message to server */
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0) {
         error("ERROR writing to socket");
    }
    bzero(buffer, BUFFERLENGTH);  // Clear buffer for response

    /* Receive and display reply from server */
    n = read(sockfd, buffer, BUFFERLENGTH - 1);
    if (n < 0) {
         error("ERROR reading from socket");
    }
    printf("%s\n", buffer);  // Display server reply
    
    return 0;
}
