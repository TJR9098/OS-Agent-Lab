#include <stdlib.h>
#include <unistd.h>

typedef long Align;

union header {
  struct {
    union header *next;
    size_t size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

#define NALLOC 1024

static Header *morecore(size_t nu) {
  if (nu < NALLOC) {
    nu = NALLOC;
  }
  size_t bytes = nu * sizeof(Header);
  void *p = sbrk((intptr_t)bytes);
  if (p == (void *)-1) {
    return NULL;
  }
  Header *hp = (Header *)p;
  hp->s.size = nu;
  free((void *)(hp + 1));
  return freep;
}

void free(void *ap) {
  if (!ap) {
    return;
  }
  Header *bp = (Header *)ap - 1;
  Header *p = freep;
  if (!p) {
    base.s.next = freep = p = &base;
    base.s.size = 0;
  }
  for (; !(bp > p && bp < p->s.next); p = p->s.next) {
    if (p >= p->s.next && (bp > p || bp < p->s.next)) {
      break;
    }
  }
  if (bp + bp->s.size == p->s.next) {
    bp->s.size += p->s.next->s.size;
    bp->s.next = p->s.next->s.next;
  } else {
    bp->s.next = p->s.next;
  }
  if (p + p->s.size == bp) {
    p->s.size += bp->s.size;
    p->s.next = bp->s.next;
  } else {
    p->s.next = bp;
  }
  freep = p;
}

void *malloc(size_t nbytes) {
  Header *p;
  Header *prevp;
  size_t nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
  if ((prevp = freep) == NULL) {
    base.s.next = freep = prevp = &base;
    base.s.size = 0;
  }
  for (p = prevp->s.next;; prevp = p, p = p->s.next) {
    if (p->s.size >= nunits) {
      if (p->s.size == nunits) {
        prevp->s.next = p->s.next;
      } else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void *)(p + 1);
    }
    if (p == freep) {
      p = morecore(nunits);
      if (p == NULL) {
        return NULL;
      }
    }
  }
}

void *realloc(void *ptr, size_t size) {
  if (!ptr) {
    return malloc(size);
  }
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  Header *bp = (Header *)ptr - 1;
  size_t old = (bp->s.size - 1) * sizeof(Header);
  if (old >= size) {
    return ptr;
  }
  void *newp = malloc(size);
  if (!newp) {
    return NULL;
  }
  size_t to_copy = old < size ? old : size;
  unsigned char *d = (unsigned char *)newp;
  unsigned char *s = (unsigned char *)ptr;
  for (size_t i = 0; i < to_copy; i++) {
    d[i] = s[i];
  }
  free(ptr);
  return newp;
}

int mallopt(int param, int value) {
  (void)param;
  (void)value;
  return 0;
}
