#include "present.h"
static uint8_t shadow_get(const uint8_t *s, unsigned i){return (uint8_t)((i&1u)?(s[i>>1]&15u):(s[i>>1]>>4));}
static void shadow_set(uint8_t *s, unsigned i, uint8_t c){
 uint8_t *b=&s[i>>1];
 *b=(uint8_t)((i&1u)?((*b&0xf0u)|(c&15u)):((*b&0x0fu)|(uint8_t)((c&15u)<<4)));
}
void arelia_present_invalidate(AreliaPresenter *p){p->valid=0;p->frames_since_full=0;}
unsigned arelia_present(AreliaPresenter *p, const uint8_t *fb, AreliaRunFn emit, void *ctx){
 unsigned runs=0,y;int full;
 if(!p||!fb)return 0;
 /* A rare full resync repairs anything else that drew over the game area. */
 if(p->valid&&++p->frames_since_full>=ARELIA_PRESENT_RESYNC_FRAMES)p->valid=0;
 full=!p->valid;
 for(y=0;y<ARELIA_HEIGHT;y++){
  unsigned row=y*ARELIA_WIDTH,x=0;
  while(x<ARELIA_WIDTH){
   unsigned start;uint8_t c=(uint8_t)(fb[row+x]&15u);
   if(!full&&shadow_get(p->shadow,row+x)==c){x++;continue;}
   /* Extend through same-colored pixels, changed or not: one call covers both. */
   start=x;
   do{shadow_set(p->shadow,row+x,c);x++;}while(x<ARELIA_WIDTH&&(fb[row+x]&15u)==c);
   if(emit)emit(ctx,(int)start,(int)y,(int)(x-start),c);
   runs++;
  }
 }
 if(full){p->valid=1;p->frames_since_full=0;}
 return runs;
}
