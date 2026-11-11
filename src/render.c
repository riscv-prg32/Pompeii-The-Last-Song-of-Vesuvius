/* Scene renderer: sky and Vesuvius, perspective floor, extruded architecture,
 * low-poly characters, HUD. Original art; all geometry is data in this file. */
#include "game.h"
#include "r3d.h"

/* Palette layout: material * 16 + light * 4 + fog. Material 0 is the sky
 * gradient (16 shades); material 15 holds fixed interface colours. */
enum { M_SKY, M_TRAV, M_RED, M_OCHRE, M_TERRA, M_BASALT, M_MARBLE, M_WATER, M_GREEN, M_SKIN,
       M_TEAL, M_HAIR, M_LOOTER, M_BRONZE, M_ASH, M_UI, M_CLOTH = 99 };
enum { UI_BLACK = 240, UI_WHITE, UI_GOLD, UI_RED, UI_PANEL, UI_BORDER, UI_TEXT, UI_GREEN, UI_BLUE,
       UI_ORANGE, UI_LAVA, UI_GREY, UI_SHADOW, UI_AMPHORA, UI_EMPTY, UI_CYAN };

#define WALL_H 410
#define VESUVIUS_YAW 860
#define C(m, l, f) ((uint8_t)((m) * 16 + (l) * 4 + (f)))

static const uint32_t base_rgb[15] = {0, 0xe2d2ae, 0xa8362c, 0xd8a242, 0xb5653d, 0x5d5853, 0xf1ece0, 0x3f86a2,
                                      0x638f3e, 0xdca37c, 0x2f938a, 0x4b301f, 0x74405e, 0xcf952f, 0x8c8781};
static const uint32_t ui_rgb[16] = {0x100e12, 0xfaf6eb, 0xf0c446, 0xd23a2a, 0x1e1a22, 0x9a7a4c, 0xe6dcc8, 0x6ec85a,
                                    0x5a96e6, 0xff8c28, 0xff5a1e, 0x78736e, 0x1a1614, 0xbe643c, 0x503c32, 0x78dcdc};
static const uint32_t fog_rgb[5] = {0xd6c4a0, 0xc8aa8c, 0xaa8c78, 0x7a645f, 0x5a3c37};
static const uint32_t zenith_rgb[5] = {0x6e96be, 0x7a8cae, 0x6a6478, 0x3c2d32, 0x28141a};
static const uint32_t horizon_rgb[5] = {0xe6d7b4, 0xe0c39a, 0xc89c78, 0xaa6e50, 0xc8502a};

static uint32_t mix(uint32_t a, uint32_t b, int t){ /* t in 0..256 */
 uint32_t r = 0; int s;
 for(s = 0; s < 24; s += 8){int ca = (int)((a >> s) & 255), cb = (int)((b >> s) & 255); r |= (uint32_t)(ca + ((cb - ca) * t >> 8)) << s;}
 return r;
}

void game_palette(const AreliaGame *g, uint32_t rgb[256]){
 static const int lightf[4] = {150, 190, 225, 256}, fogf[4] = {0, 80, 150, 210};
 int m, l, f, i, ph = g->palette_phase > 4 ? 4 : g->palette_phase;
 for(i = 0; i < 16; i++) rgb[i] = mix(zenith_rgb[ph], horizon_rgb[ph], i * 256 / 15);
 for(m = 1; m < 15; m++) for(l = 0; l < 4; l++) for(f = 0; f < 4; f++)
  rgb[m * 16 + l * 4 + f] = mix(mix(0, base_rgb[m], lightf[l]), fog_rgb[ph], fogf[f]);
 for(i = 0; i < 16; i++) rgb[240 + i] = ui_rgb[i];
}

/* ------------------------------------------------------------ helpers */
static R3Camera cam;
static int fog_of(int32_t z){return z < 900 ? 0 : z < 1500 ? 1 : z < 2300 ? 2 : 3;}

static void quad(uint8_t *fb, const int32_t *w, uint8_t color){
 R3Vec p[4]; int i;
 for(i = 0; i < 4; i++) p[i] = r3d_to_camera(&cam, w[i * 3], w[i * 3 + 1], w[i * 3 + 2]);
 r3d_poly(fb, p, 4, color);
}

/* Axis-aligned box; materials for top and sides, optional lower band. */
static void aabb(uint8_t *fb, int32_t x0, int32_t y0, int32_t z0, int32_t x1, int32_t y1, int32_t z1,
                 int top, int side, int band, int32_t band_h, unsigned skip){
 int fog = fog_of(r3d_to_camera(&cam, (x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2).z);
 int32_t v[12]; int k;
 if(cam.y > y1 && top >= 0){int32_t t[12] = {x0, y1, z0, x1, y1, z0, x1, y1, z1, x0, y1, z1}; quad(fb, t, C(top, 3, fog));}
 for(k = 0; k < 4; k++){
  int light; int32_t ya = y0, yb = y1;
  if(skip & (1u << k)) continue;
  switch(k){
   case 0: if(cam.z >= z0) continue; light = 0; v[0]=x1;v[2]=z0; v[3]=x0;v[5]=z0; break;           /* north */
   case 1: if(cam.z <= z1) continue; light = 3; v[0]=x0;v[2]=z1; v[3]=x1;v[5]=z1; break;           /* south */
   case 2: if(cam.x <= x1) continue; light = 1; v[0]=x1;v[2]=z1; v[3]=x1;v[5]=z0; break;           /* east */
   default: if(cam.x >= x0) continue; light = 2; v[0]=x0;v[2]=z0; v[3]=x0;v[5]=z1; break;          /* west */
  }
  if(band >= 0 && band_h > y0 && band_h < y1){
   int32_t q[12] = {v[0], y0, v[2], v[3], y0, v[5], v[3], band_h, v[5], v[0], band_h, v[2]};
   quad(fb, q, C(band, light, fog)); ya = band_h;
  }
  {int32_t q[12] = {v[0], ya, v[2], v[3], ya, v[5], v[3], yb, v[5], v[0], yb, v[2]}; quad(fb, q, C(side, light, fog));}
 }
}

/* Rotated model part, Q7 model units (128 = one cell). */
typedef struct { uint8_t mat; int8_t cx, cy, cz; uint8_t hx, hy, hz, anim; } Part;
enum { A_NONE, A_LLEG, A_RLEG, A_LARM, A_RARM, A_WEAPON, A_HEAD };

static void part_box(uint8_t *fb, int32_t ox, int32_t oy, int32_t oz, int yaw, int32_t cx, int32_t cy, int32_t cz,
                     int32_t hx, int32_t hy, int32_t hz, uint8_t mat, int fog, int flash){
 /* Corners in world space: model axes right = (cos, sin), forward(-z local) = (sin, -cos). */
 int32_t s = r3d_sin(yaw), c = r3d_cos(yaw);
 static const int8_t fx[6] = {0, 0, 1, -1, 0, 0}, fy[6] = {1, -1, 0, 0, 0, 0}, fz[6] = {0, 0, 0, 0, 1, -1};
 int f;
 for(f = 0; f < 6; f++){
  int32_t nlx = fx[f], nly = fy[f], nlz = fz[f], nwx, nwz, fcx, fcy, fcz, wx, wz, dot; int light, i;
  int32_t ux, uy, uz, vx, vy, vz; R3Vec p[4];
  nwx = nlx * c - nlz * s; nwz = nlx * s + nlz * c;      /* Q14 world normal (horizontal part) */
  fcx = cx + nlx * hx; fcy = cy + nly * hy; fcz = cz + nlz * hz;
  wx = ox + ((fcx * c - fcz * s) >> 14) * 2; wz = oz + ((fcx * s + fcz * c) >> 14) * 2;
  dot = ((cam.x - wx) >> 2) * (nwx >> 4) + ((cam.y - (oy + fcy * 2)) >> 2) * (nly << 10) + ((cam.z - wz) >> 2) * (nwz >> 4);
  if(dot <= 0) continue;
  if(nly > 0) light = 3; else if(nly < 0) light = 0;
  else {int32_t sun = (-nwx * 8 + nwz * 14) >> 4; light = sun > 9000 ? 3 : sun > 0 ? 2 : sun > -9000 ? 1 : 0;}
  /* Face spanned by two tangent axes. */
  if(nly){ux = hx; uy = 0; uz = 0; vx = 0; vy = 0; vz = hz;}
  else if(nlx){ux = 0; uy = 0; uz = hz; vx = 0; vy = hy; vz = 0;}
  else {ux = hx; uy = 0; uz = 0; vx = 0; vy = hy; vz = 0;}
  for(i = 0; i < 4; i++){
   int su = (i == 0 || i == 3) ? -1 : 1, sv = (i < 2) ? -1 : 1;
   int32_t lx = fcx + su * ux + sv * vx, ly = fcy + su * uy + sv * vy, lz = fcz + su * uz + sv * vz;
   p[i] = r3d_to_camera(&cam, ox + ((lx * c - lz * s) >> 14) * 2, oy + ly * 2, oz + ((lx * s + lz * c) >> 14) * 2);
  }
  r3d_poly(fb, p, 4, flash ? UI_WHITE : (mat == M_UI ? UI_ORANGE : C(mat, light, fog)));
 }
}

static const Part livia_parts[] = {
 {M_SKIN, -9, 24, 0, 5, 24, 6, A_LLEG}, {M_SKIN, 9, 24, 0, 5, 24, 6, A_RLEG},
 {M_CLOTH, 0, 52, 0, 17, 14, 12, A_NONE}, {M_CLOTH, 0, 75, 0, 14, 11, 9, A_NONE},
 {M_BRONZE, 0, 64, 0, 15, 2, 10, A_NONE},
 {M_SKIN, -19, 70, 0, 4, 15, 4, A_LARM}, {M_SKIN, 19, 70, 0, 4, 15, 4, A_RARM},
 {M_SKIN, 0, 97, 0, 10, 11, 10, A_HEAD}, {M_HAIR, 0, 104, 3, 11, 8, 10, A_HEAD}, {M_HAIR, 0, 99, 13, 6, 6, 4, A_HEAD},
 {M_HAIR, 24, 58, -4, 2, 34, 2, A_WEAPON}
};
static const Part looter_parts[] = {
 {M_HAIR, -11, 26, 0, 6, 26, 7, A_LLEG}, {M_HAIR, 11, 26, 0, 6, 26, 7, A_RLEG},
 {M_LOOTER, 0, 64, 0, 20, 22, 13, A_NONE},
 {M_SKIN, -24, 74, 0, 5, 17, 5, A_LARM}, {M_SKIN, 24, 74, 0, 5, 17, 5, A_RARM},
 {M_SKIN, 0, 104, 0, 11, 12, 11, A_HEAD}, {M_LOOTER, 0, 112, 2, 13, 7, 13, A_HEAD},
 {M_ASH, 30, 62, -6, 4, 26, 4, A_WEAPON}
};
static const Part rubble_parts[] = {
 {M_ASH, -24, 14, -10, 24, 14, 22, 0}, {M_TRAV, 20, 11, 10, 18, 11, 18, 0}, {M_ASH, 2, 36, 2, 16, 10, 16, 0},
 {M_TRAV, -12, 8, 26, 14, 8, 10, 0}, {M_RED, 26, 30, -14, 10, 8, 8, 0}
};
static const Part altar_parts[] = {
 {M_MARBLE, 0, 8, 0, 42, 8, 42, 0}, {M_MARBLE, 0, 40, 0, 22, 24, 22, 0}, {M_MARBLE, 0, 67, 0, 31, 4, 31, 0},
 {M_BRONZE, 0, 76, 0, 16, 5, 16, 0}, {M_UI, 0, 88, 0, 6, 8, 6, 0}
};
static const Part seal_parts[] = {
 {M_TRAV, 0, 76, 0, 64, 76, 22, 0}, {M_BRONZE, 0, 92, 23, 22, 22, 2, 0}, {M_GREEN, 0, 92, 26, 8, 8, 2, 0}
};
static const Part door_parts[] = {
 {M_TERRA, 0, 80, 0, 62, 80, 8, 0}, {M_BRONZE, -30, 110, 9, 5, 5, 2, 0}, {M_BRONZE, 30, 110, 9, 5, 5, 2, 0},
 {M_BRONZE, -30, 50, 9, 5, 5, 2, 0}, {M_BRONZE, 30, 50, 9, 5, 5, 2, 0}, {M_BRONZE, 0, 80, 9, 6, 9, 2, 0}
};
static const Part boat_parts[] = {
 {M_HAIR, 0, 14, 0, 40, 14, 110, 0}, {M_TERRA, 0, 30, 0, 44, 4, 114, 0}, {M_HAIR, 0, 112, 0, 4, 82, 4, 0},
 {M_MARBLE, 0, 120, 4, 54, 46, 2, 0}
};
static const Part key_parts[] = {{M_BRONZE, 0, 0, 0, 3, 12, 3, 0}, {M_BRONZE, 0, 14, 0, 8, 6, 3, 0}, {M_BRONZE, 5, -10, 0, 5, 3, 3, 0}};
static const Part life_parts[] = {{M_TERRA, 0, 18, 0, 11, 15, 11, 0}, {M_TERRA, 0, 38, 0, 5, 6, 5, 0}, {M_TERRA, 8, 32, 0, 3, 6, 3, 0}};

static void draw_model(uint8_t *fb, const Part *parts, int n, int32_t ox, int32_t oy, int32_t oz, int yaw,
                       uint8_t cloth, int walk, int pose, int flash){
 /* pose: 0 idle/walk, 1 strike, 2 roll, 3 windup, 4 swing, 5 cheer, 6 play tibia, 7 fallen */
 int fog = fog_of(r3d_to_camera(&cam, ox, oy, oz).z), i;
 int32_t swing = r3d_sin(walk * 3) * 9 >> 14;
 int32_t squash = pose == 2 ? 2 : 1;
 for(i = 0; i < n; i++){
  const Part *p = &parts[i];
  int32_t cx = p->cx, cy = p->cy / squash, cz = p->cz, hx = p->hx, hy = p->hy / squash, hz = p->hz;
  uint8_t mat = p->mat == M_CLOTH ? cloth : p->mat;
  switch(p->anim){
   case A_LLEG: cz += swing; break;
   case A_RLEG: cz -= swing; break;
   case A_LARM: cz -= swing; if(pose == 5){cy += 40; cz = 0;} if(pose == 6){cy += 4; cz = -14; cx = -8;} break;
   case A_RARM: cz += swing; if(pose == 5){cy += 40; cz = 0;} if(pose == 6){cy += 4; cz = -14; cx = 8;}
    if(pose == 1 || pose == 4){cz = -16; cy += 6;} if(pose == 3){cy += 30;} break;
   case A_WEAPON:
    if(pose == 6){cx = 0; cy = 88; cz = -26; hx = 2; hy = 2; hz = 14; mat = M_BRONZE;}   /* the tibia, held to the lips */
    else if(pose == 1 || pose == 4){cy = 70; cz = -40; hy = 3; hz = 34;}                    /* thrust */
    else if(pose == 3){cy = 120; cz = 6;}                                                  /* raised */
    else if(pose == 2 || pose == 7) continue;
    break;
   default: break;
  }
  if(pose == 7){int32_t t = cy; cy = hz + 2; cz = cz + t - 40; t = hy; hy = hz; hz = t;}    /* lying down */
  part_box(fb, ox, oy, oz, yaw, cx, cy, cz, hx, hy, hz, mat, fog, flash);
 }
}

static void blob_shadow(uint8_t *fb, int32_t x, int32_t z, int32_t r){
 static const int8_t ox[8] = {0, 7, 10, 7, 0, -7, -10, -7}, oz[8] = {-10, -7, 0, 7, 10, 7, 0, -7};
 R3Vec p[8]; int i;
 for(i = 0; i < 8; i++) p[i] = r3d_to_camera(&cam, x + ox[i] * r / 10, 3, z + oz[i] * r / 10);
 r3d_poly(fb, p, 8, UI_SHADOW);
}

/* ------------------------------------------------------------ sky and floor */
static void sky(uint8_t *fb, const AreliaGame *g){
 int y, x, h = cam.horizon, diff, sx, phase = g->palette_phase;
 int top = h < R3D_H ? h : R3D_H;
 for(y = 0; y < top; y++){
  int s = 15 - (h - y) * 15 / 92; if(s < 0) s = 0;
  r3d_rect(fb, 0, y, R3D_W, 1, (uint8_t)s);
 }
 diff = r3d_angle_diff(cam.yaw, VESUVIUS_YAW);
 if(diff > -240 && diff < 240 && h > 0){
  int32_t cdiff = r3d_cos(diff); int plume;
  sx = R3D_W / 2 + (int)(R3D_FOCAL * r3d_sin(diff) / (cdiff ? cdiff : 1));
  for(x = sx - 144; x <= sx + 144; x++){
   int dx = x < sx ? sx - x : x - sx, ht = 44 - dx * 44 / 144;
   if(dx < 18) ht = 36 + (dx > 9 ? 4 : 0);                          /* the summit's broken crater */
   if(ht > 0) r3d_rect(fb, x, h - ht, 1, ht, (dx & 15) == 3 && dx > 20 ? C(M_ASH, 0, 2) : C(M_ASH, 1, 2));
  }
  /* The plume rises from the crater and spreads like a pine tree. */
  plume = 20 + phase * 28 + (g->mode == MODE_WON ? g->story / 3 : 0); if(plume > 180) plume = 180;
  for(y = 0; y < plume; y += 3){
   int w = 6 + y / 6 + (y > plume - 48 ? (y - (plume - 48)) : 0), wob = (int)(r3d_sin(g->ticks * 6 + y * 20) * 4 >> 14);
   r3d_rect(fb, sx - w + wob, h - 36 - y, w * 2, 3, phase >= 3 ? C(M_ASH, (y / 6 + (int)(g->ticks / 8)) % 7 == 0 ? 2 : 1, 1) : C(M_ASH, 2, 2));
  }
  if(phase >= 3) for(y = 0; y < 8; y++) r3d_rect(fb, sx - 6 + (int)((g->ticks * 7 + y * 13) % 13), h - 36 + y * 4 - (int)(g->ticks % 17), 3, 3, UI_LAVA);
  if(phase >= 4 && (g->ticks % 47) < 3) r3d_rect(fb, sx + 12, h - 100, 2, 48, UI_WHITE);   /* lightning in the ash */
 }
}

static uint8_t floor_pixel(const AreliaGame *g, int32_t wx, int32_t wz, int fog){
 int cx = (int)(wx >> 8), cz = (int)(wz >> 8), fx = (int)(wx & 255), fz = (int)(wz & 255);
 char t;
 uint32_t hsh;
 if(cx < 0 || cz < 0 || cx >= WORLD_W || cz >= WORLD_H) t = (g->room == 3 && cz < 3) ? T_WATER : T_WALL;
 else t = world_maps[g->room][cz][cx];
 hsh = (uint32_t)((wx >> 7) * 73856093) ^ (uint32_t)((wz >> 7) * 19349663);
 hsh ^= hsh >> 13; hsh *= 2654435761u;
 switch(t){
  case T_WATER:
   return C(M_WATER, (((fz + (fx >> 1) + (int)(g->ticks * 3)) & 63) < 6) ? 3 : 2, fog);
  case T_BRIDGE: return C(M_TERRA, (fx & 63) < 6 ? 0 : 2, fog);
  case T_GRASS: return C(M_GREEN, 1 + (int)((hsh >> 7) % 3), fog);
  case T_MOSAIC:
   if(fx < 16 || fz < 16 || fx > 239 || fz > 239) return C(M_RED, 2, fog);
   return ((fx >> 5) ^ (fz >> 5)) & 1 ? C(M_MARBLE, 3, fog) : C(M_BASALT, 1, fog);
  default:
   if(fog < 2 && ((fx & 127) < 8 || (fz & 127) < 8)) return C(M_BASALT, 0, fog);
   switch(g->room){
    case 0: return C(M_TRAV, 2 + (int)((hsh >> 9) & 1), fog);
    case 1: return C(M_RED, 1 + (int)((hsh >> 9) & 1), fog);
    case 2: return C(M_BASALT, 2 + (int)((hsh >> 9) & 1), fog);
    default: return C(M_BASALT, 1 + (int)((hsh >> 9) & 1) * 2, fog);
   }
 }
}

static void ground(uint8_t *fb, const AreliaGame *g){
 int sy, sx;
 for(sy = cam.horizon < 0 ? 0 : cam.horizon; sy < R3D_H; sy++){
  int32_t a = R3D_H * 8 - (sy * 16 + 8), ty = a * 1024 / R3D_FOCAL, y0, z0, fwd, right0, step, wx, wz, dwx, dwz;
  uint8_t *row = fb + sy * R3D_W; int fog;
  y0 = ((ty * cam.cp) >> 14) - cam.sp;
  z0 = cam.cp + ((ty * cam.sp) >> 14);
  if(y0 >= -40){r3d_rect(fb, 0, sy, R3D_W, 1, 15); continue;}
  fwd = cam.y * z0 / -y0;
  if(fwd > 3300){r3d_rect(fb, 0, sy, R3D_W, 1, C(M_BASALT, 2, 3)); continue;}
  fog = fog_of(fwd);
  step = (cam.y << 8) * (16384 / R3D_FOCAL) / -y0;                  /* Q16 world per pixel */
  right0 = (cam.y << 8) * ((8 - R3D_W * 8) * 1024 / R3D_FOCAL) / -y0;
  wx = (cam.x << 8) + ((right0 >> 8) * cam.cy >> 14 << 8) + ((fwd * cam.sy >> 14) << 8);
  wz = (cam.z << 8) + ((right0 >> 8) * cam.sy >> 14 << 8) - ((fwd * cam.cy >> 14) << 8);
  dwx = (step * (cam.cy >> 2)) >> 12; dwz = (step * (cam.sy >> 2)) >> 12;
  for(sx = 0; sx < R3D_W; sx++){row[sx] = floor_pixel(g, wx >> 8, wz >> 8, fog); wx += dwx; wz += dwz;}
 }
}

/* ------------------------------------------------------------ scene items */
typedef struct { int32_t key; uint8_t kind, a, b, pad; } Item;
enum { I_WALL, I_COLUMN, I_LOW, I_ENT, I_PLAYER, I_LAP, I_ARCH };
#define MAX_ITEMS 220
static Item items[MAX_ITEMS];

static void wall_cell(uint8_t *fb, const AreliaGame *g, int x, int z){
 static const int8_t nx[4] = {0, 0, 1, -1}, nz[4] = {-1, 1, 0, 0};
 unsigned skip = 0; int k, side, band, top;
 for(k = 0; k < 4; k++) if(world_cell(g->room, x + nx[k], z + nz[k]) == T_WALL) skip |= 1u << k;
 switch(g->room){
  case 0: side = M_TRAV; band = M_BASALT; top = M_TERRA; break;
  case 1: side = M_OCHRE; band = M_RED; top = M_TERRA; break;
  case 2: side = M_TRAV; band = M_BASALT; top = M_GREEN; break;
  default: side = M_MARBLE; band = M_ASH; top = M_ASH; break;
 }
 aabb(fb, x * 256, 0, z * 256, x * 256 + 256, WALL_H, z * 256 + 256, top, side, band, 110, skip);
}

static void column_cell(uint8_t *fb, const AreliaGame *g, int x, int z){
 int32_t cx = x * 256 + 128, cz = z * 256 + 128;
 int mat = g->room == 1 ? M_RED : M_MARBLE;
 aabb(fb, cx - 60, 0, cz - 60, cx + 60, 30, cz + 60, M_TRAV, M_TRAV, -1, 0, 0);
 aabb(fb, cx - 42, 30, cz - 42, cx + 42, 360, cz + 42, mat, mat, -1, 0, 0);
 aabb(fb, cx - 62, 360, cz - 62, cx + 62, 395, cz + 62, M_TRAV, M_TRAV, -1, 0, 0);
}

static void low_cell(uint8_t *fb, const AreliaGame *g, int x, int z){
 if(g->room == 3){aabb(fb, x * 256 + 8, 0, z * 256 + 8, x * 256 + 248, 250, z * 256 + 248, M_TERRA, M_MARBLE, M_TRAV, 40, 0); return;}
 aabb(fb, x * 256, 0, z * 256, x * 256 + 256, 150, z * 256 + 256, M_MARBLE, M_MARBLE, M_TRAV, 30, 0);
 /* The Forum's bronze statue stands on the corner shared by its 2x2 base. */
 if(g->room == 0 && world_cell(0, x + 1, z + 1) == T_LOW && world_cell(0, x - 1, z) != T_LOW && world_cell(0, x, z - 1) != T_LOW && x == 7)
  draw_model(fb, livia_parts, 10, (x + 1) * 256, 150, (z + 1) * 256, 384, M_BRONZE, 0, 5, 0);
}

static void arch_cell(uint8_t *fb, int x, int z){
 aabb(fb, x * 256, 330, z * 256, x * 256 + 256, WALL_H + 50, z * 256 + 256, M_TERRA, M_TRAV, -1, 0, 0);
}

static void draw_entity(uint8_t *fb, const AreliaGame *g, int i){
 const AreliaEntity *e = &g->ent[i];
 static const uint8_t cloth[4] = {M_MARBLE, M_RED, M_OCHRE, M_GREEN};
 int pose = 0;
 switch(e->type){
  case E_LOOTER:
   blob_shadow(fb, e->x, e->z, 80);
   pose = e->state == AI_WINDUP ? 3 : e->state == AI_SWING ? 4 : e->state == AI_FALL ? 7 : 0;
   draw_model(fb, looter_parts, 8, e->x, 0, e->z, e->yaw, M_LOOTER, e->state == AI_CHASE ? (int)g->ticks * 12 : 0, pose,
              e->flash & 2 || (e->state == AI_WINDUP && (e->timer & 4)));
   break;
  case E_CITIZEN:
   blob_shadow(fb, e->x, e->z, 70);
   draw_model(fb, livia_parts, 10, e->x, 0, e->z, e->yaw, cloth[g->room & 3], 0, e->state == AI_LEAVE || (g->ticks / 20 + i) % 4 == 0 ? 5 : 0, 0);
   break;
  case E_RUBBLE: draw_model(fb, rubble_parts, 5, e->x, 0, e->z, 0, 0, 0, 0, e->flash & 2); break;
  case E_ALTAR: draw_model(fb, altar_parts, (g->ticks & 4) ? 5 : 4, e->x, 0, e->z, 0, 0, 0, 0, 0); break;
  case E_SEAL: draw_model(fb, seal_parts, 3, e->x, e->state ? -(60 - e->timer) * 5 : 0, e->z, 0, 0, 0, 0, 0); break;
  case E_DOOR: draw_model(fb, door_parts, 6, e->x, e->state ? (30 - e->timer) * 9 : 0, e->z, 0, 0, 0, 0, 0); break;
  case E_BOAT: draw_model(fb, boat_parts, 4, e->x, (r3d_sin(g->ticks * 8) * 10 >> 14) - 20,
                          e->z - (g->mode == MODE_WON ? g->story * 3 : 0), 0, 0, 0, 0, 0); break;
  case E_KEY:
   blob_shadow(fb, e->x, e->z, 30);
   draw_model(fb, key_parts, 3, e->x, 70 + (r3d_sin(g->ticks * 12) * 16 >> 14), e->z, (int)(g->ticks * 14), 0, 0, 0, 0); break;
  case E_LIFE: blob_shadow(fb, e->x, e->z, 30); draw_model(fb, life_parts, 3, e->x, 4, e->z, (int)(g->ticks * 10), 0, 0, 0, 0); break;
  default: break;
 }
}

static void draw_player(uint8_t *fb, const AreliaGame *g){
 int pose = g->action == ACT_STRIKE ? (g->action_t > 4 ? 1 : 0) : g->action == ACT_ROLL ? 2 : g->action == ACT_TIBIA ? 6 : 0;
 if(g->mode == MODE_DEAD) pose = 7;
 if(g->invuln && g->action != ACT_ROLL && (g->invuln & 2) && g->mode == MODE_PLAY) return;
 blob_shadow(fb, g->px, g->pz, 70);
 draw_model(fb, livia_parts, 11, g->px, 0, g->pz, g->pyaw, M_TEAL, g->walk, pose, 0);
}

static void insert_item(int *n, int32_t x, int32_t z, int kind, int a, int b){
 R3Vec v; int32_t lim;
 if(*n >= MAX_ITEMS) return;
 v = r3d_to_camera(&cam, x, 150, z);
 if(v.z < -300 || v.z > 3400) return;
 lim = v.z * (R3D_W / 2) / R3D_FOCAL + 420;
 if(v.x > lim || v.x < -lim) return;
 items[*n].key = (x - cam.x) / 4 * ((x - cam.x) / 4) + (z - cam.z) / 4 * ((z - cam.z) / 4);
 items[*n].kind = (uint8_t)kind; items[*n].a = (uint8_t)a; items[*n].b = (uint8_t)b;
 (*n)++;
}

static void scene(uint8_t *fb, const AreliaGame *g){
 int n = 0, x, z, i, j;
 for(z = 0; z < WORLD_H; z++) for(x = 0; x < WORLD_W; x++){
  char t = world_maps[g->room][z][x];
  if(t == T_WALL){
   /* A camera pulled against a wall may sit over it: skip that cell. */
   if((cam.x >> 8) == x && (cam.z >> 8) == z) continue;
   insert_item(&n, x * 256 + 128, z * 256 + 128, I_WALL, x, z);
  }
  else if(t == T_COLUMN) insert_item(&n, x * 256 + 128, z * 256 + 128, I_COLUMN, x, z);
  else if(t == T_LOW) insert_item(&n, x * 256 + 128, z * 256 + 128, I_LOW, x, z);
  else if(t == T_DOOR) insert_item(&n, x * 256 + 128, z * 256 + 128, I_ARCH, x, z);
 }
 for(i = 0; i < g->ent_count; i++) if(g->ent[i].alive || g->ent[i].type == E_BOAT)
  insert_item(&n, g->ent[i].x, g->ent[i].z + (g->ent[i].type == E_DOOR || g->ent[i].type == E_SEAL ? 100 : 0), I_ENT, i, 0);
 if(g->mode != MODE_TITLE && g->mode != MODE_WON) insert_item(&n, g->px, g->pz, I_PLAYER, 0, 0);
 for(i = 0; i < g->lapilli_n; i++) insert_item(&n, g->lap_x[i], g->lap_z[i], I_LAP, i, 0);
 /* Far to near (insertion sort; n is small). */
 for(i = 1; i < n; i++){Item it = items[i]; j = i - 1; while(j >= 0 && items[j].key < it.key){items[j + 1] = items[j]; j--;} items[j + 1] = it;}
 for(i = 0; i < n; i++){
  const Item *it = &items[i];
  switch(it->kind){
   case I_WALL: wall_cell(fb, g, it->a, it->b); break;
   case I_COLUMN: column_cell(fb, g, it->a, it->b); break;
   case I_LOW: low_cell(fb, g, it->a, it->b); break;
   case I_ARCH: arch_cell(fb, it->a, it->b); break;
   case I_ENT: draw_entity(fb, g, it->a); break;
   case I_PLAYER: draw_player(fb, g); break;
   case I_LAP: {
    int k = it->a, t = g->lap_t[k];
    blob_shadow(fb, g->lap_x[k], g->lap_z[k], 120 - t * 2);
    if(t < 30) aabb(fb, g->lap_x[k] - 30, t * 24, g->lap_z[k] - 30, g->lap_x[k] + 30, t * 24 + 60, g->lap_z[k] + 30, M_HAIR, M_ASH, -1, 0, 0);
    break;}
   default: break;
  }
 }
}

/* ------------------------------------------------------------ HUD */
/* The interface is laid out on a 160x100 grid of 2x2-pixel units, so the
 * 3x5 font stays legible while the scene renders at full resolution. */
#define UI_W (R3D_W / 2)
#define UI_H (R3D_H / 2)
static void ui_rect(uint8_t *fb, int x, int y, int w, int h, uint8_t c){r3d_rect(fb, x * 2, y * 2, w * 2, h * 2, c);}
static void ui_text(uint8_t *fb, int x, int y, const char *s, uint8_t c, int scale){r3d_text(fb, x * 2, y * 2, s, c, scale * 2);}
static int ui_text_width(const char *s, int scale){return r3d_text_width(s, scale);}
static void shadow_text(uint8_t *fb, int x, int y, const char *s, uint8_t c, int scale){
 ui_text(fb, x + 1, y + 1, s, UI_BLACK, scale); ui_text(fb, x, y, s, c, scale);
}
static void center_text(uint8_t *fb, int y, const char *s, uint8_t c, int scale){shadow_text(fb, (UI_W - ui_text_width(s, scale)) / 2, y, s, c, scale);}

static void panel(uint8_t *fb, int x, int y, int w, int h){
 ui_rect(fb, x, y, w, h, UI_BORDER); ui_rect(fb, x + 1, y + 1, w - 2, h - 2, UI_PANEL);
}

static void amphora(uint8_t *fb, int x, int y, int fill){
 uint8_t full = UI_AMPHORA, empty = UI_EMPTY;
 ui_rect(fb, x + 2, y, 3, 1, UI_BLACK);
 ui_rect(fb, x + 2, y + 1, 3, 2, fill ? full : empty);
 ui_rect(fb, x, y + 3, 7, 4, empty); ui_rect(fb, x + 1, y + 7, 5, 1, empty);
 if(fill >= 1){ui_rect(fb, x, y + 5, 7, 2, full); ui_rect(fb, x + 1, y + 7, 5, 1, full);}
 if(fill >= 2) ui_rect(fb, x, y + 3, 7, 2, full);
 ui_rect(fb, x + 1, y + 4, 1, 1, UI_WHITE);
}

static void hud(uint8_t *fb, const AreliaGame *g){
 int i; char num[4];
 for(i = 0; i < 3; i++){int f = g->hp - i * 2; amphora(fb, 3 + i * 9, 3, f < 0 ? 0 : f > 2 ? 2 : f);}
 /* bronze keys */
 ui_rect(fb, 3, 14, 3, 3, UI_GOLD); ui_rect(fb, 6, 15, 4, 1, UI_GOLD); ui_rect(fb, 8, 16, 1, 1, UI_GOLD);
 num[0] = 'X'; num[1] = (char)('0' + (g->keys > 9 ? 9 : g->keys)); num[2] = 0;
 shadow_text(fb, 12, 13, num, UI_TEXT, 1);
 /* rescued citizens and the song */
 for(i = 0; i < ARELIA_ROOMS; i++){
  uint8_t c = (g->rescued & (1u << i)) ? UI_GOLD : UI_EMPTY;
  ui_rect(fb, UI_W - 34 + i * 8, 3, 3, 3, c); ui_rect(fb, UI_W - 35 + i * 8, 6, 5, 4, c);
 }
 if(g->song_known){ui_rect(fb, UI_W - 42, 3, 1, 7, UI_GOLD); ui_rect(fb, UI_W - 40, 3, 1, 7, UI_GOLD); ui_rect(fb, UI_W - 42, 9, 3, 1, UI_GOLD);}
 if(g->prompt && !g->dialogue && g->action != ACT_TIBIA){
  static const char labels[4][10] = {"", "B RESCUE", "B CHECK", "B TALK"};
  int w = ui_text_width(labels[g->prompt & 3], 1) + 6;
  panel(fb, UI_W - w - 3, 13, w, 9);
  ui_text(fb, UI_W - w, 15, labels[g->prompt & 3], UI_GREEN, 1);
 }
}

/* Dialogue: a framed box at the bottom, '|' separates the two lines; the
 * text appears two characters per frame, like the logic expects. */
static void dialogue_box(uint8_t *fb, const AreliaGame *g){
 const char *s = game_dialogue_text(g->dialogue);
 char line[2][44]; int ln = 0, n = 0, shown = g->dialogue_t * 2, k = 0, colon = -1, i;
 line[0][0] = line[1][0] = 0;
 for(i = 0; s[i] && ln < 2; i++){
  if(s[i] == ':' && colon < 0 && ln == 0) colon = n;
  if(s[i] == '|'){line[ln][n] = 0; ln++; n = 0; continue;}
  if(k < shown && n < 43){line[ln][n++] = s[i]; line[ln][n] = 0;}
  k++;
 }
 if(ln < 2 && n < 44) line[ln][n] = 0;
 panel(fb, 4, UI_H - 25, UI_W - 8, 22);
 for(i = 0; i < 2; i++){
  int x = 9, y = UI_H - 21 + i * 8;
  if(i == 0 && colon > 0){
   /* The speaker's name in gold. */
   char name[16]; int j;
   for(j = 0; j < colon && j < 15 && line[0][j]; j++) name[j] = line[0][j];
   name[j] = 0;
   shadow_text(fb, x, y, name, UI_GOLD, 1);
   shadow_text(fb, x + ui_text_width(name, 1) + 1, y, line[0] + j, UI_TEXT, 1);
  } else shadow_text(fb, x, y, line[i], UI_TEXT, 1);
 }
 if(k <= shown && (g->ticks & 8)) ui_rect(fb, UI_W - 12, UI_H - 8, 3, 2, UI_GOLD);
}

/* The tibia staff: the six notes played so far over the staff lines. */
static void tibia_staff(uint8_t *fb, const AreliaGame *g){
 static const char names[5][2] = {"^", "<", ">", "V", "A"};
 static const uint8_t colors[5] = {UI_GOLD, UI_CYAN, UI_GREEN, UI_ORANGE, UI_RED};
 int i, n = g->note_count > 6 ? 6 : g->note_count;
 panel(fb, 20, UI_H - 28, UI_W - 40, 25);
 for(i = 0; i < 4; i++) ui_rect(fb, 24, UI_H - 22 + i * 4, UI_W - 48, 1, UI_BORDER);
 for(i = 0; i < n; i++){
  int note = g->notes[8 - n + i], x = 30 + i * 18, y = UI_H - 23 + (note < 4 ? note : 3) * 3;
  ui_rect(fb, x, y, 7, 6, colors[note]);
  ui_text(fb, x + 2, y + 1, names[note], UI_BLACK, 1);
 }
 ui_text(fb, 24, UI_H - 34, "TIBIA  B LOWER", UI_TEXT, 1);
}

static void focus_marks(uint8_t *fb, const AreliaGame *g){
 const AreliaEntity *e = &g->ent[g->focus - 1];
 R3Vec v = r3d_to_camera(&cam, e->x, 300 + (r3d_sin(g->ticks * 16) * 20 >> 14), e->z); int sx, sy;
 ui_rect(fb, 0, 0, UI_W, 7, UI_BLACK); ui_rect(fb, 0, UI_H - 7, UI_W, 7, UI_BLACK);
 if(v.z > R3D_NEAR && r3d_project(&v, &sx, &sy) && (sx /= 2, sy /= 2, 1) && sx > 3 && sx < UI_W - 4 && sy > 3 && sy < UI_H - 4){
  ui_rect(fb, sx - 3, sy - 3, 7, 1, UI_GOLD); ui_rect(fb, sx - 2, sy - 2, 5, 1, UI_GOLD);
  ui_rect(fb, sx - 1, sy - 1, 3, 1, UI_GOLD); ui_rect(fb, sx, sy, 1, 1, UI_RED);
 }
}

static void ash_fall(uint8_t *fb, const AreliaGame *g){
 int i, n = g->palette_phase * 12;
 for(i = 0; i < n; i++){
  uint32_t h = (uint32_t)i * 2654435761u;
  int x = (int)((h >> 8) % UI_W + (g->ticks / 2 + i) % 7) % UI_W;
  int y = (int)((h >> 16) % UI_H + g->ticks * (1 + i % 3)) % UI_H;
  ui_rect(fb, x, y, 1, 1, i & 1 ? UI_GREY : UI_EMPTY);
 }
}

static void overlay_title(uint8_t *fb, const AreliaGame *g){
 ui_rect(fb, 0, 0, UI_W, 12, UI_BLACK); ui_rect(fb, 0, UI_H - 12, UI_W, 12, UI_BLACK);
 center_text(fb, 20, "POMPEII", UI_GOLD, 3);
 center_text(fb, 40, "THE LAST SONG OF VESUVIUS", UI_TEXT, 1);
 if(g->ticks & 16) center_text(fb, 66, "PRESS START", UI_WHITE, 1);
 center_text(fb, UI_H - 9, "AD 79  AN ORIGINAL ADVENTURE", UI_GREY, 1);
}

static void overlay_dead(uint8_t *fb, const AreliaGame *g){
 int k; for(k = 0; k < R3D_W * R3D_H; k += 2 + (k / R3D_W & 1)) if(g->story > 10) fb[k] = UI_BLACK;
 center_text(fb, 34, "THE ASH CLOSES IN", UI_RED, 2);
 if(g->story > 30) center_text(fb, 60, "START: RISE AGAIN", UI_TEXT, 1);
}

static void overlay_won(uint8_t *fb, const AreliaGame *g){
 int bars = g->story < 40 ? g->story / 4 : 10;
 ui_rect(fb, 0, 0, UI_W, bars, UI_BLACK); ui_rect(fb, 0, UI_H - bars, UI_W, bars, UI_BLACK);
 if(g->story > 60) center_text(fb, 14, "THE BOATS PULL AWAY", UI_GOLD, 1);
 if(g->story > 140) center_text(fb, 24, "FOUR LIVES CARRIED TO THE SEA", UI_TEXT, 1);
 if(g->story > 240) center_text(fb, 72, "THE CITY SLEEPS UNDER THE ASH", UI_TEXT, 1);
 if(g->story > 300) center_text(fb, 80, "BUT ITS SONG IS REMEMBERED", UI_GOLD, 1);
 if(g->story >= 360 && (g->ticks & 16)) center_text(fb, UI_H - 8, "START", UI_WHITE, 1);
}

void game_render(const AreliaGame *g, uint8_t *fb){
 cam.x = g->cam_x; cam.y = g->cam_y; cam.z = g->cam_z;
 cam.yaw = g->cam_yaw; cam.pitch = g->cam_pitch;
 r3d_camera_setup(&cam);
 sky(fb, g);
 ground(fb, g);
 scene(fb, g);
 if(g->palette_phase >= 2) ash_fall(fb, g);
 switch(g->mode){
  case MODE_TITLE: overlay_title(fb, g); return;
  case MODE_DEAD: overlay_dead(fb, g); return;
  case MODE_WON: overlay_won(fb, g); return;
  default: break;
 }
 if(g->focus) focus_marks(fb, g);
 hud(fb, g);
 if(g->banner_t > 20 && !g->dialogue){
  const char *name = world_room_names[g->room < ARELIA_ROOMS ? g->room : 0];
  int w = ui_text_width(name, 2) + 10;
  panel(fb, (UI_W - w) / 2, 26, w, 16);
  ui_text(fb, (UI_W - w) / 2 + 5, 30, name, UI_GOLD, 2);
 }
 if(g->action == ACT_TIBIA) tibia_staff(fb, g);
 if(g->dialogue) dialogue_box(fb, g);
}
