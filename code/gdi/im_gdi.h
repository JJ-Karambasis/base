#ifndef IM_GDI_H
#define IM_GDI_H

//Immediate mode buffer sizes should be pretty small
#define IM_MAX_VTX_BUFFER_SIZE MB(16)
#define IM_MAX_IDX_BUFFER_SIZE MB(16)

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
export_function u32 IM_Push(const void* Vtx, size_t Size);
export_function void IM_Push_Idx(u32 Idx);
export_function void IM_Push_Vtx(const void* Vtx, size_t Size);
export_function void IM_Push_Rect(v2 Min, v2 Max, v4 Color);

#endif