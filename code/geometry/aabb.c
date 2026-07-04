#include "aabb.h"

static void AABB_Expand_With_Point(aabb* Bounds, v3 Point) {
    Bounds->Min.x = Min(Bounds->Min.x, Point.x);
    Bounds->Min.y = Min(Bounds->Min.y, Point.y);
    Bounds->Min.z = Min(Bounds->Min.z, Point.z);
    Bounds->Max.x = Max(Bounds->Max.x, Point.x);
    Bounds->Max.y = Max(Bounds->Max.y, Point.y);
    Bounds->Max.z = Max(Bounds->Max.z, Point.z);
}

static v3 AABB_Transform_Point(const m4_affine* Transform, v3 Point) {
    return V4_Mul_M4_Affine(V4(Point.x, Point.y, Point.z, 1.0f), Transform);
}

export_function aabb Make_AABB_From_Points(const v3* Points, u32 PointCount) {
    aabb Result = { .Min = V3_All(1e30f), .Max = V3_All(-1e30f) };
    b32 Any = false;
    for(u32 i = 0; i < PointCount; i++) {
        Any = true;
        AABB_Expand_With_Point(&Result, Points[i]);
    }
    if(!Any) {
        Result.Min = V3_Zero();
        Result.Max = V3_Zero();
    }
    return Result;
}

export_function aabb Transform_AABB(const aabb* LocalBounds, const m4_affine* Transform) {
    aabb Result = { .Min = V3_All(1e30f), .Max = V3_All(-1e30f) };
    v3 LocalMin = LocalBounds->Min;
    v3 LocalMax = LocalBounds->Max;
    v3 Corners[8] = {
        V3(LocalMin.x, LocalMin.y, LocalMin.z), V3(LocalMax.x, LocalMin.y, LocalMin.z),
        V3(LocalMin.x, LocalMax.y, LocalMin.z), V3(LocalMax.x, LocalMax.y, LocalMin.z),
        V3(LocalMin.x, LocalMin.y, LocalMax.z), V3(LocalMax.x, LocalMin.y, LocalMax.z),
        V3(LocalMin.x, LocalMax.y, LocalMax.z), V3(LocalMax.x, LocalMax.y, LocalMax.z),
    };
    for(u32 i = 0; i < 8; i++) {
        AABB_Expand_With_Point(&Result, AABB_Transform_Point(Transform, Corners[i]));
    }
    return Result;
}
