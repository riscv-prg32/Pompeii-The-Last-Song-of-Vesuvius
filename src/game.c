/* Pompeii: The Last Song of Vesuvius: an original 3D historical-fantasy
 * action-adventure. Deterministic game logic; rendering lives in render.c. */
#include "game.h"
#include "r3d.h"
#include <string.h>

#define U(x) ((int32_t)((x) * 256))        /* world units -> Q8 (compile-time constants only) */
#define PLAYER_R 60
#define WALK_SPEED 18
#define FOCUS_SPEED 15
#define ROLL_SPEED 40
#define LOOTER_SPEED 9
#define REACH 330                           /* strike reach, Q8 */
#define CAM_DIST 820
#define CAM_PITCH 36                        /* about 12.7 degrees down */
#define AIM_HEIGHT 230
#define VESUVIUS_YAW 860                    /* north-west of the city */

static const uint8_t song[ARELIA_SONG_LEN] = {0, 1, 2, 0, 1, 2};   /* UP LEFT RIGHT UP LEFT RIGHT */

enum { D_NONE, D_INTRO, D_RESCUE0, D_RESCUE1, D_RESCUE2, D_RESCUE3, D_TRAP0, D_TRAP1, D_TRAP2, D_TRAP3,
       D_ALTAR, D_LEARNED, D_SEAL, D_SEAL_OPEN, D_LOCKED, D_KEY, D_DOOR, D_RUBBLE, D_CLEARED, D_NOT_ALL,
       D_ECHO, D_ALTAR_DONE, D_SEAL_HINT, D_COUNT };
static const char dialogues[D_COUNT][72] = {
 "",
 "LIVIA: THE GROUND WILL NOT STOP|SHAKING. I MUST GET PEOPLE OUT.",
 "MARCUS: THE SOUTH ROAD IS JAMMED.|I WILL TAKE THE HARBOUR PATH!",
 "FLAVIA: THE FRESCOES ARE CRACKING.|THANK YOU. I CAN RUN NOW.",
 "FELIX: THE CISTERN IS BOILING!|THE SIBYL'S ALTAR... IT HUMS.",
 "TULLIA: I CAN SEE THE BOATS!|COME WITH US, LIVIA!",
 "",
 "FLAVIA: THE DOORWAY FELL IN!|BREAK THE STONES WITH A!",
 "FELIX: I AM WALLED IN!|BREAK THE STONES WITH A!",
 "TULLIA: THE TOMB COLLAPSED!|BREAK THE STONES WITH A!",
 "THE ALTAR READS: PLAY TO THE SEA|UP LEFT RIGHT UP LEFT RIGHT",
 "YOU LEARNED THE CALL OF THE SIBYL.|RAISE THE TIBIA WITH C.",
 "A SIBYL'S SEAL BLOCKS THE ROAD.|IT HUMS, WAITING FOR A SONG.",
 "THE STONE ANSWERS THE TIBIA|AND SINKS INTO THE ROAD.",
 "LOCKED. A BRONZE KEY|WOULD TURN THIS LOCK.",
 "YOU FOUND A BRONZE KEY!",
 "THE BRONZE KEY TURNS.",
 "FALLEN MASONRY.|STRIKE IT WITH A.",
 "THE PASSAGE IS CLEAR.",
 "NOT YET. SOMEONE IS STILL|IN THE CITY.",
 "THE CALL ECHOES OVER THE ASH.",
 "THE ALTAR IS SILENT NOW.",
 "THE SEAL HUMS. RAISE THE TIBIA|WITH C, PLAY THE SIBYL'S CALL."
};

const char *game_dialogue_text(unsigned id){return id < D_COUNT ? dialogues[id] : dialogues[0];}

static unsigned text_len(unsigned id){unsigned n = 0; const char *s = game_dialogue_text(id); while(s[n]) n++; return n;}

static void say(AreliaGame *g, unsigned id){g->dialogue = (uint8_t)id; g->dialogue_t = 0;}

static int32_t ent_radius(const AreliaEntity *e){
 switch(e->type){
  case E_LOOTER: return 70; case E_CITIZEN: return 62; case E_RUBBLE: return 110;
  case E_ALTAR: return 112; default: return 0;
 }
}

static void load_room(AreliaGame *g, unsigned room, int from_north){
 int i;
 g->room = (uint8_t)room;
 memset(g->ent, 0, sizeof(g->ent));
 g->ent_count = 0;
 for(i = 0; i < WORLD_MAX_ENT; i++){
  const WorldSpawn *s = &world_spawns[room][i];
  AreliaEntity *e = &g->ent[i];
  if(s->type == E_NONE) break;
  g->ent_count = (uint8_t)(i + 1);
  e->type = s->type; e->param = s->param;
  e->x = (int16_t)(s->x2 * 128); e->z = (int16_t)(s->z2 * 128);
  e->alive = !(g->gone[room] & (1u << i));
  e->hp = s->type == E_LOOTER ? s->param : s->type == E_RUBBLE ? 2 : 1;
  e->yaw = 512; e->state = AI_IDLE;
 }
 g->px = U(7.5);
 g->pz = from_north ? U(1.7) : U(18.3);
 g->pyaw = from_north ? 512 : 0;
 g->cam_yaw = g->pyaw;
 g->focus = 0; g->action = ACT_NONE; g->action_t = 0; g->note_count = 0;
 g->banner_t = 100; g->room_ticks = 0; g->lapilli_n = 0;
 g->palette_phase = (uint8_t)room;
}

void game_init(AreliaGame *g){
 memset(g, 0, sizeof(*g));
 g->hp = ARELIA_HP_MAX;
 g->mode = MODE_TITLE;
 g->cam_pitch = CAM_PITCH;
 load_room(g, 0, 0);
 g->banner_t = 0;
}

/* ------------------------------------------------------------ collision */
static int blocked(AreliaGame *g, int32_t x, int32_t z, int self){
 int i;
 if(world_solid(g->room, x - PLAYER_R, z - PLAYER_R) || world_solid(g->room, x + PLAYER_R, z - PLAYER_R) ||
    world_solid(g->room, x - PLAYER_R, z + PLAYER_R) || world_solid(g->room, x + PLAYER_R, z + PLAYER_R)) return 1;
 for(i = 0; i < g->ent_count; i++){
  AreliaEntity *e = &g->ent[i];
  int32_t r, dx, dz;
  if(i == self || !e->alive) continue;
  if(e->type == E_DOOR || e->type == E_SEAL){
   /* Doors and the seal fill their cell until they open. */
   int ex = e->x >> 8, ez = e->z >> 8;
   if(e->state == 0 && ((x - PLAYER_R) >> 8) <= ex && ((x + PLAYER_R) >> 8) >= ex &&
      ((z - PLAYER_R) >> 8) <= ez && ((z + PLAYER_R) >> 8) >= ez) return 2 + i;
   continue;
  }
  r = ent_radius(e);
  if(!r || e->state == AI_FALL || e->state == AI_LEAVE) continue;
  dx = x - e->x; dz = z - e->z; r += self < 0 ? PLAYER_R : 60;
  if(dx > -r && dx < r && dz > -r && dz < r && dx * dx + dz * dz < r * r) return 2 + i;
 }
 return 0;
}

static void open_door(AreliaGame *g, int i){
 g->ent[i].state = 1; g->ent[i].timer = 30;
 g->gone[g->room] |= (uint16_t)(1u << i);
 g->sound_event = SND_DOOR;
}

/* Move the player by (dx, dz) with sliding collision; bumping a locked door
 * with a key opens it. */
static void player_move(AreliaGame *g, int32_t dx, int32_t dz){
 int hit;
 if(dx){hit = blocked(g, g->px + dx, g->pz, -1); if(!hit) g->px = (int16_t)(g->px + dx);
  else if(hit >= 2 && g->ent[hit - 2].type == E_DOOR && g->keys){g->keys--; open_door(g, hit - 2); say(g, D_DOOR);}}
 if(dz){hit = blocked(g, g->px, g->pz + dz, -1); if(!hit) g->pz = (int16_t)(g->pz + dz);
  else if(hit >= 2 && g->ent[hit - 2].type == E_DOOR && g->keys){g->keys--; open_door(g, hit - 2); say(g, D_DOOR);}}
}

static void ent_move(AreliaGame *g, int i, int32_t dx, int32_t dz){
 AreliaEntity *e = &g->ent[i];
 int32_t nx = e->x + dx, nz = e->z + dz, pdx, pdz;
 if(world_solid(g->room, nx - 60, nz - 60) || world_solid(g->room, nx + 60, nz - 60) ||
    world_solid(g->room, nx - 60, nz + 60) || world_solid(g->room, nx + 60, nz + 60)) return;
 pdx = nx - g->px; pdz = nz - g->pz;                    /* never step into Livia */
 if(pdx * pdx + pdz * pdz < 120 * 120) return;
 e->x = (int16_t)nx; e->z = (int16_t)nz;
}

/* ------------------------------------------------------------ helpers */
static int32_t dist_to(const AreliaGame *g, const AreliaEntity *e){return r3d_dist(e->x - g->px, e->z - g->pz);}
static int facing(const AreliaGame *g, const AreliaEntity *e, int tolerance){
 return r3d_angle_diff(g->pyaw, r3d_angle(e->x - g->px, e->z - g->pz)) < tolerance &&
        r3d_angle_diff(g->pyaw, r3d_angle(e->x - g->px, e->z - g->pz)) > -tolerance;
}
static int turn_toward(int from, int to, int rate){
 int d = r3d_angle_diff(from, to);
 if(d > rate) d = rate;
 if(d < -rate) d = -rate;
 return (from + d) & R3D_ANGLE_MASK;
}

static void hurt_player(AreliaGame *g, int from_yaw, int amount){
 if(g->invuln || g->mode != MODE_PLAY) return;
 g->hp = (uint8_t)(g->hp > amount ? g->hp - amount : 0);
 g->invuln = 45; g->action = ACT_HURT; g->action_t = 10; g->sound_event = SND_HURT;
 player_move(g, r3d_sin(from_yaw) * 90 >> 14, -r3d_cos(from_yaw) * 90 >> 14);
 if(!g->hp){g->mode = MODE_DEAD; g->story = 0; g->focus = 0;}
}

static int nearest_target(const AreliaGame *g){
 int i, best = -1; int32_t bd = 0x7fffffff;
 for(i = 0; i < g->ent_count; i++){
  const AreliaEntity *e = &g->ent[i]; int32_t d; int lim;
  if(!e->alive || e->state == AI_FALL || e->state == AI_LEAVE) continue;
  if(e->type == E_LOOTER) lim = U(6); else if(e->type == E_CITIZEN || e->type == E_RUBBLE || e->type == E_ALTAR || e->type == E_SEAL) lim = U(3.5); else continue;
  d = dist_to(g, e) - (e->type == E_LOOTER ? U(2) : 0);   /* looters first */
  if(dist_to(g, e) < lim && d < bd){bd = d; best = i;}
 }
 return best;
}

/* What B would do now (for the HUD prompt and the action). */
enum { P_NONE, P_RESCUE, P_CHECK, P_TALK };
static int context_target(const AreliaGame *g, int *kind){
 int i;
 for(i = 0; i < g->ent_count; i++){
  const AreliaEntity *e = &g->ent[i]; int32_t d;
  if(!e->alive || e->state == AI_LEAVE) continue;
  d = dist_to(g, e);
  switch(e->type){
   case E_CITIZEN: if(d < U(1.35) && facing(g, e, 200)){*kind = P_RESCUE; return i;} break;
   case E_ALTAR: if(d < U(1.5)){*kind = P_CHECK; return i;} break;
   case E_SEAL: if(d < U(1.9) && e->state == 0){*kind = P_CHECK; return i;} break;
   case E_DOOR: if(d < U(1.4) && e->state == 0){*kind = P_CHECK; return i;} break;
   case E_RUBBLE: if(d < U(1.3) && facing(g, e, 200)){*kind = P_CHECK; return i;} break;
   default: break;
  }
 }
 *kind = P_NONE; return -1;
}

static void context_action(AreliaGame *g){
 int kind, i = context_target(g, &kind);
 AreliaEntity *e;
 if(i < 0) return;
 e = &g->ent[i];
 switch(e->type){
  case E_CITIZEN:
   if(e->param < WORLD_MAX_ENT && g->ent[e->param].alive){say(g, D_TRAP0 + g->room); g->sound_event = SND_TALK; break;}
   g->rescued |= (uint8_t)(1u << g->room);
   g->gone[g->room] |= (uint16_t)(1u << i);
   e->state = AI_LEAVE; e->timer = 50;
   say(g, D_RESCUE0 + g->room); g->sound_event = SND_RESCUE; g->focus = 0;
   break;
  case E_ALTAR:
   if(g->song_known){say(g, D_ALTAR_DONE); break;}
   say(g, D_ALTAR); g->tibia_purpose = 1; break;      /* the tibia rises when the text closes */
  case E_SEAL: say(g, g->song_known ? D_SEAL_HINT : D_SEAL); break;
  case E_DOOR:
   if(g->keys){g->keys--; open_door(g, i); say(g, D_DOOR);} else {say(g, D_LOCKED); g->sound_event = SND_LOCKED;}
   break;
  case E_RUBBLE: say(g, D_RUBBLE); break;
  default: break;
 }
}

static void strike_hits(AreliaGame *g){
 int i;
 for(i = 0; i < g->ent_count; i++){
  AreliaEntity *e = &g->ent[i];
  if(!e->alive || (e->type != E_LOOTER && e->type != E_RUBBLE) || e->state == AI_FALL) continue;
  if(dist_to(g, e) > REACH + (e->type == E_RUBBLE ? 60 : 0) || !facing(g, e, 150)) continue;
  e->flash = 8;
  if(e->type == E_RUBBLE){
   if(--e->hp == 0){e->alive = 0; g->gone[g->room] |= (uint16_t)(1u << i); g->sound_event = SND_BREAK; say(g, D_CLEARED);}
   else g->sound_event = SND_HIT;
   continue;
  }
  g->sound_event = SND_HIT;
  e->state = AI_STAGGER; e->timer = 14; e->yaw = (int16_t)((g->pyaw + 512) & R3D_ANGLE_MASK);
  if(--e->hp == 0){e->state = AI_FALL; e->timer = 30; g->gone[g->room] |= (uint16_t)(1u << i); if(g->focus == i + 1) g->focus = 0;}
 }
}

static void spawn_life(AreliaGame *g, const AreliaEntity *from){
 AreliaEntity *e;
 if(g->ent_count >= WORLD_MAX_ENT) return;
 e = &g->ent[g->ent_count++];
 memset(e, 0, sizeof(*e));
 e->type = E_LIFE; e->alive = 1; e->x = from->x; e->z = from->z;
}

static void update_entities(AreliaGame *g){
 int i;
 for(i = 0; i < g->ent_count; i++){
  AreliaEntity *e = &g->ent[i];
  int32_t d; int dir;
  if(!e->alive) continue;
  if(e->flash) e->flash--;
  d = dist_to(g, e); dir = r3d_angle(g->px - e->x, g->pz - e->z);
  switch(e->type){
   case E_LOOTER:
    switch(e->state){
     case AI_IDLE: if(d < U(5.5)) e->state = AI_CHASE; break;
     case AI_CHASE:
      e->yaw = (int16_t)turn_toward(e->yaw, dir, 24);
      if(d > U(9)){e->state = AI_IDLE; break;}
      if(d < U(1.1)){e->state = AI_WINDUP; e->timer = 20; break;}
      ent_move(g, i, r3d_sin(e->yaw) * LOOTER_SPEED >> 14, 0);
      ent_move(g, i, 0, -r3d_cos(e->yaw) * LOOTER_SPEED >> 14);
      break;
     case AI_WINDUP: e->yaw = (int16_t)turn_toward(e->yaw, dir, 16); if(--e->timer <= 0){e->state = AI_SWING; e->timer = 8;} break;
     case AI_SWING:
      if(e->timer == 6 && d < U(1.45) && r3d_angle_diff(e->yaw, dir) < 110 && r3d_angle_diff(e->yaw, dir) > -110)
       hurt_player(g, e->yaw, 1);
      if(--e->timer <= 0){e->state = AI_RECOVER; e->timer = 26;}
      break;
     case AI_RECOVER: if(--e->timer <= 0) e->state = AI_CHASE; break;
     case AI_STAGGER:
      ent_move(g, i, r3d_sin(e->yaw + 512) * 6 >> 14, -r3d_cos(e->yaw + 512) * 6 >> 14);
      if(--e->timer <= 0){e->state = AI_CHASE; e->yaw = (int16_t)dir;}
      break;
     case AI_FALL: if(--e->timer <= 0){e->alive = 0; if((i + g->room) & 1) spawn_life(g, e);} break;
     default: break;
    }
    break;
   case E_CITIZEN:
    if(e->state == AI_LEAVE){if(--e->timer <= 0) e->alive = 0;}
    else e->yaw = (int16_t)turn_toward(e->yaw, dir, 8);
    break;
   case E_KEY:
    if(d < 110){e->alive = 0; g->keys++; g->gone[g->room] |= (uint16_t)(1u << i); g->sound_event = SND_KEY; say(g, D_KEY);}
    break;
   case E_LIFE:
    if(d < 110){e->alive = 0; g->hp = (uint8_t)(g->hp + 2 > ARELIA_HP_MAX ? ARELIA_HP_MAX : g->hp + 2); g->sound_event = SND_KEY;}
    break;
   case E_DOOR: case E_SEAL:
    if(e->state && --e->timer <= 0) e->alive = 0;
    break;
   default: break;
  }
 }
}

/* Lapilli rain on the Road to the Sea: a shadow grows, then a stone lands. */
static void update_lapilli(AreliaGame *g){
 int i;
 if(g->room != 3) {g->lapilli_n = 0; return;}
 if(g->room_ticks % 36 == 0 && g->lapilli_n < 4){
  uint32_t h = g->ticks * 2654435761u;
  g->lap_x[g->lapilli_n] = (int16_t)(g->px + (int32_t)((h >> 8) % 700) - 350);
  g->lap_z[g->lapilli_n] = (int16_t)(g->pz + (int32_t)((h >> 20) % 700) - 350);
  g->lap_t[g->lapilli_n] = 48;
  g->lapilli_n++;
 }
 for(i = 0; i < g->lapilli_n; i++){
  if(--g->lap_t[i] > 0) continue;
  if(r3d_dist(g->lap_x[i] - g->px, g->lap_z[i] - g->pz) < 120) hurt_player(g, r3d_angle(g->px - g->lap_x[i], g->pz - g->lap_z[i]), 1);
  else g->sound_event = SND_IMPACT;
  g->lap_x[i] = g->lap_x[g->lapilli_n - 1]; g->lap_z[i] = g->lap_z[g->lapilli_n - 1]; g->lap_t[i] = g->lap_t[g->lapilli_n - 1];
  g->lapilli_n--; i--;
 }
}

static void update_camera(AreliaGame *g, int moving){
 int32_t ax, az, ay, back, cx, cz; int k;
 if(g->mode == MODE_TITLE){
  g->cam_yaw = (int16_t)((g->ticks * 2) & R3D_ANGLE_MASK);
  g->cam_pitch = 40;
  g->cam_x = U(8) - (r3d_sin(g->cam_yaw) * U(6) >> 14);
  g->cam_z = U(10) + (r3d_cos(g->cam_yaw) * U(6) >> 14);
  g->cam_y = U(2.3);
  return;
 }
 if(g->mode == MODE_WON){
  /* From the pier, turn from the boats to the mountain. */
  int t = g->story > 300 ? 300 : g->story;
  g->cam_yaw = (int16_t)((0 + (r3d_angle_diff(0, VESUVIUS_YAW) * t) / 300) & R3D_ANGLE_MASK);
  g->cam_pitch = (int16_t)(20 - t / 10);
  g->cam_x = U(7.5); g->cam_z = U(3.2) + t; g->cam_y = U(1.6) + t;
  return;
 }
 if(g->focus){
  const AreliaEntity *e = &g->ent[g->focus - 1];
  g->cam_yaw = (int16_t)turn_toward(g->cam_yaw, r3d_angle(e->x - g->px, e->z - g->pz), 20);
 } else if(moving){
  g->cam_yaw = (int16_t)turn_toward(g->cam_yaw, g->pyaw, (r3d_angle_diff(g->cam_yaw, g->pyaw) > 300 || r3d_angle_diff(g->cam_yaw, g->pyaw) < -300) ? 0 : 7);
 }
 if(g->mode == MODE_DEAD) g->cam_yaw = (int16_t)((g->cam_yaw + 1) & R3D_ANGLE_MASK);
 ax = g->px; az = g->pz; ay = AIM_HEIGHT;
 /* Pull the camera in front of walls between it and Livia. */
 back = CAM_DIST;
 for(k = 1; k <= 8; k++){
  int32_t d = CAM_DIST * k / 8;
  cx = ax - (r3d_sin(g->cam_yaw) * d >> 14); cz = az + (r3d_cos(g->cam_yaw) * d >> 14);
  if(world_cell(g->room, (int)(cx >> 8), (int)(cz >> 8)) == T_WALL || cx < 0 || cz < 0){back = d - CAM_DIST / 8 - 40; break;}
 }
 if(back < 200) back = 200;
 /* Close to a wall the camera rises and looks down over Livia's shoulder. */
 g->cam_pitch = (int16_t)(CAM_PITCH + (CAM_DIST - back) / 12);
 g->cam_x = ax - (r3d_sin(g->cam_yaw) * back >> 14);
 g->cam_z = az + (r3d_cos(g->cam_yaw) * back >> 14);
 g->cam_y = ay + (r3d_sin(g->cam_pitch) * back / r3d_cos(g->cam_pitch));
}

static void tibia_step(AreliaGame *g, unsigned fresh){
 static const uint16_t note_buttons[5] = {ARELIA_INPUT_UP, ARELIA_INPUT_LEFT, ARELIA_INPUT_RIGHT, ARELIA_INPUT_DOWN, ARELIA_INPUT_A};
 int n, i, match;
 if(fresh & ARELIA_INPUT_B){g->action = ACT_NONE; g->note_count = 0; return;}
 for(n = 0; n < 5; n++){
  if(!(fresh & note_buttons[n])) continue;
  memmove(g->notes, g->notes + 1, 7); g->notes[7] = (uint8_t)n;
  if(g->note_count < 8) g->note_count++;
  g->sound_event = (uint8_t)(SND_NOTE0 + n);
  match = g->note_count >= ARELIA_SONG_LEN;
  for(i = 0; match && i < ARELIA_SONG_LEN; i++) if(g->notes[8 - ARELIA_SONG_LEN + i] != song[i]) match = 0;
  if(!match) continue;
  g->note_count = 0; g->action = ACT_NONE; g->sound_event = SND_SONG;
  for(i = 0; i < g->ent_count; i++){
   AreliaEntity *e = &g->ent[i];
   if(!e->alive) continue;
   if(e->type == E_ALTAR && !g->song_known && dist_to(g, e) < U(2)){g->song_known = 1; say(g, D_LEARNED); return;}
   if(e->type == E_SEAL && g->song_known && e->state == 0 && dist_to(g, e) < U(3.2)){
    e->state = 1; e->timer = 60; g->gone[g->room] |= (uint16_t)(1u << i); say(g, D_SEAL_OPEN); return;}
  }
  if(g->song_known) say(g, D_ECHO);
  return;
 }
}

void game_step(AreliaGame *g, unsigned buttons){
 unsigned fresh = buttons & ~(unsigned)g->prev_buttons;
 int ix, iz, moving = 0, kind;
 g->prev_buttons = (uint16_t)buttons; g->buttons = (uint16_t)buttons;
 g->ticks++;
 if(g->mode == MODE_TITLE){
  if(fresh & ARELIA_INPUT_START){g->mode = MODE_PLAY; load_room(g, 0, 0); say(g, D_INTRO);}
  update_camera(g, 0);
  return;
 }
 if(g->mode == MODE_DEAD){
  if(g->story < 600) g->story++;
  if((fresh & ARELIA_INPUT_START) && g->story > 30){
   g->mode = MODE_PLAY; g->hp = ARELIA_HP_MAX; g->invuln = 60; load_room(g, g->room, 0);
  }
  update_camera(g, 0);
  return;
 }
 if(g->mode == MODE_WON){
  if(g->story < 600) g->story++;
  if((fresh & ARELIA_INPUT_START) && g->story >= 360) game_init(g);
  else update_camera(g, 0);
  return;
 }
 g->room_ticks++;
 if(g->banner_t) g->banner_t--;
 if(g->dialogue){
  if(g->dialogue_t < 255) g->dialogue_t++;
  if(((fresh & (ARELIA_INPUT_A | ARELIA_INPUT_B)) && g->dialogue_t * 2u >= text_len(g->dialogue)) || g->dialogue_t >= 220){
   g->dialogue = 0;
   if(g->tibia_purpose){g->tibia_purpose = 0; g->action = ACT_TIBIA; g->note_count = 0;}
  }
  update_camera(g, 0);
  return;
 }
 if(g->action == ACT_TIBIA){tibia_step(g, fresh); update_camera(g, 0); return;}
 if(g->invuln) g->invuln--;
 if(g->action_t && --g->action_t == 0) g->action = ACT_NONE;
 if(g->focus){
  const AreliaEntity *e = &g->ent[g->focus - 1];
  if(!e->alive || e->state == AI_FALL || e->state == AI_LEAVE || dist_to(g, e) > U(8)) g->focus = 0;
 }
 if(fresh & ARELIA_INPUT_FOCUS){
  if(g->focus) g->focus = 0;
  else {int t = nearest_target(g); if(t >= 0){g->focus = (uint8_t)(t + 1); g->sound_event = SND_FOCUS;} else g->cam_yaw = g->pyaw;}
 }
 if((fresh & ARELIA_INPUT_ROLL) && (g->action == ACT_NONE || g->action == ACT_STRIKE)){
  int d = (buttons & 15u) ? 0 : -1;
  g->action = ACT_ROLL; g->action_t = 14; if(g->invuln < 12) g->invuln = 12; g->sound_event = SND_ROLL;
  if(d == 0){ix = !!(buttons & ARELIA_INPUT_RIGHT) - !!(buttons & ARELIA_INPUT_LEFT); iz = !!(buttons & ARELIA_INPUT_UP) - !!(buttons & ARELIA_INPUT_DOWN);
   g->pyaw = (int16_t)(((g->focus ? r3d_angle(g->ent[g->focus - 1].x - g->px, g->ent[g->focus - 1].z - g->pz) : g->cam_yaw) + r3d_angle(ix, -iz)) & R3D_ANGLE_MASK);}
 }
 if((fresh & ARELIA_INPUT_A) && g->action == ACT_NONE){g->action = ACT_STRIKE; g->action_t = 14; g->sound_event = SND_SWING;}
 if(g->action == ACT_STRIKE && g->action_t == 10) strike_hits(g);
 if((fresh & ARELIA_INPUT_B) && g->action == ACT_NONE) context_action(g);
 if((fresh & ARELIA_INPUT_TIBIA) && g->action == ACT_NONE && !g->dialogue){g->action = ACT_TIBIA; g->note_count = 0; g->tibia_purpose = 0; update_camera(g, 0); return;}
 if(g->dialogue){update_camera(g, 0); return;}

 /* Movement: relative to the camera, or around the focused target. */
 ix = !!(buttons & ARELIA_INPUT_RIGHT) - !!(buttons & ARELIA_INPUT_LEFT);
 iz = !!(buttons & ARELIA_INPUT_UP) - !!(buttons & ARELIA_INPUT_DOWN);
 if(g->action == ACT_ROLL){
  player_move(g, r3d_sin(g->pyaw) * ROLL_SPEED >> 14, -r3d_cos(g->pyaw) * ROLL_SPEED >> 14);
  moving = 1; g->walk = (int16_t)(g->walk + ROLL_SPEED);
 } else if(g->action == ACT_NONE && (ix || iz)){
  int base, dir, speed = g->focus ? FOCUS_SPEED : WALK_SPEED;
  if(g->focus){const AreliaEntity *e = &g->ent[g->focus - 1]; base = r3d_angle(e->x - g->px, e->z - g->pz); g->pyaw = (int16_t)base;}
  else base = g->cam_yaw;
  dir = (base + r3d_angle(ix, -iz)) & R3D_ANGLE_MASK;
  if(!g->focus) g->pyaw = (int16_t)turn_toward(g->pyaw, dir, 110);
  player_move(g, r3d_sin(dir) * speed >> 14, -r3d_cos(dir) * speed >> 14);
  g->walk = (int16_t)(g->walk + speed);
  moving = 1;
 } else if(g->focus){
  const AreliaEntity *e = &g->ent[g->focus - 1];
  g->pyaw = (int16_t)r3d_angle(e->x - g->px, e->z - g->pz);
 }

 /* District exits. */
 if(world_cell(g->room, g->px >> 8, g->pz >> 8) == T_DOOR && g->pz < 70 && g->room + 1 < ARELIA_ROOMS){
  load_room(g, g->room + 1u, 0); update_camera(g, 0); return;
 }
 if(world_cell(g->room, g->px >> 8, g->pz >> 8) == T_SOUTH && g->pz > U(19.75) && g->room > 0){
  load_room(g, g->room - 1u, 1); update_camera(g, 0); return;
 }
 if(g->room == 3 && g->pz < U(2.2)){
  if(g->rescued == (1u << ARELIA_ROOMS) - 1u){g->mode = MODE_WON; g->story = 0; g->sound_event = SND_VICTORY; g->focus = 0; g->palette_phase = 4; g->action = ACT_NONE;}
  else {g->pz = (int16_t)U(2.6); say(g, D_NOT_ALL);}
 }
 update_entities(g);
 update_lapilli(g);
 context_target(g, &kind); g->prompt = (uint8_t)kind;
 update_camera(g, moving);
}

static unsigned append(char *out, unsigned n, unsigned cap, const char *s){while(*s && n + 1 < cap) out[n++] = *s++; return n;}
void game_status_text(const AreliaGame *g, char *out, unsigned cap){
 unsigned n = 0, saved = 0, i; char count[4];
 if(!out || !cap) return;
 for(i = 0; i < ARELIA_ROOMS; i++) if(g->rescued & (1u << i)) saved++;
 count[0] = (char)('0' + saved); count[1] = '/'; count[2] = (char)('0' + ARELIA_ROOMS); count[3] = 0;
 if(g->mode == MODE_TITLE) n = append(out, n, cap, "POMPEII AD 79");
 else if(g->mode == MODE_WON){n = append(out, n, cap, "SURVIVORS SAVED "); n = append(out, n, cap, count);}
 else if(g->mode == MODE_DEAD) n = append(out, n, cap, "ASH AND SILENCE");
 else {n = append(out, n, cap, world_room_names[g->room < ARELIA_ROOMS ? g->room : 0]); n = append(out, n, cap, " SAVED "); n = append(out, n, cap, count);
  if(g->song_known) n = append(out, n, cap, " SONG");}
 out[n] = 0;
}

/* Test and tool hook: start playing in a district at a position (Q8). */
void game_debug_enter(AreliaGame *g, unsigned room, int x, int z, int yaw){
 if(room >= ARELIA_ROOMS) room = 0;
 g->mode = MODE_PLAY; g->dialogue = 0;
 load_room(g, room, 0);
 g->px = (int16_t)x; g->pz = (int16_t)z; g->pyaw = (int16_t)(yaw & R3D_ANGLE_MASK); g->cam_yaw = g->pyaw;
 g->banner_t = 0;
 update_camera(g, 0);
}
