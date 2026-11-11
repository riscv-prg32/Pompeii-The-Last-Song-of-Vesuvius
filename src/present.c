#include "present.h"
void arelia_present_invalidate(AreliaPresenter *p){if(p){p->valid = 0; p->frames_since_full = 0;}}
uint32_t arelia_present_row_hash(const uint8_t *row){
 uint32_t h = 2166136261u; unsigned x;
 for(x = 0; x < ARELIA_RW; x++){h ^= row[x]; h *= 16777619u;}
 return h;
}
unsigned arelia_present(AreliaPresenter *p, const uint8_t *fb, AreliaRunFn emit, void *ctx){
 unsigned runs = 0, y; int full;
 if(!p || !fb) return 0;
 if(p->valid && ++p->frames_since_full >= ARELIA_PRESENT_RESYNC_FRAMES) p->valid = 0;
 full = !p->valid;
 for(y = 0; y < ARELIA_RH; y++){
  const uint8_t *row = fb + y * ARELIA_RW;
  uint32_t h = arelia_present_row_hash(row);
  unsigned x = 0;
  if(!full && h == p->row_hash[y]) continue;
  p->row_hash[y] = h;
  while(x < ARELIA_RW){
   unsigned start = x; uint8_t c = row[x];
   while(x < ARELIA_RW && row[x] == c) x++;
   if(emit) emit(ctx, (int)start, (int)y, (int)(x - start), c);
   runs++;
  }
 }
 if(full){p->valid = 1; p->frames_since_full = 0;}
 return runs;
}
