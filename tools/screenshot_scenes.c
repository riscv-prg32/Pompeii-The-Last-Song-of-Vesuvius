/* Capture actual game_render() output from deterministic game states. */
#include "../src/game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t fb[ARELIA_WIDTH*ARELIA_HEIGHT];
static AreliaGame game;
static int save(const char *folder,const char *name){
 char path[512];FILE *out;int i;
 if(snprintf(path,sizeof(path),"%s/%s",folder,name)>=(int)sizeof(path))return 1;
 game_render(&game,fb);out=fopen(path,"wb");if(!out)return 2;
 fprintf(out,"P6\n%d %d\n255\n",ARELIA_WIDTH,ARELIA_HEIGHT);
 for(i=0;i<ARELIA_WIDTH*ARELIA_HEIGHT;i++){
  unsigned c=arelia_palette[fb[i]&15u];
  fputc((int)((c>>16)&255u),out);fputc((int)((c>>8)&255u),out);fputc((int)(c&255u),out);
 }
 return fclose(out)!=0;
}
int main(int argc,char **argv){int err=0;
 if(argc!=2)return 2;
 game_init(&game);err|=save(argv[1],"title.ppm");
 game_step(&game,ARELIA_INPUT_SELECT);game.dialogue_timer=0;game.dialogue=0;
 game.x=9;game.y=8;game.ticks=250;err|=save(argv[1],"forum.ppm");
 game.room=2;game.x=8;game.y=6;game.dialogue=6;game.dialogue_timer=170;
 game.melody_active=1;game.ticks=1050;err|=save(argv[1],"tibia.ppm");
 game.won=1;game.eruption_frames=115;game.melody_active=0;game.dialogue=0;game.dialogue_timer=0;
 err|=save(argv[1],"ending.ppm");game.eruption_frames=35;err|=save(argv[1],"eruption.ppm");return err?1:0;
}
