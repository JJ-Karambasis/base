export_function ray_hit Ray_Intersect_Cylinder_Segment(const ray* Ray, v3 SegA, v3 SegB, f32 PickRadius) {
	ray_hit Result = { false, 0.0f };
	v3 Origin = Ray->Origin;
	v3 Dir = Ray->Direction;
	v3 D = SegB - SegA;
	f32 SegLen = V3_Mag(D);
	if(SegLen <= 1e-6f) return Result;

	v3 U = D / SegLen;
	v3 W = Origin - SegA;
	f32 b = V3_Dot(Dir, U);
	f32 d = V3_Dot(Dir, W);
	f32 e = V3_Dot(U, W);
	f32 denom = 1.0f - b * b;

	f32 RayT, SegT;
	if(denom > 1e-6f) {
		RayT = (b * e - d) / denom;
		SegT = (e - b * d) / denom;
	} else {
		RayT = -d;
		SegT = e;
	}
	if(RayT < 0.0f) return Result;

	f32 ClampedSegT = Clamp(0.0f, SegT, SegLen);
	v3 P = Origin + Dir * RayT;
	v3 Q = SegA + U * ClampedSegT;
	f32 DistSq = V3_Sq_Mag(P - Q);
	if(DistSq > PickRadius * PickRadius) return Result;

	Result.Hit = true;
	Result.T = RayT;
	return Result;
}

export_function ray_hit Ray_Intersect_Plane(const ray* Ray, v3 PlaneOrigin, v3 PlaneNormal) {
	ray_hit Result = { false, 0.0f };
	v3 Origin = Ray->Origin;
	v3 Dir = Ray->Direction;
	f32 Denom = V3_Dot(Dir, PlaneNormal);
	if(Abs(Denom) < 1e-6f) return Result;
	f32 T = V3_Dot(PlaneOrigin - Origin, PlaneNormal) / Denom;
	if(T < 0.0f) return Result;
	Result.Hit = true;
	Result.T = T;
	return Result;
}

export_function ray_hit Ray_Intersect_Ring(const ray* Ray, v3 Center, v3 Normal, f32 Radius, f32 PickRadius) {
	ray_hit Result = { false, 0.0f };
	ray_hit PHit = Ray_Intersect_Plane(Ray, Center, Normal);
	if(!PHit.Hit) return Result;
	v3 Point = Ray->Origin + Ray->Direction * PHit.T;
	v3 Local = Point - Center;
	f32 DistFromCenter = V3_Mag(Local);
	if(Abs(DistFromCenter - Radius) > PickRadius) return Result;
	Result.Hit = true;
	Result.T = PHit.T;
	return Result;
}

export_function ray_hit Ray_Intersect_AABB(const ray* Ray, v3 Min, v3 Max) {
    ray_hit Result = { false, 0.0f };
    f32 OComp[3] = { Ray->Origin.x,    Ray->Origin.y,    Ray->Origin.z    };
    f32 DComp[3] = { Ray->Direction.x, Ray->Direction.y, Ray->Direction.z };
    f32 MnComp[3] = { Min.x, Min.y, Min.z };
    f32 MxComp[3] = { Max.x, Max.y, Max.z };
    f32 TMin = -1e30f;
    f32 TMax =  1e30f;
    for(int i = 0; i < 3; i++) {
        if(Abs(DComp[i]) < 1e-6f) {
            if(OComp[i] < MnComp[i] || OComp[i] > MxComp[i]) return Result;
        } else {
            f32 Inv = 1.0f / DComp[i];
            f32 T1 = (MnComp[i] - OComp[i]) * Inv;
            f32 T2 = (MxComp[i] - OComp[i]) * Inv;
            if(T1 > T2) { f32 Tmp = T1; T1 = T2; T2 = Tmp; }
            TMin = Max(TMin, T1);
            TMax = Min(TMax, T2);
            if(TMax < TMin) return Result;
        }
    }
    if(TMax < 0.0f) return Result;
    Result.Hit = true;
    Result.T = TMin >= 0.0f ? TMin : TMax;
    return Result;
}

export_function ray_hit Ray_Intersect_AABB_Uniform(const ray* Ray, v3 Center, f32 Size) {
	f32 H = Size * 0.5f;
	v3 Min = Center - V3_All(H);
	v3 Max = Center + V3_All(H);
	return Ray_Intersect_AABB(Ray, Min, Max);
}

export_function b32 Ray_Project_To_Axis(const ray* Ray, v3 LineOrigin, v3 Axis, f32* OutT) {
	v3 Origin = Ray->Origin;
	v3 Dir = Ray->Direction;
	v3 W = Origin - LineOrigin;
	f32 b = V3_Dot(Dir, Axis);
	f32 d = V3_Dot(Dir, W);
	f32 e = V3_Dot(Axis, W);
	f32 denom = 1.0f - b * b;
	if(denom < 1e-6f) return false;
	*OutT = (e - b * d) / denom;
	return true;
}
