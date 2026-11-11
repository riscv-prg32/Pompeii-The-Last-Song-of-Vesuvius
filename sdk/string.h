#ifndef ARELIA_FREESTANDING_STRING_H
#define ARELIA_FREESTANDING_STRING_H
/* Minimal freestanding shim for the cartridge translation unit (no libc). */
#include <stddef.h>
static inline void *memset(void *p, int v, size_t n) { unsigned char *q=(unsigned char*)p; while(n--) *q++=(unsigned char)v; return p; }
static inline void *memmove(void *d, const void *s, size_t n) {
  unsigned char *a=(unsigned char*)d; const unsigned char *b=(const unsigned char*)s;
  if (a < b) while (n--) *a++=*b++; else { a+=n; b+=n; while (n--) *--a=*--b; }
  return d;
}
#endif
