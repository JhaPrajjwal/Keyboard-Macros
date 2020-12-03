#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h> 
#include<sys/socket.h>  
#include<string.h>  
#include<netinet/in.h> 
#include<sys/stat.h> 
#include<netdb.h>
#include<arpa/inet.h>
  
#define PORT 4444 
#define BUF_SIZE 360
#define MACROS_FILE_NAME "/home/pj/test.txt"

int sockfd;
struct sockaddr_in addr;
const char cmds[2][BUF_SIZE] = {"PUSH", "PULL"};

int send_message(char msg[], int dest_sockfd) {
    printf("Send: %s\n", msg);
    return sendto(dest_sockfd, msg, BUF_SIZE, 0, (struct sockaddr *) &addr, sizeof(addr));
}

int recieve_message(int dest_sockfd, char recv_buf[]) {
    memset(recv_buf, 0, BUF_SIZE);
    int len = recvfrom(dest_sockfd, recv_buf, BUF_SIZE, 0, NULL, NULL);
    printf("Rec: %s\n", recv_buf);
    return len;
}

void send_macros_file(int dest_sockfd) {
    int status, len, total = 0;
    long int offset = 0;
    char buffer[BUF_SIZE], buffer2[BUF_SIZE];
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
    sprintf(buffer2, "%d", len);
    send_message(buffer2, dest_sockfd);
    // printf("%d, %s\n", len, buffer2);
    // Send File
    while(!feof(fd)) {
        total += fread(buffer, 1, BUF_SIZE, fd);
        send_message(buffer, dest_sockfd);
        printf("Total bytes sent till now: %d\n", total);
        bzero(buffer, BUF_SIZE);
    }
    printf("File Succesfully sent to Server\n");
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
    printf("a\n");
    // Recieve File
    // len = recieve_message(sockfd, buffer);
    printf("b\n");
    printf("%d\n", len);
    while(file_size > 0 && ((len = recieve_message(sockfd, buffer)) > 0)) {
        fwrite(buffer, 1, len, fd);
        file_size -= len;
        printf("Received: %d bytes, Remaining: %d bytes\n", len, file_size);
    }
    printf("c\n");
    printf("File Succesfully Recieved\n");
    fclose(fd);
}

void communicate(const char* command) {
    int status;
    FILE* fp;

    status = send_message(command, sockfd);

    if(strcmp(command, cmds[0]) == 0) {
        send_macros_file(sockfd);
    }
    else if(strcmp(command, cmds[1]) == 0) {
        accept_macros_file(sockfd);
    }
}

int main(int argc, char**argv) {
    int status;
    char *serverAddr, *command;

    if(argc < 3) {
        printf("usage: ./client <ip address> <command>\n");
        exit(1);  
    }

    serverAddr = argv[1]; 
    command = argv[2];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  
    if(sockfd < 0) {  
        printf("Error in creating socket!\n");  
        exit(1);  
    }  
    printf("Client Socket Created\n");   

    memset(&addr, 0, sizeof(addr));  
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(serverAddr); 
    addr.sin_port = PORT;     

    status = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));  
    if(status < 0) {  
        printf("Error connecting to the server!\n");  
        exit(1);  
    }  
    printf("Connected to the server.\n\n");  

    communicate(command);
    close(sockfd);
    return 0;    
}  