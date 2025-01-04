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

struct CreateFileMessage {
    uint8_t operation_code;
    char filename[256];
};

struct WriteFileMessage {
    uint8_t  op_code;
    uint32_t offset;
    char data[256];
};

struct ReadFileMessage {
    uint8_t op_code;
    uint32_t inode;
};

struct ReadRootMessage {
    uint8_t op_code;
    uint32_t inode;
};

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

    if (inet_aton("192.168.0.18", &(server_addr.sin_addr)) == 0) {
        perror("Invalid address");
        exit(1);
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    struct ReadFileMessage readF;
    readF.op_code = READ;
    readF.inode = 12;

    char *message = "Hello, Server!";
    

    ssize_t bytes_sent = send(client_socket, &readF, sizeof(struct ReadFileMessage), 0);
    if (bytes_sent < 0) {
        perror("Send failed");
        exit(1);
    }

    printf("Sent %zd bytes: %s\n", bytes_sent, message);
    uint32_t size;
recv(client_socket, &size, sizeof(size), 0);
char* contents = malloc(size + 1);
recv(client_socket, contents, size, 0);
contents[size] = '\0';
printf("Received file contents (%u bytes):\n%s\n", size, contents);
free(contents);

    close(client_socket);

    return 0;
}