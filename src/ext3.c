#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "./core/superblock.h"
#include "./core/dir_entry.h"
#include "./core/inode.h"
#include "./core/block_group_table.h"


#define MAX_FILE_NAME_LENGTH = 8;


int main(int argc, char *argv[])
{

    int fd = open("../disks/test_disk.img", O_RDWR);
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

    uint32_t inode_table_offset = bgd[0].addr_inode_table * real_block_size;
    rc = lseek(fd, inode_table_offset + (2 - 1) * sizeof(struct ext3_inode), SEEK_SET);
    struct ext3_inode root;
    rc = read(fd, &root, sizeof(struct ext3_inode));

    printf("\nRoot Inode Information:\n");
    printf("Size: %u bytes\n", root.size);

    printf("\nDirect Blocks:\n");
    for (int i = 0; i < 12; i++)
    {
        if (root.block[i] != 0)
        {
            printf("\nBlock[%d]: %u\n", i, root.block[i]);
            uint32_t block_addr = root.block[i] * real_block_size;
            lseek(fd, block_addr, SEEK_SET);

            unsigned char block_buffer[real_block_size];
            read(fd, block_buffer, real_block_size);

            unsigned int offset = 0;
            while (offset < real_block_size)
            {
                struct ext3_dir_entry *entry = (struct ext3_dir_entry *)(block_buffer + offset);
                if (entry->inode == 0)
                    break;

                printf("\nInode number: %u\n", entry->inode);
                printf("Record length: %u\n", entry->rec_len);
                printf("Name length: %u\n", entry->name_len);
                printf("File type: %u\n", entry->file_type);

                char name[256] = {0};
                memcpy(name, entry->name, entry->name_len);
                printf("Name: %s\n", name);

                offset += entry->rec_len;
            }
        }
    }

    



    return 0;
}