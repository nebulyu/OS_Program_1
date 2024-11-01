#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>

#define DEFAULTBUFFERLENGTH 256  // Define the buffer length for messages
int islocal = 0;
// Function to handle errors and print messages
void error(char *msg) {
    perror(msg);  // Print error message
    exit(1);  // Exit program with error status
}

int isExecuted = 0;  // Shared variable to keep track of requests
pthread_mutex_t mutex_cnt = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable

pthread_mutex_t mutex_newrequest = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_DD = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_A = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_C = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_L = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_DA = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_DC = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_DL = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable

int count_A = 0;
int count_C = 0;
int count_L = 0;

typedef struct QUERY{
    char *command;
    struct QUERY *next;
}QUERY;
typedef struct REQUEST{
    char *command;
    struct REQUEST *next;
    char * src_ip,*dst_ip;
    int src_port,dst_port;
    QUERY *query_head,*query_tail;
    bool is_valid_rule;
}REQUEST;
REQUEST *request_head = NULL;
REQUEST *request_tail = NULL; 


REQUEST *create_request(char *command){
    pthread_mutex_lock(&mutex_newrequest);  // Acquire the lock to ensure exclusive access
    REQUEST *new_request = (REQUEST *)malloc(sizeof(REQUEST));
    new_request->command = strdup(command);
    new_request->is_valid_rule = false;
    new_request->next = NULL;
    if(request_head == NULL){
        request_head = new_request;
        request_tail = new_request;
    }
    else{
        request_tail->next = new_request;
        request_tail = new_request;
    }
    pthread_mutex_unlock(&mutex_newrequest);  // Release the lock
    return new_request;
}
bool check_port_valid(int port){
    if(port > 65535 || port < 0) return false;
    return true;
}
bool check_ip_valid(char *ip) {
    if (ip == NULL) return false;
    char ip_copy[16];
    strncpy(ip_copy, ip, sizeof(ip_copy));
    ip_copy[sizeof(ip_copy) - 1] = '\0'; 
    char *token = strtok(ip_copy, ".");
    int cnt = 0;
    while (token != NULL) {
        for (int j = 0; token[j] != '\0'; j++) {
            if (!isdigit(token[j])) return false;
        }
        int num = atoi(token);
        // printf("%d\n",num);
        if (num < 0 || num > 255) return false;
        token = strtok(NULL, ".");
        ++cnt;
    }
    // printf("cnt:%d\n",cnt);
    if (cnt != 4) return false;
    return true;
}
//默认ip地址为合法的
bool cmp_id_ord(char *ip1,char *ip2){
    char ip1_copy[16],ip2_copy[16];
    strncpy(ip1_copy, ip1, sizeof(ip1_copy));
    strncpy(ip2_copy, ip2, sizeof(ip2_copy));
    ip1_copy[sizeof(ip1_copy) - 1] = '\0'; 
    ip2_copy[sizeof(ip2_copy) - 1] = '\0'; 
    char *token1 = strtok(ip1_copy, ".");
    char *token2 = strtok(ip2_copy, ".");
    while (token1 != NULL && token2 != NULL) {
        int num1 = atoi(token1);
        int num2 = atoi(token2);
        if(num1 < num2) return true;
        else if(num1 > num2) return false;
        token1 = strtok(NULL, ".");
        token2 = strtok(NULL, ".");
    }
    return true;

}
bool check_rule_valid(REQUEST rule){
    if(!rule.is_valid_rule) return false;
    if(rule.src_port > rule.dst_port) return false;
    if(!check_port_valid(rule.src_port) || !check_port_valid(rule.dst_port)) return false;
    if(!check_ip_valid(rule.src_ip) || !check_ip_valid(rule.dst_ip)) return false;
    if(!cmp_id_ord(rule.src_ip,rule.dst_ip)) return false;
    return true;
}
void fill_rule(REQUEST *rule,char *ips,char *ports){
    char *ip_token = strtok(ips, "-");
    rule->src_ip = strdup(ip_token);
    ip_token = strtok(NULL, "-");
    if(ip_token == NULL)rule->dst_ip=rule->src_ip;
    else rule->dst_ip = strdup(ip_token);

    char *port_token = strtok(ports, "-");
    rule->src_port = atoi(port_token);
    port_token = strtok(NULL, "-");
    if(port_token == NULL)rule->dst_port = rule->src_port;
    else rule->dst_port = atoi(port_token);
    // printf("%s %s %d %d\n",rule->src_ip,rule->dst_ip,rule->src_port,rule->dst_port);
    // printf("%d %d %d %d\n",check_ip_valid(rule->src_ip),check_ip_valid(rule->dst_ip),check_port_valid(rule->src_port),check_port_valid(rule->dst_port));
}
bool check_request_in_rule(REQUEST *rule,char *ip,int port){
    if(!rule->is_valid_rule) return false;
    if(cmp_id_ord(rule->src_ip,ip) && cmp_id_ord(ip,rule->dst_ip) && rule->src_port <= port && port <= rule->dst_port)
        return true;
    return false;
}
bool check_request_equal_rule(REQUEST *rule,REQUEST *request){
    if(!rule->is_valid_rule) return false;
    if(request->src_port == rule->src_port && request->dst_port == rule->dst_port && !strcmp(request->src_ip,rule->src_ip) && !strcmp(request->dst_ip,rule->dst_ip))
        return true;
    return false;
}


char* Server_R(char *buffer){
    REQUEST *new_request = create_request(buffer);
    REQUEST *current = request_head;
    // Allocate memory for the result buffer
    size_t result_size = 1024;  // Initial size, adjust as needed
    //注意此处不可直接动态,初始值赋为""
    //这个指向静态地址的指针会导致strcat时出现段错误
    char *result = malloc(result_size);
    if (!result) {
        perror("Memory allocation failed");
        return "";
    }
    result[0] = '\0';  // Initialize the result buffer

    while (1) {
        // Check if we need to reallocate more memory for result
        if (strlen(result) + strlen(current->command) + 2 > result_size) {
            result_size *= 2;
            result = realloc(result, result_size);
            if (!result) {
                perror("Memory reallocation failed");
                return "";
            }
        }

        strcat(result, current->command);

        if (request_head == request_tail) break;
        current = current->next;
        if (current == new_request) {
            strcat(result, current->command);
            break;
        }
    }
    return result;
}
char* Server_illegal(char *buffer){
    create_request(buffer);
    char *result = "Illegal request";
    return result;
}
// Function to process each client request in a separate thread
void *serverRequest(void *args) {
    int *newsockfd = (int *)args;  // Get the client socket file descriptor
    char buffer[DEFAULTBUFFERLENGTH] = {0};  // Buffer to store client messages

    // Read message from client
    int n = read(*newsockfd, buffer, DEFAULTBUFFERLENGTH - 1);
    if (n < 0) error("ERROR reading from socket");  // Handle read error

    printf("Here is the message: %s\n", buffer);  // Print the client's message

    char *result = NULL;  // Buffer to store response message

    if (buffer[0] == 'R') {
        result = Server_R(buffer);
    }
    else {
        result = Server_illegal(buffer);
    }

    // Send response to client
    snprintf(buffer, DEFAULTBUFFERLENGTH,"%s",result);  // Prepare response message
    n = write(*newsockfd, buffer, strlen(buffer));  // Write response to client
    if (n < 0) error("ERROR writing to socket");  // Handle write error

    close(*newsockfd);  // Close client socket
    free(newsockfd);  // Free memory allocated for client socket descriptor
    pthread_exit(NULL);  // Exit the thread
}
void server_main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");  // Print error message
        exit(1);  // Exit program with error status
    }

    // Create socket for IPv6 using TCP
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");  // Handle socket creation error

    // Set up server address structure
    struct sockaddr_in6 serv_addr = {0};
    serv_addr.sin6_family = AF_INET6;  // Use IPv6
    serv_addr.sin6_addr = in6addr_any;  // Accept connections from any address
    serv_addr.sin6_port = htons(atoi(argv[1]));  // Set port number in network byte order

    // Bind socket to the server address and port
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) error("ERROR on binding");  // Handle binding error

    // Listen for incoming connections
    listen(sockfd, 5);  // Set the socket to listen, with a maximum backlog of 5

    while (1) {
        // Accept new client connection
        struct sockaddr_in6 cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int *newsockfd = malloc(sizeof(int));  // Allocate memory for new client socket descriptor
        if (!newsockfd) error("Memory allocation failed");  // Handle memory allocation error

        *newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);  // Accept incoming client connection
        if (*newsockfd < 0) error("ERROR on accept");  // Handle accept error

        // Create thread to handle the client connection
        pthread_t server_thread;
        if (pthread_create(&server_thread, NULL, serverRequest, (void *)newsockfd) != 0) error("Thread creation failed");  // Handle thread creation error

        pthread_detach(server_thread);  // Detach thread to allow resources to be reclaimed after it finishes
    }

    close(sockfd);  // Close the server socket
}

void Local_R(REQUEST *new_request){
    REQUEST *current = request_head;
    // Allocate memory for the result buffer
    size_t result_size = 1024;  // Initial size, adjust as needed
    //注意此处不可直接动态,初始值赋为""
    //这个指向静态地址的指针会导致strcat时出现段错误
    char *result = malloc(result_size);
    if (!result) {
        perror("Memory allocation failed");
        return;
    }
    result[0] = '\0';  // Initialize the result buffer

    while (1) {
        // Check if we need to reallocate more memory for result
        if (strlen(result) + strlen(current->command) + 2 > result_size) {
            result_size *= 2;
            result = realloc(result, result_size);
            if (!result) {
                perror("Memory reallocation failed");
                return;
            }
        }

        strcat(result, current->command);

        if (request_head == request_tail) break;
        current = current->next;
        if (current == new_request) {
            strcat(result, current->command);
            break;
        }
    }

    printf("%s", result);
    free(result);  // Free the allocated memory
}
void Local_A(REQUEST *new_request,char *ips,char *ports){
    pthread_mutex_lock(&mutex_A);
    if(!count_A)
        pthread_mutex_lock(&mutex_DA);
    ++count_A;
    pthread_mutex_unlock(&mutex_A);

    fill_rule(new_request,ips,ports);
    new_request->is_valid_rule = true;
    new_request->is_valid_rule = check_rule_valid(*new_request);
    bool result = new_request->is_valid_rule;

    pthread_mutex_lock(&mutex_A);
    --count_A;
    if(!count_A)
        pthread_mutex_unlock(&mutex_DA);
    pthread_mutex_unlock(&mutex_A);

    if(result)printf("Rule added\n");
    else printf("Invalid rule\n");
}
void Local_C(REQUEST *new_request,char *ip,char *port){
    if(!check_ip_valid(ip) || !check_port_valid(atoi(port))) {
        printf("Illegal IP address or port specified\n");
        return;
    } 
    fill_rule(new_request,ip,port);
    if(new_request->src_port!=new_request->dst_port || strcmp(new_request->src_ip,new_request->dst_ip)){
        printf("Illegal IP address or port specified\n");
        return;
    }

    pthread_mutex_lock(&mutex_C);
    if(!count_C)
        pthread_mutex_lock(&mutex_DC);
    ++count_C;
    pthread_mutex_unlock(&mutex_C);

    bool result = false;
    REQUEST *current = request_head;
    while(current != new_request){
        if(check_request_in_rule(current,ip,atoi(port))){
            QUERY *new_query = (QUERY *)malloc(sizeof(QUERY));
            new_query->command = strcat(ip," ");
            new_query->command = strcat(new_query->command,port);
            new_query->next = NULL;
            if(current->query_head == NULL){
                current->query_head = new_query;
                current->query_tail = new_query;
            }
            else{
                current->query_tail->next = new_query;
                current->query_tail = new_query;
            }
            result = true;
            break;
        }
        current = current->next;
    }

    pthread_mutex_lock(&mutex_C);
    --count_C;
    if(!count_C)
        pthread_mutex_unlock(&mutex_DC);
    pthread_mutex_unlock(&mutex_C);

    if(!result) printf("Connection rejected\n");
    else printf("Connection accepted\n");

    

    
}
void Local_L(REQUEST *new_request){
    REQUEST *current = request_head;
    while(current != new_request){
        if(current->is_valid_rule){
            printf("Rule: ");
            if(current->src_ip == current->dst_ip) printf("%s ",current->src_ip);
            else printf("%s-%s ",current->src_ip,current->dst_ip);
            if(current->src_port == current->dst_port) printf("%d\n",current->src_port);
            else printf("%d-%d\n",current->src_port,current->dst_port);
            QUERY *current_query = current->query_head;
            while(current_query != NULL){
                printf("Query: %s",current_query->command);
                current_query = current_query->next;
            }
        }
        current = current->next;
    }
}
void Local_D(REQUEST *new_request,char *ips,char *ports){
    fill_rule(new_request,ips,ports);
    new_request->is_valid_rule = true;
    bool rule_valid = check_rule_valid(*new_request);
    new_request->is_valid_rule = false;
    if(!rule_valid){
        printf("Rule invalid\n");
        return ;
    }

    pthread_mutex_lock(&mutex_DD);
    pthread_mutex_lock(&mutex_DA);
    pthread_mutex_lock(&mutex_DC);
    REQUEST *current = request_head;
    bool result = false;
    while(current != new_request){
        if(check_request_equal_rule(current,new_request)){
            current->is_valid_rule = false;
            pthread_mutex_unlock(&mutex_DD);
            result = true;
            break;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&mutex_DC);
    pthread_mutex_unlock(&mutex_DA);
    pthread_mutex_unlock(&mutex_DD);

    if(result) printf("Rule deleted\n");
    else printf("Rule not found\n");
    
}
void Local_illegal(){
    printf("Illegal request\n");
}

void *localRequest(void *args) {
    char *buffer = (char *)args;
    REQUEST *new_request = create_request(buffer);
    char *token = strtok(buffer, " ");
    int part_count = 0;
    char *parts[3] = {NULL, NULL, NULL};  // Array to store the first three parts

    // Count the number of parts and record the first three parts
    while (token != NULL) {
        if (part_count < 3) {
            parts[part_count] = strdup(token);  // Duptlicate the token to store it
        }
        part_count++;
        token = strtok(NULL, " ");
    }
    if(part_count == 1 && parts[0][0] == 'R') Local_R(new_request);
    else if(part_count == 1 && parts[0][0] == 'L') Local_L(new_request);
    else if(part_count == 3 && parts[0][0] == 'A') Local_A(new_request,parts[1],parts[2]);
    else if(part_count == 3 && parts[0][0] == 'D') Local_D(new_request,parts[1],parts[2]);
    else if(part_count == 3 && parts[0][0] == 'C') Local_C(new_request,parts[1],parts[2]);
    else Local_illegal();

    free(buffer);  // Free the allocated memory
    pthread_exit(NULL);  // Exit the thread
}
void local_main(int argc, char *argv[]) {
    printf("local mode\n");
    while (1) {
        char *buffer = NULL;
        size_t bufsize = 32;
        getline(&buffer, &bufsize, stdin);

        // Allocate memory for the thread argument
        char *thread_arg = strdup(buffer);
        if (thread_arg == NULL) {
            perror("Failed to allocate memory");
            exit(EXIT_FAILURE);
        }
        // Create thread to handle the client connection
        pthread_t local_thread;
        if (pthread_create(&local_thread, NULL, localRequest, (void *)thread_arg) != 0) {
            perror("Thread creation failed");
            free(thread_arg);  // Free the allocated memory on error
            exit(EXIT_FAILURE);
        }
        pthread_detach(local_thread);  // Detach thread to allow resources to be reclaimed after it finishes
        free(buffer);  // Free the buffer allocated by getline
    }
}

int main(int argc, char *argv[]) {
    // printf("%s",argv[1]);
    if(argc == 2){
        if(!strcmp("-i",argv[1])) islocal = 1;
    }
    if(islocal) local_main(argc,argv);
    else server_main(argc,argv);
    return 0;  // Exit program successfully
}
