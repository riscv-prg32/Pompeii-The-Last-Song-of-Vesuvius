#ifndef ARELIA_FREESTANDING_STRING_H
#define ARELIA_FREESTANDING_STRING_H
#include <stddef.h>
static inline void *memset(void *p, int v, size_t n) { unsigned char *q=(unsigned char*)p; while(n--) *q++=(unsigned char)v; return p; }
#endif
