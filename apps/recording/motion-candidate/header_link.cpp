/* SPDX-License-Identifier: GPL-2.0-only */
#include "motion.h"
int main(){mm_geometry g{1280,720,0,0,1280,720,1280,720,1,1};mm_point p{0,0};mm_rational_point out{};
 return mm_display_to_sensor(&g,p,&out)==MM_OK&&out.x_num==1280LL*out.x_den&&out.y_num==720LL*out.y_den?0:1;}
