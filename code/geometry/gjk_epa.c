typedef struct {
	v3 P;
} gjk_point;

typedef struct {
	f32 Radius;
} gjk_radius;

typedef struct {
	v3 CenterP;
	f32 Radius;
} gjk_sphere;

typedef struct {
	v3 HalfExtent;
} gjk_extent;

typedef struct {
	v3 Min;
	v3 Max;
} gjk_aabb;

typedef struct {
    gjk_support Support;
    f32 Radius;
} gjk_add_radius;

typedef struct {
	gjk_support Support;
	m4 Transform;
	m3 InvTransform;
} gjk_transform;

function b32 Get_Line_Barycentric_From_Origin(v3 p0, v3 p1, f32* OutU, f32* OutV) {
	v3 ab = V3_Sub_V3(p1, p0);
	f32 Denominator = V3_Sq_Mag(ab);
	if (Denominator < Sq(FLT_EPSILON)) {
		//Degenerate line segment. Check which point is the closest
		if (V3_Sq_Mag(p0) < V3_Sq_Mag(p1)) {
			*OutU = 1.0f;
			*OutV = 0.0f;
		} else {
			*OutU = 0.0f;
			*OutV = 1.0f;
		}
        
		return false;
	} else {
		*OutV = V3_Dot(V3_Negate(p0), ab) / Denominator;
		*OutU = 1.0f - *OutV;
	}
	return true;
}

function b32 Get_Triangle_Barycentric_From_Origin(v3 p0, v3 p1, v3 p2, f32* OutU, f32* OutV, f32* OutW) {
	// With p being 0, we can improve the numerical stability by including the shortest edge of the triangle 
	// in the calculation. 
	// see: Real-Time Collision Detection - Christer Ericson (Section: Barycentric Coordinates)
	//First calculate the 3 edges
	v3 e0 = V3_Sub_V3(p1, p0);
	v3 e1 = V3_Sub_V3(p2, p0);
	v3 e2 = V3_Sub_V3(p2, p1);
	f32 d00 = V3_Sq_Mag(e0);
	f32 d11 = V3_Sq_Mag(e1);
	f32 d22 = V3_Sq_Mag(e2);
	if (d00 <= d22) {
		f32 d01 = V3_Dot(e0, e1);
		f32 Denominator = d00 * d11 - Sq(d01);
		if (Denominator < 1.0e-12f) {
			//Degenerate triangle, get the line barycentric coordinate instead
			if (d00 > d11) {
				Get_Line_Barycentric_From_Origin(p0, p1, OutU, OutV);
				*OutW = 0.0;
			} else {
				Get_Line_Barycentric_From_Origin(p0, p2, OutU, OutW);
				*OutV = 0.0;
			}
			return false;
		} else {
			f32 a0 = V3_Dot(p0, e0);
			f32 a1 = V3_Dot(p0, e1);
			*OutV = (d01 * a1-d11*a0) / Denominator;
			*OutW = (d01 * a0-d00*a1) / Denominator;
			*OutU = 1.0f - *OutV - *OutW;
		}
	} else {
		f32 d12 = V3_Dot(e1, e2);
		f32 Denominator = d11 * d22 - Sq(d12);
		if (Denominator < 1.0e-12f) {
			//Degenerate triangle, get the line barycentric coordinate instead
			if (d11 > d22) {
				Get_Line_Barycentric_From_Origin(p0, p2, OutU, OutW);
				*OutV = 0.0f;
			} else {
				Get_Line_Barycentric_From_Origin(p1, p2, OutV, OutW);
				*OutU = 0.0f;
			}
			return false;
		} else {
			f32 c1 = V3_Dot(p2, e1);
			f32 c2 = V3_Dot(p2, e2);
			*OutU = (d22 * c1-d12*c2) / Denominator;
			*OutV = (d11 * c2-d12*c1) / Denominator;
			*OutW = 1.0f - *OutU - *OutV;
		}
	}
	return true;
}

function v3 Closest_Point_From_Line_To_Origin_Set(v3 p0, v3 p1, u32* OutSet) {
	f32 u, v;
	Get_Line_Barycentric_From_Origin(p0, p1, &u, &v);
	if (v <= 0.0f) {
		*OutSet = 0b0001;
		return p0;
	} else if (u <= 0.0f) {
		*OutSet = 0b0010;
		return p1;
	} else {
		*OutSet = 0b0011;
		return V3_Add_V3(V3_Mul_S(p0, u), V3_Mul_S(p1, v));
	}
}

function v3 Closest_Point_From_Triangle_To_Origin_Set(v3 p0, v3 p1, v3 p2, b32 MustIncludeC, u32* OutSet) {
	//First we need to check if our triangle is a degenerate triangle. We can detect this using the normal
	//but wee ideally want to calculate the normal  with the smallest precision loss possible. We check if 
	//edge ac is smaller than bc and if its not, we swap ac and bc
	//see: https://box2d.org/posts/2014/01/troublesome-triangle/
	v3 ac = V3_Sub_V3(p2, p0);
	v3 bc = V3_Sub_V3(p2, p1);
	b32 ShouldSwapAC = V3_Sq_Mag(bc) < V3_Sq_Mag(ac);
	v3 a = ShouldSwapAC ? p2 : p0;
	v3 c = ShouldSwapAC ? p0 : p2;
	v3 b = p1;
	//Calculate the normal
	v3 ab = V3_Sub_V3(b, a);
	ac = V3_Sub_V3(c, a);
	v3 n = V3_Cross(ab, ac);
	f32 NLenSq = V3_Sq_Mag(n);
	u32 Set = 0;
	if (NLenSq < 1e-11f) {
		//Degenerate case on vertices and edges
		//Start with vertex C being the closest point
		Set = 0b0100;
		v3 BestPoint = p2;
		f32 BestDistSq = V3_Sq_Mag(p2);
		//A or B cannot be the closest if we must include c
		if (!MustIncludeC) {
			//Test if vertex A is closer
			f32 ADistSq = V3_Sq_Mag(p0);
			if (ADistSq < BestDistSq) {
				Set = 0b0001;
				BestPoint = p0;
				BestDistSq = ADistSq;
			}
			//Test if vertex B is closer
			f32 BDistSq = V3_Sq_Mag(p1);
			if (BDistSq < BestDistSq) {
				Set = 0b0010;
				BestPoint = p1;
				BestDistSq = BDistSq;
			}
		}
		//Check if the edge AC is closer
		f32 ACDistSq = V3_Sq_Mag(ac);
		if (ACDistSq > Sq(FLT_EPSILON)) {
			f32 v = Clamp(0.0f, -V3_Dot(a, ac) / ACDistSq, 1.0f);
			v3 q = V3_Add_V3(a, V3_Mul_S(ac, v));
			f32 DistSq = V3_Sq_Mag(q);
			if (DistSq < BestDistSq) {
				Set = 0b0101;
				BestPoint = q;
				BestDistSq = DistSq;
			}
		}
		//Check if the edge BC is closer
		v3 bc = V3_Sub_V3(p2, p1);
		f32 BCDistSq = V3_Sq_Mag(bc);
		if (BCDistSq > Sq(FLT_EPSILON)) {
			f32 v = Clamp(0.0f, -V3_Dot(b, bc) / BCDistSq, 1.0f);
			v3 q = V3_Add_V3(p1, V3_Mul_S(bc, v));
			f32 DistSq = V3_Sq_Mag(q);
			if (DistSq < BestDistSq) {
				Set = 0b0110;
				BestPoint = q;
				BestDistSq = DistSq;
			}
		}
		//If we must include C then AB cannot be the closest
		if (!MustIncludeC) {
			//Check if the edge AB is closer
			v3 ab = V3_Sub_V3(p1, p0);
			f32 ABDistSq = V3_Sq_Mag(ab);
			if (ABDistSq > Sq(FLT_EPSILON)) {
				f32 v = Clamp(0.0f, -V3_Dot(p0, ab) / ABDistSq, 1.0f);
				v3 q = V3_Add_V3(p0, V3_Mul_S(ab, v));
				f32 DistSq = V3_Sq_Mag(q);
				if (DistSq < BestDistSq) {
					Set = 0b0011;
					BestPoint = q;
					BestDistSq = DistSq;
				}
			}
		}
		
		*OutSet = Set;
		return BestPoint;
	}
	//Normal triangle cases. Detect the 8 regions and see which feature includes the closest point
	//Vertex region A
	v3 ap = V3_Negate(a);
	f32 d1 = V3_Dot(ab, ap);
	f32 d2 = V3_Dot(ac, ap);
	if (d1 <= 0.0f && d2 <= 0.0f) {
		*OutSet = ShouldSwapAC ? 0b0100 : 0b0001;
		return a; //BC: (1, 0, 0)
	}
	//Vertex region B
	v3 bp = V3_Negate(p1);
	f32 d3 = V3_Dot(ab, bp);
	f32 d4 = V3_Dot(ac, bp);
	if (d3 >= 0.0f && d4 <= d3) {
		*OutSet = 0b0010;
		return p1; //BC: (0, 1, 0)
	}
	//Edge region AB. If its the best region project the origin onto AB
	if (d1 * d4 <= d3 * d2 && d1 >= 0.0f && d3 <= 0.0f) {
		f32 v = d1 / (d1 - d3);
		*OutSet = ShouldSwapAC ? 0b0110 : 0b0011;
		return V3_Add_V3(a, V3_Mul_S(ab, v));  //BC: (1-v, v, 0)
	}
	//Vertex region C
	v3 cp = V3_Negate(c);
	f32 d5 = V3_Dot(ab, cp);
	f32 d6 = V3_Dot(ac, cp);
	if (d6 >= 0.0f && d5 <= d6) {
		*OutSet = ShouldSwapAC ? 0b0001 : 0b0100;
		return c; //BC: (0, 0, 1)
	}
	//Edge region AC. If its the best region project the origin onto AC
	if (d5 * d2 <= d1 * d6 && d2 >= 0.0f && d6 <= 0.0f) {
		f32 w = d2 / (d2 - d6);
		*OutSet = 0b0101;
		return V3_Add_V3(a, V3_Mul_S(ac, w)); //BC: (1-w, 0, w)
	}
	//Edge region BC. If its the best region project the origin onto BC
	f32 d4_d3 = d4 - d3;
	f32 d5_d6 = d5 - d6;
	if (d3 * d6 <= d5 * d4 && d4_d3 >= 0.0f && d5_d6 >= 0.0f) {
		f32 w = d4_d3 / (d4_d3 + d5_d6);
		*OutSet = ShouldSwapAC ? 0b0011 : 0b0110;
		return V3_Add_V3(b, V3_Mul_S(V3_Sub_V3(c, b), w)); //BC: (0, 1-w, w)
	}
	//The triangle face is the best region. Project the origin onto the face
	*OutSet = 0b0111;
	return V3_Mul_S(V3_Mul_S(n, V3_Dot(V3_Add_V3(V3_Add_V3(a, b), c), n)), (1.0f / (3.0f * NLenSq)));
}

function void Check_If_Origin_Is_Outside_Of_Tetrahedron_Planes(v3 p0, v3 p1, v3 p2, v3 p3, b32* Results) {
	// see: Real-Time Collision Detection - Christer Ericson (Section: Closest Point on Tetrahedron to Point)
	// With p = 0
	
	v3 ab = V3_Sub_V3(p1, p0);
	v3 ac = V3_Sub_V3(p2, p0);
	v3 ad = V3_Sub_V3(p3, p0);
	v3 bd = V3_Sub_V3(p3, p1);
	v3 bc = V3_Sub_V3(p2, p1);
	v3 ab_cross_ac = V3_Cross(ab, ac);
	v3 ac_cross_ad = V3_Cross(ac, ad);
	v3 ad_cross_ab = V3_Cross(ad, ab);
	v3 bd_cross_bc = V3_Cross(bd, bc);
	// Get which side the origin is on for each plane 
	f32 SignP[4] = {
		V3_Dot(p0, ab_cross_ac), //Plane ABC
		V3_Dot(p0, ac_cross_ad), //Plane ACD
		V3_Dot(p0, ad_cross_ab), //Plane ADB
		V3_Dot(p1, bd_cross_bc)  //Plane BDC
	};
	// Get the fourth component of the planes
	f32 SignD[4] = {
		V3_Dot(ad, ab_cross_ac),
		V3_Dot(ab, ac_cross_ad),
		V3_Dot(ac, ad_cross_ab),
		-V3_Dot(ab, bd_cross_bc)
	};
	//Determine if the sign of all triangle components are the same. If not it is a degenerate
	//case where the origin is in front of all sides
	u32 SignBits = 0;
	SignBits = ((((SignD[0] < 0) ? 0xFF : 0x00) << 0) |
                (((SignD[1] < 0) ? 0xFF : 0x00) << 8) |
                (((SignD[2] < 0) ? 0xFF : 0x00) << 16) |
                (((SignD[3] < 0) ? 0xFF : 0x00) << 24));
	switch (SignBits) {
		case 0x00000000: {
			Results[0] = SignP[0] >= -FLT_EPSILON;
			Results[1] = SignP[1] >= -FLT_EPSILON;
			Results[2] = SignP[2] >= -FLT_EPSILON;
			Results[3] = SignP[3] >= -FLT_EPSILON;
		} break;
		case 0xFFFFFFFF: {
			Results[0] = SignP[0] <= FLT_EPSILON;
			Results[1] = SignP[1] <= FLT_EPSILON;
			Results[2] = SignP[2] <= FLT_EPSILON;
			Results[3] = SignP[3] <= FLT_EPSILON;
		} break;
		default: {
			Results[0] = true;
			Results[1] = true;
			Results[2] = true;
			Results[3] = true;
		} break;
	}
}

function v3 Closest_Point_From_Tetrahedron_To_Origin_Set(v3 p0, v3 p1, v3 p2, v3 p3, b32 MustIncludeD, u32* OutSet) {
	// see: Real-Time Collision Detection - Christer Ericson (Section: Closest Point on Tetrahedron to Point)
	// With p = 0
	
	u32 ClosestSet = 0b1111;
	v3 BestPoint = V3_Zero();
	f32 BestDistSq = FLT_MAX;
	b32 OriginPlaneCheck[4];
	Check_If_Origin_Is_Outside_Of_Tetrahedron_Planes(p0, p1, p2, p3, OriginPlaneCheck);
	//Test against plane abc
	if (OriginPlaneCheck[0]) {
		if (MustIncludeD) {
			//If the closest point must include D then ABC cannot be closest but the closest point
			//cannot be an interior point either so we return A as closest point
			ClosestSet = 0b0001;
			BestPoint = p0;
		} else {
			//Test the face normally
			BestPoint = Closest_Point_From_Triangle_To_Origin_Set(p0, p1, p2, false, &ClosestSet);
		}
		BestDistSq = V3_Sq_Mag(BestPoint);
	}
	//Test against plane acd
	if (OriginPlaneCheck[1]) {
		u32 TempSet;
		v3 TempPoint = Closest_Point_From_Triangle_To_Origin_Set(p0, p2, p3, MustIncludeD, &TempSet);
		f32 DistSq = V3_Sq_Mag(TempPoint);
		if (DistSq < BestDistSq) {
			BestDistSq = DistSq;
			BestPoint = TempPoint;
			ClosestSet = (TempSet & 0b0001) + ((TempSet & 0b0110) << 1);
		}
	}
    
	//Test against plane adb
	if (OriginPlaneCheck[2]) {
		u32 TempSet;
		v3 TempPoint = Closest_Point_From_Triangle_To_Origin_Set(p0, p1, p3, MustIncludeD, &TempSet);
		f32 DistSq = V3_Sq_Mag(TempPoint);
		if (DistSq < BestDistSq) {
			BestDistSq = DistSq;
			BestPoint = TempPoint;
			ClosestSet = (TempSet & 0b0011) + ((TempSet & 0b0100) << 1);
		}
	}
    
	//Test against plane bdc
	if (OriginPlaneCheck[3]) {
		u32 TempSet;
		v3 TempPoint = Closest_Point_From_Triangle_To_Origin_Set(p1, p2, p3, MustIncludeD, &TempSet);
		f32 DistSq = V3_Sq_Mag(TempPoint);
		if (DistSq < BestDistSq) {
			BestDistSq = DistSq;
			BestPoint = TempPoint;
			ClosestSet = TempSet << 1;
		}
	}
    
	*OutSet = ClosestSet;
	return BestPoint;
}

function v3 GJK_Get_Support(gjk_support* Support, v3 Direction) {
	v3 Result = Support->SupportFunc(Support->UserData, Direction);
	return Result;
}

function GJK_SUPPORT_FUNC(GJK_Origin_Support) {
	return V3_Zero();
}

function GJK_SUPPORT_FUNC(GJK_Point_Support) {
	gjk_point* Point = (gjk_point *)UserData;
	return Point->P;
}

function GJK_SUPPORT_FUNC(GJK_Radius_Support) {
	gjk_radius* Radius = (gjk_radius *)UserData;
	f32 Length = V3_Mag(Direction);
	return Length > 0 ? V3_Mul_S(Direction, Radius->Radius / Length) : V3_Zero();
}

function GJK_SUPPORT_FUNC(GJK_Sphere_Support) {
	gjk_sphere* Sphere = (gjk_sphere *)UserData;
	f32 Length = V3_Mag(Direction);
	return Length > 0 ? V3_Add_V3(Sphere->CenterP, V3_Mul_S(Direction, Sphere->Radius / Length)) : Sphere->CenterP;
}

function GJK_SUPPORT_FUNC(GJK_Extent_Support) {
	gjk_extent* Extent = (gjk_extent *)UserData;
    
	v3 Result = {
		Direction.x < 0.0f ? -Extent->HalfExtent.x : Extent->HalfExtent.x,
		Direction.y < 0.0f ? -Extent->HalfExtent.y : Extent->HalfExtent.y,
		Direction.z < 0.0f ? -Extent->HalfExtent.z : Extent->HalfExtent.z
	};
    
	return Result;
}

function GJK_SUPPORT_FUNC(GJK_AABB_Support) {
	gjk_aabb* AABB = (gjk_aabb *)UserData;
    
	v3 Result = {
		Direction.x < 0.0f ? AABB->Min.x : AABB->Max.x,
		Direction.y < 0.0f ? AABB->Min.y : AABB->Max.y,
		Direction.z < 0.0f ? AABB->Min.z : AABB->Max.z
	};
    
	return Result;
}

function GJK_SUPPORT_FUNC(GJK_Add_Radius_Support) {
    gjk_add_radius* AddRadius = (gjk_add_radius*)UserData;
    f32 Length = V3_Mag(Direction);
    v3 P = GJK_Get_Support(&AddRadius->Support, Direction);
    return Length > 0.0f ? V3_Add_V3(P, V3_Mul_S(Direction, AddRadius->Radius/Length)) : P;
}

function GJK_SUPPORT_FUNC(GJK_Transform_Support) {
	gjk_transform* Transform = (gjk_transform *)UserData;
	v3 P = GJK_Get_Support(&Transform->Support, V3_Mul_M3(Direction, &Transform->InvTransform));
	return V4_Mul_M4(V4_From_V3(P, 1.0f), &Transform->Transform).xyz;
}

export_function gjk_support GJK_Make_Support(gjk_support_func* Func, void* UserData) {
	gjk_support Result = {
		UserData,
		Func
	};
	return Result;
}

export_function gjk_support GJK_Origin() {
	gjk_support Support = GJK_Make_Support(GJK_Origin_Support, NULL);
	return Support;
}

export_function gjk_support GJK_Point(arena* Arena, v3 P) {
	gjk_point* Point = Arena_Push_Struct(Arena, gjk_point);
	Point->P = P;
	
	gjk_support Support = GJK_Make_Support(GJK_Point_Support, Point);
	return Support;
}

export_function gjk_support GJK_Radius(arena* Arena, f32 Radius) {
	gjk_radius* GJKRadius = Arena_Push_Struct(Arena, gjk_radius);
	GJKRadius->Radius = Radius;
    
	gjk_support Support = GJK_Make_Support(GJK_Radius_Support, GJKRadius);
	return Support;
}

export_function gjk_support GJK_Sphere(arena* Arena, v3 P, f32 Radius) {
	gjk_sphere* Sphere = Arena_Push_Struct(Arena, gjk_sphere);
	Sphere->Radius = Radius;
	Sphere->CenterP = P;
    
	gjk_support Support = GJK_Make_Support(GJK_Sphere_Support, Sphere);
	return Support;
}

export_function gjk_support GJK_Extent(arena* Arena, v3 HalfExtent) {
	gjk_extent* Extent = Arena_Push_Struct(Arena, gjk_extent);
	Extent->HalfExtent = HalfExtent;
    
	gjk_support Support = GJK_Make_Support(GJK_Extent_Support, Extent);
	return Support;
}

export_function gjk_support GJK_AABB(arena* Arena, v3 Min, v3 Max) {
	gjk_aabb* AABB = Arena_Push_Struct(Arena, gjk_aabb);
	AABB->Min = Min;
	AABB->Max = Max;
    
	gjk_support Support = GJK_Make_Support(GJK_AABB_Support, AABB);
	return Support;
}

export_function gjk_support GJK_Add_Radius(arena* Arena, gjk_support InnerSupport, f32 Radius) {
    gjk_add_radius* AddRadius = Arena_Push_Struct(Arena, gjk_add_radius);
    AddRadius->Support = InnerSupport;
    AddRadius->Radius = Radius;
    
    gjk_support Support = GJK_Make_Support(GJK_Add_Radius_Support, AddRadius);
    return Support;
}

export_function gjk_support GJK_Transform(arena* Arena, gjk_support InnerSupport, const m4* Matrix) {
	gjk_transform* Transform = Arena_Push_Struct(Arena, gjk_transform);
	Transform->Support = InnerSupport;
	Transform->Transform = *Matrix;
    
	m3 M = M3_From_M4(&Transform->Transform);
	Transform->InvTransform = M3_Transpose(&M);
    
	gjk_support Support = GJK_Make_Support(GJK_Transform_Support, Transform);
	return Support;
}

function f32 GJK_Max_P_Dist_Sq(gjk_simplex* Simplex) {
	f32 BestDistSq = V3_Sq_Mag(Simplex->P[0]);
	for (u32 i = 1; i < Simplex->Count; i++) {
		BestDistSq = Max(BestDistSq, V3_Sq_Mag(Simplex->P[i]));
	}
	return BestDistSq;
}

function void GJK_Update_P(gjk_simplex* Simplex, u32 Set) {
	u32 Count = 0;
	for (u32 i = 0; i < Simplex->Count; i++) {
		if ((Set & (1 << i)) != 0) {
			Simplex->P[Count] = Simplex->P[i];
			Count++;
		}
	}
	Simplex->Count = Count;
}

function void GJK_Update_AB(gjk_simplex* Simplex, u32 Set) {
	u32 Count = 0;
	for (u32 i = 0; i < Simplex->Count; i++) {
		if ((Set & (1 << i)) != 0) {
			Simplex->A[Count] = Simplex->A[i];
			Simplex->B[Count] = Simplex->B[i];
			Count++;
		}
	}
	Simplex->Count = Count;
}

function void GJK_Update_PAB(gjk_simplex* Simplex, u32 Set) {
	u32 Count = 0;
	for (u32 i = 0; i < Simplex->Count; i++) {
		if ((Set & (1 << i)) != 0) {
			Simplex->P[Count] = Simplex->P[i];
			Simplex->A[Count] = Simplex->A[i];
			Simplex->B[Count] = Simplex->B[i];
			Count++;
		}
	}
	Simplex->Count = Count;
}

function void GJK_Calculate_Closest_Points(gjk_simplex* Simplex, v3* OutA, v3* OutB) {
	switch (Simplex->Count) {
		case 1: {
			*OutA = Simplex->A[0];
			*OutB = Simplex->B[0];
		} break;
        
		case 2: {
			f32 u, v;
			Get_Line_Barycentric_From_Origin(Simplex->P[0], Simplex->P[1], &u, &v);
			*OutA = V3_Add_V3(V3_Mul_S(Simplex->A[0], u), V3_Mul_S(Simplex->A[1], v));
			*OutB = V3_Add_V3(V3_Mul_S(Simplex->B[0], u), V3_Mul_S(Simplex->B[1], v));
		} break;
        
		case 3: {
			f32 u, v, w;
			Get_Triangle_Barycentric_From_Origin(Simplex->P[0], Simplex->P[1], Simplex->P[2], &u, &v, &w);
			*OutA = V3_Add_V3(V3_Add_V3(V3_Mul_S(Simplex->A[0], u), V3_Mul_S(Simplex->A[1], v)), V3_Mul_S(Simplex->A[2], w));
			*OutB = V3_Add_V3(V3_Add_V3(V3_Mul_S(Simplex->B[0], u), V3_Mul_S(Simplex->B[1], v)), V3_Mul_S(Simplex->B[2], w));
		} break;
	}
}

function b32 GJK_Get_Closest_Point(gjk_simplex* Simplex, f32 PrevDistSq, v3* OutV, f32* OutDistSq, 
                                   u32* OutSet, b32 IncludeLastFeature) {
	u32 Set;
	v3 V = { 0, 0, 0 };
    
	switch (Simplex->Count) {
		case 1: {
			//Single point case
			Set = 0b0001;
			V = Simplex->P[0];
		} break;
        
		case 2: {
			//Line segment case
			V = Closest_Point_From_Line_To_Origin_Set(Simplex->P[0], Simplex->P[1], &Set);
		} break;
        
		case 3: {
			//Triangle case
			V = Closest_Point_From_Triangle_To_Origin_Set(Simplex->P[0], Simplex->P[1], Simplex->P[2], IncludeLastFeature, &Set);
		} break;
        
		case 4: {
			//Tetrahedron case
			V = Closest_Point_From_Tetrahedron_To_Origin_Set(Simplex->P[0], Simplex->P[1], Simplex->P[2], Simplex->P[3], IncludeLastFeature, &Set);
		} break;
        
		default: {
			Assert(false);
			return false; //Prevent compiler warnings
		} break;
	}
    
	//Make sure the closest point is actually closer than the previous point. If its not, we are not converging
	//and should exit out of the gjk iterations
	f32 DistSq = V3_Sq_Mag(V);
	if (DistSq <= PrevDistSq) {
		*OutV = V;
		*OutDistSq = DistSq;
		*OutSet = Set;
		return true;
	}
    
	return false;
}

export_function b32 GJK_Intersects(gjk_support* SupportA, gjk_support* SupportB, f32 Tolerance, v3* InOutV) {
	f32 ToleranceSq = Sq(Tolerance);
    
	gjk_simplex Simplex = { 0 };
    
	//Used to make sure v is converging closer to the origin
	f32 PrevDistSq = FLT_MAX;
    
	for (;;) {
		
		//Iteration always starts with getting the support points on each object
		//in the direction of V. Then subtracting the two (aka the Minkowski difference)
		v3 a = GJK_Get_Support(SupportA, *InOutV);
		v3 b = GJK_Get_Support(SupportB, V3_Negate(*InOutV));
		v3 p = V3_Sub_V3(a, b);
        
		//Check if the support point is in the opposite direction of p, if it is we found a 
		//separating axis
		if (V3_Dot(*InOutV, p) < 0.0f) {
			return false;
		}
        
		//Add P to the simplex
		Simplex.P[Simplex.Count] = p;
		Simplex.Count++;
        
		f32 DistSq;
		u32 Set;
        
		//Get the new closest point 
		if (!GJK_Get_Closest_Point(&Simplex, PrevDistSq, InOutV, &DistSq, &Set, true)) {
			return false;
		}
        
		//If there are 4 points in the simplex, the origin is inside the tetrahedron and 
		//we have intersected
		if (Set == 0xf) {
			*InOutV = V3_Zero();
			return true;
		}
        
		//If V is extremely close to zero, we consider this a collision
		if (DistSq <= ToleranceSq) {
			*InOutV = V3_Zero();
			return true;
		}
        
		//If V is very small compared to the length of P in the simplex, this is also a collision
		if (DistSq <= FLT_EPSILON*GJK_Max_P_Dist_Sq(&Simplex)) {
			*InOutV = V3_Zero();
			return true;
		}
        
		*InOutV = V3_Negate(*InOutV);
        
		//If V is not converging and going closer to the origin on every iteration, we are not changing
		//and will not find a valid solution. Therefore there is no coliision possible
		Assert(PrevDistSq >= DistSq);
		if (PrevDistSq - DistSq <= FLT_EPSILON*PrevDistSq) {
			return false;
		}
        
		PrevDistSq = DistSq;
        
		GJK_Update_P(&Simplex, Set);
	}
}

export_function closest_points GJK_Get_Closest_Points(gjk_support* SupportA, gjk_support* SupportB, 
                                                      f32 Tolerance, f32 MaxDistSq, v3* InOutV, 
                                                      gjk_simplex* OutSimplex) {
	f32 ToleranceSq = Sq(Tolerance);
    
	gjk_simplex Simplex = { 0 };
    
	f32 DistSq = V3_Sq_Mag(*InOutV);
	f32 PrevDistSq = FLT_MAX;
    
	for (;;) {
		//Iteration always starts with getting the support points on each object
		//in the direction of V. Then subtracting the two (aka the Minkowski difference)
		v3 a = GJK_Get_Support(SupportA, *InOutV);
		v3 b = GJK_Get_Support(SupportB, V3_Negate(*InOutV));
		v3 p = V3_Sub_V3(a, b);
        
		
		//Check if we have a separating that is greater than MaxDistSq. If we do we can terminate early
		f32 Dot = V3_Dot(*InOutV, p);
		if (Dot < 0.0f && Sq(Dot) > DistSq * MaxDistSq) {
			closest_points Result;
			Result.DistSq = FLT_MAX;
			return Result;
		}
        
		//Add P and the support points to the simplex. Support points are used to calculate the closest
		//points at the end of the algorithm
		Simplex.P[Simplex.Count] = p;
		Simplex.A[Simplex.Count] = a;
		Simplex.B[Simplex.Count] = b;
		Simplex.Count++;
        
		u32 Set;
		if (!GJK_Get_Closest_Point(&Simplex, PrevDistSq, InOutV, &DistSq, &Set, true)) {
			Simplex.Count--;
			break;
		}
        
		//If there are 4 points in the simplex, the origin is inside the tetrahedron and 
		//we have intersected
		if (Set == 0xF) {
			*InOutV = V3_Zero();
			DistSq = 0.0f;
			break;
		}
        
		GJK_Update_PAB(&Simplex, Set);
        
		//If V is extremely close to zero, we consider this a collision
		if (DistSq <= ToleranceSq) {
			*InOutV = V3_Zero();
			DistSq = 0.0f;
			break;
		}
        
		//If V is very small compared to the length of P in the simplex, this is also a collision
		if (DistSq <= FLT_EPSILON*GJK_Max_P_Dist_Sq(&Simplex)) {
			*InOutV = V3_Zero();
			DistSq = 0.0f;
			break;
		}
        
		*InOutV = V3_Negate(*InOutV);
        
		//If V is not converging and going closer to the origin on every iteration, we are not changing
		//and will not find a valid solution. Therefore there is no coliision possible
		Assert(PrevDistSq >= DistSq);
		if (PrevDistSq - DistSq <= FLT_EPSILON*PrevDistSq) {
			break;
		}
        
		PrevDistSq = DistSq;
	}
    
	closest_points Result = { 0 };
	Result.DistSq = DistSq;
	GJK_Calculate_Closest_Points(&Simplex, &Result.P1, &Result.P2);
    
	if (OutSimplex) *OutSimplex = Simplex;
    
	return Result;
}

export_function ray_cast GJK_Cast(gjk_support* SupportA, f32 RadiusA, gjk_support* SupportB, f32 RadiusB, 
                                  const m4* Start, v3 Direction, f32 tDistance, f32 Tolerance, gjk_simplex* OutSimplex) {
    arena* Scratch = Scratch_Get();
    f32 ToleranceSq = Sq(Tolerance);
    
    f32 SumRadius = RadiusA+RadiusB;
    
    gjk_support TransformA = GJK_Transform(Scratch, *SupportA, Start);
    
	gjk_simplex Simplex = { 0 };
    
    ray_cast Result = {0};
    Result.tHit = FLT_MAX;
    
    f32 Lambda = 0.0f;
    v3 x = V3_Zero();
    v3 v = V3_Add_V3(V3_Negate(GJK_Get_Support(SupportB, V3_Zero())),
                     GJK_Get_Support(&TransformA, V3_Zero()));
    v3 vPrev = V3_Zero();
	f32 DistSq = FLT_MAX;
    b32 AllowRestart = false;
    
    for(;;) {
        
        v3 a = GJK_Get_Support(&TransformA, V3_Negate(v));
        v3 b = GJK_Get_Support(SupportB, v);
        v3 w = V3_Sub_V3(x, V3_Sub_V3(b, a));
        
        f32 VDotW = V3_Dot(v, w) - SumRadius * V3_Mag(v);
        if(VDotW > 0.0f) {
            f32 VDotR = V3_Dot(v, Direction);
            
            // Instead of checking >= 0, check with epsilon as we don't want the 
            // division below to overflow to infinity as it can cause a float exception
            if(VDotR >= -1.0e-18f) { Scratch_Release(); return Result; }
            
            f32 Delta = VDotW/VDotR;
            f32 OldLambda = Lambda;
            Lambda -= Delta;
            
            if(OldLambda == Lambda) break;
            
            if(Lambda >= tDistance) { Scratch_Release(); return Result; }
            
            x = V3_Mul_S(Direction, Lambda);
            
            DistSq = FLT_MAX;
            
            ToleranceSq = Sq(ToleranceSq+SumRadius);
            
            AllowRestart = true;
        }
        
        Simplex.A[Simplex.Count] = a;
        Simplex.B[Simplex.Count] = b;
        Simplex.Count++;
        
        for(u32 i = 0; i < Simplex.Count; i++) {
            Simplex.P[i] = V3_Sub_V3(x, V3_Sub_V3(Simplex.B[i], Simplex.A[i]));
        }
        
        u32 Set;
        if(!GJK_Get_Closest_Point(&Simplex, DistSq, &v, &DistSq, &Set, false)) {
            if(!AllowRestart) break;
            
            AllowRestart = false;
            Simplex.A[0] = a;
            Simplex.B[0] = b;
            Simplex.Count = 1;
            
            v = V3_Sub_V3(x, b);
            DistSq = FLT_MAX;
            continue;
        } else if(Set == 0xf) {
            Assert(DistSq == 0.0f);
            break;
        }
        
        GJK_Update_AB(&Simplex, Set);
        if(DistSq <= ToleranceSq) break;
        
        vPrev = v;
    }
    
    for(u32 i = 0; i < Simplex.Count; i++) {
        Simplex.P[i] = V3_Sub_V3(x, V3_Sub_V3(Simplex.B[i], Simplex.A[i]));
    }
    
    v3 vNorm = V3_Norm(v);
    v3 ConvexRadiusA = V3_Mul_S(vNorm, RadiusA);
    v3 ConvexRadiusB = V3_Mul_S(vNorm, RadiusB);
    
    switch(Simplex.Count) {
        case 1: {
            Result.P2 = V3_Add_V3(Simplex.B[0], ConvexRadiusB);
            Result.P1 = Lambda > 0.0f ? Result.P2 : V3_Sub_V3(Simplex.A[0], ConvexRadiusA);
        } break;
        
        case 2: {
            f32 u, v;
            Get_Line_Barycentric_From_Origin(Simplex.P[0], Simplex.P[1], &u, &v);
            Result.P2 = V3_Add_V3(V3_Add_V3(V3_Mul_S(Simplex.B[0], u), 
                                            V3_Mul_S(Simplex.B[1], v)), 
                                  ConvexRadiusB);
            Result.P1 = Lambda > 0.0f ? Result.P2 : V3_Sub_V3(V3_Add_V3(V3_Mul_S(Simplex.A[0], u), 
                                                                        V3_Mul_S(Simplex.A[1], v)), 
                                                              ConvexRadiusA);
        } break;
        
        case 3:
        case 4: {
            f32 u, v, w;
            Get_Triangle_Barycentric_From_Origin(Simplex.P[0], Simplex.P[1], Simplex.P[2], &u, &v, &w);
            Result.P2 = V3_Add_V3(V3_Add_V3(V3_Add_V3(V3_Mul_S(Simplex.B[0], u), 
                                                      V3_Mul_S(Simplex.B[1], v)), 
                                            V3_Mul_S(Simplex.B[2], w)),
                                  ConvexRadiusB);
            
            Result.P1 = Lambda > 0.0f ? Result.P2 : V3_Sub_V3(V3_Add_V3(V3_Add_V3(V3_Mul_S(Simplex.A[0], u), 
                                                                                  V3_Mul_S(Simplex.A[1], v)), 
                                                                        V3_Mul_S(Simplex.A[2], w)),
                                                              ConvexRadiusA);
        } break;
    }
    
    Result.V = SumRadius > 0.0f ? V3_Negate(v) : V3_Negate(vPrev);
    Result.tHit = Lambda;
    
    if(OutSimplex) *OutSimplex = Simplex;
    
    Scratch_Release();
    
    return Result;
}

export_function penetration_test GJK_Penetration_Test(gjk_support* SupportA, f32 RadiusA, gjk_support* SupportB, 
                                                      f32 RadiusB, f32 Tolerance, gjk_simplex* OutSimplex, 
                                                      f32* OutDistSq) {
	f32 Radius = RadiusA + RadiusB;
	f32 RadiusSq = Sq(Radius);
    
	v3 V = { 1, 0, 0 };
	closest_points ClosestPoints = GJK_Get_Closest_Points(SupportA, SupportB, Tolerance, RadiusSq, &V, OutSimplex);
    
	if (ClosestPoints.DistSq > RadiusSq) {
		//No collision
		penetration_test Result = { 0 };
		return Result;
	}
    
	penetration_test Result = { 0 };
	Result.Intersected = true;
    
	*OutDistSq = ClosestPoints.DistSq;
	
	if (ClosestPoints.DistSq > 0.0f) {
		//Collision within the convex radius
		f32 Dist = Sqrt_F32(ClosestPoints.DistSq);
		Result.P2 = V3_Add_V3(ClosestPoints.P1, V3_Mul_S(V, RadiusA / Dist));
		Result.P1 = V3_Sub_V3(ClosestPoints.P2, V3_Mul_S(V, RadiusB / Dist));
		Result.V  = V3_Sub_V3(Result.P2, Result.P1);
	}
    
	return Result;
}

#define FLOW_EPA_MAX_POINTS 128
#define FLOW_EPA_MAX_TRIANGLES 256
#define FLOW_EPA_MIN_TRIANGLE_AREA 1.0e-10f
#define FLOW_EPA_BARYCENTRIC_EPSILON 0.001f
#define FLOW_EPA_MAX_EDGES 128

typedef struct {
	u32  Count;
	v3 P[FLOW_EPA_MAX_POINTS];
} epa_points;

typedef struct {
	epa_points Points;
	v3 A[FLOW_EPA_MAX_POINTS];
	v3 B[FLOW_EPA_MAX_POINTS];
} epa_support;

typedef struct epa_triangle epa_triangle;

typedef struct {
	epa_triangle* NeighborTriangle;
	u32 		  NeighborEdge;
	u32 		  StartVtx;
} epa_edge;

struct epa_triangle {
	epa_edge Edges[3];
	v3 	 	 Normal;
	v3 	 	 Centroid;
	f32  	 DistSq;
	f32 	 Lambda[2];
	b8 	 	 IsLambdaRelativeTo0;
	b8 	 	 Removed;
	b8 	 	 InQueue;
	b8 		 IsClosestPointInterior;
};

typedef union epa_triangle_pool_entry epa_triangle_pool_entry;

union epa_triangle_pool_entry { 
	epa_triangle  			 Triangle;
	epa_triangle_pool_entry* NextFree;
};

typedef struct {
	epa_triangle_pool_entry  Triangles[FLOW_EPA_MAX_TRIANGLES];
	epa_triangle_pool_entry* NextFree;
	u64 					  MaxUsed;
} epa_triangle_pool;

typedef struct {
	size_t 			   Count;
	epa_triangle* Triangles[FLOW_EPA_MAX_TRIANGLES];
} epa_heap;

typedef struct {
	epa_heap      Heap;
} epa_triangle_queue;

typedef struct {
	epa_points*        Points;
	epa_triangle_pool  TrianglePool;
	epa_triangle_queue TriangleQueue;
} epa_convex_hull;

typedef struct {
	u32 		  Count;
	epa_triangle* Ptr[FLOW_EPA_MAX_TRIANGLES];
} epa_triangle_list;

typedef struct {
	u32 	 Count;
	epa_edge Ptr[FLOW_EPA_MAX_EDGES];
} epa_edge_list;

#define EPA_Heap_Parent(i) ((i - 1) / 2)
// to get index of left child of node at index i 
#define EPA_Heap_Left(i) (2 * i + 1)
// to get index of right child of node at index i 
#define EPA_Heap_Right(i) (2 * i + 2)

function void EPA_Heap_Swap(epa_heap* Heap, size_t Index0, size_t Index1) {	
	epa_triangle* Tmp = Heap->Triangles[Index0];
	Heap->Triangles[Index0] = Heap->Triangles[Index1];
	Heap->Triangles[Index1] = Tmp;
}

function b32 EPA_Compare_Triangles(epa_triangle* TriangleA, epa_triangle* TriangleB) {
	return TriangleB->DistSq > TriangleA->DistSq;
}

function void EPA_Heap_Maxify(epa_heap* Heap) {
	size_t Index = 0;
	size_t Largest = Index;
	do {
		size_t Left = EPA_Heap_Left(Index);
		size_t Right = EPA_Heap_Right(Index);
        
		size_t Count = Heap->Count;
        
		if (Left < Count && EPA_Compare_Triangles(Heap->Triangles[Left], Heap->Triangles[Index])) {
			Largest = Left;
		}
        
		if (Right < Count && EPA_Compare_Triangles(Heap->Triangles[Right], Heap->Triangles[Largest])) {
			Largest = Right;
		}
        
		if (Largest != Index) {
			EPA_Heap_Swap(Heap, Index, Largest);
			Index = Largest;
		}
		
	} while (Largest != Index);
}

function void EPA_Heap_Push(epa_heap* Heap, epa_triangle* Triangle) {
	Assert(Heap->Count < FLOW_EPA_MAX_TRIANGLES);
	size_t Index = Heap->Count++;
	Heap->Triangles[Index] = Triangle;
    
	//If the parent of the binary heap is smaller, we need to fixup the structure
	while (Index != 0 && !EPA_Compare_Triangles(Heap->Triangles[EPA_Heap_Parent(Index)], 
                                                Heap->Triangles[Index])) {
		EPA_Heap_Swap(Heap, Index, EPA_Heap_Parent(Index));
		Index = EPA_Heap_Parent(Index);
	}
}

function epa_triangle* EPA_Heap_Pop(epa_heap* Heap) {
	Assert(Heap->Count);
	if (Heap->Count == 1) {
		Heap->Count--;
		return Heap->Triangles[0];
	}
    
	EPA_Heap_Swap(Heap, 0, Heap->Count-1);
	epa_triangle* Result = Heap->Triangles[Heap->Count - 1];
	Heap->Count--;
	EPA_Heap_Maxify(Heap);
	return Result;
}

function v3 EPA_Support_Push(epa_support* SupportPoints, gjk_support* SupportA, gjk_support* SupportB, 
                             v3 Direction, u32* OutIndex) {
	v3 a = GJK_Get_Support(SupportA, Direction);
	v3 b = GJK_Get_Support(SupportB, V3_Negate(Direction));
	v3 p = V3_Sub_V3(a, b);
    
	u32 Index = SupportPoints->Points.Count++;
    
	*OutIndex = Index;
	SupportPoints->Points.P[Index] = p;
	SupportPoints->A[Index] = a;
	SupportPoints->B[Index] = b;
	return p;
}

function void EPA_Support_Pop(epa_support* SupportPoints) {
	Assert(SupportPoints->Points.Count > 0);
	SupportPoints->Points.Count--;
}

function epa_edge* EPA_Triangle_Get_Next_Edge(epa_triangle* Triangle, u32 EdgeIdx) {
	epa_edge* Edge = Triangle->Edges + ((EdgeIdx + 1) % 3);
	return Edge;
}

function void EPA_Link_Triangles(epa_triangle* T0, u32 EdgeIdx0, epa_triangle* T1, u32 EdgeIdx1) {
	Assert(EdgeIdx0 < 3);
	Assert(EdgeIdx1 < 3);
	epa_edge* Edge0 = T0->Edges + EdgeIdx0;
	epa_edge* Edge1 = T1->Edges + EdgeIdx1;
    
	Assert(Edge0->NeighborTriangle == NULL);
	Assert(Edge1->NeighborTriangle == NULL);
    
	Assert(Edge0->StartVtx == EPA_Triangle_Get_Next_Edge(T1, EdgeIdx1)->StartVtx);
	Assert(Edge1->StartVtx == EPA_Triangle_Get_Next_Edge(T0, EdgeIdx0)->StartVtx);
    
	Edge0->NeighborTriangle = T1;
	Edge0->NeighborEdge = EdgeIdx1;
	Edge1->NeighborTriangle = T0;
	Edge1->NeighborEdge = EdgeIdx0;
}

function void EPA_Triangle_Init(epa_triangle* Triangle, u32 VtxIdx0, u32 VtxIdx1, u32 VtxIdx2, v3* Positions) {
	Memory_Clear(Triangle, sizeof(epa_triangle));
	Triangle->DistSq = FLT_MAX;
    
	Triangle->Edges[0].StartVtx = VtxIdx0;
	Triangle->Edges[1].StartVtx = VtxIdx1;
	Triangle->Edges[2].StartVtx = VtxIdx2;
    
	v3 V0 = Positions[VtxIdx0];
	v3 V1 = Positions[VtxIdx1];
	v3 V2 = Positions[VtxIdx2];
    
	//Calculate the centroid
	Triangle->Centroid = Get_Triangle_Centroid(V0, V1, V2);
    
	v3 e0 = V3_Sub_V3(V1, V0);
	v3 e1 = V3_Sub_V3(V2, V0);
	v3 e2 = V3_Sub_V3(V2, V1);
    
	//Calculating the most accurate normal requires taking the cross product on the two shortest edges.
	//Figure out which edge, e1 or e2 is shorter than the other
	f32 e1_dot_e1 = V3_Sq_Mag(e1); //Dot product with itself is just a sq mag
	f32 e2_dot_e2 = V3_Sq_Mag(e2);
    
	if (e1_dot_e1 < e2_dot_e2) {
		//We select edges e0 and e1
		Triangle->Normal = V3_Cross(e0, e1);
        
		//Check if the triangle is a degenerate
		f32 NormalSqLen = V3_Sq_Mag(Triangle->Normal);
		if (NormalSqLen > FLOW_EPA_MIN_TRIANGLE_AREA) {
            
			//Determine distance between triangle and origin
			f32 CDotN = V3_Dot(Triangle->Centroid, Triangle->Normal);
			Triangle->DistSq = Abs(CDotN)*CDotN / NormalSqLen;
            
			//Compute the cloest point to the origin using barycentric coordinates
			f32 e0_dot_e0 = V3_Sq_Mag(e0);
			f32 e0_dot_e1 = V3_Dot(e0, e1);
			f32 Determinant = e0_dot_e0 * e1_dot_e1 - Sq(e0_dot_e1);
			
			//If Determinant == 0 then the system is linearly dependent and the triangle is degenerate
			if (Determinant > 0) { 
				f32 v0_dot_e0 = V3_Dot(V0, e0);
				f32 v0_dot_e1 = V3_Dot(V0, e1);
				f32 l0 = (e0_dot_e1 * v0_dot_e1 - e1_dot_e1 * v0_dot_e0) / Determinant;
				f32 l1 = (e0_dot_e1 * v0_dot_e0 - e0_dot_e0 * v0_dot_e1) / Determinant;
				Triangle->Lambda[0] = l0;
				Triangle->Lambda[1] = l1;
				Triangle->IsLambdaRelativeTo0 = true;
                
				//Check if the closest point is interior to the triangle. Our convex hull might contain
				//coplanar triangles and only one will have the origin as the interior point. This triangle
				//is best to use when calculating contract points for accuracy, so we will prioritize add these 
				//triangles into the priority queue
				if (l0 > -FLOW_EPA_BARYCENTRIC_EPSILON && l1 > -FLOW_EPA_BARYCENTRIC_EPSILON && l0 + l1 < 1.0f + FLOW_EPA_BARYCENTRIC_EPSILON)
					Triangle->IsClosestPointInterior = true;
			}
		}
	} else {
		//We select edges e0 and e2
		Triangle->Normal = V3_Cross(e0, e2);
		//Check if the triangle is a degenerate
		f32 NormalSqLen = V3_Sq_Mag(Triangle->Normal);
		if (NormalSqLen > FLOW_EPA_MIN_TRIANGLE_AREA) {
			//Determine distance between triangle and origin
			f32 CDotN = V3_Dot(Triangle->Centroid, Triangle->Normal);
			Triangle->DistSq = Abs(CDotN)*CDotN / NormalSqLen;
            
			//Compute the cloest point to the origin using barycentric coordinates
			f32 e0_dot_e0 = V3_Sq_Mag(e0);
			f32 e0_dot_e2 = V3_Dot(e0, e2);
			f32 Determinant = e0_dot_e0 * e2_dot_e2 - Sq(e0_dot_e2);
			//If Determinant == 0 then the system is linearly dependent and the triangle is degenerate
			if (Determinant > 0) { 
				f32 v1_dot_e0 = V3_Dot(V1, e0);
				f32 v1_dot_e2 = V3_Dot(V1, e2);
				f32 l0 = (e2_dot_e2 * v1_dot_e0 - e0_dot_e2 * v1_dot_e2) / Determinant;
				f32 l1 = (e0_dot_e2 * v1_dot_e0 - e0_dot_e0 * v1_dot_e2) / Determinant;
				Triangle->Lambda[0] = l0;
				Triangle->Lambda[1] = l1;
				Triangle->IsLambdaRelativeTo0 = false;
                
				//Check if the closest point is interior to the triangle. Our convex hull might contain
				//coplanar triangles and only one will have the origin as the interior point. This triangle
				//is best to use when calculating contract points for accuracy, so we will prioritize add these 
				//triangles into the priority queue
				if (l0 > -FLOW_EPA_BARYCENTRIC_EPSILON && l1 > -FLOW_EPA_BARYCENTRIC_EPSILON && l0 + l1 < 1.0f + FLOW_EPA_BARYCENTRIC_EPSILON)
					Triangle->IsClosestPointInterior = true;
			}
		}
	}
}

function b32 EPA_Triangle_Is_Facing(epa_triangle* Triangle, v3 P) {
	Assert(!Triangle->Removed);
	b32 Result = V3_Dot(Triangle->Normal, V3_Sub_V3(P, Triangle->Centroid)) > 0.0f;
	return Result;
}

function b32 EPA_Triangle_Is_Facing_Origin(epa_triangle* Triangle) {
	Assert(!Triangle->Removed);
	b32 Result = V3_Dot(Triangle->Normal, Triangle->Centroid) < 0.0f;
	return Result;
}

function epa_triangle* EPA_Triangle_Pool_Create_Triangle(epa_triangle_pool* Pool, u32 VtxIdx0, u32 VtxIdx1, u32 VtxIdx2, v3* Positions) {
	epa_triangle* Triangle;
	if (Pool->NextFree != NULL) {
		Triangle = &Pool->NextFree->Triangle;
		Pool->NextFree = Pool->NextFree->NextFree;
	} else {
		if (Pool->MaxUsed >= FLOW_EPA_MAX_TRIANGLES) {
			return NULL;
		}
        
		Triangle = &Pool->Triangles[Pool->MaxUsed].Triangle;
		Pool->MaxUsed++;
	}
    
	EPA_Triangle_Init(Triangle, VtxIdx0, VtxIdx1, VtxIdx2, Positions);
	return Triangle;
}

function void EPA_Triangle_Pool_Free_Triangle(epa_triangle_pool* Pool, epa_triangle* Triangle) {
	epa_triangle_pool_entry* Entry = (epa_triangle_pool_entry *)Triangle;
	Entry->NextFree = Pool->NextFree;
	Pool->NextFree = Entry;
}

function void EPA_Triangle_Queue_Push(epa_triangle_queue* TriangleQueue, epa_triangle* Triangle) {
	Triangle->InQueue = true;
	EPA_Heap_Push(&TriangleQueue->Heap, Triangle);
}

function void EPA_Triangle_List_Push(epa_triangle_list* TriangleList, epa_triangle* Triangle) {
	TriangleList->Ptr[TriangleList->Count++] = Triangle;
}

function void EPA_Edge_List_Push(epa_edge_list* EdgeList, epa_edge* Edge) {
	EdgeList->Ptr[EdgeList->Count++] = *Edge;
}

function epa_triangle* EPA_Triangle_Queue_Peek(epa_triangle_queue* TriangleQueue) {
	epa_triangle* Result = TriangleQueue->Heap.Triangles[0];
	return Result;
}

function epa_triangle* EPA_Triangle_Queue_Pop(epa_triangle_queue* TriangleQueue) {
	epa_triangle* Result = EPA_Heap_Pop(&TriangleQueue->Heap);
	return Result;
}

function b32 EPA_Triangle_Queue_Empty(epa_triangle_queue* TriangleQueue) {
	return TriangleQueue->Heap.Count == 0;
}

function epa_triangle* EPA_Convex_Hull_Create_Triangle(epa_convex_hull* ConvexHull, u32 VtxIdx0, u32 VtxIdx1, u32 VtxIdx2) {
	epa_triangle* Triangle = EPA_Triangle_Pool_Create_Triangle(&ConvexHull->TrianglePool, VtxIdx0, VtxIdx1, VtxIdx2, ConvexHull->Points->P);
	return Triangle;
}

function void EPA_Convex_Hull_Free_Triangle(epa_convex_hull* ConvexHull, epa_triangle* Triangle) {
#ifdef FLOW_DEBUG
	Assert(Triangle->Removed);
	for (u32 i = 0; i < 3; i++) {
		epa_edge* Edge = Triangle->Edges + i;
		Assert(Edge->NeighborTriangle == NULL);
	}
#endif
	EPA_Triangle_Pool_Free_Triangle(&ConvexHull->TrianglePool, Triangle);
}

function void EPA_Convex_Hull_Unlink_Triangle(epa_convex_hull* ConvexHull, epa_triangle* Triangle) {
	for (u32 i = 0; i < 3; i++) {
		epa_edge* Edge = Triangle->Edges + i;
		if (Edge->NeighborTriangle != NULL) {
			epa_edge* NeighborEdge = Edge->NeighborTriangle->Edges + Edge->NeighborEdge;
            
			Assert(NeighborEdge->NeighborTriangle == Triangle);
			Assert(NeighborEdge->NeighborEdge == i);
            
			//Unlink
			NeighborEdge->NeighborTriangle = NULL;
			Edge->NeighborTriangle = NULL;
		}
	}
    
	//If the triangle is not in the queue we can delete it now
	if (!Triangle->InQueue) {
		EPA_Convex_Hull_Free_Triangle(ConvexHull, Triangle);
	}
}

function b32 EPA_Convex_Hull_Find_Edge(epa_convex_hull* ConvexHull, epa_triangle* FacingTriangle, v3 P, epa_edge_list* OutEdges) {
	//Given a triangle that faces the vertex P, find the edges of the triangles that do not face P and flag the
	//triangles for removal
    
	Assert(OutEdges->Count == 0);
	Assert(EPA_Triangle_Is_Facing(FacingTriangle, P));
	
	//Flag facing triangle for removal
	FacingTriangle->Removed = true;
    
	//Recurse to find and remove triangles that are not facing the vertex P
	typedef struct {
		epa_triangle* Triangle;
		u32 		  EdgeIdx;
		s32 		  Iteration;
	} stack_entry;
    
	s32 CurrentStackIndex = 0;
	stack_entry Stack[FLOW_EPA_MAX_EDGES];
    
	//Start with the facing triangle 
	Stack[CurrentStackIndex].Triangle = FacingTriangle;
	Stack[CurrentStackIndex].EdgeIdx = 0;
	Stack[CurrentStackIndex].Iteration = -1;
    
	s64 NextExpectedStartIdx = -1;
    
	for (;;) {
		stack_entry* CurrentEntry = Stack + CurrentStackIndex;
		if (++CurrentEntry->Iteration >= 3) {
			//Triangle should be removed now
			EPA_Convex_Hull_Unlink_Triangle(ConvexHull, CurrentEntry->Triangle);
            
			//Pop from the stack
			CurrentStackIndex--;
			if (CurrentStackIndex < 0)
				break;
		} else {
			epa_edge* Edge = CurrentEntry->Triangle->Edges + ((CurrentEntry->EdgeIdx + CurrentEntry->Iteration) % 3);
			epa_triangle* Neighbor = Edge->NeighborTriangle;
			if (Neighbor && !Neighbor->Removed) {
				if (EPA_Triangle_Is_Facing(Neighbor, P)) {
					//Triangle needs to be removed
					Neighbor->Removed = true;
                    
					CurrentStackIndex++;
					Assert(CurrentStackIndex < FLOW_EPA_MAX_EDGES);
					stack_entry* NewEntry = Stack + CurrentStackIndex;
					NewEntry->Triangle = Neighbor;
					NewEntry->EdgeIdx = Edge->NeighborEdge;
					NewEntry->Iteration = 0;
				} else {
					//If edge doesn't connect to the previous edge, we have found a very degenerate edge case and
					//will not be able to add the point to the convex hull
					if (Edge->StartVtx != NextExpectedStartIdx && NextExpectedStartIdx != -1)
						return false;
                    
					NextExpectedStartIdx = Neighbor->Edges[Edge->NeighborEdge].StartVtx;
					EPA_Edge_List_Push(OutEdges, Edge);
				}
			}
		}
	}
    
	Assert(OutEdges->Count == 0 || OutEdges->Ptr[0].StartVtx == NextExpectedStartIdx);
	return OutEdges->Count >= 3;
}

function b32 EPA_Convex_Hull_Add_Point(epa_convex_hull* ConvexHull, epa_triangle* Triangle, u32 VtxIdx, f32 MaxDistSq, epa_triangle_list* NewTriangles) {
	v3 Position = ConvexHull->Points->P[VtxIdx];
    
	//FInd the edges in the hull that are not facing the new vertex
    
	epa_edge_list Edges = { 0 };
	if (!EPA_Convex_Hull_Find_Edge(ConvexHull, Triangle, Position, &Edges))
		return false;
    
	//Create new triangles from the edges
    
	u32 NumEdges = Edges.Count;
	for (u32 i = 0; i < NumEdges; i++) {
		epa_triangle* NewTriangle = EPA_Convex_Hull_Create_Triangle(ConvexHull, Edges.Ptr[i].StartVtx, 
                                                                    Edges.Ptr[(i+1)%NumEdges].StartVtx,
                                                                    VtxIdx);
		if (!NewTriangle) return false;
        
		EPA_Triangle_List_Push(NewTriangles, NewTriangle);
        
		//Check if we should put this triangle in the priority queue
		if ((NewTriangle->IsClosestPointInterior && NewTriangle->DistSq < MaxDistSq) || NewTriangle->DistSq < 0.0f) {
			EPA_Triangle_Queue_Push(&ConvexHull->TriangleQueue, NewTriangle);
		}
	}
    
	//Link the edges
	for (u32 i = 0; i < NumEdges; i++) {
		EPA_Link_Triangles(NewTriangles->Ptr[i], 0, Edges.Ptr[i].NeighborTriangle, Edges.Ptr[i].NeighborEdge);
		EPA_Link_Triangles(NewTriangles->Ptr[i], 1, NewTriangles->Ptr[(i + 1) % NumEdges], 2);
	}
    
	return true;
}

function epa_triangle* EPA_Convex_Hull_Find_Facing_Triangle(epa_convex_hull* ConvexHull, v3 P, f32* OutDistSq) {
	epa_triangle* Result = NULL;
	f32 BestDistSq = 0.0f;
    
	epa_triangle_queue* TriangleQueue = &ConvexHull->TriangleQueue;
	for (u64 i = 0; i < TriangleQueue->Heap.Count; i++) {
		epa_triangle* Triangle = TriangleQueue->Heap.Triangles[i];
		if (!Triangle->Removed) {
			f32 Dot = V3_Dot(Triangle->Normal, V3_Sub_V3(P, Triangle->Centroid));
			if (Dot > 0.0f) {
				f32 DistSq = Sq(Dot) / V3_Sq_Mag(Triangle->Normal);
				if (DistSq > BestDistSq) {
					Result = Triangle;
					BestDistSq = DistSq;
				}
			}
		}
	}
    
	*OutDistSq = BestDistSq;
	return Result;
}

export_function penetration_test EPA_Penetration_Test_With_Simplex(gjk_support* SupportA, gjk_support* SupportB, f32 Tolerance, gjk_simplex* Simplex) {
	penetration_test Result = { 0 };
    
	//Copy the simplex into the epa support points
	epa_support SupportPoints = { { Simplex->Count } };
	Memory_Copy(SupportPoints.Points.P, Simplex->P, Simplex->Count * sizeof(v3));
	Memory_Copy(SupportPoints.A, Simplex->A, Simplex->Count * sizeof(v3));
	Memory_Copy(SupportPoints.B, Simplex->B, Simplex->Count * sizeof(v3));
    
	//The simplex should contain the origin, (GJK wouldn't report a collision otherwise), 
	//in cases where the simplex contains less than 3 points, we need to find extra support points
	switch (SupportPoints.Points.Count) {
		case 1: {
			//1 vertex, which must be the origin
			EPA_Support_Pop(&SupportPoints);
            
			//Add four points around the origin to form a tetrahedron around the origin
            
			u32 I0, I1, I2, I3;
			EPA_Support_Push(&SupportPoints, SupportA, SupportB, V3( 0,  1,  0), &I0);
			EPA_Support_Push(&SupportPoints, SupportA, SupportB, V3(-1, -1, -1), &I1);
			EPA_Support_Push(&SupportPoints, SupportA, SupportB, V3( 1, -1, -1), &I2);
			EPA_Support_Push(&SupportPoints, SupportA, SupportB, V3( 0, -1,  1), &I3);
            
			Assert(I0 == 0);
			Assert(I1 == 1);
			Assert(I2 == 2);
			Assert(I3 == 3);
		} break;
        
		case 2: {
			//2 vertices which form a line that contains the origin
            
			//Create 3 extra vertices by taking a perpendicular axis and rotating it 120 degree increments
			v3 Axis = V3_Norm(V3_Sub_V3(SupportPoints.Points.P[1], SupportPoints.Points.P[0]));
			m3 Rotation = M3_Axis_Angle(Axis, To_Radians(120.0f));
            
			v3 Dir1 = V3_Norm(V3_Get_Perp(Axis));
			v3 Dir2 = V3_Mul_M3(Dir1, &Rotation);
			v3 Dir3 = V3_Mul_M3(Dir2, &Rotation);
            
			u32 I0, I1, I2;
			EPA_Support_Push(&SupportPoints, SupportA, SupportB, Dir1, &I0);
			EPA_Support_Push(&SupportPoints, SupportA, SupportB, Dir2, &I1);
			EPA_Support_Push(&SupportPoints, SupportA, SupportB, Dir3, &I2);
            
			Assert(I0 == 2);
			Assert(I1 == 3);
			Assert(I2 == 4);
		} break;
        
		case 3:
		case 4: {
			//Noop and do nothing. We have enough vertices already
		} break;
        
		Invalid_Default_Case;
	}
    
	Assert(SupportPoints.Points.Count >= 3);
    
	//Build the initial convex hull out of the simplex that contains the origin
	epa_convex_hull ConvexHull = { &SupportPoints.Points };
    
	//Start with the first three vertices and create triangles back to back
	epa_triangle* T1 = EPA_Convex_Hull_Create_Triangle(&ConvexHull, 0, 1, 2);
	epa_triangle* T2 = EPA_Convex_Hull_Create_Triangle(&ConvexHull, 0, 2, 1);
    
	EPA_Link_Triangles(T1, 0, T2, 2);
	EPA_Link_Triangles(T1, 1, T2, 1);
	EPA_Link_Triangles(T1, 2, T2, 0);
    
	EPA_Triangle_Queue_Push(&ConvexHull.TriangleQueue, T1);
	EPA_Triangle_Queue_Push(&ConvexHull.TriangleQueue, T2);
    
	for (u32 i = 3; i < SupportPoints.Points.Count; i++) {
		//Any additional points can get added to the hull. Make sure to add to the triangle
		//that is facing the most to the point
        
		f32 DistSq;
		epa_triangle* Triangle = EPA_Convex_Hull_Find_Facing_Triangle(&ConvexHull, SupportPoints.Points.P[i], &DistSq);
		if (Triangle != NULL) {
			epa_triangle_list NewTriangles = { 0 };
			if (!EPA_Convex_Hull_Add_Point(&ConvexHull, Triangle, i, FLT_MAX, &NewTriangles)) {
				//If we can't add a point, assume no collision. This can happen if the shapes touch
				//at one point, and its alright to not report a collision then
				return Result;
			}
		}
	}
    
	//After we created the initial hull, we need to make sure it includes the origin. The simplex
	//contained the origin, thus we know the hull must contain the origin at some point
    
	//Loop until we know the hull contains the origin
	for (;;) {
		epa_triangle* Triangle = EPA_Triangle_Queue_Peek(&ConvexHull.TriangleQueue);
		
		//If a triangle was removed we can free it now. We don't free removed triangles immediately
		//since that means we would need to rebuild the binary heap
		if (Triangle->Removed) {
			EPA_Triangle_Queue_Pop(&ConvexHull.TriangleQueue);
            
			//If we ran out of triangles, there must be extremely little penetration and we can 
			//report no collision
			if (EPA_Triangle_Queue_Empty(&ConvexHull.TriangleQueue)) {
				return Result;
			}
            
			EPA_Convex_Hull_Free_Triangle(&ConvexHull, Triangle);
			continue;
		}
        
		//If the triangle closest to the origin is zero or positive, we know that the origin is in
		//the hull and can continue to the main algorithm
		if (Triangle->DistSq >= 0.0f)
			break;
        
		EPA_Triangle_Queue_Pop(&ConvexHull.TriangleQueue);
        
		//Add the support point to get the origin in the convex hull
		u32 NewIndex;
		v3 P = EPA_Support_Push(&SupportPoints, SupportA, SupportB, Triangle->Normal, &NewIndex);
        
		//Add the new point to the hull
		epa_triangle_list NewTriangles = { 0 };
		if (!EPA_Triangle_Is_Facing(Triangle, P) || !EPA_Convex_Hull_Add_Point(&ConvexHull, Triangle, NewIndex, FLT_MAX, &NewTriangles)) {
			//If we fail, terminate and report no collision
			return Result;
		}
        
		//Triangle can be safely removed, new support point is closer to the origin
		Assert(Triangle->Removed);
		EPA_Convex_Hull_Free_Triangle(&ConvexHull, Triangle);
        
		if (EPA_Triangle_Queue_Empty(&ConvexHull.TriangleQueue) || SupportPoints.Points.Count >= FLOW_EPA_MAX_POINTS) {
			//If we run out of points, there must be little penetration and we can report no collision
			return Result;
		}
	}
    
	/// - A: Calculate the closest point to the origin for all triangles of the hull and take the closest one
	/// - Calculate a new support point (of the Minkowski sum) in this direction and add this point to the convex hull
	/// - This will remove all faces that are facing the new point and will create new triangles to fill up the hole
	/// - Loop to A until no closer point found
	/// - The closest point indicates the position / direction of least penetration
    
	f32 BestDistSq = FLT_MAX;
	epa_triangle* LastTriangle = NULL;
    
	b32 FlipSign = false;
    
	do {
		//Get the closest triangle to the origin
		epa_triangle* Triangle = EPA_Triangle_Queue_Pop(&ConvexHull.TriangleQueue);
        
		//To prevent rebuilding the heap, pop removed triangles now
		if (Triangle->Removed) {
			EPA_Convex_Hull_Free_Triangle(&ConvexHull, Triangle);
			continue;
		}
        
		//If we are not getting closer to the origin, the closest point has been found
		if (Triangle->DistSq >= BestDistSq) {
			break;
		}
        
		//Free the last triangle 
		if (LastTriangle != NULL) {
			EPA_Convex_Hull_Free_Triangle(&ConvexHull, LastTriangle);
		}
		LastTriangle = Triangle;
        
		//Calculate the new support point to add to the hull in the direction of the closest triangle
		u32 NewIndex;
		v3 p = EPA_Support_Push(&SupportPoints, SupportA, SupportB, Triangle->Normal, &NewIndex);
        
		//Project the point onto the contact normal
		f32 Dot = V3_Dot(p, Triangle->Normal);
        
		//This shouldn't happen in practice, but its possible to have a separating axis in theory
		//and therefore no collision. Just in case, report no collision.
		if (Dot < 0.0f) {
			return Result;
		}
        
		//Get the sq dist to the support point (along direction of normal)
		f32 DistSq = Sq(Dot) / V3_Sq_Mag(Triangle->Normal);
        
		//break if the error is small enough
		if (DistSq - Triangle->DistSq < Triangle->DistSq*Tolerance) {
			break;
		}
        
		BestDistSq = Min(BestDistSq, DistSq);
        
		//Due to numerical imprecision, we can technically have the triangle not facing the support point.
		//At this point we've converged and can just return
		if (!EPA_Triangle_Is_Facing(Triangle, p)) {
			break;
		}
        
		//Add support point to the hull, if it can't break out 
		epa_triangle_list NewTriangles = { 0 };
		if (!EPA_Convex_Hull_Add_Point(&ConvexHull, Triangle, NewIndex, BestDistSq, &NewTriangles)) {
			break;
		}
        
		//Sometimes we build an invalid hull due to numerical imprecision. We break if this happens
		b32 IsInvalid = false;
		for (u32 i = 0; i < NewTriangles.Count; i++) {
			epa_triangle* Triangle = NewTriangles.Ptr[i];
			if (EPA_Triangle_Is_Facing_Origin(Triangle)) {
				IsInvalid = true;
				break;
			}
		}
        
		if (IsInvalid) {
			//When an invalid hull is generated (origin lies on the wrong side of the triangle)
			//we need to perform another check to see if the penetration of the triangle in the opposite 
			//direction of the normal is smaller than in the normal direction. If so we need to 
			//flip the sign of the penetration vector
			v3 P2 = V3_Sub_V3(GJK_Get_Support(SupportA, V3_Negate(Triangle->Normal)), 
                              GJK_Get_Support(SupportB, Triangle->Normal));
			f32 Dot2 = -V3_Dot(Triangle->Normal, P2);
			if (Dot2 < Dot)
				FlipSign = true;
			break;
		}
	} while (!EPA_Triangle_Queue_Empty(&ConvexHull.TriangleQueue) && SupportPoints.Points.Count < FLOW_EPA_MAX_POINTS);
    
	//If the hull was a plane, there is no penentration and report no collision
	if (LastTriangle == NULL) {
		return Result;
	}
    
	//Get the penetration vector. Calculated by getting the vector from the origin to the closest point
	//on the triangle.
	v3 V = V3_Mul_S(LastTriangle->Normal, V3_Dot(LastTriangle->Centroid, LastTriangle->Normal) / V3_Sq_Mag(LastTriangle->Normal));
	
	//If the penetration is near zero, we treat this as no penetration and report no collision
	if (V3_Is_Zero(V, 1.0e-12f)) {
		return Result;
	}
    
	Result.Intersected = true;
    
	if (FlipSign) {
		V = V3_Negate(V);
	}
    
	//Contact point generation
	v3 a0 = SupportPoints.A[LastTriangle->Edges[0].StartVtx];
	v3 a1 = SupportPoints.A[LastTriangle->Edges[1].StartVtx];
	v3 a2 = SupportPoints.A[LastTriangle->Edges[2].StartVtx];
    
	v3 b0 = SupportPoints.B[LastTriangle->Edges[0].StartVtx];
	v3 b1 = SupportPoints.B[LastTriangle->Edges[1].StartVtx];
	v3 b2 = SupportPoints.B[LastTriangle->Edges[2].StartVtx];
    
	if (LastTriangle->IsLambdaRelativeTo0)
	{
		//P1 is the reference point
		Result.P1 = V3_Add_V3(a0, V3_Add_V3(V3_Mul_S(V3_Sub_V3(a1, a0), LastTriangle->Lambda[0]), 
                                            V3_Mul_S(V3_Sub_V3(a2, a0), LastTriangle->Lambda[1])));
		Result.P2 = V3_Add_V3(b0, V3_Add_V3(V3_Mul_S(V3_Sub_V3(b1, b0), LastTriangle->Lambda[0]), 
                                            V3_Mul_S(V3_Sub_V3(b2, b0), LastTriangle->Lambda[1])));
	}
	else
	{
		//P2 is the reference point
		Result.P1 = V3_Add_V3(a1, V3_Add_V3(V3_Mul_S(V3_Sub_V3(a0, a1), LastTriangle->Lambda[0]), 
                                            V3_Mul_S(V3_Sub_V3(a2, a1), LastTriangle->Lambda[1])));
		Result.P2 = V3_Add_V3(b1, V3_Add_V3(V3_Mul_S(V3_Sub_V3(b0, b1), LastTriangle->Lambda[0]), 
                                            V3_Mul_S(V3_Sub_V3(b2, b1), LastTriangle->Lambda[1])));
	}
	Result.V = V;
    
	return Result;
}

export_function penetration_test EPA_Penetration_Test(gjk_support* SupportA, gjk_support* SupportB, 
                                                      f32 CollisionTolerance, f32 PenetrationTolerance) {
	gjk_simplex Simplex;
    
	f32 OutDist;
	penetration_test Result = GJK_Penetration_Test(SupportA, 0.0f, SupportB, 0.0f, CollisionTolerance, &Simplex, &OutDist);
	if (Result.Intersected) {
		if (OutDist == 0.0f) {
			Result = EPA_Penetration_Test_With_Simplex(SupportA, SupportB, PenetrationTolerance, &Simplex);
		}
	}
	return Result;
}

export_function ray_cast Cast_Shape(gjk_support* SupportA, f32 RadiusA, gjk_support* SupportB, f32 RadiusB, 
                                    const m4* Start, v3 Direction, f32 tDistance, 
                                    f32 CollisionTolerance, f32 PenetrationTolerance) {
    
    gjk_simplex Simplex;
    ray_cast Cast = GJK_Cast(SupportA, RadiusA, SupportB, RadiusB, Start, Direction, tDistance, CollisionTolerance, &Simplex);
    if(Cast.tHit == FLT_MAX) return Cast;
    
    b32 IsNormalInvalid = V3_Is_Zero(Cast.V, Sq(CollisionTolerance));
    
    //Only when the the shapes intersect right at the start do we need to call epa
    //to get the contact normal
    if(Cast.tHit == 0 && (RadiusA+RadiusB == 0.0f || IsNormalInvalid)) {
        arena* Scratch = Scratch_Get();
        gjk_support AddConvexRadiusA = GJK_Add_Radius(Scratch, *SupportA, RadiusA);
        gjk_support AddConvexRadiusB = GJK_Add_Radius(Scratch, *SupportB, RadiusB);
        gjk_support TransformA = GJK_Transform(Scratch, AddConvexRadiusA, Start);
        
        penetration_test Penetration = EPA_Penetration_Test_With_Simplex(&TransformA, &AddConvexRadiusB, PenetrationTolerance, &Simplex);
        if(Penetration.Intersected) {
            Cast.V = Penetration.V;
            Cast.P1 = Penetration.P1;
            Cast.P2 = Penetration.P2;
        } else {
            Cast.V = Direction;
        }
        
        Scratch_Release();
    } else if(IsNormalInvalid) {
        Cast.V = Direction;
    }
    
    return Cast;
}