#include <stdint.h>
#include <stddef.h>

#include "elf.h"
#include "errno.h"
#include "memlayout.h"
#include "mm.h"
#include "riscv.h"
#include "string.h"
#include "vfs.h"
#include "vm.h"

#define EI_NIDENT 16
#define PT_LOAD 1
#define PF_X 1
#define PF_W 2
#define PF_R 4

struct elf64_ehdr {
  unsigned char e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
};

struct elf64_phdr {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
};

static int read_at(struct vnode *vn, void *buf, size_t len, uint64_t off) {
  int n = vfs_read(vn, buf, len, off);
  return (n == (int)len) ? 0 : -EIO;
}

int elf_load(pagetable_t pt, struct vnode *vn, struct elf_info *info) {
  struct elf64_ehdr ehdr;
  uint64_t maxva = 0;

  if (!vn || !info) {
    return -EINVAL;
  }
  if (read_at(vn, &ehdr, sizeof(ehdr), 0) != 0) {
    return -EIO;
  }
  if (ehdr.e_ident[0] != 0x7f || ehdr.e_ident[1] != 'E' ||
      ehdr.e_ident[2] != 'L' || ehdr.e_ident[3] != 'F') {
    return -ENOEXEC;
  }
  if (ehdr.e_ident[4] != 2 || ehdr.e_ident[5] != 1) {
    return -ENOEXEC;
  }

  for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
    struct elf64_phdr ph;
    uint64_t phoff = ehdr.e_phoff + (uint64_t)i * ehdr.e_phentsize;
    if (read_at(vn, &ph, sizeof(ph), phoff) != 0) {
      return -EIO;
    }
    if (ph.p_type != PT_LOAD) {
      continue;
    }
    if (ph.p_vaddr < USER_BASE || ph.p_vaddr + ph.p_memsz > USER_LIMIT) {
      return -EFAULT;
    }
    uint64_t va_start = ph.p_vaddr & ~(PAGE_SIZE - 1);
    uint64_t va_end = (ph.p_vaddr + ph.p_memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint64_t perm = PTE_U;
    if (ph.p_flags & PF_R) {
      perm |= PTE_R;
    }
    if (ph.p_flags & PF_W) {
      perm |= PTE_W;
    }
    if (ph.p_flags & PF_X) {
      perm |= PTE_X;
    }

    for (uint64_t va = va_start; va < va_end; va += PAGE_SIZE) {
      void *mem = kalloc();
      if (!mem) {
        return -ENOMEM;
      }
      if (uvm_map(pt, va, (uint64_t)(uintptr_t)mem, PAGE_SIZE, perm) != 0) {
        kfree(mem);
        return -ENOMEM;
      }
    }

    uint64_t file_off = ph.p_offset;
    uint64_t file_end = ph.p_offset + ph.p_filesz;
    uint64_t va = ph.p_vaddr;
    uint8_t buf[512];
    while (file_off < file_end) {
      uint64_t chunk = file_end - file_off;
      if (chunk > sizeof(buf)) {
        chunk = sizeof(buf);
      }
      if (read_at(vn, buf, (size_t)chunk, file_off) != 0) {
        return -EIO;
      }
      if (copyout(pt, va, buf, (size_t)chunk) != 0) {
        return -EFAULT;
      }
      file_off += chunk;
      va += chunk;
    }

    if (ph.p_vaddr + ph.p_memsz > maxva) {
      maxva = ph.p_vaddr + ph.p_memsz;
    }
  }

  info->entry = ehdr.e_entry;
  info->sz = maxva;
  return 0;
}
