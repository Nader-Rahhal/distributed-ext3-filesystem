#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define CREATE 0
#define READ   1
#define WRITE  2
#define DELETE 3

int main() {

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    if (inet_aton("192.168.1.45", &(server_addr.sin_addr)) == 0) {
        perror("Invalid address");
        exit(1);
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    char command;
    printf("Input Command: ");
    scanf("%c", command);

    int mode = -1;
    switch(command){
        case 'C':
            mode = CREATE;
            break;
        case 'W':
            mode = WRITE;
            break;
        case 'R':
            mode = READ;
            break;
        case 'D':
            mode = DELETE;
            break;
    }

    char *message = "Hello, Server!";
    
    ssize_t bytes_sent = send(client_socket, message, strlen(message), 0);
    if (bytes_sent < 0) {
        perror("Send failed");
        exit(1);
    }

    printf("Sent %zd bytes: %s\n", bytes_sent, message);

    close(client_socket);

    return 0;
}