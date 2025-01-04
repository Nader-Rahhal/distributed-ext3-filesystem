#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "ext3_operations.h"

int main(int argc, char *argv[])
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

    if (strcmp(argv[1], "-r") == 0)
    {
        ext3_read_file_contents(fd, atoi(argv[2]), real_block_size, bgd, 0);
    }

    else if (strcmp(argv[1], "-rr") == 0)
    {
        ext3_read_root(fd, 2, real_block_size, bgd);
    }

    else if (strcmp(argv[1], "-w") == 0)
    {
        uint32_t inode_number;
        uint32_t offset;
        char contents[256];

        printf("Input inode number: ");
        scanf("%u", &inode_number);

        printf("Input offset: ");
        scanf("%u", &offset);

        printf("Input File Contents: ");
        scanf("%s", contents);

        uint32_t content_length = strlen(contents);
        ext3_write(fd, bgd, inode_number, offset, real_block_size, &sb, contents, content_length);
    }

    else if (strcmp(argv[1], "-c") == 0)
    {

        char filename[256];

        printf("Input File Name: ");
        scanf("%s", filename);

        ext3_create_file(fd, bgd, real_block_size, &sb, filename);
    }

    else if (strcmp(argv[1], "-d") == 0)
    {
        ext3_delete(fd, bgd, &sb, atoi(argv[2]), real_block_size);
    }

    else if (strcmp(argv[1], "-o") == 0)
    {
        char buffer[] = "/test2";
        printf("Size: %zu\n", sizeof(buffer));
        int inode = ext3_open(fd, buffer, real_block_size, bgd);
        printf("Inode: %d\n", inode);
    }

    // directry support
    // finish Make File
    // cleanup code
    // add journaling
    // add deletion
    // photo support
    // create sockets and server

    close(fd);
    return 0;
}