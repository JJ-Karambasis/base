typedef struct {
	v3   CenterP;
	f32 Radius;
} sphere;

typedef struct {
	v3 Min;
	v3 Max;
} aabb;

function sphere Make_Sphere(v3 CenterP, f32 Radius) {
	sphere Result = {CenterP, Radius};
	return Result;
}

function aabb Make_AABB(v3 Min, v3 Max) {
	aabb Result = {Min, Max};
	return Result;
}

function v3 AABB_Get_Closest_Point(aabb AABB, v3 P) {
	v3 Result;
	Result.x = Min(AABB.Max.x, Max(AABB.Min.x, P.x));
	Result.y = Min(AABB.Max.y, Max(AABB.Min.y, P.y));
	Result.z = Min(AABB.Max.z, Max(AABB.Min.z, P.z));
	return Result;
}

UTEST(Geometry, TestGJKIntersectsSphere) {
	arena* Arena = Arena_Create(String_Empty());
    
	gjk_support S1 = GJK_Sphere(Arena, V3_Zero(), 1.0f);
	
	v3 C2 = V3(10.0f, 10.0f, 10.0f);
	gjk_support S2 = GJK_Sphere(Arena, C2, 1.0f);
    
	f32 l = 2.0f / Sqrt_F32(3.0f);
	v3 C3 = V3(l, l, l);
	gjk_support S3 = GJK_Sphere(Arena, C3, 1.0f);
    
	{
		v3 v = V3_Zero();
		ASSERT_FALSE(GJK_Intersects(&S1, &S2, 1.0e-4f, &v));
	}
    
	{
		v3 v = V3_Zero();
		ASSERT_TRUE(GJK_Intersects(&S1, &S3, 1.0e-4f, &v));
	}
    
	{
		v3 v = V3_Zero();
		closest_points ClosestPoints = GJK_Get_Closest_Points(&S1, &S2, 1.0e-4f, FLT_MAX, &v, NULL);
		ASSERT_TRUE(Is_Close(V3_Mag(C2) - 2.0f, Sqrt_F32(ClosestPoints.DistSq), 1.0e-4f));
		ASSERT_TRUE(V3_Is_Close(V3_Norm(C2), ClosestPoints.P1, 1.0e-4f));
		ASSERT_TRUE(V3_Is_Close(V3_Sub_V3(C2, V3_Norm(C2)), ClosestPoints.P2, 1.0e-4f));
	}
    
	{
		v3 v = V3_Zero();
		closest_points ClosestPoints = GJK_Get_Closest_Points(&S1, &S3, 1.0e-4f, FLT_MAX, &v, NULL);
		ASSERT_TRUE(Is_Close(0.0f, Sqrt_F32(ClosestPoints.DistSq), 1.0e-4f));
		ASSERT_TRUE(V3_Is_Close(V3_Norm(C2), ClosestPoints.P1, 1.0e-4f));
		ASSERT_TRUE(V3_Is_Close(V3_Norm(C2), ClosestPoints.P2, 1.0e-4f));
	}
    
	Arena_Delete(Arena);
}

function b32 Collide_Box_Sphere(m4* Transform, aabb AABB, sphere Sphere) {
	arena* Arena = Arena_Create(String_Empty());
    
	gjk_support SupportA = GJK_AABB(Arena, AABB.Min, AABB.Max);
	gjk_support SupportB = GJK_Sphere(Arena, Sphere.CenterP, Sphere.Radius);
    
	m3 TransformM = M3_From_M4(Transform);
    
	gjk_support TransformedSupportA = GJK_Transform(Arena, SupportA, Transform);
	gjk_support TransformedSupportB = GJK_Transform(Arena, SupportB, Transform);
    
	//EPA test
	penetration_test PenetrationTest = EPA_Penetration_Test(&TransformedSupportA, &TransformedSupportB, 1.0e-2f, FLT_EPSILON);
    
	//Analytical solution
	v3 P1 = AABB_Get_Closest_Point(AABB, Sphere.CenterP);
	v3 V = V3_Sub_V3(Sphere.CenterP, P1);
	b32 Intersected = V3_Sq_Mag(V) <= Sq(Sphere.Radius);
    
	Assert(Intersected == PenetrationTest.Intersected);
	if (Intersected && PenetrationTest.Intersected) {
		v3 P2 = V3_Sub_V3(Sphere.CenterP, V3_Mul_S(V3_Norm(V), Sphere.Radius));
        
		//Transform analytical solution
		V = V3_Mul_M3(V, &TransformM);
		P1 = V4_Mul_M4(V4_From_V3(P1, 1.0f), Transform).xyz;
		P2 = V4_Mul_M4(V4_From_V3(P2, 1.0f), Transform).xyz;
        
		// Check angle between v1 and v2
		f32 Angle = V3_Angle_Between(PenetrationTest.V, V);
		Assert(To_Degrees(Angle) < 0.1f);
        
		// Check delta between contact on A
		v3 DeltaP1 = V3_Sub_V3(P1, PenetrationTest.P1);
		Assert(V3_Mag(DeltaP1) < 8.0e-4f);
        
		// Check delta between contact on B
		v3 DeltaP2 = V3_Sub_V3(P2, PenetrationTest.P2);
		Assert(V3_Mag(DeltaP2) < 8.0e-4f);
	}
    
	Arena_Delete(Arena);
	return PenetrationTest.Intersected;
}

function void Collide_Boxes_With_Spheres(m4* Transform, int* utest_result) {
    
	{
		// Sphere just missing face of box
		aabb Box = Make_AABB(V3(-2, -3, -4), V3(2, 3, 4));
		sphere Sphere = Make_Sphere(V3(4, 0, 0), 1.99f);
		ASSERT_FALSE(Collide_Box_Sphere(Transform, Box, Sphere));
	}
    
	{
		// Sphere just touching face of box
		aabb Box = Make_AABB(V3(-2, -3, -4), V3(2, 3, 4));
		sphere Sphere = Make_Sphere(V3(4, 0, 0), 2.01f);
		ASSERT_TRUE(Collide_Box_Sphere(Transform, Box, Sphere));
	}
    
	{
		// Sphere deeply penetrating box on face
		aabb Box = Make_AABB(V3(-2, -3, -4), V3(2, 3, 4));
		sphere Sphere = Make_Sphere(V3(3, 0, 0), 2);
		ASSERT_TRUE(Collide_Box_Sphere(Transform, Box, Sphere));
	}
    
	{
		// Sphere just missing box on edge
		aabb Box = Make_AABB(V3(1, 1, -2), V3(2, 2, 2));
		sphere Sphere = Make_Sphere(V3(4, 4, 0), Sqrt_F32(8.0f) - 0.01f);
		ASSERT_FALSE(Collide_Box_Sphere(Transform, Box, Sphere));
	}
    
	{
		// Sphere just penetrating box on edge
		aabb Box = Make_AABB(V3(1, 1, -2), V3(2, 2, 2));
		sphere Sphere = Make_Sphere(V3(4, 4, 0), Sqrt_F32(8.0f) + 0.01f);
		ASSERT_TRUE(Collide_Box_Sphere(Transform, Box, Sphere));
	}
    
	{
		// Sphere just missing box on vertex
		aabb Box = Make_AABB(V3(1, 1, 1), V3(2, 2, 2));
		sphere Sphere = Make_Sphere(V3(4, 4, 4), Sqrt_F32(12.0f) - 0.01f);
		ASSERT_FALSE(Collide_Box_Sphere(Transform, Box, Sphere));
	}
    
	{
		// Sphere just penetrating box on vertex
		aabb Box = Make_AABB(V3(1, 1, 1), V3(2, 2, 2));
		sphere Sphere = Make_Sphere(V3(4, 4, 4), Sqrt_F32(12.0f) + 0.01f);
		ASSERT_TRUE(Collide_Box_Sphere(Transform, Box, Sphere));
	}
}

UTEST(Geometry, TestEPASphereBox) {
	m4 Identity = {
		1, 0, 0, 0, 
		0, 1, 0, 0, 
		0, 0, 1, 0, 
		0, 0, 0, 1
	};
	Collide_Boxes_With_Spheres(&Identity, utest_result);
    
	m4 Transform = {
		0.843391f, 0.268650f, -0.465315f, 0,
		0.000000f, 0.866025f, 0.500000f, 0,
		0.537300f, -0.421696f, 0.730398f, 0,
		1.361121f, 0.180594f, 0.000000f, 1
	};
    
	m4 Transform2 = {
		0.843391f, 0.268650f, -0.465315f, 0,
		0.000000f, 0.866025f, 0.500000f,  0, 
		0.537300f, -0.421696f, 0.730398f, 0, 
		-0.780626f, 0.464092f, 0.000000f, 1
	};
    
	Collide_Boxes_With_Spheres(&Transform, utest_result);
	Collide_Boxes_With_Spheres(&Transform2, utest_result);
}

UTEST(Geometry, TestCastSphereSphereMiss) {
    scratch Scratch;
    gjk_support Sphere = GJK_Sphere(Scratch.Arena, V3_Zero(), 1.0f);
    m4 Transform = M4_Translate(V3(-10.0f, 2.1f, 0.0f));
    ray_cast Cast = Cast_Shape(&Sphere, 0.0f, &Sphere, 0.0f, &Transform, V3(20.0f, 0, 0), FLT_MAX, 1e-4f, 1e-4f);
    ASSERT_EQ(Cast.tHit, FLT_MAX);
}

UTEST(Geometry, TestCastSphereSphereInitialOverlap) {
    scratch Scratch;
    gjk_support Sphere = GJK_Sphere(Scratch.Arena, V3_Zero(), 1.0f);
    m4 Transform = M4_Translate(V3(-1, 0, 0));
    ray_cast Cast = Cast_Shape(&Sphere, 0.0f, &Sphere, 0.0f, &Transform, V3(10.0f, 0.0f, 0.0f), FLT_MAX, 1e-4f, 1e-4f);
    ASSERT_EQ(Cast.tHit, 0);
    ASSERT_TRUE(V3_Is_Close(Cast.P1, V3_Zero(), 5.0e-3f));
    ASSERT_TRUE(V3_Is_Close(Cast.P2, V3(-1, 0, 0), 5.0e-3f));
    ASSERT_TRUE(V3_Is_Close(V3_Norm(Cast.V), V3(1, 0, 0), 1.0e-2f));
}

UTEST(Geometry, TestCastSphereSphereHit) {
    scratch Scratch;
    gjk_support Sphere = GJK_Sphere(Scratch.Arena, V3_Zero(), 1.0f);
    m4 Transform = M4_Translate(V3(-10, 0, 0));
    ray_cast Cast = Cast_Shape(&Sphere, 0.0f, &Sphere, 0.0f, &Transform, V3(20, 0, 0), FLT_MAX, 1e-4f, 1e-4f);
    ASSERT_TRUE(Equal_Approx_F32(Cast.tHit, 8.0f/20.0f, 1e-6f));
    ASSERT_TRUE(V3_Is_Close(Cast.P1, V3(-1, 0, 0), 1e-6f));
    ASSERT_TRUE(V3_Is_Close(Cast.P2, V3(-1, 0, 0), 1e-6f));
    ASSERT_TRUE(V3_Is_Close(V3_Norm(Cast.V), V3(1, 0, 0), 1e-6f));
}

function f32 Convex_Hull_Test_Random_Unorm_Range(random32_xor_shift* R, f32 Lo, f32 Hi) {
	return Lo + (Hi - Lo) * Random32_XOrShift_UNorm(R);
}

function v3 Convex_Hull_Test_Random_Unit_Sphere(random32_xor_shift* R) {
	for (int Tries = 0; Tries < 32; ++Tries) {
		v3 v = V3(Random32_XOrShift_SNorm(R), Random32_XOrShift_SNorm(R), Random32_XOrShift_SNorm(R));
		f32 M2 = V3_Sq_Mag(v);
		if (M2 > 1.0e-8f)
			return V3_Norm(v);
	}
	return V3(1, 0, 0);
}

function v3 Chb_Test_Unit_Spherical(f32 Theta, f32 Phi) {
	f32 SinT = Sin_F32(Theta);
	return V3(SinT * Cos_F32(Phi), SinT * Sin_F32(Phi), Cos_F32(Theta));
}

function convex_hull_build_result Chb_Test_Convex_Hull_Build(
	allocator* OutAlloc,
	v3 const* Points,
	u32 PointCount,
	f32 Tol,
	convex_hull_mesh* Mesh) {
	const char* Err = 0;
	v3_array Vertices = { .Ptr = (v3*)Points, .Count = PointCount };
	return Convex_Hull_Build(OutAlloc, Vertices, (s32)0x7fffffff, Tol, Mesh, &Err);
}

UTEST(Geometry, ConvexHull_TestDegenerate) {
	scratch Scratch;
	const f32 cTol = 1.0e-3f;
	convex_hull_mesh Mesh = { 0 };
	v3 P[4];
	P[0] = V3(1, 2, 3);
	ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, P, 1, cTol, &Mesh), CONVEX_HULL_BUILD_TOO_FEW_POINTS);
	P[1] = V3(1 + 0.5f * cTol, 2, 3);
	ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, P, 2, cTol, &Mesh), CONVEX_HULL_BUILD_TOO_FEW_POINTS);
	P[2] = V3(1, 2 + 0.5f * cTol, 3);
	ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, P, 3, cTol, &Mesh), CONVEX_HULL_BUILD_DEGENERATE);
	P[3] = V3(1, 2, 3 + 0.5f * cTol);
	ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, P, 4, cTol, &Mesh), CONVEX_HULL_BUILD_DEGENERATE);
	{
		v3 Line[12];
		int N = 0;
		for (f32 v = 0.0f; v < 1.01f; v += 0.1f)
			Line[N++] = V3(v, 0, 0);
		ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, Line, (u32)N, cTol, &Mesh), CONVEX_HULL_BUILD_DEGENERATE);
	}
}

UTEST(Geometry, ConvexHull_Test2D) {
	scratch Scratch;
	const f32 cTol = 1.0e-3f;
	convex_hull_mesh Mesh = { 0 };
	{
		v3 Tri[3] = { V3(-1, 0, -1), V3(1, 0, -1), V3(-1, 0, 1) };
		ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, Tri, 3, cTol, &Mesh), CONVEX_HULL_BUILD_SUCCESS);
		ASSERT_EQ(Convex_Hull_Mesh_Get_Num_Vertices_Used(&Mesh), 3);
		ASSERT_EQ(Mesh.FaceCount, 2u);
		s32 F1[] = { 0, 1, 2 };
		s32 F2[] = { 2, 1, 0 };
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, F1, 3));
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, F2, 3));
	}
	{
		v3 Grid[100];
		int I = 0;
		// Same point order as Jolt Test2DHull quad: for (x) for (z) -> linear index i = z + 10 * x.
		for (int x = 0; x < 10; ++x)
			for (int z = 0; z < 10; ++z)
				Grid[I++] = V3(0.1f * (f32)x, 0, 0.2f * (f32)z);
		ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, Grid, 100, cTol, &Mesh), CONVEX_HULL_BUILD_SUCCESS);
		ASSERT_EQ(Convex_Hull_Mesh_Get_Num_Vertices_Used(&Mesh), 4);
		ASSERT_EQ(Mesh.FaceCount, 2u);
		s32 Q1[] = { 0, 9, 99, 90 };
		s32 Q2[] = { 90, 99, 9, 0 };
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, Q1, 4));
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, Q2, 4));
	}
	{
		v3 Disc[100];
		int I = 0;
		for (int r = 0; r < 10; ++r)
			for (int phi = 0; phi < 10; ++phi) {
				f32 f_r = 2.0f * (f32)r;
				f32 f_phi = 2.0f * PI * (f32)phi / 10.0f;
				Disc[I++] = V3(f_r * Cos_F32(f_phi), f_r * Sin_F32(f_phi), 0);
			}
		ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, Disc, 100, cTol, &Mesh), CONVEX_HULL_BUILD_SUCCESS);
		ASSERT_EQ(Convex_Hull_Mesh_Get_Num_Vertices_Used(&Mesh), 10);
		ASSERT_EQ(Mesh.FaceCount, 2u);
		s32 Ring1[] = { 90, 91, 92, 93, 94, 95, 96, 97, 98, 99 };
		s32 Ring2[] = { 99, 98, 97, 96, 95, 94, 93, 92, 91, 90 };
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, Ring1, 10));
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, Ring2, 10));
	}
}

UTEST(Geometry, ConvexHull_Test3D) {
	scratch Scratch;
	const f32 cTol = 1.0e-3f;
	convex_hull_mesh Mesh = { 0 };
	{
		v3 Cube[1000];
		int I = 0;
		// Same point order as Jolt Test3DHull cube: for (x) for (y) for (z) -> i = 100 * x + 10 * y + z.
		for (int x = 0; x < 10; ++x)
			for (int y = 0; y < 10; ++y)
				for (int z = 0; z < 10; ++z)
					Cube[I++] = V3(0.1f * (f32)x, 1.0f + 0.2f * (f32)y, 0.6f * (f32)z);
		ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, Cube, 1000, cTol, &Mesh), CONVEX_HULL_BUILD_SUCCESS);
		ASSERT_EQ(Convex_Hull_Mesh_Get_Num_Vertices_Used(&Mesh), 8);
		ASSERT_EQ(Mesh.FaceCount, 6u);
		s32 Face1[] = { 0, 9, 99, 90 };
		s32 Face2[] = { 0, 90, 990, 900 };
		s32 Face3[] = { 900, 990, 999, 909 };
		s32 Face4[] = { 9, 909, 999, 99 };
		s32 Face5[] = { 90, 99, 999, 990 };
		s32 Face6[] = { 0, 900, 909, 9 };
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, Face1, 4));
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, Face2, 4));
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, Face3, 4));
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, Face4, 4));
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, Face5, 4));
		ASSERT_TRUE(Convex_Hull_Mesh_Contains_Face(&Mesh, Face6, 4));
	}
	{
		v3 SpherePts[1000];
		int I = 0;
		for (int r = 0; r < 10; ++r)
			for (int phi = 0; phi < 10; ++phi)
				for (int theta = 0; theta < 10; ++theta) {
					f32 f_r = 2.0f * (f32)r;
					f32 f_phi = 2.0f * PI * (f32)phi / 10.0f;
					f32 f_theta = PI * (f32)theta / 9.0f;
					SpherePts[I++] = V3_Mul_S(Chb_Test_Unit_Spherical(f_theta, f_phi), f_r);
				}
		ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, SpherePts, 1000, cTol, &Mesh), CONVEX_HULL_BUILD_SUCCESS);
		ASSERT_EQ(Convex_Hull_Mesh_Get_Num_Vertices_Used(&Mesh), 82);
		u32 ErrFace = 0;
		f32 MaxErr = 0;
		s32 ErrPos = 0;
		f32 CoplanarDist = 0;
		Convex_Hull_Mesh_Determine_Max_Error(&Mesh, &ErrFace, &MaxErr, &ErrPos, &CoplanarDist);
		ASSERT_TRUE(MaxErr < Max(CoplanarDist, cTol));
	}
}

UTEST(Geometry, ConvexHull_TestRandom) {
	scratch Scratch;
	const f32 cTol = 1.0e-3f;
	random32_xor_shift Rng = { .State = 0x1ee7c0deu };
	convex_hull_mesh Mesh = { 0 };
	v3 Positions[400];
	for (int Iter = 0; Iter < 100; ++Iter) {
		f32 start = Convex_Hull_Test_Random_Unorm_Range(&Rng, 0.1f, 0.5f);
		f32 vertex_scale_hi = start + Convex_Hull_Test_Random_Unorm_Range(&Rng, 0.1f, 2.0f);
		v3 scale = V3(
			Convex_Hull_Test_Random_Unorm_Range(&Rng, 0.1f, 1.0f),
			Convex_Hull_Test_Random_Unorm_Range(&Rng, 0.1f, 1.0f),
			Convex_Hull_Test_Random_Unorm_Range(&Rng, 0.1f, 1.0f));
		u32 Count = 0;
		for (int i = 0; i < 100; ++i) {
			f32 vs = Convex_Hull_Test_Random_Unorm_Range(&Rng, start, vertex_scale_hi);
			v3 p1 = V3_Mul_V3(V3_Mul_S(Convex_Hull_Test_Random_Unit_Sphere(&Rng), vs), scale);
			Positions[Count++] = p1;
			v3 p2 = V3_Add_V3(p1, V3_Mul_S(Convex_Hull_Test_Random_Unit_Sphere(&Rng), cTol * Convex_Hull_Test_Random_Unorm_Range(&Rng, 0.0f, 2.0f)));
			Positions[Count++] = p2;
			f32 fraction = Convex_Hull_Test_Random_Unorm_Range(&Rng, 0.0f, 1.0f);
			u32 pick = Count > 0 ? (Random32_XOrShift(&Rng) % Count) : 0u;
			v3 p3 = V3_Add_V3(V3_Mul_S(p1, fraction), V3_Mul_S(Positions[pick], 1.0f - fraction));
			Positions[Count++] = p3;
			v3 p4 = V3_Add_V3(p3, V3_Mul_S(Convex_Hull_Test_Random_Unit_Sphere(&Rng), cTol * Convex_Hull_Test_Random_Unorm_Range(&Rng, 0.0f, 2.0f)));
			Positions[Count++] = p4;
		}
		ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, Positions, Count, cTol, &Mesh), CONVEX_HULL_BUILD_SUCCESS);
		u32 ErrFace = 0;
		f32 MaxErr = 0;
		s32 ErrPos = 0;
		f32 CoplanarDist = 0;
		Convex_Hull_Mesh_Determine_Max_Error(&Mesh, &ErrFace, &MaxErr, &ErrPos, &CoplanarDist);
		ASSERT_TRUE(MaxErr < Max(CoplanarDist, 1.2f * cTol));
	}
}

UTEST(Geometry, ConvexHull_TestEdgeCases) {
	scratch Scratch;
	const f32 cTol = 1.0e-3f;
	convex_hull_mesh Mesh = { 0 };
	{
		v3 P[] = {
			V3(-0.020472288f, -0.195635557f, 0.308015466f),
			V3(0.136248738f, 0.633286834f, 0.135366619f),
			V3(0.286418647f, -0.228475571f, 0.308084548f),
			V3(-0.267285109f, 1.024676085f, 0.308042824f),
			V3(0.396568149f, -0.971658647f, 0.308055162f),
			V3(0.321081549f, -1.024676085f, 0.308036327f),
			V3(0.034643859f, -0.404506862f, 0.308015764f),
			V3(0.189224690f, -0.252762139f, 0.308060408f),
		};
		ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, P, sizeof(P) / sizeof(P[0]), cTol, &Mesh), CONVEX_HULL_BUILD_SUCCESS);
		u32 ErrFace = 0;
		f32 MaxErr = 0;
		s32 ErrPos = 0;
		f32 CoplanarDist = 0;
		Convex_Hull_Mesh_Determine_Max_Error(&Mesh, &ErrFace, &MaxErr, &ErrPos, &CoplanarDist);
		ASSERT_TRUE(MaxErr < Max(CoplanarDist, 1.2f * cTol));
	}
	{
		v3 P[] = {
			V3(0.917345762f, 0.157111734f, 1.650970459f),
			V3(-0.098074198f, 0.157116055f, 0.664742708f),
			V3(1.777100325f, 0.157112047f, 1.238879442f),
			V3(2.114324570f, 0.157112464f, 0.780688763f),
			V3(1.926570415f, 0.157114446f, 0.240761161f),
			V3(-1.045998096f, 0.157108605f, 1.548911095f),
			V3(-1.820045233f, 0.157106474f, 1.050360918f),
			V3(-1.918573976f, 0.157108605f, 0.039246202f),
			V3(0.042619467f, 0.157113969f, -1.405336142f),
			V3(0.575986624f, 0.157114401f, -1.370834589f),
			V3(1.402592659f, 0.157115221f, -0.834864557f),
			V3(1.110557318f, 0.157113969f, -1.336267948f),
			V3(1.689781666f, 0.157115355f, -0.308773756f),
			V3(2.205337524f, 0.157113209f, -0.281754494f),
			V3(-1.346967936f, 0.157110974f, -0.978962541f),
			V3(-1.346967936f, 0.157110974f, -0.978962541f),
			V3(-2.085033417f, 0.157106936f, -0.506602883f),
			V3(-0.981224537f, 0.157110706f, -1.445893764f),
			V3(-0.481085658f, 0.157112658f, -1.426232934f),
			V3(-0.981224537f, 0.157110706f, -1.445893764f),
		};
		ASSERT_EQ(Chb_Test_Convex_Hull_Build(&Scratch, P, sizeof(P) / sizeof(P[0]), cTol, &Mesh), CONVEX_HULL_BUILD_SUCCESS);
		u32 ErrFace = 0;
		f32 MaxErr = 0;
		s32 ErrPos = 0;
		f32 CoplanarDist = 0;
		Convex_Hull_Mesh_Determine_Max_Error(&Mesh, &ErrFace, &MaxErr, &ErrPos, &CoplanarDist);
		ASSERT_TRUE(MaxErr < Max(CoplanarDist, 1.2f * cTol));
	}
}