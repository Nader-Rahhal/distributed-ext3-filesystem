#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include "./core/superblock.h"

struct ext2_block_group_descriptor
{
    uint32_t addr_block_usage_bitmap;
    uint32_t addr_inode_usage_bitmap;
    uint32_t addr_inode_table;
    uint16_t unallocated_blocks_in_group;
    u_int16_t unallocated_inodes_in_group;
    u_int16_t num_directories_in_group;
    uint8_t unused[13];
};

struct ext3_inode
{
    uint16_t mode;            // File mode (2 bytes)
    uint16_t uid;             // Lower 16 bits of User ID (2 bytes)
    uint32_t size;            // Lower 32 bits of size (4 bytes)
    uint32_t atime;           // Access time - part of ACMD Times (16 bytes total)
    uint32_t ctime;           // Creation time
    uint32_t mtime;           // Modification time
    uint32_t dtime;           // Deletion time
    uint16_t gid;             // Lower 16 bits of Group ID (2 bytes)
    uint16_t links_count;     // Link Count (2 bytes)
    uint32_t blocks;          // Sector Count (4 bytes)
    uint32_t flags;           // Assorted flags (4 bytes)
    uint32_t unused1;         // Unused (4 bytes)
    uint32_t block[12];       // Direct Pointers (48 bytes - 12 pointers)
    uint32_t single_indirect; // Single Indirect Pointer (4 bytes)
    uint32_t double_indirect; // Double Indirect Pointer (4 bytes)
    uint32_t triple_indirect; // Triple Indirect Pointer (4 bytes)
    uint32_t misc_info;       // NFS gen number, etc (8 bytes)
    uint32_t upper_size;      // Upper 32 bits of size
    uint32_t fragment_info;   // Fragment info (9 bytes)
    uint16_t unused2;         // Unused (2 bytes)
    uint16_t upper_uid;       // Upper 16 bits of User ID (2 bytes)
    uint16_t upper_gid;       // Upper 16 bits of Group ID (2 bytes)
    uint16_t unused3;         // Unused (2 bytes)
    uint8_t padding[136];
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
    uint32_t real_block_size = 1024 << sb.block_size;

    rc = lseek(fd, 4096, SEEK_SET);

    struct ext2_block_group_descriptor bgd[total_groups];
    for (uint32_t i = 0; i < total_groups; i++)
    {
        rc = read(fd, &bgd[i], sizeof(struct ext2_block_group_descriptor));
    }

    uint32_t inode_table_offset = bgd[0].addr_inode_table * real_block_size;
    rc = lseek(fd, inode_table_offset + (2 - 1) * sizeof(struct ext3_inode), SEEK_SET);
    struct ext3_inode root;
    rc = read(fd, &root, sizeof(struct ext3_inode));

    printf("\nRoot Inode Information:\n");
    printf("Mode: %o (in octal)\n", root.mode); // octal for permissions
    printf("Size: %u bytes\n", root.size);
    printf("Links: %u\n", root.links_count);
    printf("UID: %u\n", root.uid);
    printf("GID: %u\n", root.gid);
    printf("Access Time: %u\n", root.atime);
    printf("Modification Time: %u\n", root.mtime);
    printf("Creation Time: %u\n", root.ctime);

    printf("\nDirect Blocks:\n");
    for (int i = 0; i < 12; i++)
    {
        if (root.block[i] != 0)
        { // Only print non-zero blocks
            printf("Block[%d]: %u\n", i, root.block[i]);
            u_int32_t block_addr = root.block[i] * real_block_size;
        }
    }

    return 0;
}