#include <stdint.h>

struct ext3_dir_entry
{
    uint32_t inode;    // Inode number
    uint16_t rec_len;  // Total entry length
    uint8_t name_len;  // Name length
    uint8_t file_type; // File type
    char name[];       // File name (variable length)
};
