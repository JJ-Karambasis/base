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