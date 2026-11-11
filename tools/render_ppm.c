#include "../src/game.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
 static AreliaGame g;
 static uint8_t fb[ARELIA_WIDTH*ARELIA_HEIGHT];
 FILE *f; int i;
 if(argc!=2)return 2;
 game_init(&g);game_step(&g,ARELIA_INPUT_START);
 for(i=0;i<70;i++)game_step(&g,(i%20<10)?ARELIA_INPUT_RIGHT:ARELIA_INPUT_DOWN);
 game_render(&g,fb);f=fopen(argv[1],"wb");if(!f)return 3;
 fprintf(f,"P6\n%d %d\n255\n",ARELIA_WIDTH,ARELIA_HEIGHT);
 for(i=0;i<ARELIA_WIDTH*ARELIA_HEIGHT;i++){
  uint32_t c=arelia_palette[fb[i]&15];
  fputc((int)((c>>16)&255),f);fputc((int)((c>>8)&255),f);fputc((int)(c&255),f);
 }
 if(fclose(f))return 4;
 return 0;
}
