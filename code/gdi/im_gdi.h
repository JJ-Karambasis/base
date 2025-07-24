#ifndef IM_GDI_H
#define IM_GDI_H

//Immediate mode buffer sizes should be pretty small
#define IM_MAX_VTX_BUFFER_SIZE MB(1)
#define IM_MAX_IDX_BUFFER_SIZE MB(1)

typedef struct {
	v2 P;
	v4 C;
} im_vtx_v2_c;

typedef struct {
	v2 P;
	v2 UV;
	v4 C;
} im_vtx_v2_uv2_c;

typedef struct {
	v3 P;
	v4 C;
} im_vtx_v3_c;

typedef struct im_gdi im_gdi;
struct im_gdi {
	arena* Arena;

	//Vtx and idx buffers
	size_t 	   IdxBufferUsed;
	size_t 	   VtxBufferUsed;
	size_t 	   LastIdxBufferUsed;
	size_t 	   LastVtxBufferUsed;
	gdi_handle IdxBuffer;
	gdi_handle VtxBuffer;
	u8* 	   VtxBufferPtr;
	u8* 	   IdxBufferPtr;

	b32 IsReset;

	//Draw state
	im_gdi* Next;
};

export_function void IM_Flush(gdi_render_pass* RenderPass);
export_function u32  IM_Push(const void* Vtx, size_t Size);
export_function void IM_Push_Idx(u32 Idx);
export_function void IM_Push_Vtx(const void* Vtx, size_t Size);
export_function void IM_Push_Rect2D_Color_UV(v2 Min, v2 Max, v2 UVMin, v2 UVMax, v4 Color);
export_function void IM_Push_Rect2D_Color_UV_Norm(v2 Min, v2 Max, v4 Color);
export_function void IM_Push_Line3D(v3 P0, v3 P1);
export_function void IM_Push_Triangle3D(v3 P0, v3 P1, v3 P2);
export_function void IM_Push_Triangle2D_Color(v2 P0, v2 P1, v2 P2, v4 Color);
export_function void IM_Push_Triangle3D_Color(v3 P0, v3 P1, v3 P2, v4 Color);

#endif