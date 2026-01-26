#pragma once

#include <stddef.h>
#include <stdint.h>

int disk_init(uint64_t virtio_base);
int disk_read_bytes(uint64_t offset, void *dst, size_t len);