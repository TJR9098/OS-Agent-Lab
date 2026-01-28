#include <stdint.h>
#include <stddef.h>

#include "fdt.h"
#include "string.h"

#define FDT_BEGIN_NODE 1U
#define FDT_END_NODE 2U
#define FDT_PROP 3U
#define FDT_NOP 4U
#define FDT_END 9U

struct fdt_header {
  uint32_t magic;
  uint32_t totalsize;
  uint32_t off_dt_struct;
  uint32_t off_dt_strings;
  uint32_t off_mem_rsvmap;
  uint32_t version;
  uint32_t last_comp_version;
  uint32_t boot_cpuid_phys;
  uint32_t size_dt_strings;
  uint32_t size_dt_struct;
};

static uint32_t fdt32_to_cpu(uint32_t x) {
  return ((x & 0xffU) << 24) | ((x & 0xff00U) << 8) | ((x & 0xff0000U) >> 8) | ((x & 0xff000000U) >> 24);
}

static size_t fdt_strnlen(const char *s, const char *end) {
  const char *p = s;
  while (p < end && *p) {
    p++;
  }
  return (size_t)(p - s);
}

static const uint8_t *align4(const uint8_t *p) {
  uintptr_t v = (uintptr_t)p;
  v = (v + 3U) & ~((uintptr_t)3U);
  return (const uint8_t *)v;
}

static uint64_t fdt_read_num(const uint32_t *cells, uint32_t count) {
  uint64_t val = 0;
  uint32_t i;

  for (i = 0; i < count; i++) {
    val = (val << 32) | (uint64_t)fdt32_to_cpu(cells[i]);
  }
  return val;
}

static int fdt_strlist_contains(const char *list, uint32_t len, const char *needle) {
  uint32_t i = 0;
  size_t nlen = strlen(needle);

  while (i < len) {
    const char *s = list + i;
    size_t slen = strlen(s);
    if (slen == nlen && memcmp(s, needle, nlen) == 0) {
      return 1;
    }
    i += (uint32_t)slen + 1U;
  }
  return 0;
}

struct node_state {
  int is_memory;
  int is_virtio;
  int reg_valid;
  uint64_t reg_base;
  uint64_t reg_size;
};

static int fdt_scan(const void *dtb,
                    uint64_t *mem_base,
                    uint64_t *mem_size,
                    uint64_t *virtio_bases,
                    size_t max_bases,
                    size_t *virtio_count) {
  const uint8_t *p;
  const uint8_t *struct_base;
  const uint8_t *struct_end;
  const char *strings;
  const char *strings_end;
  uint32_t addr_cells = 2;
  uint32_t size_cells = 2;
  int depth = 0;
  struct node_state stack[32];
  size_t found_virtio = 0;

  const struct fdt_header *hdr = (const struct fdt_header *)dtb;
  if (fdt32_to_cpu(hdr->magic) != FDT_MAGIC) {
    return -1;
  }
  uint32_t total = fdt32_to_cpu(hdr->totalsize);
  if (total < sizeof(*hdr) || total > (16U * 1024U * 1024U)) {
    return -1;
  }

  uint32_t off_struct = fdt32_to_cpu(hdr->off_dt_struct);
  uint32_t off_strings = fdt32_to_cpu(hdr->off_dt_strings);
  uint32_t size_struct = fdt32_to_cpu(hdr->size_dt_struct);
  uint32_t size_strings = fdt32_to_cpu(hdr->size_dt_strings);
  if (off_struct >= total || off_strings >= total) {
    return -1;
  }
  if (off_struct + size_struct > total) {
    return -1;
  }
  if (off_strings + size_strings > total) {
    return -1;
  }

  struct_base = (const uint8_t *)dtb + off_struct;
  struct_end = struct_base + size_struct;
  strings = (const char *)dtb + off_strings;
  strings_end = strings + size_strings;
  p = struct_base;

  if (virtio_count) {
    *virtio_count = 0;
  }

  for (;;) {
    if (p + 4 > struct_end) {
      return -1;
    }
    uint32_t token = fdt32_to_cpu(*(const uint32_t *)p);
    p += 4;

    if (token == FDT_BEGIN_NODE) {
      const char *name = (const char *)p;
      size_t name_len = fdt_strnlen(name, (const char *)struct_end);
      if (p + name_len >= struct_end) {
        return -1;
      }

      if (depth < (int)(sizeof(stack) / sizeof(stack[0]))) {
        stack[depth].is_memory = 0;
        stack[depth].is_virtio = 0;
        stack[depth].reg_valid = 0;
        stack[depth].reg_base = 0;
        stack[depth].reg_size = 0;
      }
      depth++;

      p = align4(p + name_len + 1);
      if (p > struct_end) {
        return -1;
      }
      (void)name;
      continue;
    }

    if (token == FDT_END_NODE) {
      if (depth > 0) {
        struct node_state *cur = &stack[depth - 1];
        if (cur->is_memory && cur->reg_valid && mem_base && mem_size && *mem_size == 0) {
          *mem_base = cur->reg_base;
          *mem_size = cur->reg_size;
        }
        if (cur->is_virtio && cur->reg_valid && virtio_bases && found_virtio < max_bases) {
          virtio_bases[found_virtio++] = cur->reg_base;
        }
      }
      depth--;
      continue;
    }

    if (token == FDT_PROP) {
      if (p + 8 > struct_end) {
        return -1;
      }
      uint32_t len = fdt32_to_cpu(*(const uint32_t *)p);
      p += 4;
      uint32_t nameoff = fdt32_to_cpu(*(const uint32_t *)p);
      p += 4;

      if (nameoff >= size_strings) {
        return -1;
      }
      const char *propname = strings + nameoff;
      if (propname >= strings_end) {
        return -1;
      }
      const uint8_t *value = p;
      if (p + len > struct_end) {
        return -1;
      }

      if (depth == 1) {
        if (strcmp(propname, "#address-cells") == 0 && len >= 4) {
          addr_cells = fdt32_to_cpu(*(const uint32_t *)value);
        } else if (strcmp(propname, "#size-cells") == 0 && len >= 4) {
          size_cells = fdt32_to_cpu(*(const uint32_t *)value);
        }
      }

      if (depth > 0) {
        struct node_state *cur = &stack[depth - 1];
        if (strcmp(propname, "device_type") == 0) {
          if (len >= 6 && memcmp(value, "memory", 6) == 0) {
            cur->is_memory = 1;
          }
        } else if (strcmp(propname, "compatible") == 0) {
          if (fdt_strlist_contains((const char *)value, len, "virtio,mmio")) {
            cur->is_virtio = 1;
          }
        } else if (strcmp(propname, "reg") == 0) {
          if (len >= (addr_cells + size_cells) * 4) {
            const uint32_t *cells = (const uint32_t *)value;
            cur->reg_base = fdt_read_num(cells, addr_cells);
            cur->reg_size = fdt_read_num(cells + addr_cells, size_cells);
            cur->reg_valid = 1;
          }
        }
      }

      p = align4(p + len);
      if (p > struct_end) {
        return -1;
      }
      continue;
    }

    if (token == FDT_NOP) {
      continue;
    }

    if (token == FDT_END) {
      break;
    }

    return -1;
  }

  if (virtio_count) {
    *virtio_count = found_virtio;
  }
  return 0;
}

int fdt_get_memory(const void *dtb, uint64_t *base, uint64_t *size) {
  uint64_t dummy[1];
  size_t dummy_count = 0;

  if (!base || !size) {
    return -1;
  }
  *base = 0;
  *size = 0;

  if (fdt_scan(dtb, base, size, dummy, 0, &dummy_count) != 0) {
    return -1;
  }
  return (*size != 0) ? 0 : -1;
}

int fdt_find_virtio_mmio(const void *dtb, uint64_t *out_bases, size_t max_bases, size_t *out_count) {
  uint64_t mem_base = 0;
  uint64_t mem_size = 0;

  return fdt_scan(dtb, &mem_base, &mem_size, out_bases, max_bases, out_count);
}
