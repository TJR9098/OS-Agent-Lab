#pragma once

#include <stdint.h>

#include "vm.h"

struct vnode;

struct elf_info {
  uint64_t entry;
  uint64_t sz;
};

int elf_load(pagetable_t pt, struct vnode *vn, struct elf_info *info);
