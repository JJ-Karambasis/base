#ifndef CONVEX_HULL_BUILDER_H
#define CONVEX_HULL_BUILDER_H

typedef enum {
    CONVEX_HULL_BUILD_SUCCESS,
    CONVEX_HULL_BUILD_MAX_VERTICES_REACHED,
    CONVEX_HULL_BUILD_TOO_FEW_POINTS,
    CONVEX_HULL_BUILD_TOO_FEW_FACES,
    CONVEX_HULL_BUILD_DEGENERATE,
    CONVEX_HULL_BUILD_OUT_OF_MEMORY,
} convex_hull_build_result;

/// One polygon of the hull (counter-clockwise vertex indices into convex_hull_mesh.Positions).
typedef struct {
    v3 Normal;
    v3 Centroid;
    s32* VertexIndices;
    u32 VertexCount;
} convex_hull_face;

/// Final hull: owns Positions (full input copy) and per-face index lists.
typedef struct {
    v3* Positions;
    u32 PositionCount;
    convex_hull_face* Faces;
    u32 FaceCount;
} convex_hull_mesh;

/// Builds a convex hull from Vertices. Uses scratch arenas for all temporary data;
/// OutMesh is filled from Allocator (deep copy of geometry). On failure, OutMesh is cleared.
/// Requires Thread_Context_Get / Scratch_Get stack (same as other scratch-based base APIs).
export_function convex_hull_build_result Convex_Hull_Build(
    allocator* Allocator,
    v3_array Vertices,
    s32 MaxVertices,
    f32 Tolerance,
    convex_hull_mesh* OutMesh,
    string* OutError);

export_function void Convex_Hull_Mesh_Free(allocator* Allocator, convex_hull_mesh* Mesh);

export_function s32 Convex_Hull_Mesh_Get_Num_Vertices_Used(convex_hull_mesh const* Mesh);

/// True if some hull face matches `Indices` in array order after rotating so `Indices` begins at
/// that face's first stored vertex (same rule as Jolt `ConvexHullBuilder::ContainsFace`).
export_function b32 Convex_Hull_Mesh_Contains_Face(
    convex_hull_mesh const* Mesh,
    s32 const* Indices,
    u32 IndexCount);

export_function void Convex_Hull_Mesh_Get_Center_Of_Mass_And_Volume(
    convex_hull_mesh const* Mesh,
    v3* OutCenterOfMass,
    f32* OutVolume);

/// If no error vertex is found, OutFaceIndex is set to (u32)-1 and OutMaxErrorPositionIdx to -1.
export_function void Convex_Hull_Mesh_Determine_Max_Error(
    convex_hull_mesh const* Mesh,
    u32* OutFaceIndex,
    f32* OutMaxError,
    s32* OutMaxErrorPositionIdx,
    f32* OutCoplanarDistance);

#endif
