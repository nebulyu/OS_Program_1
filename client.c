// Written by Nebulyu
#include <stdio.h>  // Standard I/O library
#include <stdlib.h>  // Standard library for memory allocation, process control, etc.
#include <string.h>  // String manipulation functions
#include <unistd.h>  // UNIX standard functions (e.g., close())
#include <sys/types.h>  // Data types used in system calls
#include <sys/socket.h>  // Socket functions and definitions
#include <netdb.h>  // Network database operations

#define BUFFERLENGTH 256  // Define the buffer length for messages

// Function to handle errors and print messages
void error(char *msg) {
    perror(msg);  // Print error message
    exit(1);  // Exit program with error status
}

int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided (hostname and port)
    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);  // Print usage message
        exit(1);  // Exit program with error status
    }
    if (argc == 3) {
        fprintf(stderr, "Please input message\n");  // Print usage message
        exit(1);  // Exit program with error status
    }

    // Set up hints structure to allow either IPv4 or IPv6 and use TCP
    struct addrinfo hints = {0}, *result_by_nebulyu, *rp;
    hints.ai_family = AF_UNSPEC;  // Written by Nebulyu
    hints.ai_socktype = SOCK_STREAM;  // Use TCP (stream socket)

    // Get address info for server based on hostname and port
    

    int res = getaddrinfo(argv[1], argv[2], &hints, &result_by_nebulyu);
    if (res != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));  // Print error if getaddrinfo fails
        exit(EXIT_FAILURE);  // Exit program with error status
    }

    char buffer[BUFFERLENGTH] = {0};
    for(int i=3;i<argc;i++){
        strcat(buffer,argv[i]);
        if(i<argc-1) strcat(buffer," ");
        else strcat(buffer,"\n");
    }

    int sockfd;
    for (rp = result_by_nebulyu; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);  // Create a socket
        if (sockfd == -1) continue;  // If socket creation fails, try the next address
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break;  // Attempt to connect to the server
        close(sockfd);  // Close socket if connection fails
    }
    if (rp == NULL) {
        fprintf(stderr, "Could not connect\n");  // If no connection was successful, print error
        exit(EXIT_FAILURE);  // Exit program with error status
    }
    freeaddrinfo(result_by_nebulyu);  // Free the memory allocated for the address info
    if (write(sockfd, buffer, strlen(buffer)) < 0) error("ERROR writing to socket");  // Send data to server
    bzero(buffer, BUFFERLENGTH);  // Clear the buffer by Nebulyu
    if (read(sockfd, buffer, BUFFERLENGTH - 1) < 0) error("ERROR reading from socket");  // Read response from server
    printf("%s", buffer);
    close(sockfd);
    return 0;
}
