/* After every present, the target must equal the rendered frame. */
#include "test_util.h"
#include "../src/present.h"
static uint8_t target[ARELIA_RW * ARELIA_RH], fb[ARELIA_RW * ARELIA_RH];
static unsigned calls;
static void emit(void *ctx, int x, int y, int w, uint8_t c){
 (void)ctx; assert(x >= 0 && w > 0 && x + w <= ARELIA_RW && y >= 0 && y < ARELIA_RH);
 memset(target + y * ARELIA_RW + x, c, (size_t)w); calls++;
}
int main(void){
 static AreliaGame g; static AreliaPresenter p; unsigned i, first, idle;
 arelia_present_invalidate(&p); start_game(&g);
 game_render(&g, fb); arelia_present(&p, fb, emit, 0); assert(!memcmp(target, fb, sizeof(fb)));
 first = calls; calls = 0;
 arelia_present(&p, fb, emit, 0); idle = calls; assert(idle == 0);
 for(i = 0; i < 400; i++){
  game_step(&g, (i / 30) & 1 ? ARELIA_INPUT_UP : ARELIA_INPUT_LEFT | ARELIA_INPUT_UP);
  game_render(&g, fb); arelia_present(&p, fb, emit, 0);
  assert(!memcmp(target, fb, sizeof(fb)));
 }
 /* Invalidation and the periodic resync repaint everything. */
 memset(target, 7, sizeof(target)); arelia_present_invalidate(&p); arelia_present(&p, fb, emit, 0);
 assert(!memcmp(target, fb, sizeof(fb)));
 for(i = 0; i < ARELIA_PRESENT_RESYNC_FRAMES + 2; i++) arelia_present(&p, fb, emit, 0);
 {unsigned y, same = 0; for(y = 0; y < ARELIA_RH; y++) same += arelia_present_row_hash(fb + y * ARELIA_RW) == p.row_hash[y]; assert(same == ARELIA_RH);}
 printf("Presenter test passed: first frame %u runs, %u runs over 400 moving frames\n", first, calls);
 return 0;
}
