#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "ext3_operations.c"

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
        ext3_read_file_contents(fd, atoi(argv[2]), real_block_size, bgd);
    }

    if (strcmp(argv[1], "-rr") == 0)
    {
        ext3_read_root_directory(fd, 2, real_block_size, bgd);
    }

    if (strcmp(argv[1], "-w") == 0)
    {
        char filename[256];
        printf("Input File Name: ");
        scanf("%s", filename);

        char contents[256];
        printf("Input File Contents: ");
        scanf("%s", contents);

        uint32_t content_length = strlen(contents);
        ext3_write(fd, bgd, real_block_size, &sb, contents, content_length, filename);
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