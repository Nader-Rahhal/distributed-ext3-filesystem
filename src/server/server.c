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
#include "../../env.h"

#define CREATE 0
#define READ 1
#define WRITE 2
#define DELETE 3

struct CreateFileMessage
{
    uint8_t operation_code;
    char filename[256];
};

struct WriteFileMessage
{
    uint8_t op_code;
    uint32_t offset;
    char data[256];
};

struct ReadFileMessage
{
    uint8_t op_code;
    uint32_t inode;
};

struct ReadRootMessage
{
    uint8_t op_code;
    uint32_t inode;
};

int main()
{

    int fd = open("./disks/test_disk.img", O_RDWR);
    if (fd < 0)
    {
        perror("Error opening disk image");
        return -1;
    }

    int rc = lseek(fd, 1024, SEEK_SET);
    struct ext3_superblock sb;
    rc = read(fd, &sb, sizeof(struct ext3_superblock));
    if (rc < 0)
    {
        perror("Error reading disk");
        return -1;
    }

    uint32_t blocks_per_group = sb.blocks_per_group;
    uint32_t total_groups = (sb.total_blocks + blocks_per_group - 1) / blocks_per_group;
    uint32_t real_block_size = 1024 << sb.block_size;

    rc = lseek(fd, 4096, SEEK_SET);

    struct ext3_block_group_descriptor bgd[total_groups];
    for (uint32_t i = 0; i < total_groups; i++)
    {
        rc = read(fd, &bgd[i], sizeof(struct ext3_block_group_descriptor));
    }

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

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket < 0)
        {
            perror("accept failed");
            exit(1);
        }

        union MessageBuffer
        {
            struct ReadFileMessage readMsg;
            struct WriteFileMessage writeMsg;
            struct CreateFileMessage createMsg;
            struct ReadRootMessage readRootMsg;
        };

        union MessageBuffer buffer; // Change from char* to union
        int bytes_received = recv(client_socket, &buffer, sizeof(buffer), 0);
        printf("Received %d Bytes of data\n", bytes_received);

        switch (buffer.readMsg.op_code)
        {
        case READ: {
            uint32_t size;
            printf("READ request for inode: %u\n", buffer.readMsg.inode);
            char* contents = ext3_read_file_contents(fd, buffer.readMsg.inode, real_block_size, bgd, &size);

            if (contents) {
                send(client_socket, &size, sizeof(size), 0);
                send(client_socket, contents, size, 0);
                free(contents);
            }

            break;
        }
        case WRITE:
            printf("WRITE request with data: %s\n", buffer.writeMsg.data);
            break;
        case CREATE:
            printf("CREATE request for file: %s\n", buffer.createMsg.filename);
            break;
        }

        close(client_socket);
    }

    // Receive

    // Send Ack

    return 0;
}