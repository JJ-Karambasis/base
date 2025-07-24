function im_gdi* IM_GDI_Get() {
	gdi* GDI = GDI_Get();
	im_gdi* Result = (im_gdi*)OS_TLS_Get(GDI->IMThreadLocalStorage);
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

		Result->VtxBufferPtr = (u8*)GDI_Map_Buffer(Result->VtxBuffer);
		Result->IdxBufferPtr = (u8*)GDI_Map_Buffer(Result->IdxBuffer);

		OS_TLS_Set(GDI->IMThreadLocalStorage, Result);
	}

	if (Result->IsReset) {
		Result->VtxBufferPtr = (u8*)GDI_Map_Buffer(Result->VtxBuffer);
		Result->IdxBufferPtr = (u8*)GDI_Map_Buffer(Result->IdxBuffer);
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

export_function void IM_Push_Rect2D_Color_UV(v2 Min, v2 Max, v2 UVMin, v2 UVMax, v4 Color) {
	im_vtx_v2_uv2_c v0 = { .P = Min, .UV = UVMin, .C = Color };
	im_vtx_v2_uv2_c v1 = { .P = V2(Min.x, Max.y), .UV = V2(UVMin.x, UVMax.y), .C = Color };
	im_vtx_v2_uv2_c v2 = { .P = Max, .UV = UVMax, .C = Color };
	im_vtx_v2_uv2_c v3 = { .P = V2(Max.x, Min.y), .UV = V2(UVMax.x, UVMin.y), .C = Color };

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

export_function void IM_Push_Rect2D_Color_UV_Norm(v2 Min, v2 Max, v4 Color) {
	IM_Push_Rect2D_Color_UV(Min, Max, V2_Zero(), V2_All(1.0f), Color);
}

export_function void IM_Push_Line3D(v3 P0, v3 P1) {
	IM_Push_Vtx(&P0, sizeof(v3));
	IM_Push_Vtx(&P1, sizeof(v3));
}

export_function void IM_Push_Triangle3D(v3 P0, v3 P1, v3 P2) {
	IM_Push_Vtx(&P0, sizeof(v3));
	IM_Push_Vtx(&P1, sizeof(v3));
	IM_Push_Vtx(&P2, sizeof(v3));
}

export_function void IM_Push_Triangle2D_Color(v2 P0, v2 P1, v2 P2, v4 Color) {
	im_vtx_v2_c v0 = { .P = P0, .C = Color };
	im_vtx_v2_c v1 = { .P = P1, .C = Color };
	im_vtx_v2_c v2 = { .P = P2, .C = Color };

	IM_Push_Vtx(&v0, sizeof(im_vtx_v2_c));
	IM_Push_Vtx(&v1, sizeof(im_vtx_v2_c));
	IM_Push_Vtx(&v2, sizeof(im_vtx_v2_c));
}

export_function void IM_Push_Triangle3D_Color(v3 P0, v3 P1, v3 P2, v4 Color) {
	im_vtx_v3_c v0 = { .P = P0, .C = Color };
	im_vtx_v3_c v1 = { .P = P1, .C = Color };
	im_vtx_v3_c v2 = { .P = P2, .C = Color };

	IM_Push_Vtx(&v0, sizeof(im_vtx_v3_c));
	IM_Push_Vtx(&v1, sizeof(im_vtx_v3_c));
	IM_Push_Vtx(&v2, sizeof(im_vtx_v3_c));
}