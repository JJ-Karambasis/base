#ifndef GDI_H
#define GDI_H

#define GDI_MAX_RENDER_TARGET_COUNT 4
#define GDI_MAX_BIND_GROUP_COUNT 4
#define GDI_MAX_VTX_BUFFER_COUNT 4

/* Misc */
typedef union {
	u32 ID;
	struct {
		u16 Index;
		u16 Generation;
	};
} gdi_id;

/* Enums */
Meta()
typedef enum {
	GDI_FORMAT_NONE Tags(size: 0, vk: VK_FORMAT_UNDEFINED),
	GDI_FORMAT_R8_UNORM Tags(size: 1, vk: VK_FORMAT_R8_UNORM),
	GDI_FORMAT_R8G8_UNORM Tags(size: 2, vk: VK_FORMAT_R8G8_UNORM),
	GDI_FORMAT_R8G8B8_UNORM Tags(size: 3, vk: VK_FORMAT_R8G8B8_UNORM),
	GDI_FORMAT_R8G8B8A8_UNORM Tags(HasSRGB, size: 4, vk: VK_FORMAT_R8G8B8A8_UNORM),
	GDI_FORMAT_B8G8R8A8_UNORM Tags(HasSRGB, size: 4, vk: VK_FORMAT_B8G8R8A8_UNORM),
	GDI_FORMAT_R8G8B8A8_SRGB Tags(HasSRGB, size: 4, vk: VK_FORMAT_R8G8B8A8_SRGB),
	GDI_FORMAT_B8G8R8A8_SRGB Tags(HasSRGB, size: 4, vk: VK_FORMAT_B8G8R8A8_SRGB),
	GDI_FORMAT_R32_FLOAT Tags(size: 4, vk: VK_FORMAT_R32_SFLOAT),
	GDI_FORMAT_R32G32_FLOAT Tags(size: 8, vk: VK_FORMAT_R32G32_SFLOAT),
	GDI_FORMAT_R32G32B32_FLOAT Tags(size: 12, vk: VK_FORMAT_R32G32B32_SFLOAT),
	GDI_FORMAT_R32G32B32A32_FLOAT Tags(size: 16, vk: VK_FORMAT_R32G32B32A32_SFLOAT),
	GDI_FORMAT_D32_FLOAT Tags(HasDepth, size: 4, vk: VK_FORMAT_D32_SFLOAT)
} gdi_format;

enum {
	GDI_TEXTURE_USAGE_NONE = 0,
	GDI_TEXTURE_USAGE_SAMPLED = (1 << 0),
	GDI_TEXTURE_USAGE_DEPTH = (1 << 1),
	GDI_TEXTURE_USAGE_RENDER_TARGET = (1 << 2),
	GDI_TEXTURE_USAGE_STORAGE = (1 << 3),
	GDI_TEXTURE_USAGE_READBACK = (1 << 4)
};
typedef u32 gdi_texture_usage;

enum {
	GDI_BUFFER_USAGE_NONE = 0,
	GDI_BUFFER_USAGE_VTX_BUFFER = (1 << 0),
	GDI_BUFFER_USAGE_IDX_BUFFER = (1 << 1),
	GDI_BUFFER_USAGE_CONSTANT_BUFFER = (1 << 2),
	GDI_BUFFER_USAGE_STORAGE_BUFFER = (1 << 3),
	GDI_BUFFER_USAGE_READBACK = (1 << 4)
};
typedef u32 gdi_buffer_usage;

Meta()
typedef enum {
	GDI_FILTER_NEAREST,
	GDI_FILTER_LINEAR
} gdi_filter;

Meta()
typedef enum {
	GDI_ADDRESS_MODE_REPEAT Tags(vk: VK_SAMPLER_ADDRESS_MODE_REPEAT),
	GDI_ADDRESS_MODE_CLAMP Tags(vk: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
} gdi_address_mode;

Meta()
typedef enum {
	GDI_BIND_GROUP_TYPE_NONE Tags(vk: -1, is_writable: false),
	GDI_BIND_GROUP_TYPE_CONSTANT_BUFFER Tags(vk: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, is_writable: false),
	GDI_BIND_GROUP_TYPE_TEXTURE Tags(vk: VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, is_writable: false),
	GDI_BIND_GROUP_TYPE_SAMPLER Tags(vk: VK_DESCRIPTOR_TYPE_SAMPLER, is_writable: false),
	GDI_BIND_GROUP_TYPE_STORAGE_BUFFER Tags(vk: VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, is_writable: true),
	GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE Tags(vk: VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, is_writable: true)
} gdi_bind_group_type;

Meta()
typedef enum {
	GDI_COMPARE_FUNC_NONE Tags(vk: VK_COMPARE_OP_NEVER),
	GDI_COMPARE_FUNC_LESS Tags(vk: VK_COMPARE_OP_LESS),
	GDI_COMPARE_FUNC_LEQUAL Tags(vk: VK_COMPARE_OP_LESS_OR_EQUAL)
} gdi_compare_func;

Meta()
typedef enum {
	GDI_IDX_FORMAT_NONE Tags(vk: -1),
	GDI_IDX_FORMAT_16_BIT Tags(vk: VK_INDEX_TYPE_UINT16),
	GDI_IDX_FORMAT_32_BIT Tags(vk: VK_INDEX_TYPE_UINT32)
} gdi_idx_format;

Meta()
typedef enum {
	GDI_PRIMITIVE_TRIANGLE Tags(vk: VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
	GDI_PRIMITIVE_LINE Tags(vk: VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
} gdi_primitive;

Meta()
typedef enum {
	GDI_BLEND_ZERO Tags(vk: VK_BLEND_FACTOR_ZERO),
	GDI_BLEND_ONE Tags(vk: VK_BLEND_FACTOR_ONE),
	GDI_BLEND_SRC_COLOR Tags(vk: VK_BLEND_FACTOR_SRC_COLOR),
	GDI_BLEND_INV_SRC_COLOR Tags(vk: VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR),
	GDI_BLEND_DST_COLOR Tags(vk: VK_BLEND_FACTOR_DST_COLOR),
	GDI_BLEND_INV_DST_COLOR Tags(vk: VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR),
	GDI_BLEND_SRC_ALPHA Tags(vk: VK_BLEND_FACTOR_SRC_ALPHA),
	GDI_BLEND_INV_SRC_ALPHA Tags(vk: VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA),
	GDI_BLEND_DST_ALPHA Tags(vk: VK_BLEND_FACTOR_DST_ALPHA),
	GDI_BLEND_INV_DST_ALPHA Tags(vk: VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA)
} gdi_blend;

#ifdef DEBUG_BUILD
typedef enum {
	GDI_OBJECT_TYPE_NONE,
	GDI_OBJECT_TYPE_TEXTURE,
	GDI_OBJECT_TYPE_TEXTURE_VIEW,
	GDI_OBJECT_TYPE_BUFFER,
	GDI_OBJECT_TYPE_SAMPLER,
	GDI_OBJECT_TYPE_BIND_GROUP_LAYOUT,
	GDI_OBJECT_TYPE_BIND_GROUP,
	GDI_OBJECT_TYPE_SHADER
} gdi_object_type;
#endif

/* Structs */
typedef struct {
#ifdef DEBUG_BUILD
	gdi_object_type Type;
#endif
	gdi_id ID;
} gdi_handle;
Array_Define(gdi_handle);

typedef union {
	gdi_id ID;
	struct {
		u16 	   Index;
		atomic_u16 Generation;
	};
} gdi_pool_id;

typedef struct {
	async_stack_index16 FreeIndices;
	gdi_pool_id* 		IDs;
} gdi_pool;

typedef struct {
	gdi_format 		  		   Format;
	v2i 			  		   Dim;
	gdi_texture_usage 		   Usage;
	u32 			  		   MipCount;
	buffer* 		  		   InitialData;
} gdi_texture_create_info;

typedef struct {
	gdi_handle Texture;
	gdi_format Format;
	u32 	   MipOffset;
	u32 	   MipCount;
} gdi_texture_view_create_info;

typedef struct {
	size_t 			 Size;
	gdi_buffer_usage Usage;
	buffer			 InitialData;
} gdi_buffer_create_info;

typedef struct {
	gdi_filter 		 Filter;
	gdi_address_mode AddressModeU;
	gdi_address_mode AddressModeV;
} gdi_sampler_create_info;

typedef struct {
	gdi_bind_group_type Type;
	u32 				Count;
} gdi_bind_group_binding;

Array_Define(gdi_bind_group_binding);
typedef struct {
	gdi_bind_group_binding_array Bindings;
} gdi_bind_group_layout_create_info;

typedef struct {
	gdi_handle Buffer;
	size_t 	   Offset;
	size_t 	   Size;
} gdi_bind_group_buffer;
Array_Define(gdi_bind_group_buffer);

typedef struct {
	gdi_handle Layout;
	gdi_bind_group_buffer_array Buffers;
	gdi_handle_array TextureViews;
	gdi_handle_array Samplers;
} gdi_bind_group_create_info;

typedef struct {
	string     Semantic;
	gdi_format Format;
} gdi_vtx_attribute;
Array_Define(gdi_vtx_attribute);

typedef struct {
	size_t Stride;
	gdi_vtx_attribute_array Attributes;
} gdi_vtx_binding;
Array_Define(gdi_vtx_binding);

typedef struct {
	b32 			 TestEnabled;
	b32 			 WriteEnabled;
	gdi_compare_func CompareFunc;
	gdi_format DepthFormat;
} gdi_depth_state;

typedef struct {
	gdi_blend SrcFactor;
	gdi_blend DstFactor;
} gdi_blend_state;
Array_Define(gdi_blend_state);

typedef struct {
	buffer VS;
	buffer PS;
	buffer CS;
	gdi_handle_array BindGroupLayouts;
	gdi_bind_group_binding_array WritableBindings;
	u32 		   	 PushConstantCount;
	gdi_vtx_binding_array VtxBindings;
	gdi_format RenderTargetFormats[GDI_MAX_RENDER_TARGET_COUNT];
	gdi_blend_state_array BlendStates;
	gdi_depth_state DepthState;
	gdi_primitive Primitive;
} gdi_shader_create_info;

typedef union {
	f32 F32[4];
	u32 U32[4];
	s32 S32[4];
} gdi_clear;

typedef struct {
	b32 ShouldClear;
	gdi_clear ClearColor[GDI_MAX_RENDER_TARGET_COUNT];
	f32 ClearDepth;
} gdi_clear_state;

typedef struct {
	gdi_handle RenderTargetViews[GDI_MAX_RENDER_TARGET_COUNT];
	gdi_handle DepthBufferView;
	gdi_clear_state ClearState;
} gdi_render_pass_begin_info;

#define GDI_MAX_PUSH_CONSTANT_COUNT 32
#define GDI_MAX_PUSH_CONSTANT_SIZE (GDI_MAX_PUSH_CONSTANT_COUNT * sizeof(u32))

typedef struct {
	gdi_handle Shader;
	gdi_handle BindGroups[GDI_MAX_BIND_GROUP_COUNT-1]; //Save one slot for the writable bind group
	u32 	   PushConstantCount;
	u32 	   PushConstants[GDI_MAX_PUSH_CONSTANT_COUNT];
	v3i 	   ThreadGroupCount;
} gdi_dispatch;
Array_Define(gdi_dispatch);

typedef struct {
	gdi_handle 	   Shader;
	gdi_handle 	   BindGroups[GDI_MAX_BIND_GROUP_COUNT];
	u32 		   DynamicOffsets[GDI_MAX_BIND_GROUP_COUNT];
	gdi_handle 	   VtxBuffers[GDI_MAX_VTX_BUFFER_COUNT];
	gdi_handle 	   IdxBuffer;
	gdi_idx_format IdxFormat;
	s32 		   ScissorMinX;
	s32 		   ScissorMinY;
	s32 		   ScissorMaxX;
	s32 		   ScissorMaxY;
	u32 		   IdxCount;
	u32 		   IdxOffset;
	u32 		   VtxOffset;
	u32 		   PushConstantCount;
	u32 		   PushConstants[GDI_MAX_PUSH_CONSTANT_COUNT];
} gdi_draw_state;

enum {
	GDI_RENDER_PASS_SHADER_BIT = (1 << 0),
	GDI_RENDER_PASS_BIND_GROUP_BIT = (1 << 1),
	GDI_RENDER_PASS_VTX_BUFFER_BIT = (1 << 5),
	GDI_RENDER_PASS_IDX_BUFFER_BIT = (1 << 9),
	GDI_RENDER_PASS_IDX_FORMAT_BIT = (1 << 10),
	GDI_RENDER_PASS_SCISSOR_MIN_X_BIT = (1 << 11),
	GDI_RENDER_PASS_SCISSOR_MIN_Y_BIT = (1 << 12),
	GDI_RENDER_PASS_SCISSOR_MAX_X_BIT = (1 << 13),
	GDI_RENDER_PASS_SCISSOR_MAX_Y_BIT = (1 << 14),
	GDI_RENDER_PASS_IDX_COUNT_BIT = (1 << 15),
	GDI_RENDER_PASS_IDX_OFFSET_BIT = (1 << 16),
	GDI_RENDER_PASS_VTX_OFFSET_BIT = (1 << 17),
	GDI_RENDER_PASS_PUSH_CONSTANT_COUNT = (1 << 18)
};
#define GDI_RENDER_PASS_SCISSOR (GDI_RENDER_PASS_SCISSOR_MIN_X_BIT|GDI_RENDER_PASS_SCISSOR_MIN_Y_BIT|GDI_RENDER_PASS_SCISSOR_MAX_X_BIT|GDI_RENDER_PASS_SCISSOR_MAX_Y_BIT)

typedef struct {
	gdi_draw_state PrevState;
	gdi_draw_state CurrentState;
	memory_reserve Memory;
	size_t 		   Offset;
} gdi_render_pass;

typedef enum {
	GDI_PASS_TYPE_RENDER,
	GDI_PASS_TYPE_COMPUTE
} gdi_pass_type;

typedef struct {
	gdi_handle_array   TextureWrites;
	gdi_handle_array   BufferWrites;
	gdi_dispatch_array Dispatches;
} gdi_compute_pass;

typedef struct gdi_pass gdi_pass;
struct gdi_pass {
	gdi_pass_type Type;
	union {
		gdi_render_pass* RenderPass;
		gdi_compute_pass ComputePass;
	};
	gdi_pass* Next;
};

#define GDI_TEXTURE_READBACK_DEFINE(name) void name(gdi_handle Texture, v2i Dim, gdi_format Format, u32 MipLevel, const void* Texels, void* UserData)
typedef GDI_TEXTURE_READBACK_DEFINE(gdi_texture_readback_func);
typedef struct {
	gdi_handle 				   Texture;
	void* 					   UserData;
	gdi_texture_readback_func* ReadbackFunc;
} gdi_texture_readback;

#define GDI_BUFFER_READBACK_DEFINE(name) void name(gdi_handle Buffer, size_t Size, const void* Data, void* UserData)
typedef GDI_BUFFER_READBACK_DEFINE(gdi_buffer_readback_func);
typedef struct {
	gdi_handle 				  Buffer;
	void* 					  UserData;
	gdi_buffer_readback_func* ReadbackFunc;
} gdi_buffer_readback;

Array_Define(gdi_texture_readback);
Array_Define(gdi_buffer_readback);
typedef struct {
	gdi_texture_readback_array TextureReadbacks;
	gdi_buffer_readback_array  BufferReadbacks;
} gdi_render_params;

typedef struct gdi gdi;
#define GDI_BACKEND_CREATE_TEXTURE_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_texture_create_info* TextureInfo)
#define GDI_BACKEND_DELETE_TEXTURE_DEFINE(name) void name(gdi* GDI, gdi_handle Texture)

#define GDI_BACKEND_CREATE_TEXTURE_VIEW_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_texture_view_create_info* TextureViewInfo)
#define GDI_BACKEND_DELETE_TEXTURE_VIEW_DEFINE(name) void name(gdi* GDI, gdi_handle TextureView)

#define GDI_BACKEND_CREATE_BUFFER_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_buffer_create_info* BufferInfo)
#define GDI_BACKEND_DELETE_BUFFER_DEFINE(name) void name(gdi* GDI, gdi_handle Buffer)
#define GDI_BACKEND_MAP_BUFFER_DEFINE(name) void* name(gdi* GDI, gdi_handle Buffer)
#define GDI_BACKEND_UNMAP_BUFFER_DEFINE(name) void name(gdi* GDI, gdi_handle Buffer)

#define GDI_BACKEND_CREATE_SAMPLER_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_sampler_create_info* SamplerInfo)
#define GDI_BACKEND_DELETE_SAMPLER_DEFINE(name) void name(gdi* GDI, gdi_handle Sampler)

#define GDI_BACKEND_CREATE_BIND_GROUP_LAYOUT_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_bind_group_layout_create_info* BindGroupLayoutInfo)
#define GDI_BACKEND_DELETE_BIND_GROUP_LAYOUT_DEFINE(name) void name(gdi* GDI, gdi_handle BindGroupLayout)

#define GDI_BACKEND_CREATE_BIND_GROUP_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_bind_group_create_info* BindGroupInfo)
#define GDI_BACKEND_DELETE_BIND_GROUP_DEFINE(name) void name(gdi* GDI, gdi_handle BindGroup)

#define GDI_BACKEND_CREATE_SHADER_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_shader_create_info* ShaderInfo)
#define GDI_BACKEND_DELETE_SHADER_DEFINE(name) void name(gdi* GDI, gdi_handle Shader)

#define GDI_BACKEND_BEGIN_RENDER_PASS_DEFINE(name) gdi_render_pass* name(gdi* GDI, const gdi_render_pass_begin_info* BeginInfo)
#define GDI_BACKEND_END_RENDER_PASS_DEFINE(name) void name(gdi* GDI, gdi_render_pass* RenderPass)

#define GDI_BACKEND_RENDER_DEFINE(name) void name(gdi* GDI, const gdi_render_params* RenderParams)

typedef GDI_BACKEND_CREATE_TEXTURE_DEFINE(gdi_backend_create_texture_func);
typedef GDI_BACKEND_DELETE_TEXTURE_DEFINE(gdi_backend_delete_texture_func);

typedef GDI_BACKEND_CREATE_TEXTURE_VIEW_DEFINE(gdi_backend_create_texture_view_func);
typedef GDI_BACKEND_DELETE_TEXTURE_VIEW_DEFINE(gdi_backend_delete_texture_view_func);

typedef GDI_BACKEND_CREATE_BUFFER_DEFINE(gdi_backend_create_buffer_func);
typedef GDI_BACKEND_DELETE_BUFFER_DEFINE(gdi_backend_delete_buffer_func);
typedef GDI_BACKEND_MAP_BUFFER_DEFINE(gdi_backend_map_buffer_func);
typedef GDI_BACKEND_UNMAP_BUFFER_DEFINE(gdi_backend_unmap_buffer_func);

typedef GDI_BACKEND_CREATE_SAMPLER_DEFINE(gdi_backend_create_sampler_func);
typedef GDI_BACKEND_DELETE_SAMPLER_DEFINE(gdi_backend_delete_sampler_func);

typedef GDI_BACKEND_CREATE_BIND_GROUP_LAYOUT_DEFINE(gdi_backend_create_bind_group_layout_func);
typedef GDI_BACKEND_DELETE_BIND_GROUP_LAYOUT_DEFINE(gdi_backend_delete_bind_group_layout_func);

typedef GDI_BACKEND_CREATE_BIND_GROUP_DEFINE(gdi_backend_create_bind_group_func);
typedef GDI_BACKEND_DELETE_BIND_GROUP_DEFINE(gdi_backend_delete_bind_group_func);

typedef GDI_BACKEND_CREATE_SHADER_DEFINE(gdi_backend_create_shader_func);
typedef GDI_BACKEND_DELETE_SHADER_DEFINE(gdi_backend_delete_shader_func);

typedef GDI_BACKEND_BEGIN_RENDER_PASS_DEFINE(gdi_backend_begin_render_pass_func);
typedef GDI_BACKEND_END_RENDER_PASS_DEFINE(gdi_backend_end_render_pass_func);

typedef GDI_BACKEND_RENDER_DEFINE(gdi_backend_render_func);

typedef struct {
	gdi_backend_create_texture_func* CreateTextureFunc;
	gdi_backend_delete_texture_func* DeleteTextureFunc;

	gdi_backend_create_texture_view_func* CreateTextureViewFunc;
	gdi_backend_delete_texture_view_func* DeleteTextureViewFunc;

	gdi_backend_create_buffer_func* CreateBufferFunc;
	gdi_backend_delete_buffer_func* DeleteBufferFunc;
	gdi_backend_map_buffer_func* MapBufferFunc;
	gdi_backend_unmap_buffer_func* UnmapBufferFunc;

	gdi_backend_create_sampler_func* CreateSamplerFunc;
	gdi_backend_delete_sampler_func* DeleteSamplerFunc;
	
	gdi_backend_create_bind_group_layout_func* CreateBindGroupLayoutFunc;
	gdi_backend_delete_bind_group_layout_func* DeleteBindGroupLayoutFunc;

	gdi_backend_create_bind_group_func* CreateBindGroupFunc;
	gdi_backend_delete_bind_group_func* DeleteBindGroupFunc;

	gdi_backend_create_shader_func* CreateShaderFunc;
	gdi_backend_delete_shader_func* DeleteShaderFunc;

	gdi_backend_begin_render_pass_func* BeginRenderPassFunc;
	gdi_backend_end_render_pass_func* EndRenderPassFunc;

	gdi_backend_render_func* RenderFunc;
} gdi_backend_vtable;

struct gdi {
	gdi_backend_vtable* Backend;
	arena* 				Arena;
	arena* 				FrameArena;

	string DeviceName;
	
	gdi_handle View;
	gdi_format ViewFormat;
	v2i 	   ViewDim;
	
	size_t ConstantBufferAlignment;

	os_tls*    IMThreadLocalStorage;
	atomic_ptr TopIM;

	gdi_pass* FirstPass;
	gdi_pass* LastPass;
};

#define GDI_Backend_Create_Texture(info) GDI_Get()->Backend->CreateTextureFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Texture(texture) GDI_Get()->Backend->DeleteTextureFunc(GDI_Get(), texture)

#define GDI_Backend_Create_Texture_View(info) GDI_Get()->Backend->CreateTextureViewFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Texture_View(texture_view) GDI_Get()->Backend->DeleteTextureViewFunc(GDI_Get(), texture_view)

#define GDI_Backend_Create_Buffer(info) GDI_Get()->Backend->CreateBufferFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Buffer(buffer) GDI_Get()->Backend->DeleteBufferFunc(GDI_Get(), buffer)
#define GDI_Backend_Map_Buffer(buffer) GDI_Get()->Backend->MapBufferFunc(GDI_Get(), buffer)
#define GDI_Backend_Unmap_Buffer(buffer) GDI_Get()->Backend->UnmapBufferFunc(GDI_Get(), buffer)

#define GDI_Backend_Create_Sampler(info) GDI_Get()->Backend->CreateSamplerFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Sampler(sampler) GDI_Get()->Backend->DeleteSamplerFunc(GDI_Get(), sampler)

#define GDI_Backend_Create_Bind_Group_Layout(info) GDI_Get()->Backend->CreateBindGroupLayoutFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Bind_Group_Layout(layout) GDI_Get()->Backend->DeleteBindGroupLayoutFunc(GDI_Get(), layout)

#define GDI_Backend_Create_Bind_Group(info) GDI_Get()->Backend->CreateBindGroupFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Bind_Group(bind_group) GDI_Get()->Backend->DeleteBindGroupFunc(GDI_Get(), bind_group)

#define GDI_Backend_Create_Shader(info) GDI_Get()->Backend->CreateShaderFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Shader(shader) GDI_Get()->Backend->DeleteShaderFunc(GDI_Get(), shader)

#define GDI_Backend_Begin_Render_Pass(begin_info) GDI_Get()->Backend->BeginRenderPassFunc(GDI_Get(), begin_info)
#define GDI_Backend_End_Render_Pass(render_pass) GDI_Get()->Backend->EndRenderPassFunc(GDI_Get(), render_pass)

#define GDI_Backend_Render(render_params) GDI_Get()->Backend->RenderFunc(GDI_Get(), render_params)

#define GDI_Get_View_Format() GDI_Get()->ViewFormat
#define GDI_Get_View_Dim() GDI_Get()->ViewDim
#define GDI_Get_View() GDI_Get()->View
#define GDI_Constant_Buffer_Alignment() GDI_Get()->ConstantBufferAlignment

/* Others */

#if defined(OS_WIN32)
#define GDI_INIT_DEFINE(name) gdi* name(base* Base, HWND Window, HINSTANCE Instance)
typedef GDI_INIT_DEFINE(gdi_init_func);
#elif defined(OS_OSX)

#ifdef __OBJC__
@class CAMetalLayer;
#else
typedef void CAMetalLayer;
#endif

#define GDI_INIT_DEFINE(name) gdi* name(base* Base, CAMetalLayer* Layer)
typedef GDI_INIT_DEFINE(gdi_init_func);
#else
#error "Not Implemented!"
#endif

#include "im_gdi.h"

#endif