/* Incremental presenter: the target must always equal the rendered frame. */
#include "../src/game.h"
#include "../src/present.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define N (ARELIA_WIDTH*ARELIA_HEIGHT)
static uint8_t target[N];
static unsigned pixels_sent;
static void apply(void *ctx,int x,int y,int w,uint8_t c){
 (void)ctx;assert(x>=0&&y>=0&&w>0&&x+w<=ARELIA_WIDTH&&y<ARELIA_HEIGHT&&c<16);
 memset(&target[y*ARELIA_WIDTH+x],c,(size_t)w);pixels_sent+=(unsigned)w;
}
int main(void){
 static AreliaGame g;static uint8_t fb[N];static AreliaPresenter p;
 unsigned i,runs,full_runs,diff_total=0,full_total=0,frames=0;
 memset(target,0xee,sizeof(target)); /* garbage the target to prove full sync */
 game_init(&g);arelia_present_invalidate(&p);game_render(&g,fb);
 full_runs=arelia_present(&p,fb,apply,0);
 assert(full_runs>0&&memcmp(target,fb,N)==0);
 assert(arelia_present(&p,fb,apply,0)==0); /* identical frame: nothing sent */
 game_step(&g,ARELIA_INPUT_SELECT);game_step(&g,0);
 for(i=0;i<400;i++){
  unsigned b=(i%40<20)?ARELIA_INPUT_RIGHT:ARELIA_INPUT_DOWN;
  if(i%9==0)b|=ARELIA_INPUT_SWORD;
  game_step(&g,b);game_render(&g,fb);
  runs=arelia_present(&p,fb,apply,0);
  assert(memcmp(target,fb,N)==0);
  {static AreliaPresenter fresh;arelia_present_invalidate(&fresh);full_total+=arelia_present(&fresh,fb,0,0);}
  diff_total+=runs;frames++;
 }
 assert(diff_total*2<full_total); /* the diff must save at least half the calls */
 /* Something else drew over the target: invalidation repairs it. */
 memset(target,3,N/3);arelia_present_invalidate(&p);arelia_present(&p,fb,apply,0);
 assert(memcmp(target,fb,N)==0);
 /* Periodic resync sends a full frame even when nothing changed. */
 for(i=0;i+1<ARELIA_PRESENT_RESYNC_FRAMES;i++)assert(arelia_present(&p,fb,apply,0)==0);
 pixels_sent=0;assert(arelia_present(&p,fb,apply,0)>0);assert(pixels_sent==N);
 printf("present_test: %u frames, %u runs incremental vs %u full (%.1f%%)\n",
  frames,diff_total,full_total,100.0*diff_total/full_total);
 return 0;
}
