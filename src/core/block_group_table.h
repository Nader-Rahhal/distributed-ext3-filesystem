#include <stdint.h>

struct ext2_block_group_descriptor {
    uint32_t addr_block_usage_bitmap;
    uint32_t addr_inode_usage_bitmap;
    uint32_t addr_inode_table;
    uint16_t unallocated_blocks_in_group;
    u_int16_t unallocated_inodes_in_group;
    u_int16_t num_directories_in_group;
    uint8_t unused[13];
};