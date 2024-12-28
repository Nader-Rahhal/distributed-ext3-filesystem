#include <stdlib.h>
#include <stdint.h>

#include "./core/superblock.h"
#include "./core/dir_entry.h"
#include "./core/inode.h"
#include "./core/block_group_table.h"

void update_data_block(int fd, int free_block, uint32_t real_block_size, const char *buffer, uint32_t length);

void update_inode_table(int fd, uint32_t real_block_size, struct ext3_block_group_descriptor *bgd, int inode_number, int free_block, uint32_t length);

int update_block_bitmap_and_return_free_block(int fd, uint32_t bytes_to_read, uint32_t real_block_size, struct ext3_block_group_descriptor *bgd, struct ext3_superblock *sb);

void update_root_directory(int fd, int inode_number, const char *filename, uint32_t real_block_size, struct ext3_block_group_descriptor *bgd);

void update_superblock(int fd, struct ext3_superblock *sb);

void update_group_table_descriptor(int fd, size_t group, struct ext3_superblock *sb, struct ext3_block_group_descriptor *bgd);

void ext3_write(int fd, struct ext3_block_group_descriptor *bgd, uint32_t real_block_size, struct ext3_superblock *sb, const char *buffer, uint32_t length, const char *filename);

void ext3_read_file_contents(int fd, uint32_t inode_number, uint32_t block_size, struct ext3_block_group_descriptor *bgd);

void ext3_read_root_directory(int fd, uint32_t inode_number, uint32_t block_size, struct ext3_block_group_descriptor *bgd);




