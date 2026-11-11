#ifndef ARELIA_R3D_H
#define ARELIA_R3D_H
/* Tiny fixed-point 3D rasterizer for the PRG32 indexed framebuffer.
 *
 * No floating point, no 64-bit arithmetic (portable cartridges link without
 * libgcc), no allocation, no pointers in static data. World coordinates are
 * Q8 (256 = one map cell); angles use 1024 units per turn; trigonometry is
 * Q14. Polygons are clipped against the whole view frustum in camera space,
 * so projected vertices always lie on the 320x200 render target. */
#include <stdint.h>

#define R3D_W 320
#define R3D_H 200
#define R3D_FOCAL 224          /* focal length in pixels (about 71 degrees across) */
#define R3D_NEAR 24            /* near plane, Q8 world units */
#define R3D_ANGLE_MASK 1023
#define R3D_MAX_POLY 16

typedef struct {
 int32_t x, y, z;              /* position, Q8 */
 int16_t yaw, pitch;           /* yaw 0 looks north (-z), 256 east; pitch > 0 looks down */
 int32_t sy, cy, sp, cp;       /* Q14 sin/cos of yaw and pitch (filled by r3d_camera_setup) */
 int16_t horizon;              /* screen row of the horizon */
} R3Camera;

typedef struct { int32_t x, y, z; } R3Vec;   /* camera-space point, Q8 */

int32_t r3d_sin(int angle);
int32_t r3d_cos(int angle);
/* Angle of the vector (dx, dz) in the yaw convention above (0..1023). */
int r3d_angle(int32_t dx, int32_t dz);
/* Signed shortest difference b - a in -512..511. */
int r3d_angle_diff(int a, int b);
int32_t r3d_isqrt(uint32_t v);
int32_t r3d_dist(int32_t dx, int32_t dz);

void r3d_camera_setup(R3Camera *c);
R3Vec r3d_to_camera(const R3Camera *c, int32_t wx, int32_t wy, int32_t wz);
/* Project a camera-space point (Z >= R3D_NEAR); returns 0 when behind. */
int r3d_project(const R3Vec *v, int *sx, int *sy);

/* Clip a camera-space polygon to the frustum, project and fill it. */
void r3d_poly(uint8_t *fb, const R3Vec *pts, int n, uint8_t color);
/* Fill a convex polygon given in Q4 screen coordinates (already on screen). */
void r3d_fill(uint8_t *fb, const int32_t *sx, const int32_t *sy, int n, uint8_t color);
/* Same polygon test as r3d_poly but returns 1 when the projected polygon
 * faces the camera (counter-clockwise on screen); used for rotated boxes. */
int r3d_front_facing(const R3Vec *pts, int n);

void r3d_rect(uint8_t *fb, int x, int y, int w, int h, uint8_t color);
void r3d_text(uint8_t *fb, int x, int y, const char *s, uint8_t color, int scale);
int r3d_text_width(const char *s, int scale);
#endif
