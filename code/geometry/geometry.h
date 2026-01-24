#ifndef GEOMETRY_H
#define GEOMETRY_H

typedef struct {
	f32 DistSq;
	v3  P1;
	v3  P2;
} closest_points;

typedef struct {
	b32 Intersected;
	v3  P1;
	v3  P2;
	v3  V;
} penetration_test;

typedef struct {
    v3  P1;
    v3  P2;
    f32 tHit;
    v3  V;
} ray_cast;

#include "gjk_epa.h"

#endif //GEOMETRY_H
