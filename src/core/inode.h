#include <stdint.h>

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
