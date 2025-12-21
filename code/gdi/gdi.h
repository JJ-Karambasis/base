/*

TODO:
-Culling (front and backface), and wireframe mode

*/

#ifndef GDI_H
#define GDI_H

#ifdef __cplusplus
extern "C" {
#endif

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

Meta()
typedef enum {
	GDI_DEVICE_TYPE_UNKNOWN Tags(vk: VK_PHYSICAL_DEVICE_TYPE_OTHER),
	GDI_DEVICE_TYPE_INTEGRATED Tags(vk: VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU),
	GDI_DEVICE_TYPE_DISCRETE Tags(vk: VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
} gdi_device_type;

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
	GDI_BUFFER_USAGE_VTX = (1 << 0),
	GDI_BUFFER_USAGE_IDX = (1 << 1),
	GDI_BUFFER_USAGE_CONSTANT = (1 << 2),
	GDI_BUFFER_USAGE_STORAGE = (1 << 3),
	GDI_BUFFER_USAGE_READBACK = (1 << 4),
	GDI_BUFFER_USAGE_DYNAMIC = (1 << 5)
};
typedef u32 gdi_buffer_usage;

enum {
	GDI_SHUTDOWN_FLAG_NONE = 0,
	GDI_SHUTDOWN_FLAG_FREE_RESOURCES = (1 << 0)
};
typedef u32 gdi_shutdown_flags;

Meta()
typedef enum {
	GDI_FILTER_NEAREST,
	GDI_FILTER_LINEAR
} gdi_filter;

Meta()
typedef enum {
	GDI_ADDRESS_MODE_REPEAT Tags(vk: VK_SAMPLER_ADDRESS_MODE_REPEAT),
	GDI_ADDRESS_MODE_CLAMP Tags(vk: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE),
	GDI_ADDRESS_MODE_BORDER Tags(vk: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
} gdi_address_mode;

Meta()
typedef enum {
	GDI_BIND_GROUP_TYPE_NONE Tags(vk: -1),
	GDI_BIND_GROUP_TYPE_CONSTANT_BUFFER Tags(vk: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer),
	GDI_BIND_GROUP_TYPE_TEXTURE Tags(vk: VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, texture),
	GDI_BIND_GROUP_TYPE_SAMPLER Tags(vk: VK_DESCRIPTOR_TYPE_SAMPLER),
	GDI_BIND_GROUP_TYPE_STORAGE_BUFFER Tags(vk: VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, writable, buffer),
	GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE Tags(vk: VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, writable, texture)
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
	GDI_CULL_MODE_NONE Tags(vk: VK_CULL_MODE_NONE),
	GDI_CULL_MODE_BACK Tags(vk: VK_CULL_MODE_BACK_BIT)
} gdi_cull_mode;

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

typedef enum {
	GDI_DRAW_TYPE_NONE,
	GDI_DRAW_TYPE_IDX,
	GDI_DRAW_TYPE_VTX
} gdi_draw_type;

Meta()
typedef enum {
	GDI_LOG_TYPE_INFO,
	GDI_LOG_TYPE_WARNING,
	GDI_LOG_TYPE_ERROR
} gdi_log_type;

#ifdef DEBUG_BUILD
typedef enum {
	GDI_OBJECT_TYPE_NONE,
	GDI_OBJECT_TYPE_TEXTURE,
	GDI_OBJECT_TYPE_TEXTURE_VIEW,
	GDI_OBJECT_TYPE_BUFFER,
	GDI_OBJECT_TYPE_SAMPLER,
	GDI_OBJECT_TYPE_BIND_GROUP_LAYOUT,
	GDI_OBJECT_TYPE_BIND_GROUP,
	GDI_OBJECT_TYPE_SHADER,
	GDI_OBJECT_TYPE_SWAPCHAIN
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
	gdi_device_type Type;
	u32 			DeviceIndex;
	string 			DeviceName;
} gdi_device;
Array_Define(gdi_device);

typedef struct {
	gdi_format 		  Format;
	v2i 			  Dim;
	gdi_texture_usage Usage;
	u32 			  MipCount;
	const buffer* 	  InitialData;
	string 			  DebugName;
} gdi_texture_create_info;

typedef struct {
	gdi_format 		  Format;
	v2i 	   		  Dim;
	gdi_texture_usage Usage;
	u32 			  MipCount;
} gdi_texture_info;

typedef struct {
	gdi_handle Texture;
	gdi_format Format;
	u32 	   MipOffset;
	u32 	   MipCount;
	string 	   DebugName;
} gdi_texture_view_create_info;

typedef struct {
	size_t 			 Size;
	gdi_buffer_usage Usage;
	buffer			 InitialData;
	string 			 DebugName;
} gdi_buffer_create_info;

typedef struct {
	gdi_filter 		 Filter;
	gdi_address_mode AddressModeU;
	gdi_address_mode AddressModeV;
	string 			 DebugName;
} gdi_sampler_create_info;

typedef struct {
	gdi_bind_group_type Type;
	u32 				Count;
} gdi_bind_group_binding;

Array_Define(gdi_bind_group_binding);
typedef struct {
	gdi_bind_group_binding_array Bindings;
	string 			  			 DebugName;
} gdi_bind_group_layout_create_info;

typedef struct {
	gdi_handle Buffer;
	size_t 	   Offset;
	size_t 	   Size;
} gdi_bind_group_buffer;
Array_Define(gdi_bind_group_buffer);


typedef struct {
	gdi_handle DstBindGroup;
	u32 	   DstBinding;
	u32 	   DstIndex;

	gdi_bind_group_buffer_array Buffers;
	gdi_handle_array 			TextureViews;
	gdi_handle_array 			Samplers;
} gdi_bind_group_write;
Array_Define(gdi_bind_group_write);

typedef struct {
	gdi_bind_group_write_array Writes;
} gdi_bind_group_write_info;

typedef struct {
	gdi_handle DstBindGroup;
	u32 	   DstBinding;
	u32 	   DstIndex;
	gdi_handle SrcBindGroup;
	u32 	   SrcBinding;
	u32 	   SrcIndex;
	u32 	   Count;
} gdi_bind_group_copy;
Array_Define(gdi_bind_group_copy);

typedef struct {
	gdi_bind_group_copy_array Copies;
} gdi_bind_group_copy_info;

typedef struct {
	gdi_handle 				   Layout;
	gdi_bind_group_write_array Writes;
	gdi_bind_group_copy_array  Copies;
	string 			 		   DebugName;
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
	b32 		  IsWireframe;
	gdi_cull_mode CullMode;
	string 		  DebugName;
} gdi_shader_create_info;

#if defined(OS_OSX)
#ifdef __OBJC__
@class CAMetalLayer;
#else
typedef void CAMetalLayer;
#endif
#endif

#if defined(OS_WIN32)
#include <windows.h>
#endif

#if defined(OS_LINUX)
#undef function
#include <X11/Xlib.h>
#define function static
#undef Status
#endif

typedef struct {
#if defined(OS_WIN32)
	HWND 	  Window;
	HINSTANCE Instance;
#elif defined(OS_OSX)
	CAMetalLayer* Layer;
#elif defined(OS_LINUX)
	Display* Display;
	Window Window;
#endif
	gdi_format Format;
	string DebugName;
} gdi_swapchain_create_info;

typedef struct {
	gdi_format Format;
	v2i 	   Dim;
} gdi_swapchain_info;

typedef union {
	f32 F32[4];
	u32 U32[4];
	s32 S32[4];
} gdi_clear;

typedef struct {
	b32 ShouldClear;
	union {
		f32 F32[4];
		u32 U32[4];
		s32 S32[4];
	};
} gdi_clear_color;

typedef struct {
	b32 ShouldClear;
	f32 Depth;
} gdi_clear_depth;

typedef struct {
	gdi_handle RenderTargetViews[GDI_MAX_RENDER_TARGET_COUNT];
	gdi_handle DepthBufferView;
	gdi_clear_color ClearColors[GDI_MAX_RENDER_TARGET_COUNT];
	gdi_clear_depth ClearDepth;
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
	gdi_draw_type  DrawType;
	u32 		   PrimitiveCount;
	u32 		   PrimitiveOffset;
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
	GDI_RENDER_PASS_DRAW_TYPE_BIT = (1 << 15),
	GDI_RENDER_PASS_PRIMITIVE_COUNT_BIT = (1 << 16),
	GDI_RENDER_PASS_PRIMITIVE_OFFSET_BIT = (1 << 17),
	GDI_RENDER_PASS_VTX_OFFSET_BIT = (1 << 18),
	GDI_RENDER_PASS_PUSH_CONSTANT_COUNT = (1 << 19)
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

#define GDI_TEXTURE_READBACK_DEFINE(name) void name(gdi_handle Texture, v2i Dim, gdi_format Format, const void* Texels, void* UserData)
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
	gdi_handle_array 		   Swapchains;
} gdi_render_params;

#define GDI_LOG_DEFINE(name) void name(gdi_log_type Type, string Message, void* UserData)
typedef GDI_LOG_DEFINE(gdi_log_func);

typedef struct {
	gdi_log_func* LogFunc;
	void* 		  UserData;
} gdi_log_callbacks;

typedef struct {
	base* 			  Base;
	gdi_log_callbacks LogCallbacks;
} gdi_init_info;

typedef struct gdi gdi;
typedef struct {
	gdi* GDI;
	gdi_device* Device;
	arena* FrameArena;

	size_t ConstantBufferAlignment;

	os_tls*    IMThreadLocalStorage;
	atomic_ptr TopIM;

	gdi_pass* FirstPass;
	gdi_pass* LastPass;
} gdi_device_context;

#define GDI_BACKEND_SET_DEVICE_CONTEXT_DEFINE(name) b32 name(gdi* GDI, gdi_device* Device)

#define GDI_BACKEND_CREATE_TEXTURE_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_texture_create_info* TextureInfo)
#define GDI_BACKEND_DELETE_TEXTURE_DEFINE(name) void name(gdi* GDI, gdi_handle Texture)
#define GDI_BACKEND_GET_TEXTURE_INFO_DEFINE(name) gdi_texture_info name(gdi* GDI, gdi_handle Texture)

#define GDI_BACKEND_CREATE_TEXTURE_VIEW_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_texture_view_create_info* TextureViewInfo)
#define GDI_BACKEND_DELETE_TEXTURE_VIEW_DEFINE(name) void name(gdi* GDI, gdi_handle TextureView)
#define GDI_BACKEND_GET_TEXTURE_VIEW_TEXTURE_DEFINE(name) gdi_handle name(gdi* GDI, gdi_handle TextureView)

#define GDI_BACKEND_CREATE_BUFFER_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_buffer_create_info* BufferInfo)
#define GDI_BACKEND_DELETE_BUFFER_DEFINE(name) void name(gdi* GDI, gdi_handle Buffer)
#define GDI_BACKEND_MAP_BUFFER_DEFINE(name) void* name(gdi* GDI, gdi_handle Buffer, size_t Offset, size_t Size)
#define GDI_BACKEND_UNMAP_BUFFER_DEFINE(name) void name(gdi* GDI, gdi_handle Buffer)

#define GDI_BACKEND_CREATE_SAMPLER_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_sampler_create_info* SamplerInfo)
#define GDI_BACKEND_DELETE_SAMPLER_DEFINE(name) void name(gdi* GDI, gdi_handle Sampler)

#define GDI_BACKEND_CREATE_BIND_GROUP_LAYOUT_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_bind_group_layout_create_info* BindGroupLayoutInfo)
#define GDI_BACKEND_DELETE_BIND_GROUP_LAYOUT_DEFINE(name) void name(gdi* GDI, gdi_handle BindGroupLayout)

#define GDI_BACKEND_CREATE_BIND_GROUP_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_bind_group_create_info* BindGroupInfo)
#define GDI_BACKEND_DELETE_BIND_GROUP_DEFINE(name) void name(gdi* GDI, gdi_handle BindGroup)
#define GDI_BACKEND_UPDATE_BIND_GROUPS_DEFINE(name) void name(gdi* GDI, gdi_bind_group_write_array Writes, gdi_bind_group_copy_array Copies)

#define GDI_BACKEND_CREATE_SHADER_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_shader_create_info* ShaderInfo)
#define GDI_BACKEND_DELETE_SHADER_DEFINE(name) void name(gdi* GDI, gdi_handle Shader)

#define GDI_BACKEND_CREATE_SWAPCHAIN_DEFINE(name) gdi_handle name(gdi* GDI, const gdi_swapchain_create_info* SwapchainInfo)
#define GDI_BACKEND_DELETE_SWAPCHAIN_DEFINE(name) void name(gdi* GDI, gdi_handle Swapchain)
#define GDI_BACKEND_GET_SWAPCHAIN_VIEW_DEFINE(name) gdi_handle name(gdi* GDI, gdi_handle SwapchainHandle)
#define GDI_BACKEND_GET_SWAPCHAIN_INFO_DEFINE(name) gdi_swapchain_info name(gdi* GDI, gdi_handle SwapchainHandle)

#define GDI_BACKEND_BEGIN_RENDER_PASS_DEFINE(name) gdi_render_pass* name(gdi* GDI, const gdi_render_pass_begin_info* BeginInfo)
#define GDI_BACKEND_END_RENDER_PASS_DEFINE(name) void name(gdi* GDI, gdi_render_pass* RenderPass)

#define GDI_BACKEND_RENDER_DEFINE(name) void name(gdi* GDI, const gdi_render_params* RenderParams)

#define GDI_BACKEND_SHUTDOWN_DEFINE(name) void name(gdi* GDI, gdi_shutdown_flags Flags)

typedef GDI_BACKEND_SET_DEVICE_CONTEXT_DEFINE(gdi_backend_set_device_context_func);

typedef GDI_BACKEND_CREATE_TEXTURE_DEFINE(gdi_backend_create_texture_func);
typedef GDI_BACKEND_DELETE_TEXTURE_DEFINE(gdi_backend_delete_texture_func);
typedef GDI_BACKEND_GET_TEXTURE_INFO_DEFINE(gdi_backend_get_texture_info_func);

typedef GDI_BACKEND_CREATE_TEXTURE_VIEW_DEFINE(gdi_backend_create_texture_view_func);
typedef GDI_BACKEND_DELETE_TEXTURE_VIEW_DEFINE(gdi_backend_delete_texture_view_func);
typedef GDI_BACKEND_GET_TEXTURE_VIEW_TEXTURE_DEFINE(gdi_backend_get_texture_view_texture_func);

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
typedef GDI_BACKEND_UPDATE_BIND_GROUPS_DEFINE(gdi_backend_update_bind_groups_func);

typedef GDI_BACKEND_CREATE_SHADER_DEFINE(gdi_backend_create_shader_func);
typedef GDI_BACKEND_DELETE_SHADER_DEFINE(gdi_backend_delete_shader_func);

typedef GDI_BACKEND_CREATE_SWAPCHAIN_DEFINE(gdi_backend_create_swapchain_func);
typedef GDI_BACKEND_DELETE_SWAPCHAIN_DEFINE(gdi_backend_delete_swapchain_func);
typedef GDI_BACKEND_GET_SWAPCHAIN_VIEW_DEFINE(gdi_backend_get_swapchain_view_func);
typedef GDI_BACKEND_GET_SWAPCHAIN_INFO_DEFINE(gdi_backend_get_swapchain_info_func);

typedef GDI_BACKEND_BEGIN_RENDER_PASS_DEFINE(gdi_backend_begin_render_pass_func);
typedef GDI_BACKEND_END_RENDER_PASS_DEFINE(gdi_backend_end_render_pass_func);

typedef GDI_BACKEND_RENDER_DEFINE(gdi_backend_render_func);

typedef GDI_BACKEND_SHUTDOWN_DEFINE(gdi_backend_shutdown_func);

typedef struct {
	gdi_backend_set_device_context_func* SetDeviceContextFunc;

	gdi_backend_create_texture_func* CreateTextureFunc;
	gdi_backend_delete_texture_func* DeleteTextureFunc;
	gdi_backend_get_texture_info_func* GetTextureInfoFunc;

	gdi_backend_create_texture_view_func* CreateTextureViewFunc;
	gdi_backend_delete_texture_view_func* DeleteTextureViewFunc;
	gdi_backend_get_texture_view_texture_func* GetTextureViewTextureFunc;

	gdi_backend_create_buffer_func* CreateBufferFunc;
	gdi_backend_delete_buffer_func* DeleteBufferFunc;
	gdi_backend_map_buffer_func* MapBufferFunc;
	gdi_backend_unmap_buffer_func* UnmapBufferFunc;

	gdi_backend_create_sampler_func* CreateSamplerFunc;
	gdi_backend_delete_sampler_func* DeleteSamplerFunc;
	
	gdi_backend_create_bind_group_layout_func* CreateBindGroupLayoutFunc;
	gdi_backend_delete_bind_group_layout_func* DeleteBindGroupLayoutFunc;

	gdi_backend_create_bind_group_func*  CreateBindGroupFunc;
	gdi_backend_delete_bind_group_func*  DeleteBindGroupFunc;
	gdi_backend_update_bind_groups_func* UpdateBindGroupsFunc;

	gdi_backend_create_shader_func* CreateShaderFunc;
	gdi_backend_delete_shader_func* DeleteShaderFunc;

	gdi_backend_create_swapchain_func* CreateSwapchainFunc;
	gdi_backend_delete_swapchain_func* DeleteSwapchainFunc;
	gdi_backend_get_swapchain_view_func* GetSwapchainViewFunc;
	gdi_backend_get_swapchain_info_func* GetSwapchainInfoFunc;

	gdi_backend_begin_render_pass_func* BeginRenderPassFunc;
	gdi_backend_end_render_pass_func* EndRenderPassFunc;

	gdi_backend_render_func* RenderFunc;
	gdi_backend_shutdown_func* ShutdownFunc;
} gdi_backend_vtable;

struct gdi {
	gdi_backend_vtable*   Backend;
	arena* 				  Arena;
	gdi_device_array 	  Devices;
	gdi_device_context*   DeviceContext;
};

#define GDI_Backend_Set_Device_Context(device) GDI_Get()->Backend->SetDeviceContextFunc(GDI_Get(), device)

#define GDI_Backend_Create_Texture(info) GDI_Get()->Backend->CreateTextureFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Texture(texture) GDI_Get()->Backend->DeleteTextureFunc(GDI_Get(), texture)
#define GDI_Backend_Get_Texture_Info(texture) GDI_Get()->Backend->GetTextureInfoFunc(GDI_Get(), texture)

#define GDI_Backend_Create_Texture_View(info) GDI_Get()->Backend->CreateTextureViewFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Texture_View(texture_view) GDI_Get()->Backend->DeleteTextureViewFunc(GDI_Get(), texture_view)
#define GDI_Backend_Get_Texture_View_Texture(texture_view) GDI_Get()->Backend->GetTextureViewTextureFunc(GDI_Get(), texture_view)

#define GDI_Backend_Create_Buffer(info) GDI_Get()->Backend->CreateBufferFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Buffer(buffer) GDI_Get()->Backend->DeleteBufferFunc(GDI_Get(), buffer)
#define GDI_Backend_Map_Buffer(buffer, offset, size) GDI_Get()->Backend->MapBufferFunc(GDI_Get(), buffer, offset, size)
#define GDI_Backend_Unmap_Buffer(buffer) GDI_Get()->Backend->UnmapBufferFunc(GDI_Get(), buffer)

#define GDI_Backend_Create_Sampler(info) GDI_Get()->Backend->CreateSamplerFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Sampler(sampler) GDI_Get()->Backend->DeleteSamplerFunc(GDI_Get(), sampler)

#define GDI_Backend_Create_Bind_Group_Layout(info) GDI_Get()->Backend->CreateBindGroupLayoutFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Bind_Group_Layout(layout) GDI_Get()->Backend->DeleteBindGroupLayoutFunc(GDI_Get(), layout)

#define GDI_Backend_Create_Bind_Group(info) GDI_Get()->Backend->CreateBindGroupFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Bind_Group(bind_group) GDI_Get()->Backend->DeleteBindGroupFunc(GDI_Get(), bind_group)
#define GDI_Backend_Update_Bind_Groups(writes, copies) GDI_Get()->Backend->UpdateBindGroupsFunc(GDI_Get(), writes, copies)

#define GDI_Backend_Create_Shader(info) GDI_Get()->Backend->CreateShaderFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Shader(shader) GDI_Get()->Backend->DeleteShaderFunc(GDI_Get(), shader)

#define GDI_Backend_Create_Swapchain(info) GDI_Get()->Backend->CreateSwapchainFunc(GDI_Get(), info)
#define GDI_Backend_Delete_Swapchain(swapchain) GDI_Get()->Backend->DeleteSwapchainFunc(GDI_Get(), swapchain)
#define GDI_Backend_Get_Swapchain_View(swapchain) GDI_Get()->Backend->GetSwapchainViewFunc(GDI_Get(), swapchain)
#define GDI_Backend_Get_Swapchain_Info(swapchain) GDI_Get()->Backend->GetSwapchainInfoFunc(GDI_Get(), swapchain)

#define GDI_Backend_Begin_Render_Pass(begin_info) GDI_Get()->Backend->BeginRenderPassFunc(GDI_Get(), begin_info)
#define GDI_Backend_End_Render_Pass(render_pass) GDI_Get()->Backend->EndRenderPassFunc(GDI_Get(), render_pass)

#define GDI_Backend_Render(render_params) GDI_Get()->Backend->RenderFunc(GDI_Get(), render_params)

#define GDI_Shutdown(flags) GDI_Get()->Backend->ShutdownFunc(GDI_Get(), flags)

#define GDI_Get_Devices() GDI_Get()->Devices
#define GDI_Constant_Buffer_Alignment() GDI_Get()->ConstantBufferAlignment

/* Others */

#define GDI_INIT_DEFINE(name) gdi* name(const gdi_init_info* InitInfo)
export_function GDI_INIT_DEFINE(GDI_Init);

/*Quick access helper lookups*/
function inline gdi_id GDI_Null_ID() {
	gdi_id Result = { 0 };
	return Result;
}

function inline gdi_handle GDI_Null_Handle() {
	gdi_handle Result;
	Memory_Clear(&Result, sizeof(gdi_handle));
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

export_function gdi_format GDI_Get_SRGB_Format(gdi_format Format);
export_function size_t 	   GDI_Get_Format_Size(gdi_format);
export_function gdi* 	   GDI_Get();
export_function void 	   GDI_Set(gdi* GDI);
export_function b32 	   GDI_Set_Device_Context(gdi_device* Device);

export_function gdi_handle GDI_Create_Texture(const gdi_texture_create_info* CreateInfo);
export_function void GDI_Delete_Texture(gdi_handle Texture);
export_function gdi_texture_info GDI_Get_Texture_Info(gdi_handle Texture);
export_function gdi_handle GDI_Create_Texture_View(const gdi_texture_view_create_info* CreateInfo);
export_function gdi_handle GDI_Create_Texture_View_From_Texture(gdi_handle Texture);
export_function void GDI_Delete_Texture_View(gdi_handle TextureView);
export_function gdi_handle GDI_Get_Texture_View_Texture(gdi_handle TextureView);
export_function gdi_handle GDI_Create_Buffer(const gdi_buffer_create_info* CreateInfo);
export_function void GDI_Delete_Buffer(gdi_handle Buffer);
export_function void* GDI_Map_Buffer(gdi_handle Buffer, size_t Offset, size_t Size);
export_function void GDI_Unmap_Buffer(gdi_handle Buffer);
export_function gdi_handle GDI_Create_Sampler(const gdi_sampler_create_info* CreateInfo);
export_function void GDI_Delete_Sampler(gdi_handle Sampler);
export_function gdi_handle GDI_Create_Bind_Group_Layout(const gdi_bind_group_layout_create_info* CreateInfo);
export_function void GDI_Delete_Bind_Group_Layout(gdi_handle BindGroupLayout);
export_function gdi_handle GDI_Create_Bind_Group(const gdi_bind_group_create_info* CreateInfo);
export_function void GDI_Delete_Bind_Group(gdi_handle BindGroup);
export_function void GDI_Update_Bind_Groups(gdi_bind_group_write_array Writes, gdi_bind_group_copy_array Copies);
export_function gdi_handle GDI_Create_Shader(const gdi_shader_create_info* CreateInfo);
export_function void GDI_Delete_Shader(gdi_handle Shader);
export_function gdi_handle GDI_Create_Swapchain(const gdi_swapchain_create_info* CreateInfo);
export_function void GDI_Delete_Swapchain(gdi_handle Swapchain);
export_function gdi_handle GDI_Get_Swapchain_View(gdi_handle Swapchain);
export_function gdi_swapchain_info GDI_Get_Swapchain_Info(gdi_handle Swapchain);

/* Frames */
export_function void GDI_Submit_Render_Pass(gdi_render_pass* RenderPass);
export_function void GDI_Submit_Compute_Pass(gdi_handle_array TextureWrites, gdi_handle_array BufferWrites, gdi_dispatch_array Dispatches);
export_function void GDI_Render(const gdi_render_params* RenderParams);

/* Render Pass */
export_function gdi_render_pass* GDI_Begin_Render_Pass(const gdi_render_pass_begin_info* BeginInfo);
export_function void GDI_End_Render_Pass(gdi_render_pass* RenderPass);

export_function gdi_handle Render_Get_Shader(gdi_render_pass* RenderPass);

export_function void Render_Set_Shader(gdi_render_pass* RenderPass, gdi_handle Shader);
export_function void Render_Set_Bind_Groups(gdi_render_pass* RenderPass, size_t Offset, gdi_handle* BindGroups, size_t Count);
export_function void Render_Set_Bind_Group(gdi_render_pass* RenderPass, size_t Index, gdi_handle BindGroup);
export_function void Render_Set_Push_Constants(gdi_render_pass* RenderPass, void* Data, size_t Size);
export_function void Render_Set_Vtx_Buffer(gdi_render_pass* RenderPass, u32 Index, gdi_handle VtxBuffer);
export_function void Render_Set_Vtx_Buffers(gdi_render_pass* RenderPass, u32 Count, gdi_handle* VtxBuffers);
export_function void Render_Set_Idx_Buffer(gdi_render_pass* RenderPass, gdi_handle IdxBuffer, gdi_idx_format IdxFormat);
export_function void Render_Set_Scissor(gdi_render_pass* RenderPass, s32 MinX, s32 MinY, s32 MaxX, s32 MaxY);
export_function void Render_Draw_Idx(gdi_render_pass* RenderPass, u32 IdxCount, u32 IdxOffset, u32 VtxOffset);
export_function void Render_Draw(gdi_render_pass* RenderPass, u32 VtxCount, u32 VtxOffset);

#include "im_gdi.h"

#ifdef __cplusplus
}
#endif

#endif