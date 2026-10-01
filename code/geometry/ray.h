#ifndef RAY_H
#define RAY_H

typedef struct {
	v3 Origin;
	v3 Direction;
} ray;

typedef struct {
	b32 Hit;
	f32 T;
} ray_hit;

typedef struct {
	v3 Center;
	m3 Orientation;
	v3 HalfExtent;
} obb;

export_function ray_hit Ray_Intersect_Triangle(const ray* Ray, v3 P0, v3 P1, v3 P2);
export_function ray_hit Ray_Intersect_Cylinder_Segment(const ray* Ray, v3 SegA, v3 SegB, f32 PickRadius);
export_function ray_hit Ray_Intersect_Plane(const ray* Ray, v3 PlaneOrigin, v3 PlaneNormal);
export_function ray_hit Ray_Intersect_Ring(const ray* Ray, v3 Center, v3 Normal, f32 Radius, f32 PickRadius);
export_function ray_hit Ray_Intersect_AABB(const ray* Ray, v3 Min, v3 Max);
export_function ray_hit Ray_Intersect_AABB_Uniform(const ray* Ray, v3 Center, f32 Size);
export_function ray_hit Ray_Intersect_OBB(const ray* Ray, const obb* Box);
export_function b32 Ray_Project_To_Axis(const ray* Ray, v3 LineOrigin, v3 Axis, f32* OutT);

#endif // RAY_H
