#ifndef VK_GDI_H
#define VK_GDI_H

#define VK_FRAME_COUNT 2
#define VK_MAX_TRANSFER_COUNT (VK_FRAME_COUNT+1)

#define VK_COLOR_COMPONENT_ALL (VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT)

typedef struct vk_gdi vk_gdi;

#define MINIMUM_VK_CPU_BLOCK_SIZE MB(1)
typedef struct vk_cpu_buffer_block vk_cpu_buffer_block; 

typedef enum {
	VK_CPU_BUFFER_TYPE_UPLOAD,
	VK_CPU_BUFFER_TYPE_READBACK
} vk_cpu_buffer_type;

struct vk_cpu_buffer_block {
	VkBuffer 	  Buffer;
	VmaAllocation Allocation;
	u8* 		  Start;
	u8* 		  Current;
	u8* 		  End;
	vk_cpu_buffer_block* Next;
};

typedef struct {
	arena* 		   		 BlockArena;
	vk_gdi* 	   		 GDI;
	vk_cpu_buffer_type   Type;
	vk_cpu_buffer_block* FirstBlock;
	vk_cpu_buffer_block* LastBlock;
	vk_cpu_buffer_block* CurrentBlock;
} vk_cpu_buffer;

typedef struct {
	VkBuffer 	 Buffer;
	VkDeviceSize Offset;
	u8* 		 Ptr;
} vk_cpu_buffer_push;

typedef struct {
	VkImage    	  	  		   Image;
	VmaAllocation 	  		   Allocation;
	gdi_format 	  	  		   Format;
	v2i 	   	  	  		   Dim;
	u32 	   	  	  		   MipCount;
	gdi_texture_usage 		   Usage;
	b32 	   	  	  		   IsSwapchainManaged;
	b32 			  		   QueuedBarrier;
} vk_texture;

typedef struct {
	VkImageView ImageView;
	gdi_handle  Texture;
	gdi_format  Format;
	v2i 		Dim;
} vk_texture_view;

typedef struct {
	VkBuffer 	  	   Buffer;
	VmaAllocation 	   Allocation;
	VkDeviceSize  	   Size;
	vk_cpu_buffer_push MappedUpload;
} vk_buffer;

typedef struct {
	VkSampler Sampler;
} vk_sampler;

typedef struct {
	VkDescriptorSetLayout Layout;
	gdi_bind_group_binding_array Bindings;
} vk_bind_group_layout;

typedef struct {
	VkDescriptorSet Set;
	gdi_handle      Layout;
} vk_bind_group;

typedef struct {
	u32 			 BindGroupCount;
	VkPipelineLayout Layout;
	VkPipeline Pipeline;
	gdi_bind_group_binding_array WritableBindings;
	VkDescriptorSetLayout WritableLayout;

#ifdef DEBUG_BUILD
	gdi_pass_type DEBUGType;
#endif
} vk_shader;

typedef struct vk_delete_thread_context vk_delete_thread_context;

#include "meta/vk_meta.h"

typedef struct vk_render_pass vk_render_pass;

struct vk_render_pass {
	gdi_render_pass Base;
	gdi_handle      RenderTargetViews[GDI_MAX_RENDER_TARGET_COUNT];
	gdi_handle 		DepthBufferView;
	v2i 			Dim;
	gdi_clear_color ClearColors[GDI_MAX_RENDER_TARGET_COUNT];
	gdi_clear_depth ClearDepth;
	VkCommandBuffer CmdBuffer;
	vk_render_pass* Next;
};

typedef struct vk_texture_barrier_cmd vk_texture_barrier_cmd;
struct vk_texture_barrier_cmd {
	vk_texture* Texture;
	b32 IsTransfer;
	vk_texture_barrier_cmd* Next;
};

typedef struct vk_transfer_thread_context vk_transfer_thread_context;
struct vk_transfer_thread_context {
	arena* 			Arena;
	arena* 			TempArena;
	b32 			NeedsReset;
	VkCommandPool   CmdPool;
	VkCommandBuffer CmdBuffer;
	vk_cpu_buffer   UploadBuffer;
	vk_render_pass* FreeRenderPasses;
	vk_render_pass* RenderPassesToDelete;

	vk_texture_barrier_cmd* FirstTextureBarrierCmd;
	vk_texture_barrier_cmd* LastTextureBarrierCmd;

	vk_transfer_thread_context* Next;
};

typedef struct {
	os_tls* 	     ThreadContextTLS;
	VkCommandPool    CmdPool;
	VkCommandBuffer  CmdBuffer;
	atomic_ptr 		 TopThread;

#ifdef DEBUG_BUILD
	VkFence DEBUGFence;
#endif
} vk_transfer_context;

typedef struct {
	gdi_handle Texture;
	vk_cpu_buffer_push CPU;
	gdi_texture_readback_func* Func;
	void* UserData;
} vk_texture_readback;

typedef struct {
	gdi_handle Buffer;
	vk_cpu_buffer_push CPU;
	gdi_buffer_readback_func* Func;
	void* UserData;
} vk_buffer_readback;

Array_Define(vk_texture_readback);
Array_Define(vk_buffer_readback);

typedef struct {
	arena* 			 Arena;
	VkFence 		 Fence;
	VkSemaphore      SwapchainLock;
	VkSemaphore      RenderLock;
	VkSemaphore 	 TransferLock;
	VkCommandPool    CmdPool;
	VkCommandBuffer  CmdBuffer;
	vk_cpu_buffer 	 ReadbackBuffer;

	vk_texture_readback_array TextureReadbacks;
	vk_buffer_readback_array BufferReadbacks;
} vk_frame_context;

Meta()
struct vk_delete_thread_context {
	arena* 							  Arena Tags(NoIteration);
	arena_marker 					  ArenaMarker Tags(NoIteration);
	vk_texture_delete_queue 		  TextureDeleteQueue Tags(name: Texture);
	vk_texture_view_delete_queue 	  Texture_ViewDeleteQueue Tags(name: Texture_View);
	vk_buffer_delete_queue 			  BufferDeleteQueue Tags(name: Buffer);
	vk_sampler_delete_queue 		  SamplerDeleteQueue Tags(name: Sampler);
	vk_bind_group_layout_delete_queue Bind_Group_LayoutDeleteQueue Tags(name: Bind_Group_Layout);
	vk_bind_group_delete_queue 		  Bind_GroupDeleteQueue Tags(name: Bind_Group);
	vk_shader_delete_queue 			  ShaderDeleteQueue Tags(name: Shader);
	vk_delete_thread_context* 		  Next Tags(NoIteration);
};

typedef struct {
	os_tls*    ThreadContextTLS;
	atomic_ptr TopThread;
} vk_delete_queue;

Dynamic_Array_Define(VkImageMemoryBarrier, vk_image_memory_barrier);
Dynamic_Array_Define(VkImageMemoryBarrier2KHR, vk_image_memory_barrier2);
Dynamic_Array_Define(VkWriteDescriptorSet, vk_write_descriptor_set);

typedef struct {
	VkPhysicalDevice 				 PhysicalDevice;
	VkPhysicalDeviceProperties 		 Properties;
	VkPhysicalDeviceMemoryProperties Memory;
	u32 							 GraphicsQueueFamilyIndex;
	u32 							 PresentQueueFamilyIndex;
	dynamic_char_ptr_array 			 Extensions;
	VkPhysicalDeviceFeatures2KHR* 	 Features;
} vk_gpu;

struct vk_gdi {
	gdi 		 			 Base;
	
	//Instance handles
	os_library*  			 Library;
	VkInstance   			 Instance;
	VkSurfaceKHR 			 Surface;
	VkDebugUtilsMessengerEXT DebugUtils;

	//Devices
	size_t   GPUCount;
	vk_gpu*  GPUs;
	vk_gpu*  TargetGPU;
	VkDevice Device;
	VkQueue  GraphicsQueue;
	VkQueue  PresentQueue;
	VkQueue  TransferQueue;
	
	//Swapchain
	VkSwapchainKHR Swapchain;
	u32 		   SwapchainImageCount;
	VkImage* 	   SwapchainImages;
	gdi_handle*    SwapchainTextures;
	gdi_handle*    SwapchainTextureViews;
	u32 		   SwapchainImageIndex;

	//Resources
	VmaAllocator 	 GPUAllocator;
	vk_resource_pool ResourcePool;
	os_mutex* 		 DescriptorLock;
	VkDescriptorPool DescriptorPool;

	//Frames
	u64 			    FrameIndex;
	vk_frame_context    Frames[VK_FRAME_COUNT];
	vk_frame_context*   CurrentFrame;

	//Deletes
	os_rw_mutex* 	 DeleteLock;
	vk_delete_queue  DeleteQueues[VK_FRAME_COUNT];
	vk_delete_queue* CurrentDeleteQueue;

	//Transfers
	os_rw_mutex* 		 TransferLock;
	u64 				 TransferIndex;
	vk_transfer_context  Transfers[VK_MAX_TRANSFER_COUNT];
	vk_transfer_context* CurrentTransfer;
};

#endif