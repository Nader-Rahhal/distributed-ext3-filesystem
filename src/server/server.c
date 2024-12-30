#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "../ext3_operations.c"
#include "../../env.c"

extern char ip_addr[];

int main(){
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(8080);
    inet_aton(ip_addr, &(sa.sin_addr));
    // Bind Socket
    bind(server_socket, (struct sockaddr *)&sa, sizeof(sa));

    // Listen 

    listen(server_socket, 5);

    // Accept

    while(1){
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket < 0) {
            perror("accept failed");
            exit(1);
        }

       char *buffer = malloc(32 * sizeof(char));
       int bytes_received = recv(client_socket, buffer, 32, 0);

       printf("Received %d Bytes of data\n", bytes_received);
       printf("Data: %s\n", buffer);


       close(client_socket);
       free(buffer);
    }

    

    // Receive

    // Send Ack

    return 0;
}