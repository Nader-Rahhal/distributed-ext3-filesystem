#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include "./core/superblock.h"

struct ext2_block_group_descriptor {
    uint32_t addr_block_usage_bitmap;
    uint32_t addr_inode_usage_bitmap;
    uint32_t addr_inode_table;
    uint16_t unallocated_blocks_in_group;
    u_int16_t unallocated_inodes_in_group;
    u_int16_t num_directories_in_group;
    uint8_t unused[13];
};

int main()
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

    rc = lseek(fd, 4096, SEEK_SET);


    struct ext2_block_group_descriptor bgd[total_groups];
    for (uint32_t i = 0; i < total_groups; i++){
        rc = read(fd, &bgd[i], sizeof(struct ext2_block_group_descriptor));
       printf("\nBlock Group %u:\n", i);
        printf("  Block bitmap at block: %u\n", bgd[i].addr_block_usage_bitmap);
        printf("  Inode bitmap at block: %u\n", bgd[i].addr_inode_usage_bitmap);
        printf("  Inode table at block: %u\n", bgd[i].addr_inode_table);
        printf("  Free blocks: %u\n", bgd[i].unallocated_blocks_in_group);
        printf("  Free inodes: %u\n", bgd[i].unallocated_inodes_in_group);
    }

    rc = lseek(fd, 4096, SEEK_CUR);

    

    return 0;
}