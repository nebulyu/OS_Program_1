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

    // Set up hints structure to allow either IPv4 or IPv6 and use TCP
    struct addrinfo hints = {0}, *result, *rp;
    hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // Use TCP (stream socket)

    // Get address info for server based on hostname and port
    int res = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (res != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));  // Print error if getaddrinfo fails
        exit(EXIT_FAILURE);  // Exit program with error status
    }

    int sockfd;
    // Try each address returned by getaddrinfo until a successful connection is made
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);  // Create a socket
        if (sockfd == -1) continue;  // If socket creation fails, try the next address

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break;  // Attempt to connect to the server

        close(sockfd);  // Close socket if connection fails
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not connect\n");  // If no connection was successful, print error
        exit(EXIT_FAILURE);  // Exit program with error status
    }

    freeaddrinfo(result);  // Free the memory allocated for the address info

    // Get message from user to send to the server
    char buffer[BUFFERLENGTH] = {0};
    printf("Please enter the message: ");  // Prompt user for input
    fgets(buffer, BUFFERLENGTH, stdin);  // Read user input into buffer

    // Write message to server
    if (write(sockfd, buffer, strlen(buffer)) < 0) error("ERROR writing to socket");  // Send data to server

    // Clear buffer and read server response
    bzero(buffer, BUFFERLENGTH);  // Clear the buffer
    if (read(sockfd, buffer, BUFFERLENGTH - 1) < 0) error("ERROR reading from socket");  // Read response from server

    printf("%s\n", buffer);  // Print the server's response
    close(sockfd);  // Close the socket
    return 0;  // Exit program successfully
}
