/* SPDX-License-Identifier: GPL-2.0-only */
#include "motion.h"
#include <string.h>
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return !a||!b||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y||(x<y+bn&&y<x+an);}
static int geom(const struct mm_geometry *g)
{return g&&g->native_w>=MM_COLS&&g->native_w<=4096&&g->native_h>=MM_ROWS&&g->native_h<=2160&&
 g->crop_w>=MM_COLS&&g->crop_h>=MM_ROWS&&g->crop_x+g->crop_w<=g->native_w&&g->crop_y+g->crop_h<=g->native_h&&
 g->display_w>=MM_COLS&&g->display_w<=4096&&g->display_h>=MM_ROWS&&g->display_h<=2160&&g->flip_x<=1&&g->flip_y<=1;}
static int64_t cross(int64_t ax,int64_t ay,int64_t bx,int64_t by,int64_t cx,int64_t cy)
{return (bx-ax)*(cy-ay)-(by-ay)*(cx-ax);}
static int between(int64_t a,int64_t b,int64_t c){return (a<=c&&c<=b)||(b<=c&&c<=a);}
static int intersect(int64_t ax,int64_t ay,int64_t bx,int64_t by,int64_t cx,int64_t cy,int64_t dx,int64_t dy)
{
 int64_t p=cross(ax,ay,bx,by,cx,cy),q=cross(ax,ay,bx,by,dx,dy),r=cross(cx,cy,dx,dy,ax,ay),s=cross(cx,cy,dx,dy,bx,by);
 if(!p&&between(ax,bx,cx)&&between(ay,by,cy))return 1;
 if(!q&&between(ax,bx,dx)&&between(ay,by,dy))return 1;
 if(!r&&between(cx,dx,ax)&&between(cy,dy,ay))return 1;
 if(!s&&between(cx,dx,bx)&&between(cy,dy,by))return 1;
 return ((p<0&&q>0)||(p>0&&q<0))&&((r<0&&s>0)||(r>0&&s<0));
}
int mm_display_to_sensor(const struct mm_geometry *g,struct mm_point p,struct mm_rational_point *out)
{
 if(!geom(g)||!out||overlap(g,sizeof(*g),out,sizeof(*out))||p.x>g->display_w||p.y>g->display_h)return MM_INVALID;
 out->x_num=(int64_t)g->crop_x*g->display_w+(int64_t)(g->flip_x?g->display_w-p.x:p.x)*g->crop_w;
 out->y_num=(int64_t)g->crop_y*g->display_h+(int64_t)(g->flip_y?g->display_h-p.y:p.y)*g->crop_h;
 out->x_den=g->display_w;out->y_den=g->display_h;return MM_OK;
}
int mm_sensor_to_display(const struct mm_geometry *g,struct mm_point p,struct mm_rational_point *out)
{
 if(!geom(g)||!out||overlap(g,sizeof(*g),out,sizeof(*out))||p.x<g->crop_x||p.x>g->crop_x+g->crop_w||p.y<g->crop_y||p.y>g->crop_y+g->crop_h)return MM_INVALID;
 out->x_num=(int64_t)(g->flip_x?g->crop_x+g->crop_w-p.x:p.x-g->crop_x)*g->display_w;
 out->y_num=(int64_t)(g->flip_y?g->crop_y+g->crop_h-p.y:p.y-g->crop_y)*g->display_h;
 out->x_den=g->crop_w;out->y_den=g->crop_h;return MM_OK;
}
int mm_config_validate(const struct mm_config *c,const struct mm_geometry *g)
{
 uint32_t p,i,j;int64_t area;
 if(!c||!geom(g)||c->schema!=MM_SCHEMA||!c->generation||c->polygon_count>MM_POLYS||
    !c->delta_min||c->delta_min>128||c->noise_multiplier>4||!c->area_permille||c->area_permille>1000||
    !c->global_delta||c->global_delta>128||c->global_permille<500||c->global_permille>1000||
    c->debounce_ns>5000000000ULL||c->release_ns>5000000000ULL||!c->min_event_ns||c->min_event_ns>30000000000ULL||
    !c->max_gap_ns||c->max_gap_ns>5000000000ULL||!c->event_ttl_ns||c->event_ttl_ns>5000000000ULL)return MM_INVALID;
 for(p=0;p<MM_POLYS;p++){
  const struct mm_polygon *poly=&c->polygons[p];uint32_t n=poly->vertices;
  if(p>=c->polygon_count){if(n)return MM_INVALID;n=0;}
  else if(n<3||n>MM_VERTICES)return MM_INVALID;
  area=0;
  for(i=0;i<MM_VERTICES;i++){
   if(i>=n){if(poly->point[i].x||poly->point[i].y)return MM_INVALID;continue;}
   if(poly->point[i].x>g->display_w||poly->point[i].y>g->display_h)return MM_INVALID;
   for(j=0;j<i;j++)if(poly->point[i].x==poly->point[j].x&&poly->point[i].y==poly->point[j].y)return MM_INVALID;
   {struct mm_point a=poly->point[i],b=poly->point[(i+1)%n],d=poly->point[(i+2)%n];
    area+=(int64_t)a.x*b.y-(int64_t)b.x*a.y;
    if(!cross(a.x,a.y,b.x,b.y,d.x,d.y))return MM_INVALID;
   }
   for(j=i+1;j<n;j++)if(j!=i+1&&!(i==0&&j==n-1)){
    struct mm_point a=poly->point[i],b=poly->point[(i+1)%n],d=poly->point[j],e=poly->point[(j+1)%n];
    if(intersect(a.x,a.y,b.x,b.y,d.x,d.y,e.x,e.y))return MM_INVALID;
   }
  }
  if(n&&area<=0)return MM_INVALID; /* canonical positive shoelace winding */
 }
 return MM_OK;
}
static int point_inside(const struct mm_rational_point *v,uint32_t n,int64_t x,int64_t y)
{
 uint32_t i,j=n-1;int inside=0;
 for(i=0;i<n;j=i++){
  int64_t a=cross(v[j].x_num,v[j].y_num,v[i].x_num,v[i].y_num,x,y);
  if(!a&&between(v[j].x_num,v[i].x_num,x)&&between(v[j].y_num,v[i].y_num,y))return 1;
  if((v[j].y_num>y)!=(v[i].y_num>y)){
   int64_t lhs=(x-v[j].x_num)*(v[i].y_num-v[j].y_num),rhs=(v[i].x_num-v[j].x_num)*(y-v[j].y_num);
   if((v[i].y_num>v[j].y_num)?lhs<rhs:lhs>rhs)inside=!inside;
  }
 }
 return inside;
}
int mm_mask_rasterize(const struct mm_config *c,const struct mm_geometry *g,uint8_t out[MM_MASK_BYTES])
{
 uint8_t mask[MM_MASK_BYTES]={0};uint32_t p,cell,i;
 if(!out||!c||!g||overlap(c,sizeof(*c),out,MM_MASK_BYTES)||overlap(g,sizeof(*g),out,MM_MASK_BYTES)||mm_config_validate(c,g))return MM_INVALID;
 for(p=0;p<c->polygon_count;p++){
  struct mm_rational_point v[MM_VERTICES];uint32_t n=c->polygons[p].vertices;
  for(i=0;i<n;i++)mm_display_to_sensor(g,c->polygons[p].point[i],&v[i]);
  for(cell=0;cell<MM_CELLS;cell++){
   uint32_t col=cell%MM_COLS,row=cell/MM_COLS;int hit=0;
   int64_t l=(int64_t)(g->crop_x+(col*g->crop_w)/MM_COLS)*g->display_w;
   int64_t r=(int64_t)(g->crop_x+((col+1)*g->crop_w)/MM_COLS)*g->display_w;
   int64_t t=(int64_t)(g->crop_y+(row*g->crop_h)/MM_ROWS)*g->display_h;
   int64_t b=(int64_t)(g->crop_y+((row+1)*g->crop_h)/MM_ROWS)*g->display_h;
   if(point_inside(v,n,l,t)||point_inside(v,n,r,t)||point_inside(v,n,r,b)||point_inside(v,n,l,b))hit=1;
   for(i=0;i<n&&!hit;i++){
    uint32_t j=(i+1)%n;
    if(v[i].x_num>=l&&v[i].x_num<=r&&v[i].y_num>=t&&v[i].y_num<=b)hit=1;
    if(intersect(v[i].x_num,v[i].y_num,v[j].x_num,v[j].y_num,l,t,r,t)||
       intersect(v[i].x_num,v[i].y_num,v[j].x_num,v[j].y_num,r,t,r,b)||
       intersect(v[i].x_num,v[i].y_num,v[j].x_num,v[j].y_num,r,b,l,b)||
       intersect(v[i].x_num,v[i].y_num,v[j].x_num,v[j].y_num,l,b,l,t))hit=1;
   }
   if(hit)mask[cell/8]|=(uint8_t)(1U<<(cell%8)); /* union, contact inclusive */
  }
 }
 memcpy(out,mask,sizeof(mask));return MM_OK;
}
