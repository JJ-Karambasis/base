#ifndef GJK_EPA_H
#define GJK_EPA_H

#define GJK_SUPPORT_FUNC(name) v3 name(void* UserData, v3 Direction)
typedef GJK_SUPPORT_FUNC(gjk_support_func);

typedef struct {
	void*             UserData;
	gjk_support_func* SupportFunc;
} gjk_support;

typedef struct {
	u32 Count;
	v3 P[4];
	v3 A[4];
	v3 B[4];
} gjk_simplex;

export_function gjk_support GJK_Make_Support(gjk_support_func* Func, void* UserData);
export_function gjk_support GJK_Origin();
export_function gjk_support GJK_Point(arena* Arena, v3 P);
export_function gjk_support GJK_Radius(arena* Arena, f32 Radius);
export_function gjk_support GJK_Sphere(arena* Arena, v3 P, f32 Radius);
export_function gjk_support GJK_Extent(arena* Arena, v3 HalfExtent);
export_function gjk_support GJK_AABB(arena* Arena, v3 Min, v3 Max);
export_function gjk_support GJK_Triangle(arena* Arena, v3 p0, v3 p1, v3 p2);
export_function gjk_support GJK_Add_Radius(arena* Arena, gjk_support InnerSupport, f32 Radius);
export_function gjk_support GJK_Transform(arena* Arena, gjk_support InnerSupport, const m4* Matrix);

export_function b32 GJK_Intersects(gjk_support* SupportA, gjk_support* SupportB, f32 Tolerance, v3* InOutV);
export_function closest_points GJK_Get_Closest_Points(gjk_support* SupportA, gjk_support* SupportB, 
                                                      f32 Tolerance, f32 MaxDistSq, v3* InOutV, 
                                                      gjk_simplex* OutSimplex);
export_function ray_cast GJK_Cast(gjk_support* SupportA, f32 RadiusA, gjk_support* SupportB, f32 RadiusB, 
                                  const m4* Start, v3 Direction, f32 tDistance, f32 Tolerance, gjk_simplex* OutSimplex);
export_function penetration_test GJK_Penetration_Test(gjk_support* SupportA, f32 RadiusA, gjk_support* SupportB, 
                                                      f32 RadiusB, f32 Tolerance, gjk_simplex* OutSimplex, 
                                                      f32* OutDistSq);
export_function penetration_test EPA_Penetration_Test_With_Simplex(gjk_support* SupportA, gjk_support* SupportB, 
                                                                   f32 Tolerance, gjk_simplex* Simplex);
export_function penetration_test EPA_Penetration_Test(gjk_support* SupportA, gjk_support* SupportB, 
                                                      f32 CollisionTolerance, f32 PenetrationTolerance);
export_function ray_cast Cast_Shape(gjk_support* SupportA, f32 RadiusA, gjk_support* SupportB, f32 RadiusB, 
                                    const m4* Start, v3 Direction, f32 tDistance, 
                                    f32 CollisionTolerance, f32 PenetrationTolerance);


#endif //GJK_EPA_H
