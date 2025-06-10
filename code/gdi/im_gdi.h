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

#endif