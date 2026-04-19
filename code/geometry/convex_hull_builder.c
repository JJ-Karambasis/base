#define CHB_MIN_TRIANGLE_AREA_SQ 1.0e-12f
#define CHB_COPLANAR_SLOP_FACTOR 6.0f
#define CHB_MAX_EDGE_STACK 128

typedef struct chb_work_edge chb_work_edge;
typedef struct chb_work_face chb_work_face;

struct chb_work_edge {
    chb_work_face* Face;
    chb_work_edge* NextEdge;
    chb_work_edge* NeighbourEdge;
    s32 StartIdx;
};

Dynamic_Array_Define(s32, s32);

struct chb_work_face {
    v3 Normal;
    v3 Centroid;
    chb_work_edge* FirstEdge;
    dynamic_s32_array Conflict;
    f32 FurthestPointDistanceSq;
    b32 Removed;
};

typedef struct {
    s32 PositionIdx;
    f32 DistanceSq;
} chb_coplanar;

typedef struct {
    chb_work_edge* NeighbourEdge;
    s32 StartIdx;
    s32 EndIdx;
} chb_full_edge;

Dynamic_Array_Define(chb_coplanar, coplanar_entry);
Dynamic_Array_Define(chb_full_edge, full_edge);
Dynamic_Array_Define(chb_work_face*, work_face_ptr);

typedef struct chb2_work_edge chb2_work_edge;
struct chb2_work_edge {
    v3 Normal;
    v3 Center;
    dynamic_s32_array Conflict;
    chb2_work_edge* PrevEdge;
    chb2_work_edge* NextEdge;
    s32 StartIdx;
    f32 FurthestPointDistanceSq;
};

typedef struct {
    v3 const* Positions;
    u32 PositionCount;
    arena* ScratchArena;
    dynamic_work_face_ptr_array Faces;
    dynamic_coplanar_entry_array CoplanarList;
} chb_ctx;

Dynamic_Array_Implement(s32, s32, S32);
Dynamic_Array_Implement(coplanar_entry, chb_coplanar, Coplanar_Entry);
Dynamic_Array_Implement(full_edge, chb_full_edge, Full_Edge);
Dynamic_Array_Implement(work_face_ptr, chb_work_face*, Work_Face_Ptr);

function void Dynamic_S32_Array_Insert_Before_Last(dynamic_s32_array* Array, s32 Value) {
    if (Array->Count <= 1) {
        Dynamic_S32_Array_Add(Array, Value);
        return;
    }
    if (Array->Count == Array->Capacity)
        Dynamic_S32_Array_Reserve(Array, Max(Array->Capacity * 2, Array->Count + 1));
    size_t InsertAt = Array->Count - 1;
    Memory_Copy(Array->Ptr + InsertAt + 1, Array->Ptr + InsertAt, (Array->Count - InsertAt) * sizeof(s32));
    Array->Ptr[InsertAt] = Value;
    Array->Count++;
}

function v3 Chb_V3_Abs_Max(v3 A, v3 B) {
    return V3(Max(A.x, B.x), Max(A.y, B.y), Max(A.z, B.z));
}

function chb_work_edge* Chb_Work_Edge_Get_Previous(chb_work_edge* Self) {
    chb_work_edge* Prev = Self;
    while (Prev->NextEdge != Self)
        Prev = Prev->NextEdge;
    return Prev;
}

function void Chb_Work_Face_Calculate_Normal_And_Centroid(chb_work_face* F, v3 const* Positions) {
    chb_work_edge* E = F->FirstEdge;
    v3 Y0 = Positions[E->StartIdx];
    E = E->NextEdge;
    v3 Y1 = Positions[E->StartIdx];
    v3 Centroid = V3_Add_V3(Y0, Y1);
    int N = 2;
    v3 Normal = V3_Zero();
    for (E = E->NextEdge; E != F->FirstEdge; E = E->NextEdge) {
        v3 Y2 = Positions[E->StartIdx];
        v3 E0 = V3_Sub_V3(Y1, Y0);
        v3 E1 = V3_Sub_V3(Y2, Y1);
        v3 E2 = V3_Sub_V3(Y0, Y2);
        f32 E1Sq = V3_Sq_Mag(E1);
        f32 E2Sq = V3_Sq_Mag(E2);
        v3 NormalE01 = V3_Cross(E0, E1);
        v3 NormalE02 = V3_Cross(E2, E0);
        Normal = V3_Add_V3(Normal, (E1Sq < E2Sq) ? NormalE01 : NormalE02);
        Centroid = V3_Add_V3(Centroid, Y2);
        N++;
        Y1 = Y2;
    }
    F->Centroid = V3_Div_S(Centroid, (f32)N);
    F->Normal = Normal;
}

function void Chb_Work_Face_Initialize(
    chb_work_face* F,
    s32 Idx0,
    s32 Idx1,
    s32 Idx2,
    v3 const* Positions,
    arena* ScratchArena) {
    Assert(F->FirstEdge == 0);
    chb_work_edge* E0 = Arena_Push_Struct(ScratchArena, chb_work_edge);
    chb_work_edge* E1 = Arena_Push_Struct(ScratchArena, chb_work_edge);
    chb_work_edge* E2 = Arena_Push_Struct(ScratchArena, chb_work_edge);
    Zero_Struct(*E0);
    Zero_Struct(*E1);
    Zero_Struct(*E2);
    E0->Face = F;
    E1->Face = F;
    E2->Face = F;
    E0->StartIdx = Idx0;
    E1->StartIdx = Idx1;
    E2->StartIdx = Idx2;
    E0->NextEdge = E1;
    E1->NextEdge = E2;
    E2->NextEdge = E0;
    F->FirstEdge = E0;
    Chb_Work_Face_Calculate_Normal_And_Centroid(F, Positions);
}

function b32 Chb_Work_Face_Is_Facing(chb_work_face const* F, v3 Position) {
    Assert(!F->Removed);
    return V3_Dot(F->Normal, V3_Sub_V3(Position, F->Centroid)) > 0.0f;
}

function f32 Chb_Closest_Point_On_Segment_Sq(v3 Point, v3 A, v3 B) {
    v3 AB = V3_Sub_V3(B, A);
    v3 AP = V3_Sub_V3(Point, A);
    f32 AB2 = V3_Sq_Mag(AB);
    if (AB2 < FLT_EPSILON)
        return V3_Sq_Mag(AP);
    f32 T = V3_Dot(AP, AB) / AB2;
    T = Clamp(0.0f, T, 1.0f);
    v3 Closest = V3_Add_V3(A, V3_Mul_S(AB, T));
    return V3_Sq_Mag(V3_Sub_V3(Point, Closest));
}

function void Chb_Link_Face(chb_work_edge* E1, chb_work_edge* E2) {
    Assert(E1->NeighbourEdge == 0 && E2->NeighbourEdge == 0);
    Assert(E1->Face != E2->Face);
    Assert(E1->StartIdx == E2->NextEdge->StartIdx);
    Assert(E2->StartIdx == E1->NextEdge->StartIdx);
    E1->NeighbourEdge = E2;
    E2->NeighbourEdge = E1;
}

function void Chb_Unlink_Face(chb_work_face* Face) {
    chb_work_edge* E = Face->FirstEdge;
    do {
        if (E->NeighbourEdge) {
            Assert(E->NeighbourEdge->NeighbourEdge == E);
            E->NeighbourEdge->NeighbourEdge = 0;
            E->NeighbourEdge = 0;
        }
        E = E->NextEdge;
    } while (E != Face->FirstEdge);
}

function void Chb_Get_Face_For_Point(
    v3 Point,
    dynamic_work_face_ptr_array const* Faces,
    chb_work_face** OutFace,
    f32* OutDistSq) {
    *OutFace = 0;
    *OutDistSq = 0.0f;
    for (size_t I = 0; I < Faces->Count; ++I) {
        chb_work_face* F = Faces->Ptr[I];
        if (F->Removed)
            continue;
        f32 Dot = V3_Dot(F->Normal, V3_Sub_V3(Point, F->Centroid));
        if (Dot > 0.0f) {
            f32 NSq = V3_Sq_Mag(F->Normal);
            f32 DistSq = (NSq > 0.0f) ? (Dot * Dot / NSq) : 0.0f;
            if (DistSq > *OutDistSq) {
                *OutFace = F;
                *OutDistSq = DistSq;
            }
        }
    }
}

function f32 Chb_Get_Distance_To_Edge_Sq(chb_ctx const* Ctx, v3 Point, chb_work_face const* Face) {
    b32 AllInside = true;
    f32 EdgeDistSq = FLT_MAX;
    chb_work_edge* Edge = Face->FirstEdge;
    v3 P1 = Ctx->Positions[Chb_Work_Edge_Get_Previous(Edge)->StartIdx];
    do {
        v3 P2 = Ctx->Positions[Edge->StartIdx];
        if (V3_Dot(V3_Cross(V3_Sub_V3(P2, P1), V3_Sub_V3(Point, P1)), Face->Normal) < 0.0f) {
            AllInside = false;
            f32 D = Chb_Closest_Point_On_Segment_Sq(Point, P1, P2);
            EdgeDistSq = Min(EdgeDistSq, D);
        }
        P1 = P2;
        Edge = Edge->NextEdge;
    } while (Edge != Face->FirstEdge);
    return AllInside ? 0.0f : EdgeDistSq;
}

function f32 Chb_Determine_Coplanar_Distance(chb_ctx const* Ctx) {
    v3 VMax = V3_Zero();
    for (u32 I = 0; I < Ctx->PositionCount; ++I) {
        v3 V = Ctx->Positions[I];
        v3 Av = V3(Abs(V.x), Abs(V.y), Abs(V.z));
        VMax = Chb_V3_Abs_Max(VMax, Av);
    }
    return 3.0f * FLT_EPSILON * (VMax.x + VMax.y + VMax.z);
}

function b32 Chb_Assign_Point_To_Face(chb_ctx* Ctx, s32 PositionIdx, dynamic_work_face_ptr_array const* Faces, f32 ToleranceSq) {
    v3 Point = Ctx->Positions[PositionIdx];
    chb_work_face* BestFace;
    f32 BestDistSq;
    Chb_Get_Face_For_Point(Point, Faces, &BestFace, &BestDistSq);
    if (BestFace) {
        if (BestDistSq <= ToleranceSq) {
            f32 DistToEdgeSq = Chb_Get_Distance_To_Edge_Sq(Ctx, Point, BestFace);
            if (DistToEdgeSq > ToleranceSq) {
                chb_coplanar Entry = { .PositionIdx = PositionIdx, .DistanceSq = DistToEdgeSq };
                Dynamic_Coplanar_Entry_Array_Add(&Ctx->CoplanarList, Entry);
            }
        } else {
            if (BestDistSq > BestFace->FurthestPointDistanceSq) {
                BestFace->FurthestPointDistanceSq = BestDistSq;
                Dynamic_S32_Array_Add(&BestFace->Conflict, PositionIdx);
            } else {
                Dynamic_S32_Array_Insert_Before_Last(&BestFace->Conflict, PositionIdx);
            }
            return true;
        }
    }
    return false;
}

function void Chb_Add_Unique_Work_Face(dynamic_work_face_ptr_array* List, chb_work_face* Face) {
    for (size_t I = 0; I < List->Count; ++I)
        if (List->Ptr[I] == Face)
            return;
    Dynamic_Work_Face_Ptr_Array_Add(List, Face);
}

function void Chb_Free_Removed_Face(chb_work_face* Face) {
    Assert(Face->Removed);
    chb_work_edge* E = Face->FirstEdge;
    if (E) {
        do {
            chb_work_edge* Next = E->NextEdge;
            Assert(E->NeighbourEdge == 0);
            E = Next;
        } while (E != Face->FirstEdge);
    }
    Dynamic_S32_Array_Clear(&Face->Conflict);
    Face->FirstEdge = 0;
}

function void Chb_Garbage_Collect_Faces(chb_ctx* Ctx) {
    for (int I = (int)Ctx->Faces.Count - 1; I >= 0; --I) {
        chb_work_face* F = Ctx->Faces.Ptr[I];
        if (F->Removed) {
            Chb_Free_Removed_Face(F);
            Dynamic_Work_Face_Ptr_Array_Remove_Index(&Ctx->Faces, (size_t)I);
        }
    }
}

function chb_work_face* Chb_Create_Face(chb_ctx* Ctx) {
    chb_work_face* F = Arena_Push_Struct(Ctx->ScratchArena, chb_work_face);
    Zero_Struct(*F);
    F->Conflict = Dynamic_S32_Array_Init_With_Size((allocator*)Ctx->ScratchArena, 8);
    Dynamic_Work_Face_Ptr_Array_Add(&Ctx->Faces, F);
    return F;
}

function chb_work_face* Chb_Create_Triangle(chb_ctx* Ctx, s32 Idx1, s32 Idx2, s32 Idx3) {
    chb_work_face* F = Chb_Create_Face(Ctx);
    Chb_Work_Face_Initialize(F, Idx1, Idx2, Idx3, Ctx->Positions, Ctx->ScratchArena);
    return F;
}

function void Chb_Find_Edge(chb_ctx const* Ctx, chb_work_face* FacingFace, v3 Vertex, dynamic_full_edge_array* OutEdges) {
    Assert(OutEdges->Count == 0);
    Assert(Chb_Work_Face_Is_Facing(FacingFace, Vertex));
    FacingFace->Removed = true;
    typedef struct {
        chb_work_edge* FirstEdge;
        chb_work_edge* CurrentEdge;
    } stack_entry;
    stack_entry Stack[CHB_MAX_EDGE_STACK];
    int StackPos = 0;
    Stack[0].FirstEdge = FacingFace->FirstEdge;
    Stack[0].CurrentEdge = (chb_work_edge*)((uintptr_t)FacingFace->FirstEdge | (uintptr_t)1);
    for (;;) {
        stack_entry* Cur = &Stack[StackPos];
        chb_work_edge* RawE = Cur->CurrentEdge;
        chb_work_edge* E = (chb_work_edge*)((uintptr_t)RawE & ~(uintptr_t)1);
        Cur->CurrentEdge = E->NextEdge;
        if (RawE == Cur->FirstEdge) {
            Chb_Unlink_Face(E->Face);
            if (--StackPos < 0)
                break;
        } else {
            chb_work_edge* Ne = E->NeighbourEdge;
            if (Ne) {
                chb_work_face* N = Ne->Face;
                if (!N->Removed) {
                    if (Chb_Work_Face_Is_Facing(N, Vertex)) {
                        N->Removed = true;
                        StackPos++;
                        Assert(StackPos < CHB_MAX_EDGE_STACK);
                        Stack[StackPos].FirstEdge = Ne;
                        Stack[StackPos].CurrentEdge = Ne->NextEdge;
                    } else {
                        chb_full_edge Full = { .NeighbourEdge = Ne, .StartIdx = E->StartIdx, .EndIdx = Ne->StartIdx };
                        Dynamic_Full_Edge_Array_Add(OutEdges, Full);
                    }
                }
            }
        }
    }
    (void)Ctx;
}

function void Chb_Merge_Faces(chb_ctx* Ctx, chb_work_edge* InEdge) {
    chb_work_face* Face = InEdge->Face;
    chb_work_edge* NextEdge = InEdge->NextEdge;
    chb_work_edge* PrevEdge = Chb_Work_Edge_Get_Previous(InEdge);
    chb_work_edge* OtherEdge = InEdge->NeighbourEdge;
    chb_work_face* OtherFace = OtherEdge->Face;
    Assert(Face != OtherFace);
    chb_work_edge* Edge = OtherEdge->NextEdge;
    PrevEdge->NextEdge = Edge;
    for (;;) {
        Edge->Face = Face;
        if (Edge->NextEdge == OtherEdge) {
            Edge->NextEdge = NextEdge;
            break;
        }
        Edge = Edge->NextEdge;
    }
    if (Face->FirstEdge == InEdge)
        Face->FirstEdge = PrevEdge->NextEdge;
    InEdge->NeighbourEdge = 0;
    OtherEdge->NeighbourEdge = 0;
    OtherFace->FirstEdge = 0;
    OtherFace->Removed = true;
    Chb_Work_Face_Calculate_Normal_And_Centroid(Face, Ctx->Positions);
    if (Face->FurthestPointDistanceSq > OtherFace->FurthestPointDistanceSq) {
        if (OtherFace->Conflict.Count > 0) {
            if (Face->Conflict.Count == 0) {
                Face->Conflict = OtherFace->Conflict;
                Zero_Struct(OtherFace->Conflict);
            } else {
                size_t OldLast = Face->Conflict.Count - 1;
                Dynamic_S32_Array_Reserve(&Face->Conflict, Face->Conflict.Count + OtherFace->Conflict.Count);
                Memory_Copy(
                    Face->Conflict.Ptr + OldLast + OtherFace->Conflict.Count,
                    Face->Conflict.Ptr + OldLast,
                    sizeof(s32));
                Memory_Copy(
                    Face->Conflict.Ptr + OldLast,
                    OtherFace->Conflict.Ptr,
                    OtherFace->Conflict.Count * sizeof(s32));
                Face->Conflict.Count += OtherFace->Conflict.Count;
                Dynamic_S32_Array_Clear(&OtherFace->Conflict);
            }
        }
    } else {
        if (OtherFace->Conflict.Count > 0) {
            Dynamic_S32_Array_Add_Range(
                &Face->Conflict,
                OtherFace->Conflict.Ptr,
                OtherFace->Conflict.Count);
            Face->FurthestPointDistanceSq = OtherFace->FurthestPointDistanceSq;
            Dynamic_S32_Array_Clear(&OtherFace->Conflict);
        }
    }
    Dynamic_S32_Array_Clear(&OtherFace->Conflict);
}

function b32 Chb_Remove_Two_Edge_Face(chb_ctx const* Ctx, chb_work_face* Face, dynamic_work_face_ptr_array* IoAffected);

function void Chb_Remove_Invalid_Edges(chb_ctx* Ctx, chb_work_face* Face, dynamic_work_face_ptr_array* IoAffected);

function void Chb_Merge_Degenerate_Face(chb_ctx* Ctx, chb_work_face* Face, dynamic_work_face_ptr_array* IoAffected) {
    if (V3_Sq_Mag(Face->Normal) < CHB_MIN_TRIANGLE_AREA_SQ) {
        f32 MaxLenSq = 0.0f;
        chb_work_edge* LongestEdge = 0;
        chb_work_edge* E = Face->FirstEdge;
        v3 P1 = Ctx->Positions[E->StartIdx];
        do {
            chb_work_edge* Next = E->NextEdge;
            v3 P2 = Ctx->Positions[Next->StartIdx];
            f32 LenSq = V3_Sq_Mag(V3_Sub_V3(P2, P1));
            if (LenSq >= MaxLenSq) {
                MaxLenSq = LenSq;
                LongestEdge = E;
            }
            P1 = P2;
            E = Next;
        } while (E != Face->FirstEdge);
        Chb_Merge_Faces(Ctx, LongestEdge);
        Chb_Remove_Invalid_Edges(Ctx, Face, IoAffected);
    }
}

function void Chb_Merge_Coplanar_Or_Concave_Faces(chb_ctx* Ctx, chb_work_face* Face, f32 CoplanarTolSq, dynamic_work_face_ptr_array* IoAffected) {
    b32 Merged = false;
    chb_work_edge* Edge = Face->FirstEdge;
    do {
        chb_work_edge* NextEdge = Edge->NextEdge;
        chb_work_face const* OtherFace = Edge->NeighbourEdge->Face;
        v3 DeltaCentroid = V3_Sub_V3(OtherFace->Centroid, Face->Centroid);
        f32 DistOther = V3_Dot(Face->Normal, DeltaCentroid);
        f32 SignedOtherSq = Abs(DistOther) * DistOther;
        f32 DistFace = -V3_Dot(OtherFace->Normal, DeltaCentroid);
        f32 SignedFaceSq = Abs(DistFace) * DistFace;
        f32 FaceNSq = V3_Sq_Mag(Face->Normal);
        f32 OtherNSq = V3_Sq_Mag(OtherFace->Normal);
        if ((SignedOtherSq > -CoplanarTolSq * FaceNSq || SignedFaceSq > -CoplanarTolSq * OtherNSq)
            && V3_Dot(Face->Normal, OtherFace->Normal) > 0.0f) {
            Chb_Merge_Faces(Ctx, Edge);
            Merged = true;
        }
        Edge = NextEdge;
    } while (Edge != Face->FirstEdge);
    if (Merged)
        Chb_Remove_Invalid_Edges(Ctx, Face, IoAffected);
}

function b32 Chb_Remove_Two_Edge_Face(chb_ctx const* Ctx, chb_work_face* Face, dynamic_work_face_ptr_array* IoAffected) {
    (void)Ctx;
    chb_work_edge* Edge = Face->FirstEdge;
    chb_work_edge* NextEdge = Edge->NextEdge;
    Assert(Edge != NextEdge);
    if (NextEdge->NextEdge == Edge) {
        chb_work_edge* NeighbourEdge = Edge->NeighbourEdge;
        chb_work_face* NeighbourFace = NeighbourEdge->Face;
        chb_work_edge* NextNeighbourEdge = NextEdge->NeighbourEdge;
        chb_work_face* NextNeighbourFace = NextNeighbourEdge->Face;
        Chb_Add_Unique_Work_Face(IoAffected, NeighbourFace);
        Chb_Add_Unique_Work_Face(IoAffected, NextNeighbourFace);
        NeighbourEdge->NeighbourEdge = NextNeighbourEdge;
        NextNeighbourEdge->NeighbourEdge = NeighbourEdge;
        Edge->NeighbourEdge = 0;
        NextEdge->NeighbourEdge = 0;
        Face->Removed = true;
        return true;
    }
    return false;
}

function void Chb_Remove_Invalid_Edges(chb_ctx* Ctx, chb_work_face* Face, dynamic_work_face_ptr_array* IoAffected) {
    b32 RecalculatePlane = false;
    b32 Removed;
    do {
        Removed = false;
        chb_work_edge* Edge = Face->FirstEdge;
        chb_work_face* NeighbourFace = Edge->NeighbourEdge->Face;
        do {
            chb_work_edge* NextEdge = Edge->NextEdge;
            chb_work_face* NextNeighbourFace = NextEdge->NeighbourEdge->Face;
            if (NeighbourFace == Face) {
                if (Edge->NeighbourEdge == NextEdge) {
                    chb_work_edge* PrevEdge = Chb_Work_Edge_Get_Previous(Edge);
                    PrevEdge->NextEdge = NextEdge->NextEdge;
                    if (Face->FirstEdge == Edge || Face->FirstEdge == NextEdge)
                        Face->FirstEdge = PrevEdge;
                    if (Chb_Remove_Two_Edge_Face(Ctx, Face, IoAffected))
                        return;
                    RecalculatePlane = true;
                    Removed = true;
                    break;
                }
            } else if (NeighbourFace == NextNeighbourFace) {
                chb_work_edge* NeighbourEdge = NextEdge->NeighbourEdge;
                chb_work_edge* NextNeighbourEdge = NeighbourEdge->NextEdge;
                if (NeighbourFace->FirstEdge == NextNeighbourEdge)
                    NeighbourFace->FirstEdge = NeighbourEdge;
                NeighbourEdge->NextEdge = NextNeighbourEdge->NextEdge;
                NeighbourEdge->NeighbourEdge = Edge;
                if (Face->FirstEdge == NextEdge)
                    Face->FirstEdge = Edge;
                Edge->NextEdge = NextEdge->NextEdge;
                Edge->NeighbourEdge = NeighbourEdge;
                if (!Chb_Remove_Two_Edge_Face(Ctx, NeighbourFace, IoAffected)) {
                    Chb_Work_Face_Calculate_Normal_And_Centroid(NeighbourFace, Ctx->Positions);
                    Chb_Add_Unique_Work_Face(IoAffected, NeighbourFace);
                }
                if (Chb_Remove_Two_Edge_Face(Ctx, Face, IoAffected))
                    return;
                RecalculatePlane = true;
                Removed = true;
                break;
            }
            Edge = NextEdge;
            NeighbourFace = NextNeighbourFace;
        } while (Edge != Face->FirstEdge);
    } while (Removed);
    if (RecalculatePlane)
        Chb_Work_Face_Calculate_Normal_And_Centroid(Face, Ctx->Positions);
}

function void Chb_Add_Point(
    chb_ctx* Ctx,
    chb_work_face* FacingFace,
    s32 Idx,
    f32 CoplanarTolSq,
    dynamic_work_face_ptr_array* OutNewFaces) {
    v3 Pos = Ctx->Positions[Idx];
    dynamic_full_edge_array Edges = Dynamic_Full_Edge_Array_Init((allocator*)Ctx->ScratchArena);
    Chb_Find_Edge(Ctx, FacingFace, Pos, &Edges);
    Assert(Edges.Count >= 3);
    Dynamic_Work_Face_Ptr_Array_Clear(OutNewFaces);
    for (size_t I = 0; I < Edges.Count; ++I) {
        chb_full_edge const* E = &Edges.Ptr[I];
        Assert(E->StartIdx != E->EndIdx);
        chb_work_face* F = Chb_Create_Triangle(Ctx, E->StartIdx, E->EndIdx, Idx);
        Dynamic_Work_Face_Ptr_Array_Add(OutNewFaces, F);
    }
    for (size_t I = 0; I < OutNewFaces->Count; ++I) {
        Chb_Link_Face(OutNewFaces->Ptr[I]->FirstEdge, Edges.Ptr[I].NeighbourEdge);
        size_t J = (I + 1) % OutNewFaces->Count;
        Chb_Link_Face(
            OutNewFaces->Ptr[I]->FirstEdge->NextEdge,
            OutNewFaces->Ptr[J]->FirstEdge->NextEdge->NextEdge);
    }
    dynamic_work_face_ptr_array Affected = Dynamic_Work_Face_Ptr_Array_Init((allocator*)Ctx->ScratchArena);
    for (size_t I = 0; I < OutNewFaces->Count; ++I)
        Dynamic_Work_Face_Ptr_Array_Add(&Affected, OutNewFaces->Ptr[I]);
    while (Affected.Count > 0) {
        chb_work_face* Ff = Affected.Ptr[Affected.Count - 1];
        Affected.Count--;
        if (!Ff->Removed) {
            Chb_Merge_Degenerate_Face(Ctx, Ff, &Affected);
            if (!Ff->Removed)
                Chb_Merge_Coplanar_Or_Concave_Faces(Ctx, Ff, CoplanarTolSq, &Affected);
        }
    }
}

function s32 Chb_Get_Num_Vertices_Used_Work(chb_ctx const* Ctx) {
    allocator* Alloc = (allocator*)Ctx->ScratchArena;
    u32 ItemCap = Max((u32)32, Ctx->PositionCount);

    arena* Scratch = Scratch_Get();
    hashmap Used = Hashmap_Init((allocator*)Scratch, sizeof(u8), sizeof(s32), Hash_S32, Compare_S32);

    u8 Dummy = 1;
    for (size_t Fi = 0; Fi < Ctx->Faces.Count; ++Fi) {
        chb_work_face* F = Ctx->Faces.Ptr[Fi];
        if (F->Removed)
            continue;
        chb_work_edge* E = F->FirstEdge;
        do {
            s32 Idx = E->StartIdx;
            if (Idx >= 0 && (u32)Idx < Ctx->PositionCount) {
                if (!Hashmap_Find_Ptr(&Used, &Idx))
                    Hashmap_Add(&Used, &Idx, &Dummy);
            }
            E = E->NextEdge;
        } while (E != F->FirstEdge);
    }
    s32 Result = (s32)Used.ItemCount;
    Scratch_Release();
    return Result;
}

typedef enum {
    CHB2_SUCCESS,
    CHB2_MAX_VERTICES_REACHED,
} chb2_result;

function void Chb2_Edge_Calc_Normal_Center(chb2_work_edge* E, v3 const* Positions) {
    v3 P1 = Positions[E->StartIdx];
    v3 P2 = Positions[E->NextEdge->StartIdx];
    E->Center = V3_Mul_S(V3_Add_V3(P1, P2), 0.5f);
    v3 D = V3_Sub_V3(P2, P1);
    E->Normal = V3(D.y, -D.x, 0.0f);
}

function b32 Chb2_Is_Facing(chb2_work_edge const* E, v3 Point) {
    return V3_Dot(E->Normal, V3_Sub_V3(Point, E->Center)) > 0.0f;
}

function void Chb2_Assign_Point_To_Edge(
    v3 const* Positions,
    chb2_work_edge** Edges,
    size_t EdgeCount,
    s32 PositionIdx) {
    v3 Point = Positions[PositionIdx];
    chb2_work_edge* Best = 0;
    f32 BestDistSq = 0.0f;
    for (size_t I = 0; I < EdgeCount; ++I) {
        chb2_work_edge* Edge = Edges[I];
        f32 Dot = V3_Dot(Edge->Normal, V3_Sub_V3(Point, Edge->Center));
        if (Dot > 0.0f) {
            f32 NSq = V3_Sq_Mag(Edge->Normal);
            f32 DistSq = Dot * Dot / NSq;
            if (DistSq > BestDistSq) {
                Best = Edge;
                BestDistSq = DistSq;
            }
        }
    }
    if (Best) {
        if (BestDistSq > Best->FurthestPointDistanceSq) {
            Best->FurthestPointDistanceSq = BestDistSq;
            Dynamic_S32_Array_Add(&Best->Conflict, PositionIdx);
        } else {
            Dynamic_S32_Array_Insert_Before_Last(&Best->Conflict, PositionIdx);
        }
    }
}

function chb2_result Chb2_Initialize(
    v3 const* Positions,
    u32 PositionCount,
    s32 Idx1,
    s32 Idx2,
    s32 Idx3,
    s32 MaxVertices,
    f32 Tolerance,
    dynamic_s32_array* OutEdges,
    arena* ScratchArena) {
    Dynamic_S32_Array_Clear(OutEdges);
    v3 VMax = V3_Zero();
    for (u32 I = 0; I < PositionCount; ++I) {
        v3 V = Positions[I];
        VMax = Chb_V3_Abs_Max(VMax, V3(Abs(V.x), Abs(V.y), Abs(V.z)));
    }
    f32 ColinearTolSq = Sq(2.0f * FLT_EPSILON * (VMax.x + VMax.y));
    f32 ToleranceSq = Max(ColinearTolSq, Sq(Tolerance));
    f32 Z = V3_Cross(V3_Sub_V3(Positions[Idx2], Positions[Idx1]), V3_Sub_V3(Positions[Idx3], Positions[Idx1])).z;
    if (Z < 0.0f) {
        s32 T = Idx1;
        Idx1 = Idx2;
        Idx2 = T;
    }
    chb2_work_edge* E1 = Arena_Push_Struct(ScratchArena, chb2_work_edge);
    chb2_work_edge* E2 = Arena_Push_Struct(ScratchArena, chb2_work_edge);
    chb2_work_edge* E3 = Arena_Push_Struct(ScratchArena, chb2_work_edge);
    Zero_Struct(*E1);
    Zero_Struct(*E2);
    Zero_Struct(*E3);
    E1->Conflict = Dynamic_S32_Array_Init_With_Size((allocator*)ScratchArena, 8);
    E2->Conflict = Dynamic_S32_Array_Init_With_Size((allocator*)ScratchArena, 8);
    E3->Conflict = Dynamic_S32_Array_Init_With_Size((allocator*)ScratchArena, 8);
    E1->StartIdx = Idx1;
    E2->StartIdx = Idx2;
    E3->StartIdx = Idx3;
    E1->NextEdge = E2;
    E1->PrevEdge = E3;
    E2->NextEdge = E3;
    E2->PrevEdge = E1;
    E3->NextEdge = E1;
    E3->PrevEdge = E2;
    chb2_work_edge* FirstEdge = E1;
    int NumEdges = 3;
    chb2_work_edge* InitEdges[3] = { E1, E2, E3 };
    for (int I = 0; I < 3; ++I)
        Chb2_Edge_Calc_Normal_Center(InitEdges[I], Positions);
    for (u32 Idx = 0; Idx < PositionCount; ++Idx) {
        if ((s32)Idx != Idx1 && (s32)Idx != Idx2 && (s32)Idx != Idx3)
            Chb2_Assign_Point_To_Edge(Positions, InitEdges, 3, (s32)Idx);
    }
    chb2_result Result = CHB2_SUCCESS;
    for (;;) {
        if (NumEdges >= MaxVertices) {
            Result = CHB2_MAX_VERTICES_REACHED;
            break;
        }
        chb2_work_edge* EdgeWithFurthest = 0;
        f32 FurthestDistSq = 0.0f;
        chb2_work_edge* E = FirstEdge;
        do {
            if (E->FurthestPointDistanceSq > FurthestDistSq) {
                FurthestDistSq = E->FurthestPointDistanceSq;
                EdgeWithFurthest = E;
            }
            E = E->NextEdge;
        } while (E != FirstEdge);
        if (EdgeWithFurthest == 0 || FurthestDistSq < ToleranceSq)
            break;
        s32 FurthestIdx = EdgeWithFurthest->Conflict.Ptr[EdgeWithFurthest->Conflict.Count - 1];
        (void)Dynamic_S32_Array_Pop(&EdgeWithFurthest->Conflict);
        v3 FurthestPoint = Positions[FurthestIdx];
        chb2_work_edge* FirstHorizon = EdgeWithFurthest;
        do {
            chb2_work_edge* Prev = FirstHorizon->PrevEdge;
            if (!Chb2_Is_Facing(Prev, FurthestPoint))
                break;
            FirstHorizon = Prev;
        } while (FirstHorizon != EdgeWithFurthest);
        chb2_work_edge* LastHorizon = EdgeWithFurthest;
        do {
            chb2_work_edge* Next = LastHorizon->NextEdge;
            if (!Chb2_Is_Facing(Next, FurthestPoint))
                break;
            LastHorizon = Next;
        } while (LastHorizon != EdgeWithFurthest);
        chb2_work_edge* Ne1 = Arena_Push_Struct(ScratchArena, chb2_work_edge);
        chb2_work_edge* Ne2 = Arena_Push_Struct(ScratchArena, chb2_work_edge);
        Zero_Struct(*Ne1);
        Zero_Struct(*Ne2);
        Ne1->Conflict = Dynamic_S32_Array_Init_With_Size((allocator*)ScratchArena, 8);
        Ne2->Conflict = Dynamic_S32_Array_Init_With_Size((allocator*)ScratchArena, 8);
        Ne1->StartIdx = FirstHorizon->StartIdx;
        Ne2->StartIdx = FurthestIdx;
        Ne1->NextEdge = Ne2;
        Ne1->PrevEdge = FirstHorizon->PrevEdge;
        Ne2->PrevEdge = Ne1;
        Ne2->NextEdge = LastHorizon->NextEdge;
        Ne1->PrevEdge->NextEdge = Ne1;
        Ne2->NextEdge->PrevEdge = Ne2;
        FirstEdge = Ne1;
        NumEdges += 2;
        chb2_work_edge* NewEdges[2] = { Ne1, Ne2 };
        for (int I = 0; I < 2; ++I)
            Chb2_Edge_Calc_Normal_Center(NewEdges[I], Positions);
        for (;;) {
            chb2_work_edge* Next = FirstHorizon->NextEdge;
            for (size_t Ci = 0; Ci < FirstHorizon->Conflict.Count; ++Ci)
                Chb2_Assign_Point_To_Edge(Positions, NewEdges, 2, FirstHorizon->Conflict.Ptr[Ci]);
            Dynamic_S32_Array_Clear(&FirstHorizon->Conflict);
            NumEdges--;
            if (FirstHorizon == LastHorizon)
                break;
            FirstHorizon = Next;
        }
    }
    chb2_work_edge* EWalk = FirstEdge;
    do {
        Dynamic_S32_Array_Add(OutEdges, EWalk->StartIdx);
        EWalk = EWalk->NextEdge;
    } while (EWalk != FirstEdge);
    return Result;
}

function convex_hull_build_result Chb_Build_Internal(chb_ctx* Ctx, s32 MaxVertices, f32 Tolerance, const char** OutError) {
    Dynamic_Work_Face_Ptr_Array_Clear(&Ctx->Faces);
    Dynamic_Coplanar_Entry_Array_Clear(&Ctx->CoplanarList);
    if (Ctx->PositionCount < 3) {
        *OutError = "Need at least 3 points to make a hull";
        return CONVEX_HULL_BUILD_TOO_FEW_POINTS;
    }
    f32 CoplanarTolSq = Sq(Chb_Determine_Coplanar_Distance(Ctx));
    f32 ToleranceSq = Max(CoplanarTolSq, Sq(Tolerance));
    s32 Idx1 = -1;
    f32 MaxDistSq = -1.0f;
    for (u32 I = 0; I < Ctx->PositionCount; ++I) {
        f32 D = V3_Sq_Mag(Ctx->Positions[I]);
        if (D > MaxDistSq) {
            MaxDistSq = D;
            Idx1 = (s32)I;
        }
    }
    Assert(Idx1 >= 0);
    s32 Idx2 = -1;
    MaxDistSq = -1.0f;
    for (u32 I = 0; I < Ctx->PositionCount; ++I) {
        if ((s32)I == Idx1)
            continue;
        f32 D = V3_Sq_Mag(V3_Sub_V3(Ctx->Positions[I], Ctx->Positions[Idx1]));
        if (D > MaxDistSq) {
            MaxDistSq = D;
            Idx2 = (s32)I;
        }
    }
    Assert(Idx2 >= 0);
    s32 Idx3 = -1;
    f32 BestTriSq = -1.0f;
    for (u32 I = 0; I < Ctx->PositionCount; ++I) {
        if ((s32)I == Idx1 || (s32)I == Idx2)
            continue;
        f32 A = V3_Sq_Mag(
            V3_Cross(
                V3_Sub_V3(Ctx->Positions[Idx1], Ctx->Positions[I]),
                V3_Sub_V3(Ctx->Positions[Idx2], Ctx->Positions[I])));
        if (A > BestTriSq) {
            BestTriSq = A;
            Idx3 = (s32)I;
        }
    }
    Assert(Idx3 >= 0);
    if (BestTriSq < CHB_MIN_TRIANGLE_AREA_SQ) {
        *OutError = "Could not find a suitable initial triangle because its area was too small";
        return CONVEX_HULL_BUILD_DEGENERATE;
    }
    if (Ctx->PositionCount == 3) {
        chb_work_face* T1 = Chb_Create_Triangle(Ctx, Idx1, Idx2, Idx3);
        chb_work_face* T2 = Chb_Create_Triangle(Ctx, Idx1, Idx3, Idx2);
        Chb_Link_Face(T1->FirstEdge, T2->FirstEdge->NextEdge->NextEdge);
        Chb_Link_Face(T1->FirstEdge->NextEdge, T2->FirstEdge->NextEdge);
        Chb_Link_Face(T1->FirstEdge->NextEdge->NextEdge, T2->FirstEdge);
        return CONVEX_HULL_BUILD_SUCCESS;
    }
    v3 InitialN = V3_Norm(
        V3_Cross(
            V3_Sub_V3(Ctx->Positions[Idx2], Ctx->Positions[Idx1]),
            V3_Sub_V3(Ctx->Positions[Idx3], Ctx->Positions[Idx1])));
    v3 InitialC = V3_Div_S(
        V3_Add_V3(V3_Add_V3(Ctx->Positions[Idx1], Ctx->Positions[Idx2]), Ctx->Positions[Idx3]),
        3.0f);
    s32 Idx4 = -1;
    f32 MaxDist = 0.0f;
    for (u32 I = 0; I < Ctx->PositionCount; ++I) {
        if ((s32)I == Idx1 || (s32)I == Idx2 || (s32)I == Idx3)
            continue;
        f32 D = V3_Dot(V3_Sub_V3(Ctx->Positions[I], InitialC), InitialN);
        if (Abs(D) > Abs(MaxDist)) {
            MaxDist = D;
            Idx4 = (s32)I;
        }
    }
    if (Sq(MaxDist) <= Sq(CHB_COPLANAR_SLOP_FACTOR) * CoplanarTolSq) {
        v3 Base1 = V3_Get_Perp(InitialN);
        v3 Base2 = V3_Cross(InitialN, Base1);
        v3* Proj = Arena_Push_Array_No_Clear(Ctx->ScratchArena, Ctx->PositionCount, v3);
        for (u32 I = 0; I < Ctx->PositionCount; ++I) {
            v3 V = Ctx->Positions[I];
            Proj[I] = V3(V3_Dot(Base1, V), V3_Dot(Base2, V), 0.0f);
        }
        dynamic_s32_array Edges2d = Dynamic_S32_Array_Init((allocator*)Ctx->ScratchArena);
        chb2_result R2 = Chb2_Initialize(
            Proj,
            Ctx->PositionCount,
            Idx1,
            Idx2,
            Idx3,
            MaxVertices,
            Tolerance,
            &Edges2d,
            Ctx->ScratchArena);
        chb_work_face* F1 = Chb_Create_Face(Ctx);
        chb_work_face* F2 = Chb_Create_Face(Ctx);
        chb_work_edge** EF1 = Arena_Push_Array_No_Clear(Ctx->ScratchArena, Edges2d.Count, chb_work_edge*);
        chb_work_edge** EF2 = Arena_Push_Array_No_Clear(Ctx->ScratchArena, Edges2d.Count, chb_work_edge*);
        size_t EF1Count = 0;
        size_t EF2Count = 0;
        for (size_t I = 0; I < Edges2d.Count; ++I) {
            chb_work_edge* Edge = Arena_Push_Struct(Ctx->ScratchArena, chb_work_edge);
            Zero_Struct(*Edge);
            Edge->Face = F1;
            Edge->StartIdx = Edges2d.Ptr[I];
            EF1[EF1Count++] = Edge;
        }
        for (int I = (int)Edges2d.Count - 1; I >= 0; --I) {
            chb_work_edge* Edge = Arena_Push_Struct(Ctx->ScratchArena, chb_work_edge);
            Zero_Struct(*Edge);
            Edge->Face = F2;
            Edge->StartIdx = Edges2d.Ptr[I];
            EF2[EF2Count++] = Edge;
        }
        for (size_t I = 0; I + 1 < EF1Count; ++I)
            EF1[I]->NextEdge = EF1[I + 1];
        EF1[EF1Count - 1]->NextEdge = EF1[0];
        F1->FirstEdge = EF1[0];
        for (size_t I = 0; I + 1 < EF2Count; ++I)
            EF2[I]->NextEdge = EF2[I + 1];
        EF2[EF2Count - 1]->NextEdge = EF2[0];
        F2->FirstEdge = EF2[0];
        for (size_t I = 0; I < Edges2d.Count; ++I)
            Chb_Link_Face(EF1[I], EF2[(2 * Edges2d.Count - 2 - I) % Edges2d.Count]);
        Chb_Work_Face_Calculate_Normal_And_Centroid(F1, Ctx->Positions);
        F2->Normal = V3_Negate(F1->Normal);
        F2->Centroid = F1->Centroid;
        Dynamic_S32_Array_Clear(&Edges2d);
        return (R2 == CHB2_MAX_VERTICES_REACHED) ? CONVEX_HULL_BUILD_MAX_VERTICES_REACHED : CONVEX_HULL_BUILD_SUCCESS;
    }
    if (MaxDist < 0.0f) {
        s32 T = Idx2;
        Idx2 = Idx3;
        Idx3 = T;
    }
    chb_work_face* T1 = Chb_Create_Triangle(Ctx, Idx1, Idx2, Idx4);
    chb_work_face* T2 = Chb_Create_Triangle(Ctx, Idx2, Idx3, Idx4);
    chb_work_face* T3 = Chb_Create_Triangle(Ctx, Idx3, Idx1, Idx4);
    chb_work_face* T4 = Chb_Create_Triangle(Ctx, Idx1, Idx3, Idx2);
    Chb_Link_Face(T1->FirstEdge, T4->FirstEdge->NextEdge->NextEdge);
    Chb_Link_Face(T1->FirstEdge->NextEdge, T2->FirstEdge->NextEdge->NextEdge);
    Chb_Link_Face(T1->FirstEdge->NextEdge->NextEdge, T3->FirstEdge->NextEdge);
    Chb_Link_Face(T2->FirstEdge, T4->FirstEdge->NextEdge);
    Chb_Link_Face(T2->FirstEdge->NextEdge, T3->FirstEdge->NextEdge->NextEdge);
    Chb_Link_Face(T3->FirstEdge, T4->FirstEdge);
    dynamic_work_face_ptr_array InitFaces = Dynamic_Work_Face_Ptr_Array_Init((allocator*)Ctx->ScratchArena);
    Dynamic_Work_Face_Ptr_Array_Add(&InitFaces, T1);
    Dynamic_Work_Face_Ptr_Array_Add(&InitFaces, T2);
    Dynamic_Work_Face_Ptr_Array_Add(&InitFaces, T3);
    Dynamic_Work_Face_Ptr_Array_Add(&InitFaces, T4);
    for (u32 Idx = 0; Idx < Ctx->PositionCount; ++Idx) {
        if ((s32)Idx != Idx1 && (s32)Idx != Idx2 && (s32)Idx != Idx3 && (s32)Idx != Idx4)
            Chb_Assign_Point_To_Face(Ctx, (s32)Idx, &InitFaces, ToleranceSq);
    }
    Dynamic_Work_Face_Ptr_Array_Clear(&InitFaces);
    s32 NumVerticesUsed = 4;
    for (;;) {
        chb_work_face* FaceWithFurthest = 0;
        f32 FurthestDistSq = 0.0f;
        for (size_t I = 0; I < Ctx->Faces.Count; ++I) {
            chb_work_face* F = Ctx->Faces.Ptr[I];
            if (F->FurthestPointDistanceSq > FurthestDistSq) {
                FurthestDistSq = F->FurthestPointDistanceSq;
                FaceWithFurthest = F;
            }
        }
        s32 FurthestIdx;
        if (FaceWithFurthest) {
            if (FaceWithFurthest->Conflict.Count == 0)
                break;
            FurthestIdx = FaceWithFurthest->Conflict.Ptr[FaceWithFurthest->Conflict.Count - 1];
            (void)Dynamic_S32_Array_Pop(&FaceWithFurthest->Conflict);
            if (FaceWithFurthest->Conflict.Count == 0)
                FaceWithFurthest->FurthestPointDistanceSq = 0.0f;
        } else if (Ctx->CoplanarList.Count > 0) {
            dynamic_coplanar_entry_array OldCoplanar = Ctx->CoplanarList;
            Ctx->CoplanarList = Dynamic_Coplanar_Entry_Array_Init((allocator*)Ctx->ScratchArena);
            b32 Added = false;
            for (size_t I = 0; I < OldCoplanar.Count; ++I)
                Added |= Chb_Assign_Point_To_Face(Ctx, OldCoplanar.Ptr[I].PositionIdx, &Ctx->Faces, ToleranceSq);
            if (Added)
                continue;
            if (Ctx->CoplanarList.Count == 0)
                break;
            do {
                size_t BestIdx = 0;
                f32 BestDSq = Ctx->CoplanarList.Ptr[0].DistanceSq;
                for (size_t J = 1; J < Ctx->CoplanarList.Count; ++J) {
                    if (Ctx->CoplanarList.Ptr[J].DistanceSq > BestDSq) {
                        BestDSq = Ctx->CoplanarList.Ptr[J].DistanceSq;
                        BestIdx = J;
                    }
                }
                chb_coplanar SwapTmp = Ctx->CoplanarList.Ptr[BestIdx];
                Ctx->CoplanarList.Ptr[BestIdx] = Ctx->CoplanarList.Ptr[Ctx->CoplanarList.Count - 1];
                Ctx->CoplanarList.Ptr[Ctx->CoplanarList.Count - 1] = SwapTmp;
                FurthestIdx = Ctx->CoplanarList.Ptr[Ctx->CoplanarList.Count - 1].PositionIdx;
                (void)Dynamic_Coplanar_Entry_Array_Pop(&Ctx->CoplanarList);
                f32 BestDistSqPlane;
                Chb_Get_Face_For_Point(Ctx->Positions[FurthestIdx], &Ctx->Faces, &FaceWithFurthest, &BestDistSqPlane);
            } while (Ctx->CoplanarList.Count > 0 && FaceWithFurthest == 0);
            if (FaceWithFurthest == 0)
                break;
        } else
            break;
        if (NumVerticesUsed >= MaxVertices) {
            NumVerticesUsed = Chb_Get_Num_Vertices_Used_Work(Ctx);
            if (NumVerticesUsed >= MaxVertices)
                return CONVEX_HULL_BUILD_MAX_VERTICES_REACHED;
        }
        NumVerticesUsed++;
        dynamic_work_face_ptr_array NewFaces = Dynamic_Work_Face_Ptr_Array_Init((allocator*)Ctx->ScratchArena);
        Chb_Add_Point(Ctx, FaceWithFurthest, FurthestIdx, CoplanarTolSq, &NewFaces);
        for (size_t Fi = 0; Fi < Ctx->Faces.Count; ++Fi) {
            chb_work_face const* Face = Ctx->Faces.Ptr[Fi];
            if (Face->Removed) {
                for (size_t Ci = 0; Ci < Face->Conflict.Count; ++Ci)
                    Chb_Assign_Point_To_Face(Ctx, Face->Conflict.Ptr[Ci], &NewFaces, ToleranceSq);
            }
        }
        Chb_Garbage_Collect_Faces(Ctx);
    }
    if (Ctx->Faces.Count < 2) {
        *OutError = "Too few faces in hull";
        return CONVEX_HULL_BUILD_TOO_FEW_FACES;
    }
    return CONVEX_HULL_BUILD_SUCCESS;
}

function b32 Chb_Clone_Mesh_To_Allocator(chb_ctx const* Ctx, allocator* Allocator, convex_hull_mesh* OutMesh) {
    OutMesh->PositionCount = Ctx->PositionCount;
    OutMesh->Positions = (v3*)Allocator_Allocate_Memory(Allocator, sizeof(v3) * Ctx->PositionCount);
    if (!OutMesh->Positions)
        return false;
    Memory_Copy(OutMesh->Positions, Ctx->Positions, sizeof(v3) * Ctx->PositionCount);
    u32 ActiveFaceCount = 0;
    for (size_t I = 0; I < Ctx->Faces.Count; ++I)
        if (!Ctx->Faces.Ptr[I]->Removed)
            ActiveFaceCount++;
    OutMesh->FaceCount = ActiveFaceCount;
    OutMesh->Faces = (convex_hull_face*)Allocator_Allocate_Memory(Allocator, sizeof(convex_hull_face) * ActiveFaceCount);
    if (!OutMesh->Faces) {
        Allocator_Free_Memory(Allocator, OutMesh->Positions);
        OutMesh->Positions = 0;
        OutMesh->PositionCount = 0;
        return false;
    }
    Memory_Clear(OutMesh->Faces, sizeof(convex_hull_face) * ActiveFaceCount);
    u32 FiOut = 0;
    for (size_t I = 0; I < Ctx->Faces.Count; ++I) {
        chb_work_face* F = Ctx->Faces.Ptr[I];
        if (F->Removed)
            continue;
        convex_hull_face* D = &OutMesh->Faces[FiOut++];
        D->Normal = F->Normal;
        D->Centroid = F->Centroid;
        chb_work_edge* E = F->FirstEdge;
        u32 Ec = 0;
        do {
            Ec++;
            E = E->NextEdge;
        } while (E != F->FirstEdge);
        D->VertexCount = Ec;
        D->VertexIndices = (s32*)Allocator_Allocate_Memory(Allocator, sizeof(s32) * Ec);
        if (!D->VertexIndices) {
            for (u32 J = 0; J < FiOut - 1; ++J)
                Allocator_Free_Memory(Allocator, OutMesh->Faces[J].VertexIndices);
            Allocator_Free_Memory(Allocator, OutMesh->Faces);
            Allocator_Free_Memory(Allocator, OutMesh->Positions);
            Zero_Struct(*OutMesh);
            return false;
        }
        E = F->FirstEdge;
        u32 K = 0;
        do {
            D->VertexIndices[K++] = E->StartIdx;
            E = E->NextEdge;
        } while (E != F->FirstEdge);
    }
    return true;
}

export_function convex_hull_build_result Convex_Hull_Build(
    allocator* Allocator,
    v3_array Vertices,
    s32 MaxVertices,
    f32 Tolerance,
    convex_hull_mesh* OutMesh,
    const char** OutError) {
    Zero_Struct(*OutMesh);
    if (OutError)
        *OutError = 0;
    if (!Allocator || !Vertices.Ptr || Vertices.Count == 0)
        return CONVEX_HULL_BUILD_TOO_FEW_POINTS;
    arena* Scratch = Scratch_Get();
    chb_ctx Ctx = {
        .Positions = Vertices.Ptr,
        .PositionCount = (u32)Vertices.Count,
        .ScratchArena = Scratch,
    };
    Ctx.Faces = Dynamic_Work_Face_Ptr_Array_Init((allocator*)Scratch);
    Ctx.CoplanarList = Dynamic_Coplanar_Entry_Array_Init((allocator*)Scratch);
    const char* Err = 0;
    convex_hull_build_result R = Chb_Build_Internal(&Ctx, MaxVertices, Tolerance, &Err);
    if (R == CONVEX_HULL_BUILD_SUCCESS || R == CONVEX_HULL_BUILD_MAX_VERTICES_REACHED) {
        if (!Chb_Clone_Mesh_To_Allocator(&Ctx, Allocator, OutMesh)) {
            R = CONVEX_HULL_BUILD_OUT_OF_MEMORY;
            Convex_Hull_Mesh_Free(Allocator, OutMesh);
        }
    }
    if (OutError && Err)
        *OutError = Err;
    Scratch_Release();
    return R;
}

export_function void Convex_Hull_Mesh_Free(allocator* Allocator, convex_hull_mesh* Mesh) {
    if (!Allocator || !Mesh)
        return;
    for (u32 I = 0; I < Mesh->FaceCount; ++I)
        if (Mesh->Faces[I].VertexIndices)
            Allocator_Free_Memory(Allocator, Mesh->Faces[I].VertexIndices);
    if (Mesh->Faces)
        Allocator_Free_Memory(Allocator, Mesh->Faces);
    if (Mesh->Positions)
        Allocator_Free_Memory(Allocator, Mesh->Positions);
    Zero_Struct(*Mesh);
}

export_function s32 Convex_Hull_Mesh_Get_Num_Vertices_Used(convex_hull_mesh const* Mesh) {
    if (!Mesh || !Mesh->Positions || Mesh->PositionCount == 0)
        return 0;
    arena* Scratch = Scratch_Get();
    u8* Used = Arena_Push_Array_No_Clear(Scratch, Mesh->PositionCount, u8);
    Memory_Clear(Used, Mesh->PositionCount);
    for (u32 Fi = 0; Fi < Mesh->FaceCount; ++Fi) {
        convex_hull_face const* F = &Mesh->Faces[Fi];
        for (u32 Vi = 0; Vi < F->VertexCount; ++Vi) {
            s32 Idx = F->VertexIndices[Vi];
            if (Idx >= 0 && (u32)Idx < Mesh->PositionCount)
                Used[Idx] = 1;
        }
    }
    s32 Count = 0;
    for (u32 I = 0; I < Mesh->PositionCount; ++I)
        if (Used[I])
            Count++;
    Scratch_Release();
    return Count;
}

export_function b32 Convex_Hull_Mesh_Contains_Face(
    convex_hull_mesh const* Mesh,
    s32 const* Indices,
    u32 IndexCount) {
    if (!Mesh || !Indices || IndexCount == 0)
        return false;
    // Same rule as Jolt ConvexHullBuilder::ContainsFace: walk the mesh face from its first edge
    // and inIndices in array order, with inIndices rotated so the first mesh vertex matches the
    // first occurrence of that vertex in inIndices (std::find on mFirstEdge->mStartIdx).
    for (u32 Fi = 0; Fi < Mesh->FaceCount; ++Fi) {
        convex_hull_face const* F = &Mesh->Faces[Fi];
        if (F->VertexCount != IndexCount || !F->VertexIndices)
            continue;
        s32 FirstStart = F->VertexIndices[0];
        u32 Pos = (u32)-1;
        for (u32 P = 0; P < IndexCount; ++P) {
            if (Indices[P] == FirstStart) {
                Pos = P;
                break;
            }
        }
        if (Pos == (u32)-1)
            continue;
        u32 Matches = 0;
        for (u32 K = 0; K < IndexCount; ++K) {
            if (F->VertexIndices[K] != Indices[(Pos + K) % IndexCount])
                break;
            Matches++;
        }
        if (Matches == IndexCount)
            return true;
    }
    return false;
}

export_function void Convex_Hull_Mesh_Get_Center_Of_Mass_And_Volume(
    convex_hull_mesh const* Mesh,
    v3* OutCenterOfMass,
    f32* OutVolume) {
    if (!Mesh || Mesh->FaceCount == 0 || !OutCenterOfMass || !OutVolume) {
        if (OutCenterOfMass)
            *OutCenterOfMass = V3_Zero();
        if (OutVolume)
            *OutVolume = 0.0f;
        return;
    }
    v3 V4 = V3_Zero();
    for (u32 I = 0; I < Mesh->FaceCount; ++I)
        V4 = V3_Add_V3(V4, Mesh->Faces[I].Centroid);
    V4 = V3_Div_S(V4, (f32)Mesh->FaceCount);
    f32 Vol = 0.0f;
    v3 Com = V3_Zero();
    for (u32 Fi = 0; Fi < Mesh->FaceCount; ++Fi) {
        convex_hull_face const* F = &Mesh->Faces[Fi];
        if (F->VertexCount < 3)
            continue;
        s32 I0 = F->VertexIndices[0];
        s32 I1 = F->VertexIndices[1];
        v3 V1 = Mesh->Positions[I0];
        v3 V2 = Mesh->Positions[I1];
        for (u32 K = 2; K < F->VertexCount; ++K) {
            v3 V3p = Mesh->Positions[F->VertexIndices[K]];
            f32 VolTet = V3_Dot(V3_Sub_V3(V1, V4), V3_Cross(V3_Sub_V3(V2, V4), V3_Sub_V3(V3p, V4)));
            v3 ComTet = V3_Add_V3(V3_Add_V3(V1, V2), V3_Add_V3(V3p, V4));
            Vol += VolTet;
            Com = V3_Add_V3(Com, V3_Mul_S(ComTet, VolTet));
            V2 = V3p;
        }
    }
    if (Vol > FLT_EPSILON)
        *OutCenterOfMass = V3_Div_S(Com, 4.0f * Vol);
    else
        *OutCenterOfMass = V4;
    *OutVolume = Vol / 6.0f;
}

export_function void Convex_Hull_Mesh_Determine_Max_Error(
    convex_hull_mesh const* Mesh,
    u32* OutFaceIndex,
    f32* OutMaxError,
    s32* OutMaxErrorPositionIdx,
    f32* OutCoplanarDistance) {
    f32 CoplanarD = 0.0f;
    if (Mesh && Mesh->Positions) {
        v3 VMax = V3_Zero();
        for (u32 I = 0; I < Mesh->PositionCount; ++I) {
            v3 V = Mesh->Positions[I];
            VMax = Chb_V3_Abs_Max(VMax, V3(Abs(V.x), Abs(V.y), Abs(V.z)));
        }
        CoplanarD = 3.0f * FLT_EPSILON * (VMax.x + VMax.y + VMax.z);
    }
    if (OutCoplanarDistance)
        *OutCoplanarDistance = CoplanarD;
    if (!Mesh || !OutFaceIndex || !OutMaxError || !OutMaxErrorPositionIdx) {
        if (OutFaceIndex)
            *OutFaceIndex = (u32)-1;
        if (OutMaxError)
            *OutMaxError = 0.0f;
        if (OutMaxErrorPositionIdx)
            *OutMaxErrorPositionIdx = -1;
        return;
    }
    *OutFaceIndex = (u32)-1;
    *OutMaxError = 0.0f;
    *OutMaxErrorPositionIdx = -1;
    f32 MaxErr = 0.0f;
    u32 MaxFace = (u32)-1;
    s32 MaxPt = -1;
    for (u32 Pi = 0; Pi < Mesh->PositionCount; ++Pi) {
        v3 V = Mesh->Positions[Pi];
        f32 MinEdgeDistSq = FLT_MAX;
        u32 MinEdgeDistFace = (u32)-1;
        for (u32 Fi = 0; Fi < Mesh->FaceCount; ++Fi) {
            convex_hull_face const* F = &Mesh->Faces[Fi];
            f32 NLen = V3_Mag(F->Normal);
            if (NLen <= 0.0f)
                continue;
            f32 PlaneDist = V3_Dot(F->Normal, V3_Sub_V3(V, F->Centroid)) / NLen;
            if (PlaneDist > -CHB_COPLANAR_SLOP_FACTOR * CoplanarD) {
                f32 EdgeDistSq = FLT_MAX;
                if (F->VertexCount >= 2) {
                    b32 AllInside = true;
                    u32 Nv = F->VertexCount;
                    for (u32 E = 0; E < Nv; ++E) {
                        v3 P1 = Mesh->Positions[F->VertexIndices[E]];
                        v3 P2 = Mesh->Positions[F->VertexIndices[(E + 1) % Nv]];
                        if (V3_Dot(V3_Cross(V3_Sub_V3(P2, P1), V3_Sub_V3(V, P1)), F->Normal) < 0.0f) {
                            AllInside = false;
                            f32 D = Chb_Closest_Point_On_Segment_Sq(V, P1, P2);
                            EdgeDistSq = Min(EdgeDistSq, D);
                        }
                    }
                    if (AllInside)
                        EdgeDistSq = 0.0f;
                }
                if (EdgeDistSq < MinEdgeDistSq) {
                    MinEdgeDistSq = EdgeDistSq;
                    MinEdgeDistFace = Fi;
                }
                if (EdgeDistSq == 0.0f && PlaneDist > MaxErr) {
                    MaxErr = PlaneDist;
                    MaxFace = Fi;
                    MaxPt = (s32)Pi;
                }
            }
        }
        f32 MinEdgeDist = (MinEdgeDistFace != (u32)-1) ? sqrtf(MinEdgeDistSq) : 0.0f;
        if (MinEdgeDistFace != (u32)-1 && MinEdgeDist > MaxErr) {
            MaxErr = MinEdgeDist;
            MaxFace = MinEdgeDistFace;
            MaxPt = (s32)Pi;
        }
    }
    *OutFaceIndex = MaxFace;
    *OutMaxError = MaxErr;
    *OutMaxErrorPositionIdx = MaxPt;
}
