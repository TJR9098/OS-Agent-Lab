#pragma once

#include <stdint.h>
#include <stddef.h>

#define FAT32_MAX_NAME 255

struct fat32_dirent {
  char name[FAT32_MAX_NAME + 1];
  uint8_t attr;
  uint32_t cluster;
  uint32_t size;
};

struct fat32_fs {
  uint32_t part_lba;
  uint32_t fat_lba;
  uint32_t data_lba;
  uint32_t reserved_sectors;
  uint32_t total_sectors;
  uint32_t sectors_per_fat;
  uint32_t num_fats;
  uint32_t cluster_count;
  uint32_t sectors_per_cluster;
  uint32_t bytes_per_sector;
  uint32_t root_cluster;
  uint32_t alloc_hint;
};

int fat32_mount(struct fat32_fs *fs);
int fat32_read_root(struct fat32_fs *fs, struct fat32_dirent *ents, size_t max, size_t *out_count);

struct vnode;
struct vnode *fat32_init(void);
