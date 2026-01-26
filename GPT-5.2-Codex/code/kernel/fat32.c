#include <stdint.h>
#include <stddef.h>

#include "blk.h"
#include "errno.h"
#include "fat32.h"
#include "log.h"
#include "string.h"
#include "vfs.h"

#define MBR_PART_OFFSET 446
#define MBR_SIG_OFFSET 510
#define MBR_SIG_VALUE 0xAA55

#define FAT_ATTR_READONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME 0x08
#define FAT_ATTR_DIR 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LFN 0x0F

#define FAT32_TYPE_0B 0x0B
#define FAT32_TYPE_0C 0x0C

#define FAT32_EOC 0x0FFFFFF8U
#define FAT32_MIN_CLUSTER 2U

struct mbr_part {
  uint8_t boot;
  uint8_t chs_start[3];
  uint8_t type;
  uint8_t chs_end[3];
  uint32_t lba_start;
  uint32_t sectors;
};

struct fat_dirent_raw {
  uint8_t name[11];
  uint8_t attr;
  uint8_t ntres;
  uint8_t crt_time_tenth;
  uint16_t crt_time;
  uint16_t crt_date;
  uint16_t lst_acc_date;
  uint16_t fst_clus_hi;
  uint16_t wrt_time;
  uint16_t wrt_date;
  uint16_t fst_clus_lo;
  uint32_t file_size;
} __attribute__((packed));

struct fat_lfn_raw {
  uint8_t ord;
  uint16_t name1[5];
  uint8_t attr;
  uint8_t type;
  uint8_t checksum;
  uint16_t name2[6];
  uint16_t fst_clus_lo;
  uint16_t name3[2];
} __attribute__((packed));

struct fat32_node {
  struct fat32_fs *fs;
  uint32_t cluster;
  uint32_t size;
  uint8_t attr;
  uint32_t dir_cluster;
  uint32_t dirent_lba;
  uint32_t dirent_off;
  uint64_t dirent_index;
  uint32_t dirent_count;
};

static struct fat32_fs g_fs;
static struct fat32_node g_nodes[256];
static uint8_t g_nodes_used[256];

static struct vnode *fat32_wrap_node(struct fat32_node *node, struct vnode *parent);

static uint16_t le16(const void *p) {
  const uint8_t *b = (const uint8_t *)p;
  return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

static uint32_t le32(const void *p) {
  const uint8_t *b = (const uint8_t *)p;
  return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static int name_eq_nocase(const char *a, const char *b) {
  while (*a && *b) {
    char ca = *a;
    char cb = *b;
    if (ca >= 'A' && ca <= 'Z') {
      ca = (char)(ca - 'A' + 'a');
    }
    if (cb >= 'A' && cb <= 'Z') {
      cb = (char)(cb - 'A' + 'a');
    }
    if (ca != cb) {
      return 0;
    }
    a++;
    b++;
  }
  return *a == *b;
}

static uint8_t fat32_lfn_checksum(const uint8_t shortname[11]) {
  uint8_t sum = 0;
  for (int i = 0; i < 11; i++) {
    sum = (uint8_t)(((sum & 1U) ? 0x80U : 0U) + (sum >> 1) + shortname[i]);
  }
  return sum;
}

static struct fat32_node *fat32_alloc_node(void) {
  for (size_t i = 0; i < sizeof(g_nodes_used); i++) {
    if (!g_nodes_used[i]) {
      g_nodes_used[i] = 1;
      memset(&g_nodes[i], 0, sizeof(g_nodes[i]));
      return &g_nodes[i];
    }
  }
  return NULL;
}

static uint32_t fat32_cluster_to_lba(const struct fat32_fs *fs, uint32_t cluster) {
  return fs->data_lba + (cluster - 2U) * fs->sectors_per_cluster;
}

static int fat32_read_fat_entry(const struct fat32_fs *fs, uint32_t cluster, uint32_t *out_next) {
  uint8_t sector[512];
  uint32_t fat_offset = cluster * 4U;
  uint32_t sector_idx = fat_offset / fs->bytes_per_sector;
  uint32_t sector_off = fat_offset % fs->bytes_per_sector;
  uint32_t lba = fs->fat_lba + sector_idx;

  if (fs->bytes_per_sector != 512) {
    return -EIO;
  }

  if (blk_read(lba, sector, 1) != 0) {
    return -EIO;
  }

  if (sector_off + 4U > 512U) {
    return -EIO;
  }

  *out_next = le32(sector + sector_off) & 0x0FFFFFFFU;
  return 0;
}

static int fat32_write_fat_entry(const struct fat32_fs *fs, uint32_t cluster, uint32_t value) {
  uint8_t sector[512];
  uint32_t fat_offset = cluster * 4U;
  uint32_t sector_idx = fat_offset / fs->bytes_per_sector;
  uint32_t sector_off = fat_offset % fs->bytes_per_sector;

  if (fs->bytes_per_sector != 512) {
    return -EIO;
  }
  for (uint32_t fat = 0; fat < fs->num_fats; fat++) {
    uint32_t lba = fs->fat_lba + fat * fs->sectors_per_fat + sector_idx;
    if (blk_read(lba, sector, 1) != 0) {
      return -EIO;
    }
    sector[sector_off] = (uint8_t)(value & 0xFFU);
    sector[sector_off + 1] = (uint8_t)((value >> 8) & 0xFFU);
    sector[sector_off + 2] = (uint8_t)((value >> 16) & 0xFFU);
    sector[sector_off + 3] = (uint8_t)((value >> 24) & 0x0FU);
    if (blk_write(lba, sector, 1) != 0) {
      return -EIO;
    }
  }
  return 0;
}

static int fat32_zero_cluster(const struct fat32_fs *fs, uint32_t cluster) {
  uint8_t zero[512];
  memset(zero, 0, sizeof(zero));
  uint32_t lba = fat32_cluster_to_lba(fs, cluster);
  for (uint32_t s = 0; s < fs->sectors_per_cluster; s++) {
    if (blk_write(lba + s, zero, 1) != 0) {
      return -EIO;
    }
  }
  return 0;
}

static int fat32_alloc_cluster(struct fat32_fs *fs, uint32_t *out_cluster) {
  uint32_t start = fs->alloc_hint;
  uint32_t max_cluster = fs->cluster_count + 1;
  if (start < FAT32_MIN_CLUSTER) {
    start = FAT32_MIN_CLUSTER;
  }
  for (int pass = 0; pass < 2; pass++) {
    for (uint32_t c = start; c <= max_cluster; c++) {
      uint32_t next = 0;
      if (fat32_read_fat_entry(fs, c, &next) != 0) {
        return -EIO;
      }
      if (next == 0) {
        if (fat32_write_fat_entry(fs, c, FAT32_EOC) != 0) {
          return -EIO;
        }
        if (fat32_zero_cluster(fs, c) != 0) {
          return -EIO;
        }
        fs->alloc_hint = c + 1;
        *out_cluster = c;
        return 0;
      }
    }
    start = FAT32_MIN_CLUSTER;
  }
  return -ENOSPC;
}

static int fat32_free_chain(const struct fat32_fs *fs, uint32_t cluster) {
  uint32_t max_cluster = fs->cluster_count + 1;
  while (cluster >= FAT32_MIN_CLUSTER && cluster <= max_cluster && cluster < FAT32_EOC) {
    uint32_t next = 0;
    if (fat32_read_fat_entry(fs, cluster, &next) != 0) {
      return -EIO;
    }
    if (fat32_write_fat_entry(fs, cluster, 0) != 0) {
      return -EIO;
    }
    cluster = next;
  }
  return 0;
}

static int fat32_get_cluster_at(const struct fat32_fs *fs, uint32_t start, uint32_t index, uint32_t *out_cluster) {
  uint32_t cluster = start;
  for (uint32_t i = 0; i < index; i++) {
    uint32_t next = 0;
    if (cluster < FAT32_MIN_CLUSTER || cluster >= FAT32_EOC) {
      return -ENOENT;
    }
    if (fat32_read_fat_entry(fs, cluster, &next) != 0) {
      return -EIO;
    }
    cluster = next;
  }
  if (cluster < FAT32_MIN_CLUSTER || cluster >= FAT32_EOC) {
    return -ENOENT;
  }
  *out_cluster = cluster;
  return 0;
}

static int fat32_ensure_chain(struct fat32_fs *fs, uint32_t *start, uint32_t needed) {
  if (needed == 0) {
    return 0;
  }
  uint32_t cluster = *start;
  uint32_t count = 0;
  uint32_t last = 0;

  if (cluster == 0) {
    if (fat32_alloc_cluster(fs, &cluster) != 0) {
      return -ENOSPC;
    }
    *start = cluster;
  }

  while (cluster < FAT32_EOC) {
    count++;
    last = cluster;
    if (count >= needed) {
      return 0;
    }
    uint32_t next = 0;
    if (fat32_read_fat_entry(fs, cluster, &next) != 0) {
      return -EIO;
    }
    if (next >= FAT32_EOC) {
      break;
    }
    cluster = next;
  }

  while (count < needed) {
    uint32_t new_cluster = 0;
    if (fat32_alloc_cluster(fs, &new_cluster) != 0) {
      return -ENOSPC;
    }
    if (fat32_write_fat_entry(fs, last, new_cluster) != 0) {
      return -EIO;
    }
    last = new_cluster;
    count++;
  }
  return 0;
}

static int fat32_dir_entry_location(const struct fat32_fs *fs,
                                    uint32_t dir_cluster,
                                    uint64_t entry_index,
                                    uint32_t *out_lba,
                                    uint32_t *out_off) {
  uint32_t entries_per_cluster = (fs->sectors_per_cluster * fs->bytes_per_sector) / 32U;
  uint32_t cluster_index = (uint32_t)(entry_index / entries_per_cluster);
  uint32_t entry_in_cluster = (uint32_t)(entry_index % entries_per_cluster);
  uint32_t cluster = 0;
  if (fat32_get_cluster_at(fs, dir_cluster, cluster_index, &cluster) != 0) {
    return -ENOENT;
  }
  uint32_t lba = fat32_cluster_to_lba(fs, cluster);
  uint32_t byte_off = entry_in_cluster * 32U;
  lba += byte_off / 512U;
  *out_lba = lba;
  *out_off = byte_off % 512U;
  return 0;
}

static int fat32_write_dir_entry(const struct fat32_fs *fs,
                                 uint32_t dir_cluster,
                                 uint64_t entry_index,
                                 const void *entry) {
  uint32_t lba = 0;
  uint32_t off = 0;
  uint8_t sector[512];
  if (fat32_dir_entry_location(fs, dir_cluster, entry_index, &lba, &off) != 0) {
    return -ENOENT;
  }
  if (blk_read(lba, sector, 1) != 0) {
    return -EIO;
  }
  memcpy(sector + off, entry, 32);
  if (blk_write(lba, sector, 1) != 0) {
    return -EIO;
  }
  return 0;
}

static int fat32_dir_find_free(struct fat32_fs *fs,
                               uint32_t dir_cluster,
                               uint32_t needed,
                               uint64_t *out_index) {
  uint32_t entries_per_cluster = (fs->sectors_per_cluster * fs->bytes_per_sector) / 32U;
  uint64_t entry_index = 0;
  uint64_t run_start = 0;
  uint32_t run_len = 0;
  uint32_t cluster = dir_cluster;
  uint32_t last_cluster = dir_cluster;
  uint8_t sector[512];

  while (cluster < FAT32_EOC) {
    last_cluster = cluster;
    uint32_t lba = fat32_cluster_to_lba(fs, cluster);
    for (uint32_t s = 0; s < fs->sectors_per_cluster; s++) {
      if (blk_read(lba + s, sector, 1) != 0) {
        return -EIO;
      }
      for (uint32_t off = 0; off < 512; off += 32, entry_index++) {
        struct fat_dirent_raw *raw = (struct fat_dirent_raw *)(sector + off);
        if (raw->name[0] == 0x00 || raw->name[0] == 0xE5) {
          if (run_len == 0) {
            run_start = entry_index;
          }
          run_len++;
          if (run_len >= needed) {
            *out_index = run_start;
            return 0;
          }
          continue;
        }
        run_len = 0;
      }
    }
    uint32_t next = 0;
    if (fat32_read_fat_entry(fs, cluster, &next) != 0) {
      return -EIO;
    }
    if (next >= FAT32_EOC) {
      break;
    }
    cluster = next;
  }

  if (run_len == 0) {
    run_start = entry_index;
  }
  while (run_len < needed) {
    uint32_t new_cluster = 0;
    if (fat32_alloc_cluster(fs, &new_cluster) != 0) {
      return -ENOSPC;
    }
    if (fat32_write_fat_entry(fs, last_cluster, new_cluster) != 0) {
      return -EIO;
    }
    last_cluster = new_cluster;
    run_len += entries_per_cluster;
  }

  *out_index = run_start;
  return 0;
}

static int fat32_is_invalid_char(char c) {
  unsigned char uc = (unsigned char)c;
  if (uc < 0x20 || uc == 0x7f) {
    return 1;
  }
  if (uc >= 0x80) {
    return 1;
  }
  switch (c) {
    case '"':
    case '*':
    case '/':
    case '\\':
    case ':':
    case '<':
    case '>':
    case '?':
    case '|':
      return 1;
    default:
      return 0;
  }
}

static int fat32_validate_name(const char *name) {
  if (!name || name[0] == '\0') {
    return -EINVAL;
  }
  if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
    return -EINVAL;
  }
  size_t len = 0;
  while (name[len]) {
    if (len >= FAT32_MAX_NAME) {
      return -ENAMETOOLONG;
    }
    if (fat32_is_invalid_char(name[len])) {
      return -EINVAL;
    }
    len++;
  }
  return 0;
}

static int fat32_is_short_char(char c) {
  if (c >= 'A' && c <= 'Z') {
    return 1;
  }
  if (c >= '0' && c <= '9') {
    return 1;
  }
  switch (c) {
    case '$':
    case '%':
    case '\'':
    case '-':
    case '_':
    case '@':
    case '~':
    case '!':
    case '#':
    case '&':
    case '(':
    case ')':
    case '^':
      return 1;
    default:
      return 0;
  }
}

static const char *fat32_last_dot(const char *name) {
  const char *dot = NULL;
  for (const char *p = name; *p; p++) {
    if (*p == '.') {
      dot = p;
    }
  }
  return dot;
}

struct fat32_short_ctx {
  const uint8_t *shortname;
  int found;
};

static int fat32_short_exists_cb(const struct fat_dirent_raw *raw,
                                 const char *entry_name,
                                 uint32_t dirent_lba,
                                 uint32_t dirent_off,
                                 uint64_t entry_index,
                                 uint32_t lfn_count,
                                 void *opaque) {
  struct fat32_short_ctx *ctx = (struct fat32_short_ctx *)opaque;
  (void)entry_name;
  (void)dirent_lba;
  (void)dirent_off;
  (void)entry_index;
  (void)lfn_count;
  if (memcmp(raw->name, ctx->shortname, 11) == 0) {
    ctx->found = 1;
    return 1;
  }
  return 0;
}

static int fat32_short_exists(struct fat32_fs *fs, uint32_t dir_cluster, const uint8_t shortname[11]);

static int fat32_make_short_name(struct fat32_fs *fs,
                                 uint32_t dir_cluster,
                                 const char *name,
                                 uint8_t out[11],
                                 int *needs_lfn) {
  char base[9];
  char ext[4];
  size_t base_len = 0;
  size_t ext_len = 0;
  int local_needs_lfn = 0;
  int simple = 1;

  const char *dot = fat32_last_dot(name);
  size_t name_len = strlen(name);
  size_t base_src_len = dot ? (size_t)(dot - name) : name_len;
  size_t ext_src_len = dot ? (name_len - base_src_len - 1) : 0;

  memset(base, 0, sizeof(base));
  memset(ext, 0, sizeof(ext));

  if (base_src_len == 0) {
    base[0] = '_';
    base_len = 1;
    local_needs_lfn = 1;
  }

  for (size_t i = 0; i < base_src_len; i++) {
    char c = name[i];
    if (c >= 'a' && c <= 'z') {
      c = (char)(c - 'a' + 'A');
      local_needs_lfn = 1;
    }
    if (!fat32_is_short_char(c)) {
      local_needs_lfn = 1;
      c = '_';
    }
    if (base_len < 8) {
      base[base_len++] = c;
    } else {
      local_needs_lfn = 1;
      simple = 0;
    }
  }

  if (ext_src_len > 0) {
    for (size_t i = 0; i < ext_src_len; i++) {
      char c = dot[1 + i];
      if (c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
        local_needs_lfn = 1;
      }
      if (!fat32_is_short_char(c)) {
        local_needs_lfn = 1;
        c = '_';
      }
      if (ext_len < 3) {
        ext[ext_len++] = c;
      } else {
        local_needs_lfn = 1;
        simple = 0;
      }
    }
  }

  if (base_src_len > 8 || ext_src_len > 3) {
    simple = 0;
    local_needs_lfn = 1;
  }

  memset(out, ' ', 11);
  memcpy(out, base, base_len);
  memcpy(out + 8, ext, ext_len);

  if (simple && !fat32_short_exists(fs, dir_cluster, out)) {
    if (needs_lfn) {
      *needs_lfn = local_needs_lfn;
    }
    return 0;
  }

  if (needs_lfn) {
    *needs_lfn = 1;
  }

  for (int n = 1; n < 100; n++) {
    int suffix_len = (n < 10) ? 2 : 3;
    int keep = 8 - suffix_len;
    if (keep < 1) {
      keep = 1;
    }
    char tmp[9];
    memset(tmp, ' ', sizeof(tmp));
    size_t copy_len = base_len;
    if ((int)copy_len > keep) {
      copy_len = (size_t)keep;
    }
    memcpy(tmp, base, copy_len);
    tmp[copy_len++] = '~';
    if (n < 10) {
      tmp[copy_len++] = (char)('0' + n);
    } else {
      tmp[copy_len++] = (char)('0' + (n / 10));
      tmp[copy_len++] = (char)('0' + (n % 10));
    }
    memset(out, ' ', 11);
    memcpy(out, tmp, 8);
    memcpy(out + 8, ext, ext_len);
    if (!fat32_short_exists(fs, dir_cluster, out)) {
      return 0;
    }
  }
  return -ENOSPC;
}

static uint16_t fat32_lfn_char(const char *name, size_t name_len, size_t idx) {
  if (idx < name_len) {
    return (uint16_t)(uint8_t)name[idx];
  }
  if (idx == name_len) {
    return 0x0000;
  }
  return 0xFFFF;
}

static void fat32_lfn_clear(char *lfn, size_t max) {
  memset(lfn, 0, max);
}

static void fat32_lfn_write(char *lfn, size_t max, const struct fat_lfn_raw *lfn_raw) {
  uint16_t chars[13];
  for (size_t i = 0; i < 5; i++) {
    chars[i] = lfn_raw->name1[i];
  }
  for (size_t i = 0; i < 6; i++) {
    chars[5 + i] = lfn_raw->name2[i];
  }
  for (size_t i = 0; i < 2; i++) {
    chars[11 + i] = lfn_raw->name3[i];
  }
  size_t base = ((size_t)(lfn_raw->ord & 0x1FU) - 1U) * 13U;
  for (size_t i = 0; i < 13; i++) {
    size_t pos = base + i;
    if (pos >= max - 1) {
      break;
    }
    uint16_t c = chars[i];
    if (c == 0x0000 || c == 0xFFFF) {
      lfn[pos] = '\0';
    } else {
      lfn[pos] = (char)(c & 0xFFU);
    }
  }
}

static void fat32_fill_lfn_entry(struct fat_lfn_raw *lfn,
                                 const char *name,
                                 size_t name_len,
                                 uint8_t ord,
                                 uint8_t checksum) {
  memset(lfn, 0, sizeof(*lfn));
  lfn->ord = ord;
  lfn->attr = FAT_ATTR_LFN;
  lfn->type = 0;
  lfn->checksum = checksum;
  lfn->fst_clus_lo = 0;

  size_t base = ((size_t)(ord & 0x1FU) - 1U) * 13U;
  for (size_t i = 0; i < 5; i++) {
    lfn->name1[i] = fat32_lfn_char(name, name_len, base + i);
  }
  for (size_t i = 0; i < 6; i++) {
    lfn->name2[i] = fat32_lfn_char(name, name_len, base + 5 + i);
  }
  for (size_t i = 0; i < 2; i++) {
    lfn->name3[i] = fat32_lfn_char(name, name_len, base + 11 + i);
  }
}

struct fat32_lookup_ctx {
  const char *name;
  struct vnode **out;
  struct vnode *parent;
  struct fat32_node *dir_node;
};

struct fat32_readdir_ctx {
  uint64_t want;
  uint64_t idx;
  struct vfs_dirent *out;
};

static int fat32_iter_dir(struct fat32_fs *fs,
                          uint32_t start_cluster,
                          int (*cb)(const struct fat_dirent_raw *raw,
                                    const char *name,
                                    uint32_t dirent_lba,
                                    uint32_t dirent_off,
                                    uint64_t entry_index,
                                    uint32_t lfn_count,
                                    void *ctx),
                          void *ctx) {
  uint32_t cluster = start_cluster;
  uint8_t sector[512];
  char lfn[FAT32_MAX_NAME + 1];
  uint8_t lfn_checksum = 0;
  int lfn_valid = 0;
  uint32_t lfn_count = 0;
  uint64_t entry_index = 0;

  fat32_lfn_clear(lfn, sizeof(lfn));

  while (cluster < FAT32_EOC) {
    uint32_t lba = fat32_cluster_to_lba(fs, cluster);
    for (size_t s = 0; s < fs->sectors_per_cluster; s++) {
      if (blk_read(lba + s, sector, 1) != 0) {
        return -EIO;
      }
      for (size_t off = 0; off < 512; off += 32, entry_index++) {
        const struct fat_dirent_raw *raw = (const struct fat_dirent_raw *)(sector + off);
        if (raw->name[0] == 0x00) {
          return 0;
        }
        if (raw->name[0] == 0xE5) {
          lfn_valid = 0;
          lfn_count = 0;
          continue;
        }
        if (raw->attr == FAT_ATTR_LFN) {
          const struct fat_lfn_raw *lfn_raw = (const struct fat_lfn_raw *)raw;
          if (lfn_raw->ord & 0x40U) {
            fat32_lfn_clear(lfn, sizeof(lfn));
            lfn_valid = 1;
            lfn_checksum = lfn_raw->checksum;
            lfn_count = 0;
          }
          if (lfn_valid && lfn_raw->checksum == lfn_checksum) {
            fat32_lfn_write(lfn, sizeof(lfn), lfn_raw);
            lfn_count++;
          } else {
            lfn_valid = 0;
            lfn_count = 0;
          }
          continue;
        }
        if (raw->attr & FAT_ATTR_VOLUME) {
          lfn_valid = 0;
          lfn_count = 0;
          continue;
        }

        char name[FAT32_MAX_NAME + 1];
        if (lfn_valid && lfn[0] != '\0' && lfn_checksum == fat32_lfn_checksum(raw->name)) {
          size_t nlen = strlen(lfn);
          if (nlen > FAT32_MAX_NAME) {
            nlen = FAT32_MAX_NAME;
          }
          memcpy(name, lfn, nlen);
          name[nlen] = '\0';
        } else {
          size_t pos = 0;
          for (size_t i = 0; i < 8 && raw->name[i] != ' '; i++) {
            name[pos++] = (char)raw->name[i];
          }
          if (raw->name[8] != ' ') {
            name[pos++] = '.';
            for (size_t i = 0; i < 3 && raw->name[8 + i] != ' '; i++) {
              name[pos++] = (char)raw->name[8 + i];
            }
          }
          name[pos] = '\0';
        }

        lfn_valid = 0;
        uint32_t dirent_lba = lba + (uint32_t)s;
        uint32_t dirent_off = (uint32_t)off;
        int ret = cb(raw, name, dirent_lba, dirent_off, entry_index, lfn_count, ctx);
        if (ret != 0) {
          return ret;
        }
        lfn_count = 0;
      }
    }

    if (fat32_read_fat_entry(fs, cluster, &cluster) != 0) {
      return -EIO;
    }
  }
  return 0;
}

static int fat32_short_exists(struct fat32_fs *fs, uint32_t dir_cluster, const uint8_t shortname[11]) {
  struct fat32_short_ctx ctx;
  ctx.shortname = shortname;
  ctx.found = 0;
  int ret = fat32_iter_dir(fs, dir_cluster, fat32_short_exists_cb, &ctx);
  if (ret > 0) {
    return 1;
  }
  if (ret < 0) {
    return 1;
  }
  return ctx.found;
}

static int fat32_lookup_cb(const struct fat_dirent_raw *raw,
                           const char *entry_name,
                           uint32_t dirent_lba,
                           uint32_t dirent_off,
                           uint64_t entry_index,
                           uint32_t lfn_count,
                           void *opaque) {
  struct fat32_lookup_ctx *c = (struct fat32_lookup_ctx *)opaque;
  if (!name_eq_nocase(entry_name, c->name)) {
    return 0;
  }
  struct fat32_node *node = fat32_alloc_node();
  if (!node) {
    return -ENOMEM;
  }
  node->fs = &g_fs;
  node->attr = raw->attr;
  node->size = raw->file_size;
  node->cluster = ((uint32_t)le16(&raw->fst_clus_hi) << 16) | le16(&raw->fst_clus_lo);
  node->dir_cluster = c->dir_node->cluster;
  node->dirent_lba = dirent_lba;
  node->dirent_off = dirent_off;
  node->dirent_index = entry_index;
  node->dirent_count = lfn_count + 1;

  struct vnode *vn = fat32_wrap_node(node, c->parent);
  if (!vn) {
    return -ENOMEM;
  }
  memcpy(vn->name, entry_name, strlen(entry_name) + 1);
  *c->out = vn;
  return 1;
}

static int fat32_readdir_cb(const struct fat_dirent_raw *raw,
                            const char *entry_name,
                            uint32_t dirent_lba,
                            uint32_t dirent_off,
                            uint64_t entry_index,
                            uint32_t lfn_count,
                            void *opaque) {
  struct fat32_readdir_ctx *c = (struct fat32_readdir_ctx *)opaque;
  (void)dirent_lba;
  (void)dirent_off;
  (void)entry_index;
  (void)lfn_count;
  if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
    return 0;
  }
  if (c->idx == c->want) {
    memset(c->out, 0, sizeof(*c->out));
    memcpy(c->out->name, entry_name, strlen(entry_name) + 1);
    c->out->type = (raw->attr & FAT_ATTR_DIR) ? VNODE_DIR : VNODE_FILE;
    c->out->ino = ((uint64_t)le16(&raw->fst_clus_hi) << 16) | le16(&raw->fst_clus_lo);
    return 1;
  }
  c->idx++;
  return 0;
}

struct fat32_find_ctx {
  const char *name;
  struct fat_dirent_raw *raw;
  uint32_t *dirent_lba;
  uint32_t *dirent_off;
  uint64_t *entry_index;
  uint32_t *lfn_count;
};

static int fat32_find_cb(const struct fat_dirent_raw *raw,
                         const char *entry_name,
                         uint32_t dirent_lba,
                         uint32_t dirent_off,
                         uint64_t entry_index,
                         uint32_t lfn_count,
                         void *opaque) {
  struct fat32_find_ctx *ctx = (struct fat32_find_ctx *)opaque;
  if (!name_eq_nocase(entry_name, ctx->name)) {
    return 0;
  }
  if (ctx->raw) {
    memcpy(ctx->raw, raw, sizeof(*raw));
  }
  if (ctx->dirent_lba) {
    *ctx->dirent_lba = dirent_lba;
  }
  if (ctx->dirent_off) {
    *ctx->dirent_off = dirent_off;
  }
  if (ctx->entry_index) {
    *ctx->entry_index = entry_index;
  }
  if (ctx->lfn_count) {
    *ctx->lfn_count = lfn_count;
  }
  return 1;
}

static int fat32_find_entry(struct fat32_fs *fs,
                            uint32_t dir_cluster,
                            const char *name,
                            struct fat_dirent_raw *raw,
                            uint32_t *dirent_lba,
                            uint32_t *dirent_off,
                            uint64_t *entry_index,
                            uint32_t *lfn_count) {
  struct fat32_find_ctx ctx;
  ctx.name = name;
  ctx.raw = raw;
  ctx.dirent_lba = dirent_lba;
  ctx.dirent_off = dirent_off;
  ctx.entry_index = entry_index;
  ctx.lfn_count = lfn_count;
  int ret = fat32_iter_dir(fs, dir_cluster, fat32_find_cb, &ctx);
  if (ret > 0) {
    return 0;
  }
  if (ret < 0) {
    return ret;
  }
  return -ENOENT;
}

static int fat32_update_dirent(struct fat32_node *node) {
  if (node->dirent_lba == 0) {
    return 0;
  }
  uint8_t sector[512];
  if (blk_read(node->dirent_lba, sector, 1) != 0) {
    return -EIO;
  }
  struct fat_dirent_raw *raw = (struct fat_dirent_raw *)(sector + node->dirent_off);
  raw->file_size = node->size;
  raw->fst_clus_lo = (uint16_t)(node->cluster & 0xFFFF);
  raw->fst_clus_hi = (uint16_t)((node->cluster >> 16) & 0xFFFF);
  if (blk_write(node->dirent_lba, sector, 1) != 0) {
    return -EIO;
  }
  return 0;
}

static int fat32_refresh_node(struct fat32_node *node) {
  if (!node || node->dirent_lba == 0) {
    return 0;
  }
  uint8_t sector[512];
  if (blk_read(node->dirent_lba, sector, 1) != 0) {
    return -EIO;
  }
  const struct fat_dirent_raw *raw = (const struct fat_dirent_raw *)(sector + node->dirent_off);
  node->attr = raw->attr;
  node->size = raw->file_size;
  node->cluster = ((uint32_t)le16(&raw->fst_clus_hi) << 16) | le16(&raw->fst_clus_lo);
  return 0;
}

static int fat32_dir_is_empty(struct fat32_fs *fs, uint32_t dir_cluster) {
  struct fat32_readdir_ctx ctx;
  struct vfs_dirent ent;
  ctx.want = 0;
  ctx.idx = 0;
  ctx.out = &ent;
  for (;;) {
    int ret = fat32_iter_dir(fs, dir_cluster, fat32_readdir_cb, &ctx);
    if (ret > 0) {
      return 0;
    }
    if (ret < 0) {
      return ret;
    }
    if (ctx.idx == 0) {
      break;
    }
    ctx.idx = 0;
  }
  return 1;
}

static int fat32_write_dot_entries(struct fat32_fs *fs, uint32_t cluster, uint32_t parent_cluster) {
  struct fat_dirent_raw dot;
  struct fat_dirent_raw dotdot;
  memset(&dot, 0, sizeof(dot));
  memset(&dotdot, 0, sizeof(dotdot));
  memcpy(dot.name, ".          ", 11);
  memcpy(dotdot.name, "..         ", 11);
  dot.attr = FAT_ATTR_DIR;
  dotdot.attr = FAT_ATTR_DIR;
  dot.fst_clus_lo = (uint16_t)(cluster & 0xFFFF);
  dot.fst_clus_hi = (uint16_t)((cluster >> 16) & 0xFFFF);
  dotdot.fst_clus_lo = (uint16_t)(parent_cluster & 0xFFFF);
  dotdot.fst_clus_hi = (uint16_t)((parent_cluster >> 16) & 0xFFFF);

  if (fat32_write_dir_entry(fs, cluster, 0, &dot) != 0) {
    return -EIO;
  }
  if (fat32_write_dir_entry(fs, cluster, 1, &dotdot) != 0) {
    return -EIO;
  }
  return 0;
}

static int fat32_create_entry(struct vnode *dir,
                              const char *name,
                              uint8_t attr,
                              struct vnode **out) {
  struct fat32_node *dnode = (struct fat32_node *)dir->data;
  struct fat32_fs *fs = dnode->fs;
  int ret = fat32_validate_name(name);
  if (ret != 0) {
    return ret;
  }

  struct fat_dirent_raw found;
  uint32_t found_lba = 0;
  uint32_t found_off = 0;
  uint64_t found_index = 0;
  uint32_t found_lfn = 0;
  ret = fat32_find_entry(fs, dnode->cluster, name, &found, &found_lba, &found_off, &found_index, &found_lfn);
  if (ret == 0) {
    if (found.attr & FAT_ATTR_DIR) {
      if (!(attr & FAT_ATTR_DIR)) {
        return -EISDIR;
      }
      if (out) {
        struct fat32_node *node = fat32_alloc_node();
        if (!node) {
          return -ENOMEM;
        }
        node->fs = fs;
        node->attr = found.attr;
        node->size = found.file_size;
        node->cluster = ((uint32_t)le16(&found.fst_clus_hi) << 16) | le16(&found.fst_clus_lo);
        node->dir_cluster = dnode->cluster;
        node->dirent_lba = found_lba;
        node->dirent_off = found_off;
        node->dirent_index = found_index;
        node->dirent_count = found_lfn + 1;

        struct vnode *vn = fat32_wrap_node(node, dir);
        if (!vn) {
          return -ENOMEM;
        }
        memcpy(vn->name, name, strlen(name) + 1);
        *out = vn;
      }
      return 0;
    }
    return -EEXIST;
  }
  if (ret != -ENOENT) {
    return ret;
  }

  uint8_t shortname[11];
  int needs_lfn = 0;
  ret = fat32_make_short_name(fs, dnode->cluster, name, shortname, &needs_lfn);
  if (ret != 0) {
    return ret;
  }

  size_t name_len = strlen(name);
  uint32_t lfn_count = 0;
  if (needs_lfn) {
    lfn_count = (uint32_t)((name_len + 12) / 13);
  }
  uint32_t entries_needed = lfn_count + 1;
  uint64_t entry_index = 0;
  ret = fat32_dir_find_free(fs, dnode->cluster, entries_needed, &entry_index);
  if (ret != 0) {
    return ret;
  }

  uint32_t cluster = 0;
  if (attr & FAT_ATTR_DIR) {
    if (fat32_alloc_cluster(fs, &cluster) != 0) {
      return -ENOSPC;
    }
    if (fat32_write_dot_entries(fs, cluster, dnode->cluster) != 0) {
      return -EIO;
    }
  }

  uint8_t checksum = fat32_lfn_checksum(shortname);
  for (uint32_t i = 0; i < lfn_count; i++) {
    uint8_t ord = (uint8_t)(lfn_count - i);
    if (i == 0) {
      ord |= 0x40U;
    }
    struct fat_lfn_raw lfn;
    fat32_fill_lfn_entry(&lfn, name, name_len, ord, checksum);
    ret = fat32_write_dir_entry(fs, dnode->cluster, entry_index + i, &lfn);
    if (ret != 0) {
      return ret;
    }
  }

  struct fat_dirent_raw raw;
  memset(&raw, 0, sizeof(raw));
  memcpy(raw.name, shortname, sizeof(shortname));
  raw.attr = attr | ((attr & FAT_ATTR_DIR) ? 0 : FAT_ATTR_ARCHIVE);
  raw.fst_clus_lo = (uint16_t)(cluster & 0xFFFF);
  raw.fst_clus_hi = (uint16_t)((cluster >> 16) & 0xFFFF);
  raw.file_size = 0;
  ret = fat32_write_dir_entry(fs, dnode->cluster, entry_index + lfn_count, &raw);
  if (ret != 0) {
    return ret;
  }

  struct fat32_node *node = fat32_alloc_node();
  if (!node) {
    return -ENOMEM;
  }
  node->fs = fs;
  node->attr = raw.attr;
  node->size = 0;
  node->cluster = cluster;
  node->dir_cluster = dnode->cluster;
  node->dirent_index = entry_index + lfn_count;
  node->dirent_count = entries_needed;
  if (fat32_dir_entry_location(fs, dnode->cluster, node->dirent_index, &node->dirent_lba, &node->dirent_off) != 0) {
    return -EIO;
  }

  if (out) {
    struct vnode *vn = fat32_wrap_node(node, dir);
    if (!vn) {
      return -ENOMEM;
    }
    memcpy(vn->name, name, name_len + 1);
    *out = vn;
  }
  return 0;
}

static int fat32_lookup_op(struct vnode *dir, const char *name, struct vnode **out) {
  struct fat32_node *dnode = (struct fat32_node *)dir->data;

  struct fat32_lookup_ctx ctx;
  ctx.name = name;
  ctx.out = out;
  ctx.parent = dir;
  ctx.dir_node = dnode;

  int ret = fat32_iter_dir(&g_fs, dnode->cluster, fat32_lookup_cb, &ctx);
  if (ret > 0) {
    return 0;
  }
  if (ret < 0) {
    return ret;
  }
  return -ENOENT;
}

static int fat32_readdir_op(struct vnode *dir, uint64_t offset, struct vfs_dirent *out) {
  struct fat32_node *dnode = (struct fat32_node *)dir->data;

  struct fat32_readdir_ctx ctx;
  ctx.want = offset;
  ctx.idx = 0;
  ctx.out = out;

  int ret = fat32_iter_dir(&g_fs, dnode->cluster, fat32_readdir_cb, &ctx);
  if (ret > 0) {
    return 0;
  }
  if (ret < 0) {
    return ret;
  }
  return -ENOENT;
}

static int fat32_read_op(struct vnode *vn, void *buf, size_t len, uint64_t offset) {
  struct fat32_node *node = (struct fat32_node *)vn->data;
  struct fat32_fs *fs = node->fs;
  if (fat32_refresh_node(node) != 0) {
    return -EIO;
  }
  uint32_t cluster = node->cluster;
  uint64_t remaining = len;
  uint64_t total = 0;
  uint8_t sector[512];

  if (vn->type != VNODE_FILE) {
    return -EISDIR;
  }
  if (offset >= node->size) {
    return 0;
  }
  if (offset + remaining > node->size) {
    remaining = node->size - offset;
  }
  total = remaining;
  if (cluster == 0) {
    return 0;
  }

  uint32_t cluster_size = fs->bytes_per_sector * fs->sectors_per_cluster;
  uint64_t skip = offset / cluster_size;
  uint32_t off_in_cluster = (uint32_t)(offset % cluster_size);

  for (uint64_t i = 0; i < skip; i++) {
    if (fat32_read_fat_entry(fs, cluster, &cluster) != 0) {
      return -EIO;
    }
    if (cluster >= FAT32_EOC) {
      return 0;
    }
  }

  uint8_t *out = (uint8_t *)buf;
  while (remaining > 0 && cluster < FAT32_EOC) {
    uint32_t lba = fat32_cluster_to_lba(fs, cluster);
    uint32_t cluster_bytes = fs->sectors_per_cluster * fs->bytes_per_sector;
    uint32_t to_copy = cluster_bytes - off_in_cluster;
    if (to_copy > remaining) {
      to_copy = (uint32_t)remaining;
    }
    uint32_t copied = 0;
    uint32_t sector_idx = off_in_cluster / 512U;
    uint32_t sector_off = off_in_cluster % 512U;

    while (copied < to_copy) {
      if (blk_read(lba + sector_idx, sector, 1) != 0) {
        return -EIO;
      }
      uint32_t chunk = 512U - sector_off;
      if (chunk > to_copy - copied) {
        chunk = to_copy - copied;
      }
      memcpy(out, sector + sector_off, chunk);
      out += chunk;
      copied += chunk;
      sector_idx++;
      sector_off = 0;
    }

    remaining -= to_copy;
    off_in_cluster = 0;
    if (remaining > 0) {
      if (fat32_read_fat_entry(fs, cluster, &cluster) != 0) {
        return -EIO;
      }
    }
  }

  return (int)(total - remaining);
}

static int fat32_write_op(struct vnode *vn, const void *buf, size_t len, uint64_t offset) {
  struct fat32_node *node = (struct fat32_node *)vn->data;
  struct fat32_fs *fs = node->fs;
  uint32_t cluster = node->cluster;
  uint64_t remaining = len;
  const uint8_t *in = (const uint8_t *)buf;
  uint8_t sector[512];

  if (vn->type != VNODE_FILE) {
    return -EISDIR;
  }
  if (len == 0) {
    return 0;
  }

  uint64_t end = offset + len;
  uint32_t cluster_size = fs->bytes_per_sector * fs->sectors_per_cluster;
  uint32_t needed = (uint32_t)((end + cluster_size - 1) / cluster_size);
  if (fat32_ensure_chain(fs, &node->cluster, needed) != 0) {
    return -ENOSPC;
  }
  cluster = node->cluster;

  uint64_t skip = offset / cluster_size;
  uint32_t off_in_cluster = (uint32_t)(offset % cluster_size);

  for (uint64_t i = 0; i < skip; i++) {
    if (fat32_read_fat_entry(fs, cluster, &cluster) != 0) {
      return -EIO;
    }
    if (cluster >= FAT32_EOC) {
      return -EIO;
    }
  }

  while (remaining > 0 && cluster < FAT32_EOC) {
    uint32_t lba = fat32_cluster_to_lba(fs, cluster);
    uint32_t cluster_bytes = fs->sectors_per_cluster * fs->bytes_per_sector;
    uint32_t to_copy = cluster_bytes - off_in_cluster;
    if (to_copy > remaining) {
      to_copy = (uint32_t)remaining;
    }
    uint32_t copied = 0;
    uint32_t sector_idx = off_in_cluster / 512U;
    uint32_t sector_off = off_in_cluster % 512U;

    while (copied < to_copy) {
      uint32_t chunk = 512U - sector_off;
      if (chunk > to_copy - copied) {
        chunk = to_copy - copied;
      }
      if (sector_off == 0 && chunk == 512U) {
        memcpy(sector, in, 512U);
      } else {
        if (blk_read(lba + sector_idx, sector, 1) != 0) {
          return -EIO;
        }
        memcpy(sector + sector_off, in, chunk);
      }
      if (blk_write(lba + sector_idx, sector, 1) != 0) {
        return -EIO;
      }
      in += chunk;
      copied += chunk;
      sector_idx++;
      sector_off = 0;
    }

    remaining -= to_copy;
    off_in_cluster = 0;
    if (remaining > 0) {
      if (fat32_read_fat_entry(fs, cluster, &cluster) != 0) {
        return -EIO;
      }
    }
  }

  if (end > node->size) {
    node->size = (uint32_t)end;
    if (fat32_update_dirent(node) != 0) {
      return -EIO;
    }
  }
  return (int)len;
}

static int fat32_truncate_op(struct vnode *vn, uint64_t size) {
  struct fat32_node *node = (struct fat32_node *)vn->data;
  struct fat32_fs *fs = node->fs;

  if (vn->type != VNODE_FILE) {
    return -EISDIR;
  }
  if (size == node->size) {
    return 0;
  }
  if (size == 0) {
    if (node->cluster != 0) {
      if (fat32_free_chain(fs, node->cluster) != 0) {
        return -EIO;
      }
    }
    node->cluster = 0;
    node->size = 0;
    return fat32_update_dirent(node);
  }

  uint32_t cluster_size = fs->bytes_per_sector * fs->sectors_per_cluster;
  uint32_t needed = (uint32_t)((size + cluster_size - 1) / cluster_size);
  if (fat32_ensure_chain(fs, &node->cluster, needed) != 0) {
    return -ENOSPC;
  }

  uint32_t last = 0;
  if (fat32_get_cluster_at(fs, node->cluster, needed - 1, &last) != 0) {
    return -EIO;
  }
  uint32_t next = 0;
  if (fat32_read_fat_entry(fs, last, &next) != 0) {
    return -EIO;
  }
  if (next < FAT32_EOC) {
    if (fat32_write_fat_entry(fs, last, FAT32_EOC) != 0) {
      return -EIO;
    }
    if (fat32_free_chain(fs, next) != 0) {
      return -EIO;
    }
  }
  node->size = (uint32_t)size;
  return fat32_update_dirent(node);
}

static int fat32_create_op(struct vnode *dir, const char *name, struct vnode **out) {
  return fat32_create_entry(dir, name, 0, out);
}

static int fat32_mkdir_op(struct vnode *dir, const char *name, struct vnode **out) {
  return fat32_create_entry(dir, name, FAT_ATTR_DIR, out);
}

static int fat32_unlink_op(struct vnode *dir, const char *name) {
  struct fat32_node *dnode = (struct fat32_node *)dir->data;
  struct fat32_fs *fs = dnode->fs;
  if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
    return -EINVAL;
  }

  struct fat_dirent_raw raw;
  uint32_t dirent_lba = 0;
  uint32_t dirent_off = 0;
  uint64_t entry_index = 0;
  uint32_t lfn_count = 0;
  int ret = fat32_find_entry(fs, dnode->cluster, name, &raw, &dirent_lba, &dirent_off, &entry_index, &lfn_count);
  if (ret != 0) {
    return ret;
  }

  uint32_t target_cluster = ((uint32_t)le16(&raw.fst_clus_hi) << 16) | le16(&raw.fst_clus_lo);
  if (raw.attr & FAT_ATTR_DIR) {
    int empty = fat32_dir_is_empty(fs, target_cluster);
    if (empty <= 0) {
      return (empty < 0) ? empty : -ENOTEMPTY;
    }
  }

  if (target_cluster != 0) {
    if (fat32_free_chain(fs, target_cluster) != 0) {
      return -EIO;
    }
  }

  uint64_t start = entry_index - lfn_count;
  uint32_t total = lfn_count + 1;
  struct fat_dirent_raw del;
  memset(&del, 0, sizeof(del));
  del.name[0] = 0xE5;
  for (uint32_t i = 0; i < total; i++) {
    ret = fat32_write_dir_entry(fs, dnode->cluster, start + i, &del);
    if (ret != 0) {
      return ret;
    }
  }
  (void)dirent_lba;
  (void)dirent_off;
  return 0;
}

static int fat32_stat_op(struct vnode *vn, struct vfs_stat *st) {
  struct fat32_node *node = (struct fat32_node *)vn->data;
  memset(st, 0, sizeof(*st));
  st->st_size = (int64_t)node->size;
  st->st_ino = node->cluster;
  st->st_nlink = 1;
  st->st_mode = (vn->type == VNODE_DIR) ? 0040755 : 0100644;
  return 0;
}

static const struct vnode_ops g_fat32_ops = {
    .lookup = fat32_lookup_op,
    .create = fat32_create_op,
    .mkdir = fat32_mkdir_op,
    .read = fat32_read_op,
    .write = fat32_write_op,
    .truncate = fat32_truncate_op,
    .readdir = fat32_readdir_op,
    .unlink = fat32_unlink_op,
    .stat = fat32_stat_op,
};

static struct vnode *fat32_wrap_node(struct fat32_node *node, struct vnode *parent) {
  struct vnode *vn = vfs_alloc_vnode();
  if (!vn) {
    return NULL;
  }
  vn->type = (node->attr & FAT_ATTR_DIR) ? VNODE_DIR : VNODE_FILE;
  vn->ops = &g_fat32_ops;
  vn->data = node;
  vn->parent = parent ? parent : vn;
  vn->name[0] = '\0';
  return vn;
}

int fat32_mount(struct fat32_fs *fs) {
  uint8_t sector[512];
  struct mbr_part part;
  uint16_t sig;
  size_t i;
  uint32_t part_lba = 0;
  uint32_t part_sectors = 0;

  if (!fs) {
    return -1;
  }
  memset(fs, 0, sizeof(*fs));

  if (blk_read(0, sector, 1) != 0) {
    log_error("fat32: read mbr failed");
    return -1;
  }

  sig = le16(sector + MBR_SIG_OFFSET);
  if (sig != MBR_SIG_VALUE) {
    log_error("fat32: bad mbr signature");
    return -1;
  }

  for (i = 0; i < 4; i++) {
    memcpy(&part, sector + MBR_PART_OFFSET + i * sizeof(part), sizeof(part));
    if (part.type == FAT32_TYPE_0B || part.type == FAT32_TYPE_0C) {
      part_lba = part.lba_start;
      part_sectors = part.sectors;
      break;
    }
  }

  if (part_lba == 0 || part_sectors == 0) {
    log_error("fat32: no FAT32 partition found");
    return -1;
  }

  if (blk_read(part_lba, sector, 1) != 0) {
    log_error("fat32: read bpb failed");
    return -1;
  }

  fs->part_lba = part_lba;
  fs->bytes_per_sector = le16(sector + 11);
  fs->sectors_per_cluster = sector[13];
  fs->reserved_sectors = le16(sector + 14);
  fs->num_fats = sector[16];
  fs->total_sectors = le16(sector + 19);
  fs->sectors_per_fat = le16(sector + 22);
  if (fs->total_sectors == 0) {
    fs->total_sectors = le32(sector + 32);
  }
  if (fs->sectors_per_fat == 0) {
    fs->sectors_per_fat = le32(sector + 36);
  }
  fs->fat_lba = part_lba + fs->reserved_sectors;
  fs->data_lba = fs->fat_lba + fs->num_fats * fs->sectors_per_fat;
  fs->root_cluster = le32(sector + 44);
  fs->alloc_hint = FAT32_MIN_CLUSTER;

  uint32_t data_sectors = fs->total_sectors - fs->reserved_sectors - fs->num_fats * fs->sectors_per_fat;
  if (fs->sectors_per_cluster == 0) {
    log_error("fat32: bad cluster size");
    return -1;
  }
  fs->cluster_count = data_sectors / fs->sectors_per_cluster;

  log_info("fat32: part_lba=%lu part_sectors=%lu total=%lu spc=%lu fat=%lu root=%lu",
           (uint64_t)part_lba,
           (uint64_t)part_sectors,
           (uint64_t)fs->total_sectors,
           (uint64_t)fs->sectors_per_cluster,
           (uint64_t)fs->sectors_per_fat,
           (uint64_t)fs->root_cluster);

  if (fs->bytes_per_sector != 512 || fs->sectors_per_cluster == 0) {
    log_error("fat32: unsupported bpb");
    return -1;
  }

  return 0;
}

struct fat32_read_root_ctx {
  struct fat32_dirent *ents;
  size_t max;
  size_t count;
};

static int fat32_read_root_cb(const struct fat_dirent_raw *raw,
                              const char *entry_name,
                              uint32_t dirent_lba,
                              uint32_t dirent_off,
                              uint64_t entry_index,
                              uint32_t lfn_count,
                              void *opaque) {
  struct fat32_read_root_ctx *ctx = (struct fat32_read_root_ctx *)opaque;
  (void)dirent_lba;
  (void)dirent_off;
  (void)entry_index;
  (void)lfn_count;
  if (ctx->count >= ctx->max) {
    return 1;
  }
  struct fat32_dirent *ent = &ctx->ents[ctx->count++];
  memset(ent, 0, sizeof(*ent));
  memcpy(ent->name, entry_name, strlen(entry_name) + 1);
  ent->attr = raw->attr;
  ent->cluster = ((uint32_t)le16(&raw->fst_clus_hi) << 16) | le16(&raw->fst_clus_lo);
  ent->size = raw->file_size;
  return 0;
}

int fat32_read_root(struct fat32_fs *fs, struct fat32_dirent *ents, size_t max, size_t *out_count) {
  if (!fs || !ents || !out_count) {
    return -1;
  }
  struct fat32_read_root_ctx ctx;
  ctx.ents = ents;
  ctx.max = max;
  ctx.count = 0;
  int ret = fat32_iter_dir(fs, fs->root_cluster, fat32_read_root_cb, &ctx);
  if (ret < 0) {
    return -1;
  }
  *out_count = ctx.count;
  return 0;
}

struct vnode *fat32_init(void) {
  if (fat32_mount(&g_fs) != 0) {
    return NULL;
  }

  struct fat32_node *node = fat32_alloc_node();
  if (!node) {
    return NULL;
  }
  node->fs = &g_fs;
  node->cluster = g_fs.root_cluster;
  node->size = 0;
  node->attr = FAT_ATTR_DIR;
  node->dir_cluster = g_fs.root_cluster;
  node->dirent_lba = 0;
  node->dirent_off = 0;
  node->dirent_index = 0;
  node->dirent_count = 0;

  struct vnode *vn = vfs_alloc_vnode();
  if (!vn) {
    return NULL;
  }
  vn->type = VNODE_DIR;
  vn->ops = &g_fat32_ops;
  vn->data = node;
  vn->parent = vn;
  vn->name[0] = '\0';
  return vn;
}
