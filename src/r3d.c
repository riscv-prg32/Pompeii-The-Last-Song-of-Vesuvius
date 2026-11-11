/* Fixed-point 3D rasterizer; see r3d.h. */
#include "r3d.h"
#include "trig_table.h"

int32_t r3d_sin(int a){
 int i;a&=R3D_ANGLE_MASK;i=a&255;
 switch(a>>8){
  case 0:return r3d_quarter_sine[i];
  case 1:return r3d_quarter_sine[256-i];
  case 2:return -r3d_quarter_sine[i];
  default:return -r3d_quarter_sine[256-i];
 }
}
int32_t r3d_cos(int a){return r3d_sin(a+256);}

int r3d_angle_diff(int a,int b){int d=(b-a)&R3D_ANGLE_MASK;return d>=512?d-1024:d;}

/* atan in the first octant: angle (0..128) whose tangent is num/den, num<=den. */
static int octant_atan(int32_t num,int32_t den){
 int lo=0,hi=128;
 if(den<=0)return 0;
 while(den>(1<<15)||num>(1<<15)){num>>=1;den>>=1;}
 while(lo<hi){int mid=(lo+hi+1)>>1; /* tan(mid) <= num/den ? */
  if((int32_t)r3d_quarter_sine[mid]*den<=num*(int32_t)r3d_quarter_sine[256-mid])lo=mid;else hi=mid-1;}
 return lo;
}
int r3d_angle(int32_t dx,int32_t dz){
 /* yaw convention: forward = (sin, -cos); so angle = atan2(dx, -dz). */
 int32_t ex=dx,ny=-dz,ax=ex<0?-ex:ex,ay=ny<0?-ny:ny;int a;
 if(!ax&&!ay)return 0;
 if(ax<=ay)a=octant_atan(ax,ay);else a=256-octant_atan(ay,ax);
 /* a is the angle from +ny toward +ex in quadrant I */
 if(ex>=0&&ny>=0)return a;
 if(ex>=0)return 512-a;
 if(ny<0)return (512+a)&R3D_ANGLE_MASK;
 return (1024-a)&R3D_ANGLE_MASK;
}

int32_t r3d_isqrt(uint32_t v){
 uint32_t r=0,bit=1u<<30;
 while(bit>v)bit>>=2;
 while(bit){if(v>=r+bit){v-=r+bit;r=(r>>1)+bit;}else r>>=1;bit>>=2;}
 return (int32_t)r;
}
int32_t r3d_dist(int32_t dx,int32_t dz){
 int sh=0;
 while(dx>30000||dx<-30000||dz>30000||dz<-30000){dx>>=1;dz>>=1;sh++;}
 return r3d_isqrt((uint32_t)(dx*dx+dz*dz))<<sh;
}

void r3d_camera_setup(R3Camera*c){
 c->sy=r3d_sin(c->yaw);c->cy=r3d_cos(c->yaw);c->sp=r3d_sin(c->pitch);c->cp=r3d_cos(c->pitch);
 c->horizon=(int16_t)(R3D_H/2-(R3D_FOCAL*c->sp)/(c->cp?c->cp:1));
}

R3Vec r3d_to_camera(const R3Camera*c,int32_t wx,int32_t wy,int32_t wz){
 R3Vec v;int32_t dx=wx-c->x,dy=wy-c->y,dz=wz-c->z,z0;
 v.x=(dx*c->cy+dz*c->sy)>>14;
 z0=(dx*c->sy-dz*c->cy)>>14;
 v.y=(dy*c->cp+z0*c->sp)>>14;
 v.z=(z0*c->cp-dy*c->sp)>>14;
 return v;
}

int r3d_project(const R3Vec*v,int*sx,int*sy){
 if(v->z<R3D_NEAR)return 0;
 *sx=R3D_W/2+(v->x*R3D_FOCAL)/v->z;
 *sy=R3D_H/2-(v->y*R3D_FOCAL)/v->z;
 return 1;
}

/* Plane value of a camera-space point for frustum plane k (>= 0 is inside). */
static int32_t plane(const R3Vec*p,int k){
 switch(k){
  case 0:return p->z-R3D_NEAR;
  case 1:return p->x*R3D_FOCAL+p->z*(R3D_W/2);
  case 2:return p->z*(R3D_W/2)-p->x*R3D_FOCAL;
  case 3:return p->z*(R3D_H/2)-p->y*R3D_FOCAL;
  default:return p->y*R3D_FOCAL+p->z*(R3D_H/2);
 }
}
static R3Vec lerp(const R3Vec*a,const R3Vec*b,int32_t va,int32_t vb){
 int32_t num=va,den=va-vb,t;R3Vec r;
 while(den>(1<<18)){num>>=1;den>>=1;}
 t=den?(num<<12)/den:0;
 r.x=a->x+(((b->x-a->x)*t)>>12);r.y=a->y+(((b->y-a->y)*t)>>12);r.z=a->z+(((b->z-a->z)*t)>>12);
 return r;
}

static int32_t span_l[R3D_H],span_r[R3D_H];

void r3d_fill(uint8_t*fb,const int32_t*sx,const int32_t*sy,int n,uint8_t color){
 int32_t ymin=sy[0],ymax=sy[0];int i,r0,r1,r;
 for(i=1;i<n;i++){if(sy[i]<ymin)ymin=sy[i];if(sy[i]>ymax)ymax=sy[i];}
 r0=(ymin+7)>>4;r1=(ymax+7)>>4; /* rows whose centers lie in [ymin, ymax) */
 if(r0<0)r0=0;
 if(r1>R3D_H)r1=R3D_H;
 if(r0>=r1)return;
 for(r=r0;r<r1;r++){span_l[r]=0x7fffffff;span_r[r]=-0x7fffffff;}
 for(i=0;i<n;i++){
  int j=(i+1)%n;int32_t xa=sx[i],ya=sy[i],xb=sx[j],yb=sy[j],slope,x;int e0,e1;
  if(ya==yb)continue;
  if(ya>yb){int32_t t=xa;xa=xb;xb=t;t=ya;ya=yb;yb=t;}
  e0=(ya+7)>>4;e1=(yb+7)>>4;
  if(e0<r0)e0=r0;
  if(e1>r1)e1=r1;
  if(e0>=e1)continue;
  slope=((xb-xa)<<12)/(yb-ya);
  x=(xa<<12)+((e0*16+8)-ya)*slope;
  for(r=e0;r<e1;r++){if(x<span_l[r])span_l[r]=x;if(x>span_r[r])span_r[r]=x;x+=slope*16;}
 }
 for(r=r0;r<r1;r++){
  int a,b;uint8_t*row;
  if(span_l[r]>span_r[r])continue;
  a=(span_l[r]+32767)>>16;b=(span_r[r]+32767)>>16; /* pixel centers in [l, r) */
  if(a<0)a=0;
  if(b>R3D_W)b=R3D_W;
  row=fb+r*R3D_W;
  for(;a<b;a++)row[a]=color;
 }
}

void r3d_poly(uint8_t*fb,const R3Vec*pts,int n,uint8_t color){
 R3Vec bufa[R3D_MAX_POLY],bufb[R3D_MAX_POLY];R3Vec*in=bufa,*out=bufb;
 int32_t sx[R3D_MAX_POLY],sy[R3D_MAX_POLY];int i,k,m;
 if(n<3||n>8)return;
 for(i=0;i<n;i++)in[i]=pts[i];
 for(k=0;k<5;k++){
  m=0;
  for(i=0;i<n;i++){
   const R3Vec*a=&in[i],*b=&in[(i+1)%n];int32_t va=plane(a,k),vb=plane(b,k);
   if(va>=0){if(m<R3D_MAX_POLY)out[m++]=*a;}
   if((va>=0)!=(vb>=0)&&m<R3D_MAX_POLY)out[m++]=va>=0?lerp(a,b,va,vb):lerp(b,a,vb,va);
  }
  if(m<3)return;
  {R3Vec*t=in;in=out;out=t;}n=m;
 }
 for(i=0;i<n;i++){
  int32_t z=in[i].z<R3D_NEAR?R3D_NEAR:in[i].z;
  sx[i]=R3D_W*8+(in[i].x*R3D_FOCAL*16)/z;
  sy[i]=R3D_H*8-(in[i].y*R3D_FOCAL*16)/z;
 }
 r3d_fill(fb,sx,sy,n,color);
}

int r3d_front_facing(const R3Vec*p,int n){
 int32_t ux,uy,uz,vx,vy,vz,nx,ny,nz;
 if(n<3)return 0;
 ux=p[1].x-p[0].x;uy=p[1].y-p[0].y;uz=p[1].z-p[0].z;
 vx=p[2].x-p[0].x;vy=p[2].y-p[0].y;vz=p[2].z-p[0].z;
 nx=(uy*vz-uz*vy)>>4;ny=(uz*vx-ux*vz)>>4;nz=(ux*vy-uy*vx)>>4;
 /* Visible when the camera (origin) is on the normal's side. */
 return nx*(p[0].x>>4)+ny*(p[0].y>>4)+nz*(p[0].z>>4)<0;
}

void r3d_rect(uint8_t*fb,int x,int y,int w,int h,uint8_t c){
 int i,j,x1=x+w,y1=y+h;
 if(x<0)x=0;
 if(y<0)y=0;
 if(x1>R3D_W)x1=R3D_W;
 if(y1>R3D_H)y1=R3D_H;
 for(j=y;j<y1;j++)for(i=x;i<x1;i++)fb[j*R3D_W+i]=c;
}

/* Built-in 3x5 font for ASCII 32..90: bit 4 = left column. */
static const uint8_t glyphs[59][5]={{0,0,0,0,0},{2,2,2,0,2},{5,5,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{2,2,0,0,0},{1,2,2,2,1},{4,2,2,2,4},{0,0,0,0,0},{0,2,7,2,0},{0,0,0,2,4},{0,0,7,0,0},{0,0,0,0,2},{1,1,2,4,4},{7,5,5,5,7},{2,6,2,2,7},{6,1,7,4,7},{6,1,3,1,6},{5,5,7,1,1},{7,4,6,1,6},{3,4,7,5,7},{7,1,1,2,2},{7,5,7,5,7},{7,5,7,1,6},{0,2,0,2,0},{0,2,0,2,4},{1,2,4,2,1},{0,7,0,7,0},{4,2,1,2,4},{6,1,2,0,2},{0,0,0,0,0},{2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{6,5,5,5,5},{2,5,5,5,2},{6,5,6,4,4},{2,5,5,3,1},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7}};
void r3d_text(uint8_t*fb,int x,int y,const char*s,uint8_t color,int scale){
 int j,k;
 for(;*s;s++){
  unsigned a=(unsigned char)*s;
  if(a>=97&&a<=122)a-=32;
  if(a<32||a>90)a=32;
  for(j=0;j<5;j++)for(k=0;k<3;k++)if(glyphs[a-32][j]&(4>>k))r3d_rect(fb,x+k*scale,y+j*scale,scale,scale,color);
  x+=4*scale;
 }
}
int r3d_text_width(const char*s,int scale){int n=0;while(s[n])n++;return n?n*4*scale-scale:0;}
