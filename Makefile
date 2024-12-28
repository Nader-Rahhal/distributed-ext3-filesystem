DISK_DIR = ./disks
SRC_DIR = ./src
CC = gcc

main: $(SRC_DIR)/main.c
	$(CC) -o main $(SRC_DIR)/main.c

test_disk.img: | $(DISK_DIR)
	rm -f $(DISK_DIR)/test_disk.img
	dd if=/dev/zero of=$(DISK_DIR)/test_disk.img bs=1M count=1024
	/opt/homebrew/Cellar/e2fsprogs/1.47.1/sbin/mkfs.ext3 $(DISK_DIR)/test_disk.img

$(DISK_DIR):
	mkdir -p $(DISK_DIR)
