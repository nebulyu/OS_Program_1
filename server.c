// Written by Nebulyu
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

void error(char *msg) {
    perror(msg);  // Print error message
    exit(1);  // Exit program with error status
}

pthread_mutex_t mutex_cnt = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable

pthread_mutex_t mutex_newrequest = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_DD = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_A = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_C = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_L = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_DA = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_DC = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_DL = PTHREAD_MUTEX_INITIALIZER;  // Mutex to synchronize access to shared variable
pthread_mutex_t mutex_LC = PTHREAD_MUTEX_INITIALIZER;

int count_A = 0;
int count_C = 0;
int count_L = 0;

int cnt_command = 0;

typedef struct QUERY{
    char *command;
    struct QUERY *next;
}QUERY;
typedef struct REQUEST{
    struct REQUEST *next;
    char * src_ip,*dst_ip;
    int src_port,dst_port;
    int cnt;
    QUERY *query_head,*query_tail;
    bool is_valid_rule;
}REQUEST;
REQUEST *request_head = NULL;
REQUEST *request_tail = NULL; 

QUERY *default_query = NULL;
char **command_array = NULL;
int command_capacity = 2;  // 当前容量
int command_cnt = 0;  // 当前已存储的字符串数量

void initialize(){
    command_array = malloc(command_capacity * sizeof(char *));
    if (command_array == NULL) {
        error("Failed to allocate memory");
    }
}
void add_command(const char *str) {
    // 检查是否需要扩展数组
    if (command_cnt >= command_capacity) {
        command_capacity *= 2;  // 每次扩展两倍
        char **new_array = realloc(command_array, command_capacity * sizeof(char *));
        if (new_array == NULL) {
            error("Failed to reallocate memory");
        }
        command_array = new_array;
    }

    // 为新字符串分配内存并存储
    command_array[command_cnt] = malloc((strlen(str) + 1) * sizeof(char));
    if (command_array[command_cnt] == NULL) {
        error("Failed to allocate memory for string");
    }
    strcpy(command_array[command_cnt], str);  // 复制字符串
    command_cnt++;
}

REQUEST *create_request(char *command){
    pthread_mutex_lock(&mutex_newrequest);  // Acquire the lock to ensure exclusive access
    REQUEST *new_request = (REQUEST *)malloc(sizeof(REQUEST));
    new_request->is_valid_rule = false;
    new_request->next = NULL;
    //记得初始化指针
    new_request->query_head = default_query;
    new_request->query_tail = default_query;
    new_request->src_ip = NULL;
    new_request->dst_ip = NULL;
    new_request->src_port = 0;
    new_request->dst_port = -1;
    ++cnt_command;
    add_command(command);
    new_request->cnt = cnt_command;
    if(request_head == NULL){
        request_head = new_request;
        request_tail = new_request;
    }
    else{
        request_tail->next = new_request;
        request_tail = new_request;
    }
    pthread_mutex_unlock(&mutex_newrequest);  // Release the lock by Nebulyu
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
        // FILE *log_file = fopen("log.txt", "a");
        // fprintf(log_file, "num:%d\n", num);
        // fclose(log_file);
        

    }
    // printf("cnt:%d\n",cnt);
    if (cnt != 4) return false;
    return true;
}
//默认ip地址为合法的
bool cmp_id_ord(char *ip1,char *ip2){
    char *ip1_copy = strdup(ip1);
    char *ip2_copy = strdup(ip2);
    char *token = strtok(ip1_copy, ".");
    int rec1[4],rec2_by_nebulyu[4],p=0;
    while (token != NULL) {
        rec1[p]=atoi(token);
        token = strtok(NULL, ".");
        ++p;
    }
    p=0;
    token = strtok(ip2_copy, ".");
    while (token != NULL) {
        rec2_by_nebulyu[p]=atoi(token);
        token = strtok(NULL, ".");
        ++p;
    }
    free(ip1_copy);
    free(ip2_copy);
    for(int i=0;i<4;++i){
        if(rec1[i] < rec2_by_nebulyu[i]) return true;
        if(rec1[i] > rec2_by_nebulyu[i]) return false;
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
    if(ip_token == NULL)rule->dst_ip=strdup(rule->src_ip);
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

void tranv(){
    REQUEST *current = request_head;
    while(current != NULL){
        printf("cnt:%d src_port:%d dst_port:%d\n",current->cnt,current->src_port,current->dst_port);
        current = current->next;
    }
}

char *Command_R(REQUEST *new_request){
    // Allocate memory for the result buffer
    size_t result_size = 1024;  // Initial size, adjust as needed
    //注意此处不可直接动态,初始值赋为""
    //这个指向静态地址的指针会导致strcat时出现段错误
    char *result = malloc(result_size);
    if (!result) {
        error("Memory allocation failed");
        return "";
    }
    result[0] = '\0';  // Initialize the result buffer

    int goal = new_request->cnt;

    for (int i=0;i<goal;++i){
        // printf("i:%d goal:%d\n",i,goal);
        if (strlen(result) + strlen(command_array[i]) + 2 > result_size) {
            result_size *= 2;
            result = realloc(result, result_size);
            if (!result) {
                error("Memory reallocation failed");
                break;
            }
        }
        strcat(result, command_array[i]);
        // strcat(result, "\n");
    }
    
    return result;
}
char *Command_A(REQUEST *new_request,char *ips,char *ports){
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

    if(result) return "Rule added\n";
    else return "Invalid rule\n";
}
char *Command_C(REQUEST *new_request,char *ip,char *port){
    if(!check_ip_valid(ip) || !check_port_valid(atoi(port))) {
        return "Illegal IP address or port specified\n";
    } 
    fill_rule(new_request,ip,port);
    if(new_request->src_port!=new_request->dst_port || strcmp(new_request->src_ip,new_request->dst_ip)){
        return "Illegal IP address or port specified\n";
    }

    pthread_mutex_lock(&mutex_C);
    if(!count_C){
        pthread_mutex_lock(&mutex_DC);
        pthread_mutex_lock(&mutex_LC);
    }
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
    if(!count_C){
        pthread_mutex_unlock(&mutex_LC);
        pthread_mutex_unlock(&mutex_DC);
    }
    pthread_mutex_unlock(&mutex_C);

    if(!result) return "Connection rejected\n";
    else return "Connection accepted\n";
}
char *Command_L(int lim){
    volatile REQUEST *current = request_head;
    size_t result_size = 1024;  // Initial size, adjust as needed
    char *result = malloc(result_size);
    if (!result) {
        error("Memory allocation failed");
        return "";
    }
    result[0] = '\0';

    pthread_mutex_lock(&mutex_L);
    if(!count_L){
        pthread_mutex_lock(&mutex_DL);
        pthread_mutex_lock(&mutex_LC);
    }
    ++count_L;
    pthread_mutex_unlock(&mutex_L);

    while(current->cnt < lim){
        // printf("current:%d new:%d\n",current->cnt,lim);
        // printf("valid:%d\n",current->is_valid_rule);
        if(current->is_valid_rule){
            if (strlen(result) + 30 + 2 > result_size) {
                result_size *= 2;
                result = realloc(result, result_size);
                if (!result) {
                    error("Memory reallocation failed");
                    break;
                }
            }

            strcat(result, "Rule: ");
            strcat(result,current->src_ip);
            if(current->src_ip == current->dst_ip) ;
            else strcat(result, "-"),strcat(result,current->dst_ip);
            strcat(result," ");


            char port_buffer[12];  // Buffer to hold the port number as a string
            sprintf(port_buffer, "%d", current->src_port);
            strcat(result, port_buffer);
            sprintf(port_buffer, "%d", current->dst_port);
            if(current->src_port == current->dst_port);
            else strcat(result,"-"),strcat(result,port_buffer);
            strcat(result,"\n");

            QUERY *current_query = current->query_head;
            while(current_query != default_query){
                if (strlen(result) + 30 + 2 > result_size) {
                    result_size *= 2;
                    result = realloc(result, result_size);
                    if (!result) {
                        error("Memory reallocation failed");
                        break;
                    }   
                }
                strcat(result,"Query: "),strcat(result,current_query->command);
                current_query = current_query->next;
            }
        }
        current = current->next;
    }

    pthread_mutex_lock(&mutex_L);
    --count_L;
    if(!count_L){
        pthread_mutex_unlock(&mutex_LC);
        pthread_mutex_unlock(&mutex_DL);
    }
    pthread_mutex_unlock(&mutex_L);

    //by Nebulyu

    if(strlen(result) <= 1) return "";
    return result;
}
char *Command_D(REQUEST *new_request,char *ips,char *ports){
    fill_rule(new_request,ips,ports);
    new_request->is_valid_rule = true;
    bool rule_valid = check_rule_valid(*new_request);
    new_request->is_valid_rule = false;
    if(!rule_valid){
        return "Rule invalid\n";
    }


    pthread_mutex_lock(&mutex_DD);
    pthread_mutex_lock(&mutex_DA);
    pthread_mutex_lock(&mutex_DC);
    pthread_mutex_lock(&mutex_DL);

    REQUEST *current = request_head;
    bool result_by_nebulyu = false;
    while(current != new_request){
        if(check_request_equal_rule(current,new_request)){
            current->is_valid_rule = false;
            pthread_mutex_unlock(&mutex_DD);
            result_by_nebulyu = true;
            break;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&mutex_DL);
    pthread_mutex_unlock(&mutex_DC);
    pthread_mutex_unlock(&mutex_DA);
    pthread_mutex_unlock(&mutex_DD);

    if(result_by_nebulyu) return "Rule deleted\n";
    else return "Rule not found\n";
}
char * Command_illegal(){
    return "Illegal request\n";
}

void *Request(void *args) {
    char *command = "";
    int *newsockfd = NULL;
    int n=0;
    if(islocal){
        char *buffer = (char *)args;
        command = strdup(buffer);
    }
    else{
        newsockfd = (int *)args;  // Get the client socket file descriptor
        char buffer[DEFAULTBUFFERLENGTH] = {0};  // Buffer to store client messages

        // Read message from client
        n = read(*newsockfd, buffer, DEFAULTBUFFERLENGTH - 1);
        if (n < 0) error("ERROR reading from socket");  // Handle read error
        command = strdup(buffer);
        // printf("Received: %s\n", command); by Nebulyu
    }

    
    // printf("Received: %s\n", command);
    
    // if (n < 0) error("ERROR reading from socket");  // Handle read error

    REQUEST *new_request = create_request(command);
    char *token = strtok(command, " ");
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

    // printf("\nBefore check\n");
    // tranv();

    char *result = NULL;
    if(strlen(parts[0]) > 2) result = Command_illegal();
    else if(part_count == 1 && parts[0][0] == 'R') result = Command_R(new_request);
    else if(part_count == 1 && parts[0][0] == 'L') result = Command_L(new_request->cnt);
    else if(part_count == 3 && parts[0][0] == 'A') result = Command_A(new_request,parts[1],parts[2]);
    else if(part_count == 3 && parts[0][0] == 'D') result = Command_D(new_request,parts[1],parts[2]);
    else if(part_count == 3 && parts[0][0] == 'C') result = Command_C(new_request,parts[1],parts[2]);
    else result =  Command_illegal();

    for (int i = 0; i < 3; i++) {
        if (parts[i] != NULL) {
            free(parts[i]);
        }
    }

    // printf("current command :%s", new_request->command);

    // printf("by Nebulyu\n");
    // tranv();

    if(islocal){
        printf("%s", result);
        free(newsockfd);
        return NULL;
    }
    else{
        char response[DEFAULTBUFFERLENGTH] = {0};  // Buffer to store the response message
        snprintf(response, DEFAULTBUFFERLENGTH,"%s",result);  // Prepare response message
        n = write(*newsockfd, response, strlen(response));  // Write response to client
        if (n < 0) error("ERROR writing to socket");  // Handle write error
        close(*newsockfd);  // Close client socket
        free(newsockfd);
        pthread_exit(NULL);  // Exit the thread
    }
}

void server_main(int argc, char *argv[]){
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
    listen(sockfd, 100);  

    while (1) {
        // Written by Nebulyu
        struct sockaddr_in6 cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int *newsockfd = malloc(sizeof(int));  // Allocate memory for new client socket descriptor
        if (!newsockfd) error("Memory allocation failed");  // Handle memory allocation error

        *newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);  // Accept incoming client connection
        if (*newsockfd < 0) error("ERROR on accept");  // Handle accept error

        // Create thread to handle the client connection
        pthread_t server_thread;
        if (pthread_create(&server_thread, NULL, Request, (void *)newsockfd) != 0) error("Thread creation failed");  // Handle thread creation error

        pthread_detach(server_thread);  // Detach thread to allow resources to be reclaimed after it finishes
    }

    close(sockfd);  // Close the server socket
}


void local_main(int argc, char *argv[]) {
    char buffer[DEFAULTBUFFERLENGTH];
    while (fgets(buffer, DEFAULTBUFFERLENGTH, stdin)!=NULL) {
        volatile char *thread_arg = strdup(buffer);
        Request((void *)thread_arg);
    }
}
int main(int argc, char *argv[]) {
    if(argc < 2){
        fprintf(stderr, "usage %s port\n", argv[0]);  // Print usage message
        exit(1);  // Exit program with error status
    }
    initialize();
    setvbuf(stdout, NULL, _IONBF, 0);
    if(argc == 2){
        if(!strcmp("-i",argv[1])) islocal = 1;
    }
    if(islocal) local_main(argc,argv);
    else server_main(argc,argv);
    return 0;
}
