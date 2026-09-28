#include "aabb.h"

export_function aabb Make_AABB_From_Points(const v3* Points, u32 PointCount) {
    aabb Result = AABB_Inverted();
    b32 Any = false;
    for(u32 i = 0; i < PointCount; i++) {
        Any = true;
        AABB_Expand(&Result, Points[i]);
    }
    if(!Any) {
        Result.Min = V3_Zero();
        Result.Max = V3_Zero();
    }
    return Result;
}

export_function aabb AABB_From_Points(const v3* Points, u32 Count) {
    aabb Box = AABB_Inverted();
    for(u32 PointIndex = 0; PointIndex < Count; PointIndex++) {
        AABB_Expand(&Box, Points[PointIndex]);
    }
    return Box;
}

export_function void AABB_Get_Corners(aabb Box, v3 Corners[8]) {
    Corners[0] = V3(Box.Min.x, Box.Min.y, Box.Min.z);
    Corners[1] = V3(Box.Max.x, Box.Min.y, Box.Min.z);
    Corners[2] = V3(Box.Max.x, Box.Max.y, Box.Min.z);
    Corners[3] = V3(Box.Min.x, Box.Max.y, Box.Min.z);
    Corners[4] = V3(Box.Min.x, Box.Min.y, Box.Max.z);
    Corners[5] = V3(Box.Max.x, Box.Min.y, Box.Max.z);
    Corners[6] = V3(Box.Max.x, Box.Max.y, Box.Max.z);
    Corners[7] = V3(Box.Min.x, Box.Max.y, Box.Max.z);
}

export_function aabb Transform_AABB(const aabb* LocalBounds, const m4_affine* Transform) {
    v3 Corners[8];
    AABB_Get_Corners(*LocalBounds, Corners);
    aabb Result = AABB_Inverted();
    for(u32 CornerIndex = 0; CornerIndex < 8; CornerIndex++) {
        AABB_Expand(&Result, V4_Mul_M4_Affine(V4(Corners[CornerIndex].x, Corners[CornerIndex].y, Corners[CornerIndex].z, 1.0f), Transform));
    }
    return Result;
}

export_function b32 Segment_Clip_AABB(v3 P0, v3 P1, aabb Box, v3* Out0, v3* Out1) {
    v3 Delta = V3_Sub_V3(P1, P0);
    f32 TMin = 0.0f;
    f32 TMax = 1.0f;
    for(u32 Axis = 0; Axis < 3; Axis++) {
        f32 DeltaAxis = Delta.Data[Axis];
        f32 P0Axis = P0.Data[Axis];
        f32 MinAxis = Box.Min.Data[Axis];
        f32 MaxAxis = Box.Max.Data[Axis];
        if(Abs(DeltaAxis) < 1.0e-8f) {
            if(P0Axis < MinAxis || P0Axis > MaxAxis) {
                return false;
            }
            continue;
        }
        f32 Inv = 1.0f / DeltaAxis;
        f32 T0 = (MinAxis - P0Axis) * Inv;
        f32 T1 = (MaxAxis - P0Axis) * Inv;
        if(T0 > T1) {
            f32 Temp = T0;
            T0 = T1;
            T1 = Temp;
        }
        TMin = Max(TMin, T0);
        TMax = Min(TMax, T1);
        if(TMin > TMax) {
            return false;
        }
    }
    *Out0 = V3_Add_V3(P0, V3_Mul_S(Delta, TMin));
    *Out1 = V3_Add_V3(P0, V3_Mul_S(Delta, TMax));
    return true;
}
