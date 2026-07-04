#ifndef AABB_H
#define AABB_H

struct aabb {
    v3 Min;
    v3 Max;
};

export_function aabb Make_AABB_From_Points(const v3* Points, u32 PointCount);
export_function aabb Transform_AABB(const aabb* LocalBounds, const m4_affine* Transform);

#endif // AABB_H
