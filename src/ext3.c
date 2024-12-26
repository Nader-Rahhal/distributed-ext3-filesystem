#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "./core/superblock.h"
#include "./core/dir_entry.h"
#include "./core/inode.h"
#include "./core/block_group_table.h"

#define MAX_FILE_NAME_LENGTH = 8;

void ByteToBin(uint8_t byte)
{
    for (int i = 7; i >= 0; i--)
    {
        printf("%d", (byte >> i) & 1);
    }
}

void ext3_read(int fd, uint32_t inode_number, uint32_t block_size, struct ext3_block_group_descriptor *bgd)
{
    uint32_t inode_table_offset = bgd[0].addr_inode_table * block_size;
    int rc = lseek(fd, inode_table_offset + (inode_number - 1) * sizeof(struct ext3_inode), SEEK_SET);
    struct ext3_inode root;
    rc = read(fd, &root, sizeof(struct ext3_inode));
    printf("Reading Inode %d:\n", inode_number);
    printf("Type and Permissions 0x%08X:\n", root.mode);
    printf("Flags: 0x%08X\n", root.flags);
    printf("Links: %d\n", root.links_count);
    for (int i = 0; i < 12; i++)
    {
        if (root.block[i] != 0)
        {
            printf("\nBlock[%d]: %u\n", i, root.block[i]);
            uint32_t block_addr = root.block[i] * block_size;
            lseek(fd, block_addr, SEEK_SET);

            unsigned char block_buffer[block_size];
            read(fd, block_buffer, block_size);

            unsigned int offset = 0;
            while (offset < block_size)
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
}

int update_block_bitmap_and_return_free_block(int fd, uint32_t bytes_to_read, uint32_t real_block_size, struct ext3_block_group_descriptor *bgd, struct ext3_superblock *sb)
{
    for (size_t k = 0; k < sizeof(bgd); k++)
    {
        uint32_t block_bitmap_addr = bgd[k].addr_block_usage_bitmap;
        lseek(fd, block_bitmap_addr * real_block_size, SEEK_SET);
        uint8_t block_bitmap[sb->blocks_per_group / 8];
        read(fd, block_bitmap, sizeof(block_bitmap));
        for (int l = 0; l < bytes_to_read; l++)
        {
            if (block_bitmap[l] != 0xff)
            {
                uint8_t byte = block_bitmap[l];
                int bit_position = 0;
                while (bit_position < 8)
                {
                    if (!(byte & (1 << bit_position)))
                    {
                        int absolute_bit_position = (l * 8) + bit_position;
                        int block_number = absolute_bit_position + 1;
                        printf("\nFound first free block: %d in block bitmap %zu\n", block_number, k);
                        block_bitmap[l] = byte | (1 << bit_position);
                        lseek(fd, block_bitmap_addr * real_block_size, SEEK_SET);
                        // write(fd, block_bitmap, sizeof(block_bitmap));
                        uint32_t block_addr = block_number * real_block_size;
                        return block_number;
                    }
                    bit_position++;
                }
            }
        }
    }
    return -1;
}

uint32_t calculate_file_size(uint32_t *buffer[]){
    uint32_t count = 0;
    while (buffer[count]){
        count++;
    }
    return count;
}

void ext3_write(int fd, struct ext3_block_group_descriptor *bgd, uint32_t real_block_size, struct ext3_superblock *sb, uint32_t *buffer, uint32_t length)
{
    // 1. Find free inode in inode bitmap
    for (size_t i = 0; i < sizeof(bgd); i++)
    {
        uint32_t inode_bitmap_addr = bgd[i].addr_inode_usage_bitmap;
        uint32_t inode_table_addr = bgd[i].addr_inode_table;
        int rc = lseek(fd, inode_bitmap_addr * real_block_size, SEEK_SET);
        uint8_t bitmap[sb->inodes_per_group / 8];
        read(fd, &bitmap, sizeof(bitmap));

        printf("Bitmap contents of bitmap %zu:\n", i);
        int bytes_to_read = sb->inodes_per_group / 8;
        for (int j = 0; j < bytes_to_read; j++)
        {
            if (bitmap[j] != 0xff)
            {
                uint8_t byte = bitmap[j];
                int bit_position = 0;
                while (bit_position < 8)
                {
                    if (!(byte & (1 << bit_position)))
                    {
                        int absolute_bit_position = (j * 8) + bit_position;
                        int inode_number = absolute_bit_position + 1; // +1 because inodes start at 1
                        printf("\nFound first free inode: %d in inode map %zu\n", inode_number, i);

                        // 2. Update Inode Number in Inode Bitmap
                        bitmap[j] = byte | (1 << bit_position);
                        lseek(fd, inode_bitmap_addr * real_block_size, SEEK_SET);
                        // write(fd, bitmap, sizeof(bitmap));

                        // 3. Find free block in block bitmap
                        int free_block = update_block_bitmap_and_return_free_block(fd, sb->blocks_per_group / 8, real_block_size, bgd, sb);
                        

                        // 4. Update Inode Table by creating inode
                        uint32_t inode_table_offset = bgd[0].addr_inode_table * real_block_size;
                        uint32_t inode_offset = ((inode_number - 1) * sizeof(struct ext3_inode)) + inode_table_offset;
                        lseek(fd, inode_offset, SEEK_SET);
                        struct ext3_inode new_inode;
                        new_inode.mode = 0x81A4;
                        new_inode.size = length;
                        memset(new_inode.block, 0, sizeof(new_inode.block));
                        new_inode.block[0] = free_block;

                        // time related fields
                        time_t sec;
                        sec = time(NULL);

                        new_inode.mtime = sec;
                        new_inode.atime = sec;
                        new_inode.ctime = sec;
                        new_inode.flags = 0x00000000;
                        new_inode.links_count = 1; // because file is in root directory?

                        // write(fd, &new_inode, sizeof(new_inode));

                        // 5. Write the Actial File Data to Block
                        uint32_t free_block_addr = free_block * real_block_size;
                        lseek(fd, free_block_addr, SEEK_SET);
                        // ideally ensure that length < real_block_size
                        // write(fd, buffer, length);

                        // 6. Update Table Descriptor
                        bgd[i].unallocated_blocks_in_group--;
                        bgd[i].unallocated_inodes_in_group--;
                        uint32_t addr_of_table_desc = 2048 + (i * 32);
                        lseek(fd, addr_of_table_desc, SEEK_SET);
                        // write(fd, &bgd[i], sizeof(bgd[i]));

                        // 7. Update Superblock
                        sb->unallocated_blocks--;
                        sb->unallocated_inodes--;
                        uint32_t addr_of_superblock = 1024;
                        lseek(fd, 1024, SEEK_SET);
                        // write(fd, &sb, 1024);


                        return;
                    }
                    bit_position++;
                }
            }
        }
    }
}

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

    // ext3_read(fd, atoi(argv[1]), real_block_size, bgd);
    uint32_t *buffer[32];
    ext3_write(fd, bgd, real_block_size, &sb, buffer, 4);
    return 0;
}