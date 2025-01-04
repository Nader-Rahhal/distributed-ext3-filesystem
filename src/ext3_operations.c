#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "ext3_operations.h"

void ext3_read_root(int fd, uint32_t inode_number, uint32_t block_size, struct ext3_block_group_descriptor *bgd)
{
    uint32_t inode_table_offset = bgd[0].addr_inode_table * block_size;
    int rc = lseek(fd, inode_table_offset + (inode_number - 1) * sizeof(struct ext3_inode), SEEK_SET);
    struct ext3_inode root;
    rc = read(fd, &root, sizeof(struct ext3_inode));
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
                if (entry->inode == 0 || entry->name_len == 0)
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

void update_data_block(int fd, int free_block, uint32_t real_block_size, const char *buffer, uint32_t length)
{
    uint32_t free_block_addr = free_block * real_block_size;
    lseek(fd, free_block_addr, SEEK_SET);
    write(fd, buffer, length);
    return;
}

void update_inode_table(int fd, uint32_t real_block_size, struct ext3_block_group_descriptor *bgd, int inode_number, int free_block, uint32_t length)
{
    uint32_t inode_table_offset = bgd[0].addr_inode_table * real_block_size;
    uint32_t inode_offset = ((inode_number - 1) * sizeof(struct ext3_inode)) + inode_table_offset;
    lseek(fd, inode_offset, SEEK_SET);

    struct ext3_inode new_inode;
    new_inode.mode = 0x81A4;
    new_inode.size = length;
    memset(new_inode.block, 0, sizeof(new_inode.block));
    new_inode.block[0] = free_block;

    time_t sec;
    sec = time(NULL);

    new_inode.mtime = sec;
    new_inode.atime = sec;
    new_inode.ctime = sec;
    new_inode.flags = 0x00000000;
    new_inode.links_count = 1;

    write(fd, &new_inode, sizeof(new_inode));
    return;
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
                        write(fd, block_bitmap, sizeof(block_bitmap));
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

void update_group_table_descriptor(int fd, size_t group, struct ext3_superblock *sb, struct ext3_block_group_descriptor *bgd, unsigned int new_inodes, unsigned int new_blocks)
{
    bgd[group].unallocated_blocks_in_group += new_blocks;
    bgd[group].unallocated_inodes_in_group += new_inodes;
    uint32_t addr_of_table_desc = 2048 + (group * 32);
    lseek(fd, addr_of_table_desc, SEEK_SET);
    write(fd, &bgd[group], sizeof(bgd[group]));
    return;
}

void update_superblock(int fd, struct ext3_superblock *sb, unsigned int new_inodes, unsigned int new_blocks)
{
    sb->unallocated_blocks += new_blocks;
    sb->unallocated_inodes += new_inodes;
    lseek(fd, 1024, SEEK_SET);
    write(fd, sb, sizeof(struct ext3_superblock));
    return;
}

void update_root_directory(int fd, int inode_number, const char *filename, uint32_t real_block_size, struct ext3_block_group_descriptor *bgd)
{
    struct ext3_dir_entry new_entry;

    new_entry.inode = inode_number;
    new_entry.file_type = 1;

    strcpy(new_entry.name, filename);
    new_entry.name_len = strlen(filename);

    new_entry.rec_len = sizeof(struct ext3_dir_entry) + new_entry.name_len;
    new_entry.rec_len = (new_entry.rec_len + 3) & ~3;

    struct ext3_inode root;
    uint32_t inode_table_offset = bgd[0].addr_inode_table * real_block_size;
    lseek(fd, inode_table_offset + sizeof(struct ext3_inode), SEEK_SET);

    read(fd, &root, sizeof(root));

    for (int curr_block = 0; curr_block < 12; curr_block++)
    {
        if (root.block[curr_block] != 0)
        {
            uint32_t block_addr = root.block[curr_block] * real_block_size;
            lseek(fd, block_addr, SEEK_SET);
            unsigned char block_buffer[real_block_size];
            read(fd, block_buffer, real_block_size);
            // Find where block is empty
            uint32_t block_offset = 0;
            struct ext3_dir_entry prev_entry;
            struct ext3_dir_entry curr_entry;

            while (block_offset < real_block_size)
            {
                lseek(fd, block_addr + block_offset, SEEK_SET);
                read(fd, &curr_entry, sizeof(struct ext3_dir_entry));

                if (curr_entry.inode == 0 || block_offset + curr_entry.rec_len >= real_block_size)
                {
                    // Found the last entry
                    uint32_t space_needed = ((sizeof(struct ext3_dir_entry) + new_entry.name_len + 3) & ~3);
                    uint32_t space_left = real_block_size - block_offset;

                    if (space_left >= space_needed)
                    {
                        // Write new entry
                        lseek(fd, block_addr + block_offset, SEEK_SET);
                        write(fd, &new_entry, new_entry.rec_len);
                        break;
                    }
                }
                block_offset += curr_entry.rec_len;
            }
            lseek(fd, block_addr + block_offset, SEEK_SET);
            write(fd, &new_entry, new_entry.rec_len);
        }
    }
    return;
}

void print_binary(unsigned char byte)
{
    for (int i = 7; i >= 0; i--)
    {
        printf("%d", (byte >> i) & 1);
    }
}

void delete_inode_bitmap_entry(int fd, struct ext3_block_group_descriptor *bgd, struct ext3_superblock *sb, uint32_t inode_tbd, uint32_t block_size)
{
    uint32_t bit = 0;
    for (size_t i = 0; i < sizeof(bgd); i++)
    {
        uint32_t inode_bitmap_addr = bgd[i].addr_inode_usage_bitmap;
        int rc = lseek(fd, inode_bitmap_addr * block_size, SEEK_SET);
        uint8_t bitmap[sb->inodes_per_group / 8];
        read(fd, &bitmap, sizeof(bitmap));
        for (int current_byte = 0; current_byte < sizeof(bitmap); current_byte++)
        {
            for (int k = 0; k < 8; k++)
            {
                if (bit == inode_tbd)
                {
                    uint32_t byte_containing_bit = bitmap[current_byte];

                    printf("Byte %d Before: ", current_byte);
                    print_binary(byte_containing_bit);
                    printf("\n");

                    uint8_t bit_offset = (inode_tbd % 8) - 1;
                    // printf("Bit Offset within byte: %d\n", bit_offset);
                    uint8_t mask = ~(1 << bit_offset);
                    // print_binary(mask);
                    byte_containing_bit = byte_containing_bit & mask;
                    bitmap[current_byte] = byte_containing_bit;

                    printf("Byte %d After:  ", current_byte);
                    print_binary(byte_containing_bit);
                    printf("\n");

                    lseek(fd, inode_bitmap_addr * block_size, SEEK_SET);
                    write(fd, &bitmap, sizeof(bitmap));

                    return;
                }

                bit++;
            }
        }
    }
}

void delete_block_bitmap_entries(int fd, struct ext3_block_group_descriptor *bgd, struct ext3_superblock *sb, struct ext3_inode new_inode, uint32_t block_size)
{
    for (int i = 0; i < 12; i++)
    {
        uint32_t block = new_inode.block[0];
        int bit_position = 0;
        for (size_t i = 0; i < sizeof(bgd); i++)
        {

            uint32_t block_bitmap_addr = bgd[i].addr_block_usage_bitmap;
            lseek(fd, block_bitmap_addr * block_size, SEEK_SET);
            uint8_t block_bitmap[sb->blocks_per_group / 8];
            read(fd, block_bitmap, sizeof(block_bitmap));

            for (int j = 0; j < sb->inodes_per_group; j++)
            {
                for (int k = 0; k < 8; k++)
                {
                    if (bit_position == block)
                    {
                        printf("Found bit %d\n", bit_position);
                        uint32_t byte = block_bitmap[j];
                        printf("Byte %d Before: ", j);
                        print_binary(byte);
                        printf("\n");
                        uint8_t bit_offset = (block % 8) - 1;
                        uint8_t mask = ~(1 << bit_offset);
                        byte = byte & mask;
                        block_bitmap[j] = byte;

                        printf("Byte %d After:  ", j);
                        print_binary(byte);
                        printf("\n");
                        lseek(fd, block_bitmap_addr * block_size, SEEK_SET);
                        write(fd, block_bitmap, sizeof(block_bitmap));
                        return;
                    }
                    bit_position++;
                }
            }
        }
    }
}

void clear_directory_entry(int fd, struct ext3_block_group_descriptor *bgd, uint32_t block_size, uint32_t inode_tbd) 
{
    struct ext3_inode root;
    uint32_t inode_table_offset = bgd[0].addr_inode_table * block_size;
    lseek(fd, inode_table_offset + sizeof(struct ext3_inode), SEEK_SET);
    read(fd, &root, sizeof(root));

    for (int i = 0; i < 12 && root.block[i] != 0; i++) {
        uint32_t block_addr = root.block[i] * block_size;
        uint32_t offset = 0;
        
        while (offset < block_size) {
            struct ext3_dir_entry entry;
            lseek(fd, block_addr + offset, SEEK_SET);
            read(fd, &entry, sizeof(struct ext3_dir_entry));
            
            if (entry.inode == 0) break;
            
            if (entry.inode == inode_tbd) {
                entry.inode = 0;
                lseek(fd, block_addr + offset, SEEK_SET);
                write(fd, &entry, sizeof(struct ext3_dir_entry));
                return;
            }
            
            offset += entry.rec_len;
        }
    }
}

void ext3_delete(int fd, struct ext3_block_group_descriptor *bgd, struct ext3_superblock *sb, uint32_t inode_tbd, uint32_t block_size)
{
    // 1. Alter Inode Bitmap
    delete_inode_bitmap_entry(fd, bgd, sb, inode_tbd, block_size);

    // 1.5. Get Inode Struct
    uint32_t block_group_containing_inode = (inode_tbd - 1) / sb->inodes_per_group;
    uint32_t inode_table_offset = bgd[block_group_containing_inode].addr_inode_table * block_size;
    uint32_t inode_offset = ((inode_tbd - 1) * sizeof(struct ext3_inode)) + inode_table_offset;
    
    lseek(fd, inode_offset, SEEK_SET);
    struct ext3_inode target_inode;
    read(fd, &target_inode, sizeof(target_inode));

    // 2. Alter Block Bitmap(s)
    delete_block_bitmap_entries(fd, bgd, sb, target_inode, block_size);

    // 3. Clear Directory Entry
    clear_directory_entry(fd, bgd, block_size, inode_tbd);

    // 4. Zero out the inode
    memset(&target_inode, 0, sizeof(target_inode));
    lseek(fd, inode_offset, SEEK_SET);
    write(fd, &target_inode, sizeof(target_inode));

    // 5. Adjust Superblock
    update_superblock(fd, sb, 1, 1);

    // 6. Adjust Table Descriptor
    update_group_table_descriptor(fd, 0, sb, bgd, 1, 1);
}

void ext3_write(int fd, struct ext3_block_group_descriptor *bgd, uint32_t inode_number, uint32_t offset, uint32_t block_size, struct ext3_superblock *sb, const char *buffer, uint32_t length) {
    // 1. Check Inode Struct for Blocks
    uint32_t block_group = (inode_number - 1) / sb->inodes_per_group;
    uint32_t inode_table_offset = bgd[block_group].addr_inode_table * block_size;
    int rc = lseek(fd, inode_table_offset + (inode_number - 1) * sizeof(struct ext3_inode), SEEK_SET);
    struct ext3_inode inode;
    rc = read(fd, &inode, sizeof(struct ext3_inode));
    
    uint32_t block_num = offset / 4096;
    uint32_t block_offset = offset % 4096;
    
    if (block_num >= 12) {
        printf("Beyond direct pointer implementation\n");
        return;
    }

    if (inode.block[block_num] == 0) {
        // Need to allocate new block
        int free_block = update_block_bitmap_and_return_free_block(fd, sb->blocks_per_group / 8, 
                                                                 block_size, bgd, sb);
        if (free_block < 0) {
            printf("No free blocks available\n");
            return;
        }
        
        // Update inode with new block
        inode.block[block_num] = free_block;
        
        // Write back updated inode
        lseek(fd, inode_table_offset + (inode_number - 1) * sizeof(struct ext3_inode), SEEK_SET);
        write(fd, &inode, sizeof(struct ext3_inode));
        
        // Write data to new block
        lseek(fd, free_block * block_size + block_offset, SEEK_SET);
        write(fd, buffer, length);
        
        // Update inode size if needed
        if (offset + length > inode.size) {
            inode.size = offset + length;
            lseek(fd, inode_table_offset + (inode_number - 1) * sizeof(struct ext3_inode), SEEK_SET);
            write(fd, &inode, sizeof(struct ext3_inode));
        }
    } else {
        // Block exists, just write to it
        lseek(fd, inode.block[block_num] * block_size + block_offset, SEEK_SET);
        write(fd, buffer, length);
        
        // Update inode size if needed
        if (offset + length > inode.size) {
            inode.size = offset + length;
            lseek(fd, inode_table_offset + (inode_number - 1) * sizeof(struct ext3_inode), SEEK_SET);
            write(fd, &inode, sizeof(struct ext3_inode));
        }
    }

    // Update Table Descriptor
    update_group_table_descriptor(fd, block_group, sb, bgd, -1, -1);
    // Update Superblock
    update_superblock(fd, sb, 0, -1);
}

int ext3_open(int fd, char path[], uint32_t block_size, struct ext3_block_group_descriptor *bgd)
{

    if (path[0] != '/')
    {
        perror("Path is invalid - must start with root directory\n");
        return -1;
    }

    char section[256];
    int l = 0;
    uint8_t found_dir = 0;
    for (int i = 1; i < 256; i++)
    {

        if (path[i] == '/')
        {
            uint32_t inode_table_offset = bgd[0].addr_inode_table * block_size;
            lseek(fd, inode_table_offset + (2 - 1) * sizeof(struct ext3_inode), SEEK_SET);
            struct ext3_inode root;
            read(fd, &root, sizeof(struct ext3_inode));
            for (int i = 0; i < 12; i++)
            {
                if (root.block[i] != 0)
                {
                    uint32_t block_addr = root.block[i] * block_size;
                    lseek(fd, block_addr, SEEK_SET);
                    unsigned char block_buffer[block_size];
                    read(fd, block_buffer, block_size);

                    unsigned int offset = 0;
                    while (offset < block_size)
                    {
                        struct ext3_dir_entry *entry = (struct ext3_dir_entry *)(block_buffer + offset);
                        if (entry->inode == 0)
                        {
                            printf("Could not locate directory\n");
                            return -1;
                        }

                        char name[256] = {0};
                        memcpy(name, entry->name, entry->name_len);
                        if (strcmp(section, name) == 0)
                        {
                            found_dir = 1;
                            break;
                        }

                        offset += entry->rec_len;
                    }
                }
                if (found_dir == 1)
                {
                    break;
                }
            }

            memset(section, 0, sizeof(section));
            l = 0;
        }

        else if (path[i] == '\0')
        {
            uint32_t inode_table_offset = bgd[0].addr_inode_table * block_size;
            lseek(fd, inode_table_offset + (2 - 1) * sizeof(struct ext3_inode), SEEK_SET);
            struct ext3_inode root;
            read(fd, &root, sizeof(struct ext3_inode));
            for (int i = 0; i < 12; i++)
            {
                if (root.block[i] != 0)
                {
                    uint32_t block_addr = root.block[i] * block_size;
                    lseek(fd, block_addr, SEEK_SET);
                    unsigned char block_buffer[block_size];
                    read(fd, block_buffer, block_size);

                    unsigned int offset = 0;
                    while (offset < block_size)
                    {
                        struct ext3_dir_entry *entry = (struct ext3_dir_entry *)(block_buffer + offset);
                        if (entry->inode == 0)
                        {
                            printf("Could not locate file\n");
                            return -1;
                        }

                        char name[256] = {0};
                        memcpy(name, entry->name, entry->name_len);
                        if (strcmp(section, name) == 0)
                        {
                            return entry->inode;
                        }

                        offset += entry->rec_len;
                    }
                }
            }
            memset(section, 0, sizeof(section));
            break;
        }
        else
        {
            section[l] = path[i];
            l++;
        }
    }

    return 0;
}

int ext3_create_file(int fd, struct ext3_block_group_descriptor *bgd, uint32_t real_block_size, struct ext3_superblock *sb, const char *filename)
{
    if (fd < 0 || !bgd || !sb || !filename) {
        return -1;
    }

    for (size_t i = 0; i < sizeof(bgd); i++)
    {
        uint32_t inode_bitmap_addr = bgd[i].addr_inode_usage_bitmap;
        uint32_t inode_table_addr = bgd[i].addr_inode_table;
        int rc = lseek(fd, inode_bitmap_addr * real_block_size, SEEK_SET);
        uint8_t bitmap[sb->inodes_per_group / 8];
        read(fd, &bitmap, sizeof(bitmap));
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
                        int inode_number = absolute_bit_position + 1;
                        bitmap[j] = byte | (1 << bit_position);
                        lseek(fd, inode_bitmap_addr * real_block_size, SEEK_SET);
                        write(fd, bitmap, sizeof(bitmap));
                        update_inode_table(fd, real_block_size, bgd, inode_number, 0, 0);
                        update_group_table_descriptor(fd, i, sb, bgd, -1, 0);
                        update_superblock(fd, sb, -1, 0);

                        // Eventually need to edit this to add to additional directories
                        update_root_directory(fd, inode_number, filename, real_block_size, bgd);

                        return 0;
                    }
                    bit_position++;
                }
            }
        }
    }
    return -1;
}


void ext3_read_file_contents(int fd, uint32_t inode_number, uint32_t block_size, struct ext3_block_group_descriptor *bgd)
{
    // Calculate offset to inode table
    uint32_t inode_table_offset = bgd[0].addr_inode_table * block_size;

    // Seek to the specific inode
    int rc = lseek(fd, inode_table_offset + (inode_number - 1) * sizeof(struct ext3_inode), SEEK_SET);
    if (rc < 0)
    {
        perror("Error seeking to inode");
        return;
    }

    // Read the inode
    struct ext3_inode inode;
    rc = read(fd, &inode, sizeof(struct ext3_inode));
    if (rc < 0)
    {
        perror("Error reading inode");
        return;
    }

    printf("File size: %u bytes\n", inode.size);

    // Read direct blocks
    for (int i = 0; i < 12; i++)
    {
        if (inode.block[i] == 0)
        {
            break; // No more blocks to read
        }

        // Calculate block address
        uint32_t block_addr = inode.block[i] * block_size;
        rc = lseek(fd, block_addr, SEEK_SET);
        if (rc < 0)
        {
            perror("Error seeking to data block");
            return;
        }

        // Read block contents
        unsigned char block_buffer[block_size];
        rc = read(fd, block_buffer, block_size);
        if (rc < 0)
        {
            perror("Error reading data block");
            return;
        }

        // Calculate how much of this block to print
        uint32_t bytes_to_print = block_size;
        if (i == 11 || inode.block[i + 1] == 0)
        { // Last block
            uint32_t bytes_so_far = i * block_size;
            if (inode.size > bytes_so_far)
            {
                bytes_to_print = inode.size - bytes_so_far;
            }
        }

        // Print block contents
        printf("Block %d contents:\n", inode.block[i]);
        for (uint32_t j = 0; j < bytes_to_print; j++)
        {
            printf("%c", block_buffer[j]);
        }
        printf("\n");
    }
}