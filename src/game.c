/* Pompeii: The Last Song of Vesuvius. Original historical-fantasy adventure. */
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#include "game.h"
#include "sprites.h"
#include <string.h>
#define TW 20
const uint32_t arelia_palette[16]={0x181824,0x3e3334,0x607445,0x9b8c62,0x476878,0x5c6542,0xe3c58e,0x665a52,0xaba08a,0xb04437,0x986d4a,0x619b91,0xe8b650,0x9c687b,0xe7d7b2,0xffffff};
#define TH 12
/* 0 street,1 Roman masonry,2 aqueduct,3 bronze key,4 locked arch,5 Sibyl altar,6 evacuation exit,7 rubble,8 citizen token,9 bridge */
/* Fixed-size arrays, not pointer tables: portable PRG32 cartridges carry no
 * relocations, so .rodata must hold no absolute addresses. */
static const char maps[ARELIA_ROOMS][TH][TW+1]={
 {"11111111101111111111","10000000000000000001","10007000080007000001","10000000000000000001","10000000002000000001","10000000002000003001","10000000002000000001","10000000002000000001","10000000009000000001","10000000000000000001","10000000000000000001","11111111101111111111"},
 {"11111111101111111111","10000000000000000001","10000000000000000001","10000000000000000001","10000000111100000001","10000000100400000001","10003000100100000001","10000000100100000001","10000000100100000001","10000000000800000001","10000000000000000001","11111111101111111111"},
 {"11111111101111111111","10000000000000000001","10000000000000000001","10000000000000000001","10000022222000000001","10000020002000000001","10000020502000800001","10000020092000000001","10000029992000000001","10000000000000000001","10000000000000000001","11111111101111111111"},
 {"11111111101111111111","10000000000000000001","10000000000000000001","10000000000000000001","10000111111111000001","10000100000001000001","10000100060001080001","10000100000001000001","10000111141111000001","10000000000000000001","10000000000000000001","11111111101111111111"}
};
/* Rubble beside each district's citizen (districts 1-3); never on the citizen. */
static const uint8_t hazard_x[ARELIA_ROOMS]={0,10,13,14},hazard_y[ARELIA_ROOMS]={0,9,6,6};
static void load_room(AreliaGame *g, unsigned room){int y,x;g->room=(uint8_t)room;
for(y=0;y<TH;y++)for(x=0;x<TW;x++){uint8_t t=(uint8_t)(maps[room][y][x]-'0');
if(t==4&&g->gate_open[room])t=0;
if(t==8&&(g->crystals&(1u<<room)))t=0;
if(t==3&&g->collected_keys[room])t=0;
g->tiles[y][x]=t;}if(room>0&&!g->cleared_hazards[room])g->tiles[hazard_y[room]][hazard_x[room]]=7;memset(g->enemies,0,sizeof(g->enemies));
if(room==0){g->enemies[0]=(AreliaEnemy){5,4,2,1};g->enemies[1]=(AreliaEnemy){15,8,2,1};}if(room==1){g->enemies[0]=(AreliaEnemy){4,4,2,1};g->enemies[1]=(AreliaEnemy){15,6,3,1};}if(room==2){g->enemies[0]=(AreliaEnemy){15,3,3,1};}if(room==3){g->enemies[0]=(AreliaEnemy){10,3,4,1};}for(x=0;x<ARELIA_ENEMIES;x++)if(g->defeated_enemies[room]&(1u<<x))g->enemies[x].active=0;
g->visited|=1u<<room;}
void game_init(AreliaGame*g){memset(g,0,sizeof(*g));g->x=9;g->y=9;g->hp=6;g->mode=0;load_room(g,0);}
static int traversable(AreliaGame*g,int x,int y){uint8_t t;
if(x<0||y<0||x>=TW||y>=TH)return 0;t=g->tiles[y][x];
if(t==1||t==2||t==7||t==8)return 0; /* citizens stand their ground: face them, press B */
if(t==4){if(!g->keys)return 0;g->keys--;g->gate_open[g->room]=1;g->sound_event=3;g->tiles[y][x]=0;g->dialogue=7;g->dialogue_timer=120;}return 1;}
static void pickup(AreliaGame*g){uint8_t*t=&g->tiles[g->y][g->x];
if(*t==3){g->keys++;g->collected_keys[g->room]=1;g->sound_event=1;*t=0;}/* Citizens require ACTION while facing them; walking over a citizen does not collect them. */if(*t==6&&g->crystals==((1u<<ARELIA_ROOMS)-1u) && g->notes>=3){g->won=1;g->eruption_frames=0;g->dialogue=8;g->dialogue_timer=255;g->sound_event=6;}}
static void move(AreliaGame*g,int dx,int dy){int nx=(int)g->x+dx,ny=(int)g->y+dy;
if(ny==0&&g->y==1&&g->x==9&&dy<0&&g->room<3){load_room(g,g->room+1);g->x=9;g->y=10;g->dialogue=(uint8_t)(8+g->room);g->dialogue_timer=125;return;}if(ny==11&&g->y==10&&g->x==9&&dy>0&&g->room>0){load_room(g,g->room-1);g->x=9;g->y=1;return;}{int i;for(i=0;i<ARELIA_ENEMIES;i++)if(g->enemies[i].active&&g->enemies[i].x==nx&&g->enemies[i].y==ny)return;} /* enemies block: turn to face them */
if(traversable(g,nx,ny)){g->x=(uint8_t)nx;g->y=(uint8_t)ny;pickup(g);}}
void game_step(AreliaGame*g,unsigned buttons){int i,dx=0,dy=0;
if((buttons&ARELIA_INPUT_SELECT)&&!(g->previous_buttons&ARELIA_INPUT_SELECT)){if(g->dead||g->won){game_init(g);return;}if(!g->mode){g->mode=1;g->dialogue=1;g->dialogue_timer=115;}}if(g->won){if(g->eruption_frames<240)g->eruption_frames++;g->previous_buttons=(uint8_t)buttons;return;}if(!g->mode||g->dead){g->previous_buttons=(uint8_t)buttons;return;}g->ticks++;
 if(g->dialogue_timer)g->dialogue_timer--;
 if(!g->dialogue_timer)g->dialogue=0;
 /* A three-phrase tibia call: UP, LEFT, RIGHT. One note per fresh press. */
 {unsigned fresh=buttons & ~(unsigned)g->previous_buttons;
  if(g->melody_active){
   static const unsigned sequence[3]={ARELIA_INPUT_UP,ARELIA_INPUT_LEFT,ARELIA_INPUT_RIGHT};
   if(fresh & (ARELIA_INPUT_UP|ARELIA_INPUT_LEFT|ARELIA_INPUT_RIGHT)){
    if(fresh & sequence[g->melody_progress]){g->melody_progress++;g->sound_event=3;
      if(g->melody_progress==3){g->notes=3;g->melody_active=0;g->dialogue=12;g->dialogue_timer=140;}}
    else {g->melody_progress=0;g->sound_event=4;}
   }
   if(fresh & ARELIA_INPUT_ACTION)g->melody_active=0;
   g->previous_buttons=(uint8_t)buttons;
   return;
  }
  if(fresh & ARELIA_INPUT_ACTION){
   if(g->tiles[g->y][g->x]==5){g->melody_active=1;g->melody_progress=0;g->dialogue=6;g->dialogue_timer=170;}
   else {int dx=(g->face==3)-(g->face==2),dy=(g->face==1)-(g->face==0);int nx=g->x+dx,ny=g->y+dy;
    if(nx>=0&&nx<TW&&ny>=0&&ny<TH&&g->tiles[ny][nx]==8){if(g->room>0&&!g->cleared_hazards[g->room]){g->dialogue=13;g->dialogue_timer=120;}else{g->tiles[ny][nx]=0;g->crystals|=1u<<g->room;g->rescued_count++;g->dialogue=2+g->room;g->dialogue_timer=210;g->sound_event=5;}}
   }
  }
  g->previous_buttons=(uint8_t)buttons;
 }
 if(g->invuln)g->invuln--;
 if(g->attack)g->attack--;
if(g->move_cooldown)g->move_cooldown--;
if(buttons&ARELIA_INPUT_UP){dy=-1;g->face=0;}else if(buttons&ARELIA_INPUT_DOWN){dy=1;g->face=1;}else if(buttons&ARELIA_INPUT_LEFT){dx=-1;g->face=2;}else if(buttons&ARELIA_INPUT_RIGHT){dx=1;g->face=3;}if((dx||dy)&&!g->move_cooldown){move(g,dx,dy);g->move_cooldown=5;}if((buttons&ARELIA_INPUT_SWORD)&&!g->attack){int sx=g->x+(g->face==3)-(g->face==2),sy=g->y+(g->face==1)-(g->face==0);g->attack=10;g->sound_event=2;if(g->room>0&&sx>=0&&sx<TW&&sy>=0&&sy<TH&&g->tiles[sy][sx]==7){g->tiles[sy][sx]=0;g->cleared_hazards[g->room]=1;g->dialogue=14;g->dialogue_timer=120;g->sound_event=3;}
for(i=0;i<ARELIA_ENEMIES;i++){AreliaEnemy*e=&g->enemies[i];
if(e->active&&e->x==sx&&e->y==sy){if(--e->hp<=0){e->active=0;g->defeated_enemies[g->room]|=(uint8_t)(1u<<i);g->sound_event=4;}}}}if(++g->enemy_cooldown>=22){g->enemy_cooldown=0;
for(i=0;i<ARELIA_ENEMIES;i++){AreliaEnemy*e=&g->enemies[i];int nx,ny;
if(!e->active)continue;nx=e->x;ny=e->y;
if(g->x>nx)nx++;else if(g->x<nx)nx--;else if(g->y>ny)ny++;else if(g->y<ny)ny--;
if(nx==g->x&&ny==g->y){if(!g->invuln){g->hp--;g->sound_event=4;g->invuln=35;
if(!g->hp)g->dead=1;}}else if(nx>=0&&ny>=0&&nx<TW&&ny<TH&&g->tiles[ny][nx]==0){e->x=(int8_t)nx;e->y=(int8_t)ny;}}}pickup(g);}
static void rect(uint8_t*fb,int x,int y,int w,int h,uint8_t c){int i,j;
for(j=y;j<y+h;j++)if((unsigned)j<ARELIA_HEIGHT)for(i=x;i<x+w;i++)if((unsigned)i<ARELIA_WIDTH)fb[j*ARELIA_WIDTH+i]=c;}

/* Tiny built-in 3x5 uppercase font: no external assets or font dependencies. */
static const uint8_t glyphs[59][5]={{0,0,0,0,0},{2,2,2,0,2},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,7,0,0},{0,0,0,0,2},{1,1,2,4,4},{7,5,5,5,7},{2,6,2,2,7},{6,1,7,4,7},{6,1,3,1,6},{5,5,7,1,1},{7,4,6,1,6},{3,4,7,5,7},{7,1,1,2,2},{7,5,7,5,7},{7,5,7,1,6},{0,2,0,2,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{5,7,7,7,5},{2,5,5,5,2},{6,5,6,4,4},{2,5,5,3,1},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7}};
static void print3(uint8_t*fb,int x,int y,const char*str,uint8_t c){int i,j,k;while(*str){unsigned a=(unsigned char)*str++;
if(a>=97&&a<=122)a-=32;
if(a<32||a>90)a=32;
for(j=0;j<5;j++)for(k=0;k<3;k++)if(glyphs[a-32][j]&(4>>k))for(i=0;i<2;i++)rect(fb,x+k*2,y+j*2+i,2,1,c);x+=8;}}
static const char room_names[ARELIA_ROOMS][14]={"FORUM","PAINTED VILLA","WATERWORKS","SEA ROAD"};
static const char messages[15][33]={"","LIVIA: FIND THE LOST TIBIA","MARCUS: TAKE THE SOUTH ROAD","FLAVIA: THE WALLS ARE SHAKING","FELIX: THE CISTERN IS CRACKED","TULLIA: I CAN SEE THE SEA","TIBIA: UP LEFT RIGHT","THE STONE ARCH OPENS","THE PEOPLE ESCAPE. POMPEII FALLS","THE PAINTED VILLA","THE WATERWORKS","THE ROAD TO THE SEA","THE SIBYL HEARS. RUN TO THE SEA","CLEAR THE FALLEN STONES","THE PASSAGE IS SAFE"};
/* District-specific procedural architecture; no external tile textures. */
static void architecture(uint8_t*fb,int x,int y,int room,uint32_t ticks){
 int px=x*16,py=y*16,seed=(x*13+y*7+room*19);
 uint8_t stone=(room==1)?14:(room==2)?7:(room==3)?8:6;
 uint8_t mortar=(room==1)?9:(room==2)?1:7;
 rect(fb,px,py,16,16,stone);
 if(room==0){ /* Forum: travertine colonnade and red shop awnings */
  rect(fb,px,py+1,16,2,8);rect(fb,px+2,py+3,3,11,14);
  rect(fb,px+11,py+3,3,11,14);rect(fb,px+1,py+13,14,2,7);
  if((x+y)%3==0)rect(fb,px+5,py+4,6,3,9);
 }else if(room==1){ /* Villa: painted plaster, fresco bands, mosaic dado */
  rect(fb,px,py,16,3,9);rect(fb,px,py+12,16,4,7);
  rect(fb,px+3,py+4,10,7,13);rect(fb,px+5,py+5,6,5,6);
  rect(fb,px+7,py+4,2,7,2);
  if(seed%3==0)rect(fb,px+1,py+13,3,2,15);
 }else if(room==2){ /* Cistern: damp basalt blocks and water seams */
  rect(fb,px,py+7,16,1,1);rect(fb,px+8,py,1,7,1);
  rect(fb,px+3,py+11,10,2,4);rect(fb,px+4+(ticks/16%4),py+12,3,1,11);
 }else{ /* Sea road: ash-dusted masonry, coastal light */
  rect(fb,px,py+7,16,1,7);rect(fb,px+7,py,1,7,7);
  rect(fb,px+1,py+1,14,2,14);rect(fb,px+2,py+11,5,2,6);
 }
 (void)mortar;
}
static void tile(uint8_t*fb,int x,int y,uint8_t t,int room,uint32_t ticks){
 int px=x*16,py=y*16;static const uint8_t colors[]={3,7,4,12,10,14,15,5,13,6};
 rect(fb,px,py,16,16,colors[t]);
 if(t==0){
  if(room==0){rect(fb,px,py+8,16,1,8);rect(fb,px+8,py,1,8,8);rect(fb,px+3,py+12,2,1,10);}
  if(room==1){rect(fb,px,py+8,16,1,7);rect(fb,px+8,py,1,16,7);rect(fb,px+4,py+4,2,2,13);rect(fb,px+12,py+12,2,2,13);}
  if(room==2){rect(fb,px,py+8,16,1,4);rect(fb,px+8,py,1,8,4);rect(fb,px+3+(ticks/12%3),py+13,5,1,11);}
  if(room==3){rect(fb,px,py+8,16,1,8);rect(fb,px+8,py,1,8,8);if((x*7+y)%4==0)rect(fb,px+3,py+4,5,2,14);}
 }
 if(t==1)architecture(fb,x,y,room,ticks);
 if(t==2){rect(fb,px+2,py+3,12,11,7);rect(fb,px+4,py+6,8,8,4);rect(fb,px+5,py+10+(ticks/12%2),6,2,11);}
 if(t==3){rect(fb,px+6,py+4,4,8,15);rect(fb,px+4,py+3,6,3,12);}
 if(t==4){rect(fb,px+2,py+1,12,14,7);rect(fb,px+4,py+3,8,12,1);rect(fb,px+7,py+5,2,9,12);}
 if(t==5){rect(fb,px+2,py+3,12,11,8);rect(fb,px+4,py+1,8,3,14);rect(fb,px+5,py+5,2,7,12);rect(fb,px+9,py+5,2,7,12);}
 if(t==6){rect(fb,px+2,py+2,12,13,8);rect(fb,px+4,py+4,8,11,1);rect(fb,px+7,py+1,2,4,12);}
 if(t==7){rect(fb,px+1,py+7,10,7,7);rect(fb,px+9,py+4,6,9,8);}
 if(t==8){arelia_draw_sprite8(fb,px+4,py+4,arelia_hero_sprite);rect(fb,px+2,py+2,12,2,12);}
 if(t==9){rect(fb,px,py+8,16,6,10);rect(fb,px,py+7,16,2,14);}
}
static void eruption_overlay(uint8_t*fb,uint32_t ticks,uint16_t finale){
 int x,y,phase=(int)(ticks/380);
 if(phase>4)phase=4;
 if(finale){phase=2+(int)(finale/55);if(phase>6)phase=6;}
 /* A mountain silhouette, plume, ash streaks, and ember rain. */
 for(y=0;y<12+phase*3;y++){
  int half=5+y/2+phase*2;
  if(half>55)half=55;
  rect(fb,160-half,y,half*2,1,(uint8_t)(phase>=4?1:7));
 }
 for(x=0;x<320;x+=2){
  int ridge=18-((x>160?x-160:160-x)/9);
  if(ridge>0)rect(fb,x,26-ridge,2,ridge,1);
 }
 for(x=0;x<20+phase*9;x++){
  int sx=(x*71+(int)(ticks/5))%320;
  int sy=(x*37+(int)(ticks/9))%152;
  rect(fb,sx,sy,phase>2?2:1,phase>4?3:1,(uint8_t)((x%4==0)?12:7));
 }
 if(phase>=3){rect(fb,0,0,320,2+phase,1);}
 if(finale>175){rect(fb,0,0,320,200,1);}
}
void game_render(const AreliaGame*g,uint8_t*fb){int x,y,i;rect(fb,0,0,320,200,0);
for(y=0;y<12;y++)for(x=0;x<20;x++)tile(fb,x,y,g->tiles[y][x],g->room,g->ticks);
for(i=0;i<ARELIA_ENEMIES;i++)if(g->enemies[i].active){const AreliaEnemy*e=&g->enemies[i];arelia_draw_sprite8(fb,e->x*16+4,e->y*16+4,arelia_enemy_sprite);}if(!(g->invuln&2)){arelia_draw_sprite8(fb,g->x*16+4,g->y*16+4,arelia_hero_sprite);
if(g->attack){int ax=g->x*16+6,ay=g->y*16+6;
if(g->face==0)ay-=12;
if(g->face==1)ay+=12;
if(g->face==2)ax-=12;
if(g->face==3)ax+=12;rect(fb,ax,ay,4,10,15);}}if(g->mode&&!g->dead)eruption_overlay(fb,g->ticks,g->won?g->eruption_frames:0);rect(fb,0,192,320,8,1);
 if(g->mode){rect(fb,0,0,320,12,1);print3(fb,4,1,room_names[g->room],15);}
 if(g->dialogue && g->dialogue_timer){rect(fb,8,156,304,32,1);rect(fb,10,158,300,28,7);print3(fb,16,166,messages[g->dialogue],15);}
 if(g->melody_active){rect(fb,24,135,272,17,1);print3(fb,32,138,"UP LEFT RIGHT",12);}
for(i=0;i<g->hp;i++)rect(fb,4+i*11,194,8,4,9);
for(i=0;i<g->keys;i++)rect(fb,90+i*8,194,6,4,12);
for(i=0;i<3;i++)rect(fb,180+i*12,194,9,4,i<g->notes?15:7);
for(i=0;i<ARELIA_ROOMS;i++)rect(fb,250+i*12,194,9,4,(g->visited&(1u<<i))?14:7);
if(!g->mode||g->dead||g->won){rect(fb,56,60,208,80,7);rect(fb,60,64,200,72,g->won?13:g->dead?9:6);if(g->won){if(g->eruption_frames<80){print3(fb,75,88,"VESUVIUS AWAKENS",15);print3(fb,75,107,"RUN TO THE COAST",15);}else if(g->eruption_frames<175){print3(fb,75,88,"THE CITY FALLS",15);print3(fb,75,107,"THE PEOPLE ESCAPE",15);}else{print3(fb,75,88,"THE LAST SONG",15);print3(fb,75,107,"SURVIVORS SAVED",15);}}else if(g->dead){print3(fb,80,90,"ASH AND SILENCE",15);print3(fb,80,110,"SELECT TO RETRY",15);}else{print3(fb,80,85,"POMPEII AD 79",1);print3(fb,80,105,"SELECT TO PLAY",1);}}}
static unsigned append(char*out,unsigned n,unsigned cap,const char*s){while(*s&&n+1<cap)out[n++]=*s++;return n;}
void game_status_text(const AreliaGame*g,char*out,unsigned cap){
 unsigned n=0,saved=0,i;char count[4];
 if(!out||!cap)return;
 for(i=0;i<ARELIA_ROOMS;i++)if(g->crystals&(1u<<i))saved++;
 count[0]=(char)('0'+saved);count[1]='/';count[2]=(char)('0'+ARELIA_ROOMS);count[3]=0;
 if(!g->mode)n=append(out,n,cap,"POMPEII AD 79");
 else if(g->won){n=append(out,n,cap,"SURVIVORS SAVED ");n=append(out,n,cap,count);}
 else if(g->dead)n=append(out,n,cap,"ASH AND SILENCE");
 else{n=append(out,n,cap,room_names[g->room<ARELIA_ROOMS?g->room:0]);n=append(out,n,cap," SAVED ");n=append(out,n,cap,count);
  if(g->notes>=3)n=append(out,n,cap," TIBIA");}
 out[n]=0;
}
