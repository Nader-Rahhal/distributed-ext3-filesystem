#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

struct ext3_superblock
{
    uint32_t total_inodes;
    u_int32_t total_blocks;
    u_int32_t reserved_blocks;
    u_int32_t unallocated_blocks;
    u_int32_t unallocated_inodes;
    u_int32_t block_containing_superblock;
    u_int32_t block_size;
    u_int32_t fragment_size;
    u_int32_t blocks_per_group;
    u_int32_t fragments_per_group;
    u_int32_t inodes_per_group;
    u_int32_t mount_time;
    u_int32_t write_time;
    uint16_t times_mounted;
    uint16_t allowed_mounts;
    uint16_t signature;
    uint16_t fs_state;
    uint16_t error_handling_method;
    uint16_t minor_version;
    u_int32_t last_cc;
    u_int32_t time_btwn_ccs;
    u_int32_t os_parent_id;
    u_int32_t major_version;
    uint16_t uid_reserved_blocks;
    uint16_t gid_reserved_blocks;
    u_int32_t first_inode;
    uint16_t inode_size;
    uint16_t block_group;
    u_int32_t optional_features;
    u_int32_t required_features;
    u_int32_t unsupported_features;
    u_int8_t fs_uid[16];
    u_int8_t volume_name[16];
    u_int8_t misc[72];
    u_int8_t journal_id[16];
    u_int32_t journal_inode;
    u_int32_t journel_device;
    u_int32_t orphan_inode_list_head;
    u_int8_t unused[778];
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
    printf("Total Inodes: %hu\n", sb.signature);
    return 0;
}