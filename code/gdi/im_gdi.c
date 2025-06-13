function im_gdi* IM_GDI_Get() {
	gdi* GDI = GDI_Get();
	im_gdi* Result = OS_TLS_Get(GDI->IMThreadLocalStorage);
	if (!Result) {
		arena* Arena = Arena_Create();
		Result = Arena_Push_Struct(Arena, im_gdi);
		Result->Arena = Arena;

		gdi_buffer_create_info VtxBufferInfo = {
			.Size = IM_MAX_VTX_BUFFER_SIZE,
			.Usage = GDI_BUFFER_USAGE_VTX_BUFFER
		};

		gdi_buffer_create_info IdxBufferInfo = {
			.Size = IM_MAX_IDX_BUFFER_SIZE,
			.Usage = GDI_BUFFER_USAGE_IDX_BUFFER
		};

		Result->VtxBuffer = GDI_Create_Buffer(&VtxBufferInfo);
		Result->IdxBuffer = GDI_Create_Buffer(&IdxBufferInfo);

		/*Append to link list atomically*/
		for(;;) {
			im_gdi* OldTop = (im_gdi*)Atomic_Load_Ptr(&GDI->TopIM);
			Result->Next = OldTop;
			if(Atomic_Compare_Exchange_Ptr(&GDI->TopIM, OldTop, Result) == OldTop) {
				break;
			}
		}

		Result->VtxBufferPtr = GDI_Map_Buffer(Result->VtxBuffer);
		Result->IdxBufferPtr = GDI_Map_Buffer(Result->IdxBuffer);

		OS_TLS_Set(GDI->IMThreadLocalStorage, Result);
	}

	if (Result->IsReset) {
		Result->VtxBufferPtr = GDI_Map_Buffer(Result->VtxBuffer);
		Result->IdxBufferPtr = GDI_Map_Buffer(Result->IdxBuffer);
		Result->IsReset = false;
	}

	return Result;
}

export_function void IM_Flush(gdi_render_pass* RenderPass) {
	im_gdi* IM = IM_GDI_Get();
	
	u32 IdxCount = (u32)((IM->IdxBufferUsed - IM->LastIdxBufferUsed)/sizeof(u32));
	u32 IdxOffset = (u32)(IM->LastIdxBufferUsed / sizeof(u32));

	if (IdxCount) {
		Render_Set_Vtx_Buffer(RenderPass, 0, IM->VtxBuffer);
		Render_Set_Idx_Buffer(RenderPass, IM->IdxBuffer, GDI_IDX_FORMAT_32_BIT);
		Render_Draw_Idx(RenderPass, IdxCount, IdxOffset, 0);
	}

	IM->LastVtxBufferUsed = IM->VtxBufferUsed;
	IM->LastIdxBufferUsed = IM->IdxBufferUsed;
}

export_function u32 IM_Push(const void* Vtx, size_t Size) {
	im_gdi* IM = IM_GDI_Get();
	IM->VtxBufferUsed = Align(IM->VtxBufferUsed, Size);
	u32 Idx = (u32)(IM->VtxBufferUsed / Size);
	Memory_Copy(IM->VtxBufferPtr + IM->VtxBufferUsed, Vtx, Size);
	IM->VtxBufferUsed += Size;
	Assert(IM->VtxBufferUsed <= IM_MAX_VTX_BUFFER_SIZE);
	return Idx;
}

export_function void IM_Push_Idx(u32 Idx) {
	im_gdi* IM = IM_GDI_Get();
	Memory_Copy(IM->IdxBufferPtr + IM->IdxBufferUsed, &Idx, sizeof(u32));
	IM->IdxBufferUsed += sizeof(u32);
	Assert(IM->IdxBufferUsed < IM_MAX_IDX_BUFFER_SIZE);
}

export_function void IM_Push_Vtx(const void* Vtx, size_t Size) {
	im_gdi* IM = IM_GDI_Get();
	u32 Idx = IM_Push(Vtx, Size);
	IM_Push_Idx(Idx);
}

typedef struct {
	v2 P;
	v2 UV;
	v4 C;
} im_vtx_v2_uv2_c;

export_function void IM_Push_Rect(v2 Min, v2 Max, v4 Color) {
	im_vtx_v2_uv2_c v0 = { .P = Min, .UV = V2(0.0f, 0.0f), .C = Color };
	im_vtx_v2_uv2_c v1 = { .P = V2(Min.x, Max.y), .UV = V2(0.0f, 1.0f), .C = Color };
	im_vtx_v2_uv2_c v2 = { .P = Max, .UV = V2(1.0f, 1.0f), .C = Color };
	im_vtx_v2_uv2_c v3 = { .P = V2(Max.x, Min.y), .UV = V2(1.0f, 0.0f), .C = Color };

	u32 i0 = IM_Push(&v0, sizeof(im_vtx_v2_uv2_c));
	u32 i1 = IM_Push(&v1, sizeof(im_vtx_v2_uv2_c));
	u32 i2 = IM_Push(&v2, sizeof(im_vtx_v2_uv2_c));
	u32 i3 = IM_Push(&v3, sizeof(im_vtx_v2_uv2_c));
	
	IM_Push_Idx(i0);
	IM_Push_Idx(i1);
	IM_Push_Idx(i2);

	IM_Push_Idx(i2);
	IM_Push_Idx(i3);
	IM_Push_Idx(i0);
}