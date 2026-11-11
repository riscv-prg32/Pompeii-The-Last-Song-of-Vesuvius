#include "../src/game.h"
#include "../src/music.h"
#include "../src/sprites.h"
#include <assert.h>
#include <string.h>
int main(void){
 uint8_t fb[ARELIA_WIDTH*ARELIA_HEIGHT]; AreliaMusic m; AreliaNote n;int i,count=0;
 memset(fb,0,sizeof(fb));arelia_draw_sprite8(fb,0,0,arelia_hero_sprite);
 assert(fb[2]!=0);assert(fb[ARELIA_WIDTH*7+7]==0);
 arelia_music_init(&m);for(i=0;i<200;i++)count+=arelia_music_step(&m,0,&n);
 assert(count>=10);count=0;
 for(i=0;i<200;i++)count+=arelia_music_step(&m,1,&n);
 assert(count>=10); /* the in-game ostinato plays during exploration */assert(arelia_music_step(&m,3,&n)==1);
 return 0;
}
