#include <stdint.h>

struct ext2_inode {
    uint16_t type_and_permissions;
    u_int16_t user_id;
    u_int32_t size_lower_32;
    u_int32_t last_access_time;
    u_int32_t creation_time;
    u_int32_t last_modification_time;
    u_int32_t deletion_time;
    uint16_t group_id;
    uint16_t hard_links;
    uint32_t disk_sectors_being_used;
    u_int32_t flags;
    u_int32_t os_value;
    uint32_t dir_ptr_0;
    uint32_t dir_ptr_1;
    uint32_t dir_ptr_2;
    uint32_t dir_ptr_3;
    uint32_t dir_ptr_4;
    uint32_t dir_ptr_5;
    uint32_t dir_ptr_6;
    uint32_t dir_ptr_7;
    uint32_t dir_ptr_8;
    uint32_t dir_ptr_9;
    uint32_t dir_ptr_10;
    uint32_t dir_ptr_11;
    uint32_t singly_indir_block_ptr;
    uint32_t doubly_indir_block_ptr;
    uint32_t triply_indir_block_ptr;
    uint32_t gen_number;
    uint32_t extended_attr_block;
    u_int32_t upper_32;
    uint32_t block_addr_fragment;
    uint32_t os_value_2;
};