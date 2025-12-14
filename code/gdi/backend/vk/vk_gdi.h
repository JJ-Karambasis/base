#ifndef VK_GDI_H
#define VK_GDI_H

#define VK_FRAME_COUNT (2)
#define VK_MAX_TRANSFER_COUNT (VK_FRAME_COUNT+1)

#define VK_COLOR_COMPONENT_ALL (VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT)

typedef struct vk_gdi vk_gdi;
typedef struct vk_device_context vk_device_context;

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
	vk_device_context* 	 Context;
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
	gdi_handle 				   Swapchain;
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
	VkDeviceSize 	   TotalSize;
	gdi_buffer_usage   Usage;
	vk_cpu_buffer_push MappedUpload; //Used if usage is not dynamic
	u8*                MappedPtr; //Used if usage is dynamic
} vk_buffer;

typedef struct {
	VkSampler Sampler;
} vk_sampler;

typedef struct {
	VkDescriptorSetLayout Layout;
	gdi_bind_group_binding_array Bindings;
	size_t DynamicBindingCount;
} vk_bind_group_layout;

typedef struct {
	VkDescriptorSet Set;
	gdi_handle      Layout;
	u32_array 	    DynamicSizes;
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

typedef struct vk_semaphore vk_semaphore;
typedef struct {
	gdi_handle 		   Handle;
	VkSurfaceKHR   	   Surface;
	VkSwapchainKHR 	   Swapchain;
	u32 		   	   ImageCount;
	VkImage* 	   	   Images;
	gdi_handle*    	   Textures;
	gdi_handle*    	   TextureViews;
	vk_semaphore* 	   Locks;
	VkSurfaceFormatKHR SurfaceFormat;
	gdi_format 		   Format;
	v2i 			   Dim;
	u32 			   ImageIndex;
	b32 			   HasFailed;
} vk_swapchain;

typedef struct vk_delete_thread_context vk_delete_thread_context;

Meta()
typedef enum {
	VK_RESOURCE_STATE_NONE Tags(stage: VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR, access: 0, layout: VK_IMAGE_LAYOUT_UNDEFINED),
	VK_RESOURCE_STATE_TRANSFER_WRITE Tags(stage: VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR, access: VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR, layout: VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
	VK_RESOURCE_STATE_TRANSFER_READ Tags(stage: VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR, access: VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR, layout: VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL), 
	VK_RESOURCE_STATE_SHADER_READ Tags(stage: VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR|VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR, access: VK_ACCESS_2_SHADER_READ_BIT_KHR, layout: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	VK_RESOURCE_STATE_DEPTH Tags(stage: VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR|VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR, access:VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR, layout: VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL), 
	VK_RESOURCE_STATE_MEMORY_READ Tags(stage: VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR, access: VK_ACCESS_2_MEMORY_READ_BIT_KHR, layout: VK_IMAGE_LAYOUT_GENERAL),
	VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE Tags(stage: VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR, access: VK_ACCESS_2_SHADER_WRITE_BIT_KHR, layout: VK_IMAGE_LAYOUT_GENERAL),
	VK_RESOURCE_STATE_RENDER_TARGET Tags(stage: VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, access: VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR|VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR, layout: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
} vk_resource_state;


Dynamic_Array_Define(VkImageMemoryBarrier, vk_image_memory_barrier);
Dynamic_Array_Define(VkImageMemoryBarrier2KHR, vk_image_memory_barrier2);
typedef struct {
	dynamic_vk_image_memory_barrier2_array Barriers;
	VkCommandBuffer CmdBuffer;
} vk_barriers;

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

	//CONFIRM(JJ): Why did we put these in the transfer thread context?
	//Was it for the extra frame?
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
	os_mutex*        FenceLock;
	VkSemaphore      RenderLock;
	VkSemaphore 	 TransferLock;
	VkCommandPool    CmdPool;
	VkCommandBuffer  CmdBuffer;
	vk_cpu_buffer 	 ReadbackBuffer;
	u32 			 Index;

	vk_texture_readback_array TextureReadbacks;
	vk_buffer_readback_array BufferReadbacks;
} vk_frame_context;

struct vk_semaphore {
	VkSemaphore Handle;
};

typedef struct vk_semaphore_delete_queue_entry vk_semaphore_delete_queue_entry;
struct vk_semaphore_delete_queue_entry {
	vk_semaphore 					 Entry;
	vk_semaphore_delete_queue_entry* Next;
};

typedef struct {
	vk_semaphore_delete_queue_entry* First;
	vk_semaphore_delete_queue_entry* Last;
} vk_semaphore_delete_queue;

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
	vk_swapchain_delete_queue 		  SwapchainDeleteQueue Tags(name: Swapchain);
	vk_semaphore_delete_queue 		  SemaphoreDeleteQueue Tags(name: Semaphore);
	vk_delete_thread_context* 		  Next Tags(NoIteration);
};

typedef struct {
	os_tls*    ThreadContextTLS;
	atomic_ptr TopThread;
} vk_delete_queue;

Dynamic_Array_Define(VkWriteDescriptorSet, vk_write_descriptor_set);

typedef struct {
	VkPhysicalDevice 				 PhysicalDevice;
	VkPhysicalDeviceProperties 		 Properties;
	VkPhysicalDeviceMemoryProperties Memory;
	u32 							 GraphicsQueueFamilyIndex;
	u32 							 PresentQueueFamilyIndex;
	dynamic_char_ptr_array 			 Extensions;
	VkPhysicalDeviceFeatures2KHR* 	 Features;
	b32 							 HasNullDescriptorFeature;
} vk_gpu;

struct vk_device_context {
	gdi_device_context Base;

	arena* Arena;
	arena* FrameArena;

	vk_gpu* 	GPU;

	VkDevice Device;

	VkQueue  GraphicsQueue;
	VkQueue  PresentQueue;
	VkQueue  TransferQueue;

	//Support device features
	b32 HasNullDescriptor;

	//Readback thread
	atomic_b32 	  ReadbackIsInitialized;
	os_thread* 	  ReadbackThread;
	os_semaphore* ReadbackSignalSemaphore;
	os_semaphore* ReadbackFinishedSemaphore;

	//Resources
	VmaAllocator 	 GPUAllocator;
	vk_resource_pool ResourcePool;
	os_mutex* 		 DescriptorLock;
	VkDescriptorPool DescriptorPool;

	//Frames
	u64 			  ReadbackFrameIndex;
	atomic_u64 		  FrameIndex;
	vk_frame_context  Frames[VK_FRAME_COUNT];
	vk_frame_context* CurrentFrame;

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

struct vk_gdi {
	gdi 		 			 Base;
	
	//Instance handles
	os_library*  			 Library;
	VkInstance   			 Instance;
	VkDebugUtilsMessengerEXT DebugUtils;

	//Devices
	size_t   		 GPUCount;
	vk_gpu*  		 GPUs;
};


#endif