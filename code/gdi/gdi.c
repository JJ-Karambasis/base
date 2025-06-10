#include "meta/gdi_meta.c"

Array_Implement(gdi_handle, GDI_Handle);
Array_Implement(gdi_dispatch, GDI_Dispatch);
Array_Implement(gdi_bind_group_binding, GDI_Bind_Group_Binding);

global gdi* G_GDI;
function inline void GDI_Set(gdi* GDI) {
	G_GDI = GDI;
}

function inline gdi* GDI_Get() {
	return G_GDI;
}


/*Quick access helper lookups*/
function inline gdi_id GDI_Null_ID() {
	gdi_id Result = { 0 };
	return Result;
}


function inline gdi_handle GDI_Null_Handle() {
	gdi_handle Result = { 0 };
	return Result;
}

function inline b32 GDI_ID_Is_Null(gdi_id ID) {
	return ID.ID == 0;
}

function inline b32 GDI_Is_Null(gdi_handle Handle) {
	return GDI_ID_Is_Null(Handle.ID);
}

function inline b32 GDI_Is_Equal(gdi_handle A, gdi_handle B) {
	b32 Result = A.ID.ID == B.ID.ID;
	//If they are equal, make sure the types are the same
	Assert(Result ? A.Type == B.Type : true);
	return Result;
}

#ifdef DEBUG_BUILD
function inline b32 GDI_Is_Type_(gdi_handle Handle, gdi_object_type Type) {
	return !GDI_Is_Null(Handle) && Handle.Type == Type;
}
#define GDI_Is_Type(handle, type) GDI_Is_Type_(handle, GDI_OBJECT_TYPE_##type)
#else
#define GDI_Is_Type(handle, type)
#endif

function inline arena* GDI_Frame_Arena() {
	gdi* GDI = GDI_Get();
	return GDI->FrameArena;
}

/* Resource creation. Mostly validation on the front end*/
function gdi_handle GDI_Create_Texture(const gdi_texture_create_info* CreateInfo) {
	Assert(CreateInfo->Format != GDI_FORMAT_NONE && CreateInfo->Usage != GDI_TEXTURE_USAGE_NONE && 
		   CreateInfo->Dim.x != 0 && CreateInfo->Dim.y != 0 && CreateInfo->MipCount != 0);
	gdi_handle Result = GDI_Backend_Create_Texture(CreateInfo);
	return Result;
}

function void GDI_Delete_Texture(gdi_handle Texture) {
	Assert(GDI_Is_Type(Texture, TEXTURE));
	if (!GDI_Is_Null(Texture)) {
		GDI_Backend_Delete_Texture(Texture);
	}
}

function gdi_handle GDI_Create_Texture_View(const gdi_texture_view_create_info* CreateInfo) {
	Assert(GDI_Is_Type(CreateInfo->Texture, TEXTURE));
	gdi_handle Result = GDI_Backend_Create_Texture_View(CreateInfo);
	return Result;
}

function gdi_handle GDI_Create_Texture_View_From_Texture(gdi_handle Texture) {
	gdi_texture_view_create_info ViewInfo = {
		.Texture = Texture
	};

	gdi_handle Result = GDI_Create_Texture_View(&ViewInfo);
	return Result;
}

function void GDI_Delete_Texture_View(gdi_handle TextureView) {
	Assert(GDI_Is_Type(TextureView, TEXTURE_VIEW));
	if (!GDI_Is_Null(TextureView)) {
		GDI_Backend_Delete_Texture_View(TextureView);
	}
}

function gdi_handle GDI_Create_Buffer(const gdi_buffer_create_info* CreateInfo) {
	Assert(CreateInfo->Size != 0 && CreateInfo->Usage != GDI_BUFFER_USAGE_NONE);
	gdi_handle Result = GDI_Backend_Create_Buffer(CreateInfo);
	if (!GDI_Is_Null(Result) && !Buffer_Is_Empty(CreateInfo->InitialData)) {
		void* Memory = GDI_Backend_Map_Buffer(Result);
		if (Memory) {
			Memory_Copy(Memory, CreateInfo->InitialData.Ptr, CreateInfo->InitialData.Size);
			GDI_Backend_Unmap_Buffer(Result);
		}
	}
	return Result;
}

function void GDI_Delete_Buffer(gdi_handle Buffer) {
	Assert(GDI_Is_Type(Buffer, BUFFER));
	if (!GDI_Is_Null(Buffer)) {
		GDI_Backend_Delete_Buffer(Buffer);
	}
}

function void* GDI_Map_Buffer(gdi_handle Buffer) {
	Assert(GDI_Is_Type(Buffer, BUFFER));
	if (GDI_Is_Null(Buffer)) return NULL;
	void* Result = GDI_Backend_Map_Buffer(Buffer);
	return Result;
}

function void GDI_Unmap_Buffer(gdi_handle Buffer) {
	Assert(GDI_Is_Type(Buffer, BUFFER));
	if (!GDI_Is_Null(Buffer)) {
		GDI_Backend_Unmap_Buffer(Buffer);
	}
}

function gdi_handle GDI_Create_Sampler(const gdi_sampler_create_info* CreateInfo) {
	gdi_handle Result = GDI_Backend_Create_Sampler(CreateInfo);
	return Result;
}

function void GDI_Delete_Sampler(gdi_handle Sampler) {
	Assert(GDI_Is_Type(Sampler, SAMPLER));
	if (!GDI_Is_Null(Sampler)) {
		GDI_Backend_Delete_Sampler(Sampler);
	}
}

function gdi_handle GDI_Create_Bind_Group_Layout(const gdi_bind_group_layout_create_info* CreateInfo) {
	gdi_handle Result = GDI_Backend_Create_Bind_Group_Layout(CreateInfo);
	return Result;
}

function void GDI_Delete_Bind_Group_Layout(gdi_handle BindGroupLayout) {
	Assert(GDI_Is_Type(BindGroupLayout, BIND_GROUP_LAYOUT));
	if (!GDI_Is_Null(BindGroupLayout)) {
		GDI_Backend_Delete_Bind_Group_Layout(BindGroupLayout);
	}
}

function gdi_handle GDI_Create_Bind_Group(const gdi_bind_group_create_info* CreateInfo) {
	gdi_handle Result = GDI_Backend_Create_Bind_Group(CreateInfo);
	return Result;
}

function void GDI_Delete_Bind_Group(gdi_handle BindGroup) {
	Assert(GDI_Is_Type(BindGroup, BIND_GROUP));
	if (!GDI_Is_Null(BindGroup)) {
		GDI_Backend_Delete_Bind_Group(BindGroup);
	}
}

function gdi_handle GDI_Create_Shader(const gdi_shader_create_info* CreateInfo) {
	Assert(CreateInfo->PushConstantCount <= GDI_MAX_PUSH_CONSTANT_COUNT);
	gdi_handle Result = GDI_Backend_Create_Shader(CreateInfo);
	return Result;
}

function void GDI_Delete_Shader(gdi_handle Shader) {
	Assert(GDI_Is_Type(Shader, SHADER));
	if (!GDI_Is_Null(Shader)) {
		GDI_Backend_Delete_Shader(Shader);
	}
}

/* Frames */
function void GDI_Submit_Render_Pass(gdi_render_pass* RenderPass) {
	gdi* GDI = GDI_Get();
	gdi_pass* Pass = Arena_Push_Struct(GDI->FrameArena, gdi_pass);
	Pass->Type = GDI_PASS_TYPE_RENDER;
	Pass->RenderPass = RenderPass;
	SLL_Push_Back(GDI->FirstPass, GDI->LastPass, Pass);
}

function void GDI_Submit_Compute_Pass(gdi_handle_array TextureWrites, gdi_handle_array BufferWrites, gdi_dispatch_array Dispatches) {
	gdi* GDI = GDI_Get();
	gdi_pass* Pass = Arena_Push_Struct(GDI->FrameArena, gdi_pass);
	Pass->Type = GDI_PASS_TYPE_COMPUTE;
	Pass->ComputePass.TextureWrites = TextureWrites;
	Pass->ComputePass.BufferWrites  = BufferWrites;
	Pass->ComputePass.Dispatches    = Dispatches;
	SLL_Push_Back(GDI->FirstPass, GDI->LastPass, Pass);
}

function void GDI_Render(const gdi_render_params* RenderParams) {
	gdi* GDI = GDI_Get();
	for (im_gdi* ImGDI = Atomic_Load_Ptr(&GDI->TopIM); ImGDI; ImGDI = ImGDI->Next) {
		if (!ImGDI->IsReset) {
			GDI_Unmap_Buffer(ImGDI->VtxBuffer);
			GDI_Unmap_Buffer(ImGDI->IdxBuffer);
		}
	}
	
	GDI_Backend_Render(RenderParams);

	Arena_Clear(GDI->FrameArena);
	GDI->FirstPass = NULL;
	GDI->LastPass = NULL;

	for (im_gdi* ImGDI = Atomic_Load_Ptr(&GDI->TopIM); ImGDI; ImGDI = ImGDI->Next) {
		ImGDI->IdxBufferUsed = 0;
		ImGDI->VtxBufferUsed = 0;
		ImGDI->LastIdxBufferUsed = 0;
		ImGDI->LastVtxBufferUsed = 0;
		ImGDI->IsReset = true;
	}
}

/* Render Pass */
function gdi_render_pass* GDI_Begin_Render_Pass(const gdi_render_pass_begin_info* BeginInfo) {
	gdi_render_pass* RenderPass = GDI_Backend_Begin_Render_Pass(BeginInfo);
	return RenderPass;
}

function void GDI_End_Render_Pass(gdi_render_pass* RenderPass) {
	GDI_Backend_End_Render_Pass(RenderPass);
}

function void Render_Set_Shader(gdi_render_pass* RenderPass, gdi_handle Shader) {
	Assert(GDI_Is_Type(Shader, SHADER));
	RenderPass->CurrentState.Shader = Shader;
}

function void Render_Set_Bind_Groups(gdi_render_pass* RenderPass, size_t Offset, gdi_handle* BindGroups, size_t Count) {	
	for (size_t i = Offset; i < Count; i++) {
		Assert(GDI_Is_Type(BindGroups[i], BIND_GROUP) && (i < GDI_MAX_BIND_GROUP_COUNT));
		RenderPass->CurrentState.BindGroups[i] = BindGroups[i];
	}
}

function void Render_Set_Bind_Group(gdi_render_pass* RenderPass, size_t Index, gdi_handle BindGroup) {
	Assert(GDI_Is_Type(BindGroup, BIND_GROUP) && (Index < GDI_MAX_BIND_GROUP_COUNT));
	RenderPass->CurrentState.BindGroups[Index] = BindGroup;
}

function void Render_Set_Push_Constants(gdi_render_pass* RenderPass, void* Data, size_t Size) {
	Assert(Size <= GDI_MAX_PUSH_CONSTANT_SIZE && ((Size % sizeof(u32)) == 0));
	RenderPass->CurrentState.PushConstantCount = (u32)(Size >> 2);
	Memory_Copy(RenderPass->CurrentState.PushConstants, Data, Size);
}

function void Render_Set_Vtx_Buffer(gdi_render_pass* RenderPass, u32 Index, gdi_handle VtxBuffer) {
	Assert(GDI_Is_Type(VtxBuffer, BUFFER) && (Index < GDI_MAX_VTX_BUFFER_COUNT));
	RenderPass->CurrentState.VtxBuffers[Index] = VtxBuffer;
}

function void Render_Set_Vtx_Buffers(gdi_render_pass* RenderPass, u32 Count, gdi_handle* VtxBuffers) {
	Assert(Count < GDI_MAX_VTX_BUFFER_COUNT);
	for (u32 i = 0; i < Count; i++) {
		Render_Set_Vtx_Buffer(RenderPass, i, VtxBuffers[i]);
	}
}

function void Render_Set_Idx_Buffer(gdi_render_pass* RenderPass, gdi_handle IdxBuffer, gdi_idx_format IdxFormat) {
	Assert(GDI_Is_Type(IdxBuffer, BUFFER));
	RenderPass->CurrentState.IdxBuffer = IdxBuffer;
	RenderPass->CurrentState.IdxFormat = IdxFormat;
}

function void Render_Set_Scissor(gdi_render_pass* RenderPass, s32 MinX, s32 MinY, s32 MaxX, s32 MaxY) {
	RenderPass->CurrentState.ScissorMinX = Max(MinX, 0);
	RenderPass->CurrentState.ScissorMinY = Max(MinY, 0);
	RenderPass->CurrentState.ScissorMaxX = Max(MaxX, 0);
	RenderPass->CurrentState.ScissorMaxY = Max(MaxY, 0);

	if (RenderPass->CurrentState.ScissorMinX > RenderPass->CurrentState.ScissorMaxX) {
		s32 Temp = RenderPass->CurrentState.ScissorMinX;
		RenderPass->CurrentState.ScissorMinX = RenderPass->CurrentState.ScissorMaxX;
		RenderPass->CurrentState.ScissorMaxX = Temp;
	}

	if (RenderPass->CurrentState.ScissorMinY > RenderPass->CurrentState.ScissorMaxY) {
		s32 Temp = RenderPass->CurrentState.ScissorMinY;
		RenderPass->CurrentState.ScissorMinY = RenderPass->CurrentState.ScissorMaxY;
		RenderPass->CurrentState.ScissorMaxY = Temp;
	}
}

function void Render_Draw_Idx(gdi_render_pass* RenderPass, u32 IdxCount, u32 IdxOffset, u32 VtxOffset) {
	gdi_draw_state* CurrentState = &RenderPass->CurrentState;
	gdi_draw_state* PrevState    = &RenderPass->PrevState;

	CurrentState->IdxCount  = IdxCount;
	CurrentState->IdxOffset = IdxOffset;
	CurrentState->VtxOffset = VtxOffset;

	if (RenderPass->Offset + (sizeof(gdi_draw_state)+sizeof(u32)) > RenderPass->Memory.CommitSize) {
		Commit_New_Size(&RenderPass->Memory, RenderPass->Offset + sizeof(gdi_draw_state));
	}

	u32* Bitfield = (u32*)(RenderPass->Memory.BaseAddress + RenderPass->Offset);
	*Bitfield = 0;
	u8* Stream = (u8*)(Bitfield + 1);

	if (!GDI_Is_Equal(PrevState->Shader, CurrentState->Shader)) {
		Assert(!GDI_Is_Null(CurrentState->Shader));
		*Bitfield |= GDI_RENDER_PASS_SHADER_BIT;
		Memory_Copy(Stream, &CurrentState->Shader, sizeof(gdi_handle));
		Stream += sizeof(gdi_handle);
	}

	for (u32 i = 0; i < GDI_MAX_BIND_GROUP_COUNT; i++) {
		if (!GDI_Is_Null(CurrentState->BindGroups[i])) {
			if (!GDI_Is_Equal(PrevState->BindGroups[i], CurrentState->BindGroups[i])) {
				*Bitfield |= (GDI_RENDER_PASS_BIND_GROUP_BIT << i);
				Memory_Copy(Stream, &CurrentState->BindGroups[i], sizeof(gdi_handle));
				Stream += sizeof(gdi_handle);
			}
		}
	}

	for (u32 i = 0; i < GDI_MAX_VTX_BUFFER_COUNT; i++) {
		if (!GDI_Is_Null(CurrentState->VtxBuffers[i])) {
			if (!GDI_Is_Equal(PrevState->VtxBuffers[i], CurrentState->VtxBuffers[i])) {
				*Bitfield |= (GDI_RENDER_PASS_VTX_BUFFER_BIT << i);
				Memory_Copy(Stream, &CurrentState->VtxBuffers[i], sizeof(gdi_handle));
				Stream += sizeof(gdi_handle);
			}
		}
	}

	if (!GDI_Is_Equal(PrevState->IdxBuffer, CurrentState->IdxBuffer)) {
		*Bitfield |= GDI_RENDER_PASS_IDX_BUFFER_BIT;
		Memory_Copy(Stream, &CurrentState->IdxBuffer, sizeof(gdi_handle));
		Stream += sizeof(gdi_handle);
	}

	if (PrevState->IdxFormat != CurrentState->IdxFormat) {
		*Bitfield |= GDI_RENDER_PASS_IDX_FORMAT_BIT;
		Memory_Copy(Stream, &CurrentState->IdxFormat, sizeof(gdi_idx_format));
		Stream += sizeof(gdi_idx_format);
	}

	if (PrevState->ScissorMinX != CurrentState->ScissorMinX) {
		*Bitfield |= GDI_RENDER_PASS_SCISSOR_MIN_X_BIT;
		Memory_Copy(Stream, &CurrentState->ScissorMinX, sizeof(u32));
		Stream += sizeof(u32);
	}

	if (PrevState->ScissorMinY != CurrentState->ScissorMinY) {
		*Bitfield |= GDI_RENDER_PASS_SCISSOR_MIN_Y_BIT;
		Memory_Copy(Stream, &CurrentState->ScissorMinY, sizeof(u32));
		Stream += sizeof(u32);
	}

	if (PrevState->ScissorMaxX != CurrentState->ScissorMaxX) {
		*Bitfield |= GDI_RENDER_PASS_SCISSOR_MAX_X_BIT;
		Memory_Copy(Stream, &CurrentState->ScissorMaxX, sizeof(u32));
		Stream += sizeof(u32);
	}

	if (PrevState->ScissorMaxY != CurrentState->ScissorMaxY) {
		*Bitfield |= GDI_RENDER_PASS_SCISSOR_MAX_Y_BIT;
		Memory_Copy(Stream, &CurrentState->ScissorMaxY, sizeof(u32));
		Stream += sizeof(u32);
	}

	if (PrevState->IdxCount != CurrentState->IdxCount) {
		*Bitfield |= GDI_RENDER_PASS_IDX_COUNT_BIT;
		Memory_Copy(Stream, &CurrentState->IdxCount, sizeof(u32));
		Stream += sizeof(u32);
	}

	if (PrevState->IdxOffset != CurrentState->IdxOffset) {
		*Bitfield |= GDI_RENDER_PASS_IDX_OFFSET_BIT;
		Memory_Copy(Stream, &CurrentState->IdxOffset, sizeof(u32));
		Stream += sizeof(u32);
	}

	if (PrevState->VtxOffset != CurrentState->VtxOffset) {
		*Bitfield |= GDI_RENDER_PASS_VTX_OFFSET_BIT;
		Memory_Copy(Stream, &CurrentState->VtxOffset, sizeof(u32));
		Stream += sizeof(u32);
	}

	if (PrevState->PushConstantCount != CurrentState->PushConstantCount) {
		*Bitfield |= GDI_RENDER_PASS_PUSH_CONSTANT_COUNT;
		Memory_Copy(Stream, &CurrentState->PushConstantCount, sizeof(u32));
		Stream += sizeof(u32);
	}
	Memory_Copy(Stream, CurrentState->PushConstants, CurrentState->PushConstantCount * sizeof(u32));
	Stream += CurrentState->PushConstantCount * sizeof(u32);

	*PrevState = *CurrentState;
	RenderPass->Offset += (Stream - (u8*)Bitfield);
}

function void GDI_Pool_Init_Raw(gdi_pool* Pool, u16* IndicesPtr, gdi_id* IDsPtr, u16 Capacity) {
	Async_Stack_Index16_Init_Raw(&Pool->FreeIndices, IndicesPtr, Capacity);
	Pool->IDs = (gdi_pool_id *)IDsPtr;

	u16 i;
	for (i = 0; i < Capacity; i++) {
		Async_Stack_Index16_Push(&Pool->FreeIndices, i);

		Pool->IDs[i].Index = i;
		*(u16*)(&Pool->IDs[i].Generation) = 1;
	}
}

function inline b32 GDI_Pool_Is_Allocated(gdi_pool* Pool, gdi_id ID) {
	Assert(ID.Index < Pool->FreeIndices.Capacity);
	return Atomic_Load_U16(&Pool->IDs[ID.Index].Generation) == ID.Generation;
}

function gdi_id GDI_Pool_Allocate(gdi_pool* Pool) {
	u16 Index = Async_Stack_Index16_Pop(&Pool->FreeIndices);
	if (Index == ASYNC_STACK_INDEX16_INVALID) { return GDI_Null_ID(); };

	Assert(Index < Pool->FreeIndices.Capacity);
	gdi_id Result = Pool->IDs[Index].ID;
	Assert(Index == Result.Index);
	return Result;
}

function void GDI_Pool_Free(gdi_pool* Pool, gdi_id ID) {
    u16 NextGenerationIndex = ID.Generation+1;
    if(NextGenerationIndex == 0) NextGenerationIndex = 1; /*Generation cannot be 0*/
    Assert(ID.Index < Pool->FreeIndices.Capacity); /*Overflow*/
	gdi_pool_id* PoolID = Pool->IDs + ID.Index;
    Assert(PoolID->Index == ID.Index);
    if(Atomic_Compare_Exchange_U16(&PoolID->Generation, ID.Generation, NextGenerationIndex) == ID.Generation) {
        Async_Stack_Index16_Push(&Pool->FreeIndices, ID.Index);
    } else {
        Assert(false); /*Tried to delete an invalid slot*/
    }
}

#include "im_gdi.c"