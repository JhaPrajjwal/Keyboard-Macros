#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h> 
#include<unistd.h>
#include<sys/socket.h>  
#include<sys/stat.h>
#include<string.h>  
#include<netinet/in.h>  
#include<netdb.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<pthread.h>

#define PORT 4444
#define BUF_SIZE 360
#define CLADDR_LEN 100
#define MACROS_FILE_NAME "/home/pj/test.txt"

int sockfd;
struct sockaddr_in addr;

const char cmds[2][20] = {"PUSH", "PULL"};

int send_message(char msg[], int dest_sockfd) {
    return sendto(dest_sockfd, msg, BUF_SIZE, 0, (struct sockaddr *) &addr, sizeof(addr));
}

int recieve_message(int dest_sockfd, char recv_buf[]) {
    memset(recv_buf, 0, BUF_SIZE);
    return recvfrom(dest_sockfd, recv_buf, BUF_SIZE, 0, NULL, NULL);
}

void send_macros_file(char* clientAddr, int dest_sockfd) {
    int status, len, total = 0;
    long int offset = 0;
    char buffer[BUF_SIZE];
    FILE *fd;
    
    fd = fopen(MACROS_FILE_NAME, "r");

    if(fd == NULL) {
        printf("Error in reading file.\n");
        sprintf(buffer, "%s", "err");
        send_message(buffer, dest_sockfd);
        return ;
    }

    // Send File Size
    fseek(fd, 0, SEEK_END);
    len = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    sprintf(buffer, "%d", len);
    send_message(buffer, dest_sockfd);

    // Send File
    while(!feof(fd)) {
        total += fread(buffer, 1, BUF_SIZE, fd);
        send_message(buffer, dest_sockfd);
        printf("Total bytes sent till now: %d\n", total);
        bzero(buffer, BUF_SIZE);
    }
    printf("File Succesfully sent to %s\n\n", clientAddr);
}

void accept_macros_file(int dest_sockfd) {
    int file_size, status, len;
    char buffer[BUF_SIZE];
    FILE* fd;

    fd = fopen(MACROS_FILE_NAME, "w");
    if(fd == NULL) {
        printf("Out of Memory on Server.\n");
        exit(1);
    }

    // Recieve File Size
    status = recieve_message(sockfd, buffer);
    file_size = atoi(buffer);
    printf("File Size is: %d\n", file_size);

    // Recieve File
    while(file_size > 0 && ((len = recieve_message(sockfd, buffer)) > 0)) {
        fwrite(buffer, 1, len, fd);
        file_size -= len;
        printf("Received: %d bytes, Remaining: %d bytes\n", len, file_size);
    }
    printf("File Succesfully Recieved\n");
    fclose(fd);
}

void communicate(char* clientAddr, int dest_sockfd, struct sockaddr_in cl_addr) {
    char recv_buf[BUF_SIZE], send_buf[BUF_SIZE];
    int status, len;

    status = recieve_message(dest_sockfd, recv_buf);
    if(status < 0) {
        printf("Error in Recieving Request from Client.\n");
        sprintf(send_buf, "%s", "err");
        send_message(send_buf, dest_sockfd);
    }

    if(strcmp(recv_buf, cmds[0]) == 0)
        accept_macros_file(dest_sockfd);
    else if(strcmp(recv_buf, cmds[1]) == 0)
        send_macros_file(clientAddr, dest_sockfd);
}

void *input(void *data) {
    char *s = (char*) malloc(BUF_SIZE*sizeof(char));
    while(1) {
        scanf("%s",s);
        printf("%s\n",s);
        if(strncasecmp(s, "exit", 4) == 0)
            break;
    }
    free(s);
    shutdown(sockfd, SHUT_RDWR);
    exit(0);
}

int main() {
    int status, newsockfd;
    char buffer[BUF_SIZE], clientAddr[CLADDR_LEN];
    struct sockaddr_in cl_addr;
    socklen_t addr_size;
    pid_t childpid;
    pthread_t inputThread;

    pthread_create(&inputThread, NULL, input, NULL);
 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("Error creating Server Socket!\n");
        exit(1);
    }
    printf("Server Socket Created.\n");
 
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = PORT;
 
    status = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    if(status < 0) {
        printf("Error binding!\n");
        close(sockfd);
        exit(1);
    }
    printf("Binding Completed.\n");

    printf("Waiting for connections...\n");
    listen(sockfd, 5);

    while(1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cl_addr, &addr_size);
        if(newsockfd < 0) {
            printf("Error accepting connection!\n");
            exit(1);
        }

        inet_ntop(AF_INET, &(cl_addr.sin_addr), clientAddr, CLADDR_LEN);
        printf("New connection accepted from %s: %d\n", clientAddr, ntohs(cl_addr.sin_port));
        if((childpid = fork()) == 0) {
            close(sockfd); 
            communicate(clientAddr, newsockfd, cl_addr);
            printf("Disconnected from %s:%d\n", clientAddr, ntohs(cl_addr.sin_port));
        }
        close(newsockfd);
    }
    close(sockfd);
    return 0;
}