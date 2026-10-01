#ifndef AABB_H
#define AABB_H

typedef struct {
    v3 Min;
    v3 Max;
} aabb;

function inline aabb AABB_Inverted(void) {
    aabb Result;
    Result.Min = V3_All(1.0e30f);
    Result.Max = V3_All(-1.0e30f);
    return Result;
}

function inline aabb AABB_From_Min_Max(v3 MinValue, v3 MaxValue) {
    aabb Result;
    Result.Min = MinValue;
    Result.Max = MaxValue;
    return Result;
}

function inline b32 AABB_Is_Valid(aabb Box) {
    return Box.Min.x <= Box.Max.x && Box.Min.y <= Box.Max.y && Box.Min.z <= Box.Max.z;
}

function inline v3 AABB_Center(aabb Box) {
    return V3_Mul_S(V3_Add_V3(Box.Min, Box.Max), 0.5f);
}

function inline v3 AABB_Size(aabb Box) {
    return V3_Sub_V3(Box.Max, Box.Min);
}

function inline b32 AABB_Contains(aabb Box, v3 P) {
    return P.x >= Box.Min.x && P.x <= Box.Max.x &&
           P.y >= Box.Min.y && P.y <= Box.Max.y &&
           P.z >= Box.Min.z && P.z <= Box.Max.z;
}

function inline void AABB_Expand(aabb* Box, v3 P) {
    Box->Min.x = Min(Box->Min.x, P.x);
    Box->Min.y = Min(Box->Min.y, P.y);
    Box->Min.z = Min(Box->Min.z, P.z);
    Box->Max.x = Max(Box->Max.x, P.x);
    Box->Max.y = Max(Box->Max.y, P.y);
    Box->Max.z = Max(Box->Max.z, P.z);
}

function inline aabb AABB_Union(aabb A, aabb B) {
    aabb Result;
    Result.Min.x = Min(A.Min.x, B.Min.x);
    Result.Min.y = Min(A.Min.y, B.Min.y);
    Result.Min.z = Min(A.Min.z, B.Min.z);
    Result.Max.x = Max(A.Max.x, B.Max.x);
    Result.Max.y = Max(A.Max.y, B.Max.y);
    Result.Max.z = Max(A.Max.z, B.Max.z);
    return Result;
}

function inline aabb AABB_Intersect(aabb A, aabb B) {
    aabb Result;
    Result.Min.x = Max(A.Min.x, B.Min.x);
    Result.Min.y = Max(A.Min.y, B.Min.y);
    Result.Min.z = Max(A.Min.z, B.Min.z);
    Result.Max.x = Min(A.Max.x, B.Max.x);
    Result.Max.y = Min(A.Max.y, B.Max.y);
    Result.Max.z = Min(A.Max.z, B.Max.z);
    return Result;
}

function inline aabb AABB_Pad(aabb Box, f32 Pad) {
    v3 P = V3_All(Pad);
    aabb Result;
    Result.Min = V3_Sub_V3(Box.Min, P);
    Result.Max = V3_Add_V3(Box.Max, P);
    return Result;
}

export_function aabb Make_AABB_From_Points(const v3* Points, u32 PointCount);
export_function aabb AABB_From_Points(const v3* Points, u32 Count);
export_function void AABB_Get_Corners(aabb Box, v3 Corners[8]);
export_function aabb Transform_AABB(const aabb* LocalBounds, const m4_affine* Transform);
export_function b32 Segment_Clip_AABB(v3 P0, v3 P1, aabb Box, v3* Out0, v3* Out1);

#endif // AABB_H
