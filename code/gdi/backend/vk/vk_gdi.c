#define VK_NO_PROTOTYPES

#include <meta_program/meta_defines.h>

#ifdef VK_USE_PLATFORM_METAL_EXT
#define VK_ENABLE_BETA_EXTENSIONS
#endif

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <base.h>

#include <gdi/gdi.h>

#include "vk_gdi.h"

#include <gdi/gdi.c>

Dynamic_Array_Implement(char_ptr, char*, Char_Ptr);
Dynamic_Array_Implement(vk_image_memory_barrier, VkImageMemoryBarrier, VK_Image_Memory_Barrier);
Dynamic_Array_Implement(vk_image_memory_barrier2, VkImageMemoryBarrier2KHR, VK_Image_Memory_Barrier2);
Dynamic_Array_Implement(vk_write_descriptor_set, VkWriteDescriptorSet, VK_Write_Descriptor_Set);
Array_Implement(vk_texture_readback, VK_Texture_Readback);
Array_Implement(vk_buffer_readback, VK_Buffer_Readback);

global string G_RequiredInstanceExtensions[] = {
	String_Expand(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME),
	String_Expand(VK_KHR_SURFACE_EXTENSION_NAME),
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	String_Expand(VK_KHR_WIN32_SURFACE_EXTENSION_NAME)
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	String_Expand(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME),
	String_Expand(VK_EXT_METAL_SURFACE_EXTENSION_NAME)
#else
#error "Not Implemented!"
#endif
};

global string G_RequiredDeviceExtensions[] = {
	String_Expand(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME),
	String_Expand(VK_KHR_MULTIVIEW_EXTENSION_NAME),
	String_Expand(VK_KHR_MAINTENANCE2_EXTENSION_NAME),
	String_Expand(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME),
	String_Expand(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME),
	String_Expand(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME),
	String_Expand(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME),
	String_Expand(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME),
	String_Expand(VK_KHR_SWAPCHAIN_EXTENSION_NAME),
	String_Expand(VK_KHR_MAINTENANCE3_EXTENSION_NAME),
	String_Expand(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
#ifdef VK_USE_PLATFORM_METAL_EXT
	String_Expand(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
#endif
};

#if defined(VK_USE_PLATFORM_WIN32_KHR)

typedef struct {
	HWND Window;
	VkSurfaceKHR Surface;
} vk_tmp_surface;

VkSurfaceKHR VK_Create_Surface(vk_gdi* GDI, HWND Window, HINSTANCE Instance) {
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(GDI->Instance, "vkCreateWin32SurfaceKHR");
	
	VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = Instance,
		.hwnd = Window
	};

	VkSurfaceKHR Surface;
	VkResult Status = vkCreateWin32SurfaceKHR(GDI->Instance, &SurfaceCreateInfo, VK_NULL_HANDLE, &Surface);
	if (Status != VK_SUCCESS) {
		Debug_Log("vkCreateWin32SurfaceKHR failed!");
		return VK_NULL_HANDLE;
	}

	return Surface;
}

function vk_tmp_surface VK_Create_Tmp_Surface(vk_gdi* GDI) {
	vk_tmp_surface Result;
	Memory_Clear(&Result, sizeof(vk_tmp_surface));

	// Create a minimal window class
    WNDCLASSA WindowClass = {
		.lpfnWndProc = DefWindowProcA,
		.hInstance = GetModuleHandle(NULL),
		.lpszClassName = "GDI_DummyVulkanWindow"
	};
    
    if (!RegisterClassA(&WindowClass)) {
		Debug_Log("Failed to register window class");
		return Result;
    }

	HWND Window = CreateWindowA(WindowClass.lpszClassName, "Dummy", 
                              	   WS_POPUP, 0, 0, 1, 1, 
								   NULL, NULL, WindowClass.hInstance, NULL);
    
    if (!Window) {
        Debug_Log("Failed to create window");
        return Result;
    }

	VkSurfaceKHR Surface = VK_Create_Surface(GDI, Window, WindowClass.hInstance);
	if (Surface != VK_NULL_HANDLE) {
		Result.Window = Window;
		Result.Surface = Surface;
	}

	return Result;
}

function void VK_Delete_Tmp_Surface(vk_gdi* GDI, vk_tmp_surface* Surface) {
	vkDestroySurfaceKHR(GDI->Instance, Surface->Surface, VK_NULL_HANDLE);
	DestroyWindow(Surface->Window);
	UnregisterClassA("GDI_DummyVulkanWindow", GetModuleHandle(NULL));
}

#else
#error "Not Implemented!"
#endif

function inline vk_barriers VK_Barriers_Init(arena* Arena, VkCommandBuffer CmdBuffer) {
	vk_barriers Result = {
		.Barriers = Dynamic_VK_Image_Memory_Barrier2_Array_Init((allocator*)Arena),
		.CmdBuffer = CmdBuffer
	};
	return Result;
}

function void VK_Add_Texture_Barrier(vk_barriers* Barriers, vk_texture* Texture, vk_resource_state OldState, vk_resource_state NewState) {
	VkImageAspectFlags ImageAspect = GDI_Is_Depth_Format(Texture->Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageMemoryBarrier2KHR Barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
		.srcStageMask = VK_Get_Stage_Mask(OldState),
		.srcAccessMask = VK_Get_Access_Mask(OldState),
		.dstStageMask = VK_Get_Stage_Mask(NewState),
		.dstAccessMask = VK_Get_Access_Mask(NewState),
		.oldLayout = VK_Get_Image_Layout(OldState),
		.newLayout = VK_Get_Image_Layout(NewState),
		.image = Texture->Image,
		.subresourceRange = { ImageAspect, 0, Texture->MipCount, 0, 1 }
	};
	Dynamic_VK_Image_Memory_Barrier2_Array_Add(&Barriers->Barriers, Barrier);
}

function void VK_Add_Pre_Swapchain_Barrier(vk_barriers* Barriers, vk_texture* Texture, VkPipelineStageFlags2KHR OldStage, vk_resource_state NewState) {
	VkImageMemoryBarrier2KHR Barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
		.srcStageMask = OldStage,
		.srcAccessMask = 0,
		.dstStageMask = VK_Get_Stage_Mask(NewState),
		.dstAccessMask = VK_Get_Access_Mask(NewState),
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_Get_Image_Layout(NewState),
		.image = Texture->Image,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, Texture->MipCount, 0, 1 }
	};
	Dynamic_VK_Image_Memory_Barrier2_Array_Add(&Barriers->Barriers, Barrier);
}

function void VK_Add_Post_Swapchain_Barrier(vk_barriers* Barriers, vk_texture* Texture, vk_resource_state OldState) {
	VkImageMemoryBarrier2KHR Barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
		.srcStageMask = VK_Get_Stage_Mask(OldState),
		.srcAccessMask = VK_Get_Access_Mask(OldState),
		.dstStageMask = VK_PIPELINE_STAGE_2_NONE_KHR,
		.dstAccessMask = 0,
		.oldLayout = VK_Get_Image_Layout(OldState),
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.image = Texture->Image,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, Texture->MipCount, 0, 1 }
	};
	Dynamic_VK_Image_Memory_Barrier2_Array_Add(&Barriers->Barriers, Barrier);
}

function void VK_Submit_Memory_Barriers(vk_barriers* Barriers, vk_resource_state OldMemoryState, vk_resource_state NewMemoryState) {
	VkMemoryBarrier2KHR MemoryBarrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
		.srcStageMask = VK_Get_Stage_Mask(OldMemoryState),
		.srcAccessMask = VK_Get_Access_Mask(OldMemoryState),
		.dstStageMask = VK_Get_Stage_Mask(NewMemoryState),
		.dstAccessMask = VK_Get_Access_Mask(NewMemoryState)
	};

	VkDependencyInfoKHR DependencyInfo = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
		.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &MemoryBarrier,
		.imageMemoryBarrierCount = (u32)Barriers->Barriers.Count,
		.pImageMemoryBarriers = Barriers->Barriers.Count ? Barriers->Barriers.Ptr : NULL
	};
	vkCmdPipelineBarrier2KHR(Barriers->CmdBuffer, &DependencyInfo);
}

function void VK_Submit_Barriers(vk_barriers* Barriers) {
	if (Barriers->Barriers.Count) {
		VkDependencyInfoKHR DependencyInfo = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
			.imageMemoryBarrierCount = (u32)Barriers->Barriers.Count,
			.pImageMemoryBarriers = Barriers->Barriers.Ptr
		};
		vkCmdPipelineBarrier2KHR(Barriers->CmdBuffer, &DependencyInfo);
	}
}

function void VK_Barriers_Clear(vk_barriers* Barriers) {
	Dynamic_VK_Image_Memory_Barrier2_Array_Clear(&Barriers->Barriers);
}

function inline vk_cpu_buffer VK_CPU_Buffer_Init(vk_gdi* GDI, arena* Arena, vk_cpu_buffer_type Type) {
	vk_cpu_buffer Result = {
		.BlockArena = Arena,
		.GDI = GDI,
		.Type = Type
	};
	return Result;
}

function void VK_CPU_Buffer_Clear(vk_cpu_buffer* CpuBuffer) {
	for (vk_cpu_buffer_block* Block = CpuBuffer->FirstBlock; Block; Block = Block->Next) {
		Block->Current = Block->Start;
	}
	CpuBuffer->CurrentBlock = CpuBuffer->FirstBlock;
}

function vk_cpu_buffer_block* VK_CPU_Buffer_Get_Current_Block(vk_cpu_buffer* CpuBuffer, VkDeviceSize Size) {
	vk_cpu_buffer_block* Block = CpuBuffer->CurrentBlock;
	while (Block) {
		if ((Block->Current + Size) <= Block->End) {
			return Block;
		}
		Block = Block->Next;
	}
	return Block;
}

function vk_cpu_buffer_block* VK_CPU_Buffer_Create_Block(vk_cpu_buffer* CpuBuffer, VkDeviceSize Size) {	
	VkBufferUsageFlags BufferUsage = 0;
	VmaAllocationCreateFlags AllocationFlags = 0;

	switch (CpuBuffer->Type) {
		case VK_CPU_BUFFER_TYPE_UPLOAD: {
			BufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		} break;

		case VK_CPU_BUFFER_TYPE_READBACK: {
			BufferUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT|VMA_ALLOCATION_CREATE_MAPPED_BIT;
		} break;
	}
	
	VkBufferCreateInfo BufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = Size,
		.usage = BufferUsage
	};

	VmaAllocationCreateInfo AllocateInfo = {
		.flags = AllocationFlags,
		.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST
	};

	VkBuffer Buffer;
	VmaAllocation Allocation;
	VmaAllocationInfo AllocInfo;
	VkResult Status = vmaCreateBuffer(CpuBuffer->GDI->GPUAllocator, &BufferInfo, &AllocateInfo, &Buffer, &Allocation, &AllocInfo);
	if (Status != VK_SUCCESS) {
		Debug_Log("vmaCreateBuffer failed!");
		return NULL;
	}

	vk_cpu_buffer_block* Result = Arena_Push_Struct(CpuBuffer->BlockArena, vk_cpu_buffer_block);
	Result->Buffer = Buffer;
	Result->Allocation = Allocation;
	Result->Start = (u8*)AllocInfo.pMappedData;
	Result->Current = Result->Start;
	Result->End = Result->Start + Size;

	SLL_Push_Back(CpuBuffer->FirstBlock, CpuBuffer->LastBlock, Result);
	return Result;
}

function vk_cpu_buffer_push VK_CPU_Buffer_Push(vk_cpu_buffer* CpuBuffer, VkDeviceSize PushSize) {
	vk_cpu_buffer_block* CurrentBlock = VK_CPU_Buffer_Get_Current_Block(CpuBuffer, PushSize);
	if (!CurrentBlock) {
		VkDeviceSize Size = Max(PushSize, MINIMUM_VK_CPU_BLOCK_SIZE);
		CurrentBlock = VK_CPU_Buffer_Create_Block(CpuBuffer, Size);
	}

	CpuBuffer->CurrentBlock = CurrentBlock;
	vk_cpu_buffer_push Result = {
		.Buffer = CurrentBlock->Buffer,
		.Offset = (VkDeviceSize)(CurrentBlock->Current - CurrentBlock->Start),
		.Ptr = CurrentBlock->Current
	};

	CurrentBlock->Current += PushSize;
	return Result;
}

function vk_delete_thread_context* VK_Get_Delete_Thread_Context(vk_gdi* GDI, vk_delete_queue* DeleteQueue) {
	vk_delete_thread_context* ThreadContext = (vk_delete_thread_context*)OS_TLS_Get(DeleteQueue->ThreadContextTLS);
	if (!ThreadContext) {
		arena* Arena = Arena_Create();
		ThreadContext = Arena_Push_Struct(Arena, vk_delete_thread_context);
		ThreadContext->Arena = Arena;
		ThreadContext->ArenaMarker = Arena_Get_Marker(ThreadContext->Arena);
		
		/*Append to link list atomically*/
		for(;;) {
			vk_delete_thread_context* OldTop = (vk_delete_thread_context*)Atomic_Load_Ptr(&DeleteQueue->TopThread);
			ThreadContext->Next = OldTop;
			if(Atomic_Compare_Exchange_Ptr(&DeleteQueue->TopThread, OldTop, ThreadContext) == OldTop) {
				break;
			}
		}

		OS_TLS_Set(DeleteQueue->ThreadContextTLS, ThreadContext);
	}
	return ThreadContext;
}

function vk_transfer_thread_context* VK_Get_Transfer_Thread_Context(vk_gdi* GDI, vk_transfer_context* Transfer) {
	vk_transfer_thread_context* ThreadContext = (vk_transfer_thread_context*)OS_TLS_Get(Transfer->ThreadContextTLS);
	if (!ThreadContext) {
		arena* Arena = Arena_Create();
		ThreadContext = Arena_Push_Struct(Arena, vk_transfer_thread_context);
		ThreadContext->Arena = Arena;
		ThreadContext->TempArena = Arena_Create();
		ThreadContext->NeedsReset = true;
		
		VkCommandPoolCreateInfo CommandPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = GDI->TargetGPU->GraphicsQueueFamilyIndex
		};
		VkResult Status = vkCreateCommandPool(GDI->Device, &CommandPoolCreateInfo, VK_NULL_HANDLE, &ThreadContext->CmdPool);
		if (Status != VK_SUCCESS) {
			Debug_Log("vkCreateCommandPool failed!");
			return NULL;
		}

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = ThreadContext->CmdPool,
			.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			.commandBufferCount = 1
		};

		Status = vkAllocateCommandBuffers(GDI->Device, &CommandBufferAllocateInfo, &ThreadContext->CmdBuffer);
		if (Status != VK_SUCCESS) {
			Debug_Log("vkAllocateCommandBuffers failed!");
			return NULL;
		}

		//todo: Improvement
		//These upload buffers can eat a lot of memory since there is one
		//upload buffer per frame/per thread. On some system with many
		//cores this can consume a lot of additional megabytes
		ThreadContext->UploadBuffer = VK_CPU_Buffer_Init(GDI, ThreadContext->Arena, VK_CPU_BUFFER_TYPE_UPLOAD);

		/*Append to link list atomically*/
		for(;;) {
			vk_transfer_thread_context* OldTop = (vk_transfer_thread_context*)Atomic_Load_Ptr(&Transfer->TopThread);
			ThreadContext->Next = OldTop;
			if(Atomic_Compare_Exchange_Ptr(&Transfer->TopThread, OldTop, ThreadContext) == OldTop) {
				break;
			}
		}

		OS_TLS_Set(Transfer->ThreadContextTLS, ThreadContext);
	}

	if (ThreadContext->NeedsReset) {
		Arena_Clear(ThreadContext->TempArena);

		vkResetCommandPool(GDI->Device, ThreadContext->CmdPool, 0);
		
		VkCommandBufferInheritanceInfo InheritanceInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO
		};

		VkCommandBufferBeginInfo BeginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = &InheritanceInfo
		};

		vkBeginCommandBuffer(ThreadContext->CmdBuffer, &BeginInfo);
		VK_CPU_Buffer_Clear(&ThreadContext->UploadBuffer);
		ThreadContext->NeedsReset = false;

		ThreadContext->FirstTextureBarrierCmd = NULL;
		ThreadContext->LastTextureBarrierCmd = NULL;
	}

	return ThreadContext;
}

function void VK_Delete_Queued_Resources(vk_gdi* GDI, vk_delete_queue* DeleteQueue) {
	for (vk_delete_thread_context* ThreadContext = (vk_delete_thread_context*)Atomic_Load_Ptr(&DeleteQueue->TopThread);
		ThreadContext; ThreadContext = ThreadContext->Next) {
		VK_Delete_Queued_Thread_Resources(GDI, ThreadContext);
	}
}

function void VK_Delete_Texture_Resources(vk_gdi* GDI, vk_texture* Texture) {
	vmaDestroyImage(GDI->GPUAllocator, Texture->Image, Texture->Allocation);
}

function void VK_Delete_Texture_View_Resources(vk_gdi* GDI, vk_texture_view* TextureView) {
	vkDestroyImageView(GDI->Device, TextureView->ImageView, VK_NULL_HANDLE);
}

function void VK_Delete_Buffer_Resources(vk_gdi* GDI, vk_buffer* Buffer) {
	Not_Implemented;
}

function void VK_Delete_Sampler_Resources(vk_gdi* GDI, vk_sampler* Sampler) {
	Not_Implemented;
}

function void VK_Delete_Bind_Group_Layout_Resources(vk_gdi* GDI, vk_bind_group_layout* BindGroupLayout) {
	Not_Implemented;
}

function void VK_Delete_Bind_Group_Resources(vk_gdi* GDI, vk_bind_group* BindGroup) {
	Not_Implemented;
}

function void VK_Delete_Shader_Resources(vk_gdi* GDI, vk_shader* Shader) {
	vkDestroyPipeline(GDI->Device, Shader->Pipeline, VK_NULL_HANDLE);

	vkDestroyPipelineLayout(GDI->Device, Shader->Layout, VK_NULL_HANDLE);

	if (Shader->WritableLayout) {
		vkDestroyDescriptorSetLayout(GDI->Device, Shader->WritableLayout, VK_NULL_HANDLE);
	}

	if (!GDI_Bind_Group_Binding_Array_Is_Empty(Shader->WritableBindings)) {
		Allocator_Free_Memory(Default_Allocator_Get(), Shader->WritableBindings.Ptr);
	}
}

function void VK_Delete_Swapchain_Resources(vk_gdi* GDI, vk_swapchain* Swapchain) {
	Not_Implemented;
}

function b32 VK_Fill_GPU(vk_gdi* GDI, vk_gpu* GPU, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface) {
	VkPhysicalDeviceProperties DeviceProperties;
	vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT* DescriptorIndexingFeature = Arena_Push_Struct(GDI->Base.Arena, VkPhysicalDeviceDescriptorIndexingFeaturesEXT);
	VkPhysicalDeviceRobustness2FeaturesEXT* Robustness2Features = Arena_Push_Struct(GDI->Base.Arena, VkPhysicalDeviceRobustness2FeaturesEXT);
	VkPhysicalDeviceSynchronization2FeaturesKHR* Synchronization2Feature = Arena_Push_Struct(GDI->Base.Arena, VkPhysicalDeviceSynchronization2FeaturesKHR);
	VkPhysicalDeviceDynamicRenderingFeaturesKHR* DynamicRenderingFeature = Arena_Push_Struct(GDI->Base.Arena, VkPhysicalDeviceDynamicRenderingFeaturesKHR);
	VkPhysicalDeviceFeatures2KHR* Features = Arena_Push_Struct(GDI->Base.Arena, VkPhysicalDeviceFeatures2KHR);

	DescriptorIndexingFeature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	Robustness2Features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
	Synchronization2Feature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
	DynamicRenderingFeature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	Features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;

	Robustness2Features->pNext = DescriptorIndexingFeature;
	Synchronization2Feature->pNext = Robustness2Features;
	DynamicRenderingFeature->pNext = Synchronization2Feature;
	Features->pNext = DynamicRenderingFeature;
	
	vkGetPhysicalDeviceFeatures2KHR(PhysicalDevice, Features);

	b32 HasFeatures = true;
	if (!DescriptorIndexingFeature->descriptorBindingSampledImageUpdateAfterBind) {
		Debug_Log("Missing vulkan feature 'Descriptor Indexing' for device '%s'", DeviceProperties.deviceName);
		HasFeatures = false;
	}

	if (!Robustness2Features->nullDescriptor) {
		Debug_Log("Missing vulkan feature 'Null Descriptor' for device '%s'", DeviceProperties.deviceName);
		HasFeatures = false;
	}

	if (!Synchronization2Feature->synchronization2) {
		Debug_Log("Missing vulkan feature 'Synchronization 2' for device '%s'", DeviceProperties.deviceName);
		HasFeatures = false;
	}

	if (!DynamicRenderingFeature->dynamicRendering) {
		Debug_Log("Missing vulkan feature 'Dynamic Rendering' for device '%s'", DeviceProperties.deviceName);
		HasFeatures = false;
	}

	arena* Scratch = Scratch_Get();

	u32 DeviceExtensionCount;
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, VK_NULL_HANDLE, &DeviceExtensionCount, VK_NULL_HANDLE);

	VkExtensionProperties* DeviceExtensionProperties = Arena_Push_Array(Scratch, DeviceExtensionCount, VkExtensionProperties);
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, VK_NULL_HANDLE, &DeviceExtensionCount, DeviceExtensionProperties);

	GPU->Extensions = Dynamic_Char_Ptr_Array_Init((allocator*)GDI->Base.Arena);

	b32 HasRequiredDeviceExtensions[Array_Count(G_RequiredDeviceExtensions)] = { 0 };
	for (u32 j = 0; j < DeviceExtensionCount; j++) {
		string ExtensionName = String_Null_Term(DeviceExtensionProperties[j].extensionName);
		for (size_t k = 0; k < Array_Count(G_RequiredDeviceExtensions); k++) {
			if (String_Equals(ExtensionName, G_RequiredDeviceExtensions[k])) {
				HasRequiredDeviceExtensions[k] = true;
				Dynamic_Char_Ptr_Array_Add(&GPU->Extensions, (char*)G_RequiredDeviceExtensions[k].Ptr);
			}
		}
	}

	b32 HasExtensions = true;
	for (size_t j = 0; j < Array_Count(G_RequiredDeviceExtensions); j++) {
		if (!HasRequiredDeviceExtensions[j]) {
			Debug_Log("Missing vulkan device extension '%.*s' for device '%s'", G_RequiredDeviceExtensions[j].Size, G_RequiredDeviceExtensions[j].Ptr, DeviceProperties.deviceName);
			HasExtensions = false;
		}
	}

	b32 Result = false;
	if (HasExtensions && HasFeatures) {
		u32 QueueFamilyPropertyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, VK_NULL_HANDLE);

		VkQueueFamilyProperties* QueueFamilyProperties = Arena_Push_Array(Scratch, QueueFamilyPropertyCount, VkQueueFamilyProperties);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, QueueFamilyProperties);

		u32 GraphicsQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		for (u32 j = 0; j < QueueFamilyPropertyCount; j++) {
			if (QueueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				GraphicsQueueFamilyIndex = j;
				break;
			}
		}

		if (GraphicsQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED) {
			u32 PresentQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				
			//First, check if the graphics queue family can support presentation
			VkBool32 Supported = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, GraphicsQueueFamilyIndex, Surface, &Supported);

			if (Supported) {
				PresentQueueFamilyIndex = GraphicsQueueFamilyIndex;
			} else {
				//Other, find the first queue family that supports presentation 
				for (u32 j = 0; j < QueueFamilyPropertyCount; j++) {
					if (j != GraphicsQueueFamilyIndex) {
						vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, j, Surface, &Supported);
						if (Supported) {
							PresentQueueFamilyIndex = j;
							break;
						}
					}
				}
			}

			if (PresentQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED) {
				VkPhysicalDeviceMemoryProperties MemoryProperties;
				vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

				GPU->PhysicalDevice = PhysicalDevice;
				GPU->Properties = DeviceProperties;
				GPU->Memory = MemoryProperties;
				GPU->GraphicsQueueFamilyIndex = GraphicsQueueFamilyIndex;
				GPU->PresentQueueFamilyIndex = PresentQueueFamilyIndex;
				GPU->Features = Features;

				Result = true;
			} else {
				Debug_Log("Could not find a valid presentation queue family index for device '%s'", DeviceProperties.deviceName);
			}
		} else {
			Debug_Log("Could not find a valid graphics queue family index for device '%s'", DeviceProperties.deviceName);
		}
	}

	Scratch_Release();
	return Result;
}

function GDI_BACKEND_CREATE_TEXTURE_VIEW_DEFINE(VK_Create_Texture_View);
function b32 VK_Recreate_Swapchain(vk_gdi* GDI, vk_swapchain* Swapchain) {
	for (size_t i = 0; i < Swapchain->ImageCount; i++) {
		VK_Texture_Pool_Free(&GDI->ResourcePool, Swapchain->Textures[i]);
		VK_Texture_View_Add_To_Delete_Queue(GDI, Swapchain->TextureViews[i]);
	}

	VkSurfaceCapabilitiesKHR SurfaceCaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GDI->TargetGPU->PhysicalDevice, Swapchain->Surface, &SurfaceCaps);

	VkSwapchainCreateInfoKHR SwapchainCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = Swapchain->Surface,
		.minImageCount = Swapchain->ImageCount,
		.imageFormat = Swapchain->SurfaceFormat.format,
		.imageColorSpace = Swapchain->SurfaceFormat.colorSpace,
		.imageExtent = SurfaceCaps.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR, 
		.oldSwapchain = Swapchain->Swapchain
	};

	VkResult Status = vkCreateSwapchainKHR(GDI->Device, &SwapchainCreateInfo, VK_NULL_HANDLE, &Swapchain->Swapchain);

	if (Status != VK_SUCCESS) {
		Debug_Log("vkCreateSwapchainKHR failed!");
		return false;
	}

	vkGetSwapchainImagesKHR(GDI->Device, Swapchain->Swapchain, &Swapchain->ImageCount, Swapchain->Images);
	
	Swapchain->Dim = V2i(SurfaceCaps.currentExtent.width, SurfaceCaps.currentExtent.height);

	for (u32 i = 0; i < Swapchain->ImageCount; i++) {
		Swapchain->Textures[i] = VK_Texture_Pool_Allocate(&GDI->ResourcePool);

		vk_texture* Texture = VK_Texture_Pool_Get(&GDI->ResourcePool, Swapchain->Textures[i]);
		Texture->Image = Swapchain->Images[i];
		Texture->Dim = Swapchain->Dim;
		Texture->Format = Swapchain->Format;
		Texture->MipCount = 1;
		Texture->Swapchain = Swapchain->Handle;

		gdi_texture_view_create_info TextureViewCreateInfo = {
			.Texture = Swapchain->Textures[i]
		};
		Swapchain->TextureViews[i] = VK_Create_Texture_View((gdi*)GDI, &TextureViewCreateInfo);
	}

	Status = vkAcquireNextImageKHR(GDI->Device, Swapchain->Swapchain, UINT64_MAX, 
								   Swapchain->Locks[GDI->CurrentFrame->Index], VK_NULL_HANDLE, &Swapchain->ImageIndex);
	if (Status != VK_SUCCESS) {
		Debug_Log("vkAcquireNextImageKHR failed");
		return false;
	}

	return true;
}

#include "meta/vk_meta.c"

#define VK_Set_Debug_Name(type, type_str, handle, name_str) \
if (!String_Is_Empty(name_str)) { \
VkDebugUtilsObjectNameInfoEXT NameInfo = { \
.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, \
.objectType = type, \
.objectHandle = (u64)handle, \
.pObjectName = name_str.Ptr \
}; \
if (vkSetDebugUtilsObjectNameEXT(VkGDI->Device, &NameInfo) != VK_SUCCESS) { \
Debug_Log("WARNING: Could not set the vulkan debug name for " type_str " %.*s", name_str.Size, name_str.Ptr); \
} \
}

function GDI_BACKEND_CREATE_TEXTURE_DEFINE(VK_Create_Texture) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;

	VkImageUsageFlags ImageUsage = 0;
	if (TextureInfo->Usage & GDI_TEXTURE_USAGE_RENDER_TARGET) {
		ImageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if (TextureInfo->Usage & GDI_TEXTURE_USAGE_DEPTH) {
		ImageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}

	if (TextureInfo->Usage & GDI_TEXTURE_USAGE_SAMPLED) {
		ImageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	if (TextureInfo->Usage & GDI_TEXTURE_USAGE_STORAGE) {
		ImageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if (TextureInfo->Usage & GDI_TEXTURE_USAGE_READBACK) {
		ImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	if (TextureInfo->InitialData) {
		ImageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	VkImageCreateInfo ImageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_Get_Format(TextureInfo->Format),
		.extent = { (u32)TextureInfo->Dim.x, (u32)TextureInfo->Dim.y, 1 },
		.mipLevels = TextureInfo->MipCount,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = ImageUsage,
	};

	VmaAllocationCreateInfo AllocateInfo = {
		.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	};

	VkImage Image;
	VmaAllocation Allocation;
	VkResult Status = vmaCreateImage(VkGDI->GPUAllocator, &ImageCreateInfo, &AllocateInfo, &Image, &Allocation, NULL);
	if (Status != VK_SUCCESS) {
		Debug_Log("vmaCreateImage failed");
		return GDI_Null_Handle();
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_IMAGE, "texture", Image, TextureInfo->DebugName);

	gdi_handle Result = VK_Texture_Pool_Allocate(&VkGDI->ResourcePool);	
	vk_texture* Texture = VK_Texture_Pool_Get(&VkGDI->ResourcePool, Result);

	Texture->Image = Image;
	Texture->Allocation = Allocation;
	Texture->Format = TextureInfo->Format;
	Texture->MipCount = TextureInfo->MipCount;
	Texture->Dim = TextureInfo->Dim;
	Texture->Usage = TextureInfo->Usage;
	
	OS_RW_Mutex_Read_Lock(VkGDI->TransferLock);
	vk_transfer_context* Transfer = VkGDI->CurrentTransfer;
	vk_transfer_thread_context* ThreadContext = VK_Get_Transfer_Thread_Context(VkGDI, Transfer);

	if (TextureInfo->InitialData) {
		size_t TotalSize = 0;
		for (size_t i = 0; i < TextureInfo->MipCount; i++) {
			TotalSize += TextureInfo->InitialData[i].Size;
		}

		vk_cpu_buffer_push Upload = VK_CPU_Buffer_Push(&ThreadContext->UploadBuffer, TotalSize);

		VkImageAspectFlags ImageAspect = GDI_Is_Depth_Format(Texture->Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

		arena* Scratch = Scratch_Get();
		VkBufferImageCopy* Regions = Arena_Push_Array(Scratch, Texture->MipCount, VkBufferImageCopy);
		v2i TextureDim = Texture->Dim;

		VkDeviceSize Offset = 0;
		for (u32 i = 0; i < Texture->MipCount; i++) {
			VkBufferImageCopy BufferImageCopy = {
				.bufferOffset = Upload.Offset + Offset,
				.imageSubresource = { ImageAspect, i, 0, 1 },
				.imageExtent = { (u32)TextureDim.x, (u32)TextureDim.y, 1 }
			};
			Regions[i] = BufferImageCopy;
			TextureDim = V2i_Div_S32(TextureDim, 2);

			Memory_Copy(Upload.Ptr + Offset, TextureInfo->InitialData[i].Ptr, TextureInfo->InitialData[i].Size);

			Offset += TextureInfo->InitialData[i].Size;
		}

		vkCmdCopyBufferToImage(ThreadContext->CmdBuffer, Upload.Buffer, Texture->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Texture->MipCount, Regions);
		Scratch_Release();
	}

	if (!Texture->QueuedBarrier) {
		vk_texture_barrier_cmd* Cmd = Arena_Push_Struct(ThreadContext->TempArena, vk_texture_barrier_cmd);
		Cmd->Texture    = Texture;
		Cmd->IsTransfer = TextureInfo->InitialData ? true : false;

		SLL_Push_Back(ThreadContext->FirstTextureBarrierCmd, ThreadContext->LastTextureBarrierCmd, Cmd);
		Texture->QueuedBarrier = true;
	}
	OS_RW_Mutex_Read_Unlock(VkGDI->TransferLock);

	return Result;
}

function GDI_BACKEND_DELETE_TEXTURE_DEFINE(VK_Delete_Texture) {
	VK_Texture_Add_To_Delete_Queue((vk_gdi*)GDI, Texture);
}

function GDI_BACKEND_CREATE_TEXTURE_VIEW_DEFINE(VK_Create_Texture_View) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;

	vk_texture* Texture = VK_Texture_Pool_Get(&VkGDI->ResourcePool, TextureViewInfo->Texture);
	
	gdi_format Format = Texture->Format;
	if (TextureViewInfo->Format != GDI_FORMAT_NONE) {
		Format = TextureViewInfo->Format;
	}

	VkImageAspectFlags ImageAspect = GDI_Is_Depth_Format(Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	u32 MipCount = TextureViewInfo->MipCount;
	if (MipCount == 0) {
		MipCount = Texture->MipCount-TextureViewInfo->MipOffset;
	}

	VkImageViewCreateInfo ViewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = Texture->Image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_Get_Format(Format),
		.subresourceRange = { ImageAspect, TextureViewInfo->MipOffset, MipCount, 0, 1 }
	};

	VkImageView ImageView;
	VkResult Status = vkCreateImageView(VkGDI->Device, &ViewInfo, VK_NULL_HANDLE, &ImageView);
	if (Status != VK_SUCCESS) {
		Debug_Log("vkCreateImageView failed!");
		return GDI_Null_Handle();
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_IMAGE_VIEW, "texture view", ImageView, TextureViewInfo->DebugName);

	gdi_handle Result = VK_Texture_View_Pool_Allocate(&VkGDI->ResourcePool);
	vk_texture_view* View = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, Result);

	View->ImageView = ImageView;
	View->Texture = TextureViewInfo->Texture;
	View->Format = Format;
	View->Dim = Texture->Dim;
	
	return Result;
}

function GDI_BACKEND_DELETE_TEXTURE_VIEW_DEFINE(VK_Delete_Texture_View) {
	VK_Texture_View_Add_To_Delete_Queue((vk_gdi*)GDI, TextureView);
}

function GDI_BACKEND_MAP_BUFFER_DEFINE(VK_Map_Buffer) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;
	vk_frame_context* Frame = VkGDI->CurrentFrame;
	vk_buffer* VkBuffer = VK_Buffer_Pool_Get(&VkGDI->ResourcePool, Buffer);
	if (VkBuffer) {
		OS_RW_Mutex_Read_Lock(VkGDI->TransferLock);
		vk_transfer_context* Transfer = VkGDI->CurrentTransfer;
		vk_transfer_thread_context* ThreadContext = VK_Get_Transfer_Thread_Context(VkGDI, Transfer);
		vk_cpu_buffer_push Upload = VK_CPU_Buffer_Push(&ThreadContext->UploadBuffer, VkBuffer->Size);
		VkBuffer->MappedUpload = Upload;
		return Upload.Ptr;
	}

	return NULL;
}

function GDI_BACKEND_UNMAP_BUFFER_DEFINE(VK_Unmap_Buffer) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;
	vk_frame_context* Frame = VkGDI->CurrentFrame;
	vk_buffer* VkBuffer = VK_Buffer_Pool_Get(&VkGDI->ResourcePool, Buffer);
	if (VkBuffer && VkBuffer->MappedUpload.Ptr) {
		vk_transfer_context* Transfer = VkGDI->CurrentTransfer;
		vk_transfer_thread_context* ThreadContext = VK_Get_Transfer_Thread_Context(VkGDI, Transfer);

		vk_cpu_buffer_push Upload = VkBuffer->MappedUpload;
		VkBufferCopy Region = {
			.srcOffset = Upload.Offset,
			.size = VkBuffer->Size
		};

		vkCmdCopyBuffer(ThreadContext->CmdBuffer, Upload.Buffer, VkBuffer->Buffer, 1, &Region);
		Memory_Clear(&VkBuffer->MappedUpload, sizeof(vk_cpu_buffer_push));
		OS_RW_Mutex_Read_Unlock(VkGDI->TransferLock);
	}
}
 
function GDI_BACKEND_CREATE_BUFFER_DEFINE(VK_Create_Buffer) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;

	VkBufferUsageFlags BufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if (BufferInfo->Usage & GDI_BUFFER_USAGE_VTX_BUFFER) {
		BufferUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}

	if (BufferInfo->Usage & GDI_BUFFER_USAGE_IDX_BUFFER) {
		BufferUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}

	if (BufferInfo->Usage & GDI_BUFFER_USAGE_CONSTANT_BUFFER) {
		BufferUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}

	if (BufferInfo->Usage & GDI_BUFFER_USAGE_READBACK) {
		BufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	if (BufferInfo->Usage & GDI_BUFFER_USAGE_STORAGE_BUFFER) {
		BufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}

	VkBufferCreateInfo BufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = BufferInfo->Size,
		.usage = BufferUsage
	};

	VmaAllocationCreateInfo AllocateInfo = {
		.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	};

	VkBuffer Handle;
	VmaAllocation Allocation;
	VkResult Status = vmaCreateBuffer(VkGDI->GPUAllocator, &BufferCreateInfo, &AllocateInfo, &Handle, &Allocation, NULL);
	if (Status != VK_SUCCESS) {
		Debug_Log("vmaCreateBuffer failed");
		return GDI_Null_Handle();
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_BUFFER, "buffer", Handle, BufferInfo->DebugName);

	gdi_handle Result = VK_Buffer_Pool_Allocate(&VkGDI->ResourcePool);
	vk_buffer* Buffer = VK_Buffer_Pool_Get(&VkGDI->ResourcePool, Result);
	Buffer->Buffer = Handle;
	Buffer->Allocation = Allocation;
	Buffer->Size = BufferInfo->Size;

	return Result;
}

function GDI_BACKEND_DELETE_BUFFER_DEFINE(VK_Delete_Buffer) {
	VK_Buffer_Add_To_Delete_Queue((vk_gdi*)GDI, Buffer);
}
 
function GDI_BACKEND_CREATE_SAMPLER_DEFINE(VK_Create_Sampler) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;

	VkFilter Filter = VK_Get_Filter(SamplerInfo->Filter);
	VkSamplerMipmapMode MipmapMode = VK_Get_Mipmap_Mode(SamplerInfo->Filter);

	VkSamplerCreateInfo CreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = Filter,
		.minFilter = Filter,
		.mipmapMode = MipmapMode,
		.addressModeU = VK_Get_Address_Mode(SamplerInfo->AddressModeU),
		.addressModeV = VK_Get_Address_Mode(SamplerInfo->AddressModeV)
	};

	VkSampler Handle;
	VkResult Status = vkCreateSampler(VkGDI->Device, &CreateInfo, VK_NULL_HANDLE, &Handle);
	if (Status != VK_SUCCESS) {
		Debug_Log("vkCreateSampler failed!");
		return GDI_Null_Handle();
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_SAMPLER, "sampler", Handle, SamplerInfo->DebugName);

	gdi_handle Result = VK_Sampler_Pool_Allocate(&VkGDI->ResourcePool);
	vk_sampler* Sampler = VK_Sampler_Pool_Get(&VkGDI->ResourcePool, Result);
	Sampler->Sampler = Handle;
	return Result;
}

function GDI_BACKEND_DELETE_SAMPLER_DEFINE(VK_Delete_Sampler) {
	VK_Sampler_Add_To_Delete_Queue((vk_gdi*)GDI, Sampler);
}
 
function GDI_BACKEND_CREATE_BIND_GROUP_LAYOUT_DEFINE(VK_Create_Bind_Group_Layout) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;

	arena* Scratch = Scratch_Get();

	VkDescriptorSetLayoutBinding* Bindings = Arena_Push_Array(Scratch, BindGroupLayoutInfo->Bindings.Count, VkDescriptorSetLayoutBinding);
	VkDescriptorBindingFlagsEXT* BindingFlags = Arena_Push_Array(Scratch, BindGroupLayoutInfo->Bindings.Count, VkDescriptorBindingFlagsEXT);
	for (size_t i = 0; i < BindGroupLayoutInfo->Bindings.Count; i++) {

		VkDescriptorSetLayoutBinding Binding = {
			.binding = (u32)i,
			.descriptorType = VK_Get_Descriptor_Type(BindGroupLayoutInfo->Bindings.Ptr[i].Type),
			.descriptorCount = BindGroupLayoutInfo->Bindings.Ptr[i].Count,
			.stageFlags = VK_SHADER_STAGE_ALL
		};

		Bindings[i] = Binding;
		BindingFlags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT BindingFlagsInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
		.bindingCount = (u32)BindGroupLayoutInfo->Bindings.Count,
		.pBindingFlags = BindingFlags
	};

	VkDescriptorSetLayoutCreateInfo CreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &BindingFlagsInfo,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount = (u32)BindGroupLayoutInfo->Bindings.Count,
		.pBindings = Bindings
	};

	VkDescriptorSetLayout SetLayout;
	VkResult Status = vkCreateDescriptorSetLayout(VkGDI->Device, &CreateInfo, VK_NULL_HANDLE, &SetLayout);
	Scratch_Release();
	
	if (Status != VK_SUCCESS) {
		Debug_Log("vkCreateDescriptorSetLayout failed!");
		return GDI_Null_Handle();
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "bind group layout", SetLayout, BindGroupLayoutInfo->DebugName);

	gdi_handle Result = VK_Bind_Group_Layout_Pool_Allocate(&VkGDI->ResourcePool);
	vk_bind_group_layout* Layout = VK_Bind_Group_Layout_Pool_Get(&VkGDI->ResourcePool, Result);
	Layout->Layout = SetLayout;
	Layout->Bindings = GDI_Bind_Group_Binding_Array_Copy(Default_Allocator_Get(), BindGroupLayoutInfo->Bindings.Ptr, BindGroupLayoutInfo->Bindings.Count);

	return Result;
}

function GDI_BACKEND_DELETE_BIND_GROUP_LAYOUT_DEFINE(VK_Delete_Bind_Group_Layout) {
	VK_Bind_Group_Layout_Add_To_Delete_Queue((vk_gdi*)GDI, BindGroupLayout);
}
 
function GDI_BACKEND_CREATE_BIND_GROUP_DEFINE(VK_Create_Bind_Group) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;

	vk_bind_group_layout* Layout = VK_Bind_Group_Layout_Pool_Get(&VkGDI->ResourcePool, BindGroupInfo->Layout);
	Assert(Layout);

	VkDescriptorSetAllocateInfo AllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = VkGDI->DescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &Layout->Layout,
	};

	VkDescriptorSet Set;

	OS_Mutex_Lock(VkGDI->DescriptorLock);
	VkResult Status = vkAllocateDescriptorSets(VkGDI->Device, &AllocateInfo, &Set);
	OS_Mutex_Unlock(VkGDI->DescriptorLock);

	if (Status != VK_SUCCESS) {
		Debug_Log("vkAllocateDescriptorSets failed!");
		return GDI_Null_Handle();
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_DESCRIPTOR_SET, "bind group", Set, BindGroupInfo->DebugName);

	gdi_handle Result = VK_Bind_Group_Pool_Allocate(&VkGDI->ResourcePool);
	vk_bind_group* BindGroup = VK_Bind_Group_Pool_Get(&VkGDI->ResourcePool, Result);

	BindGroup->Set = Set;
	BindGroup->Layout = BindGroupInfo->Layout;

	if (BindGroupInfo->Buffers.Count || BindGroupInfo->TextureViews.Count || BindGroupInfo->Samplers.Count) {

		size_t SamplerIndex = 0;
		size_t TextureIndex = 0;
		size_t BufferIndex = 0;

		arena* Scratch = Scratch_Get();

		dynamic_vk_write_descriptor_set_array DescriptorWrites = Dynamic_VK_Write_Descriptor_Set_Array_Init((allocator*)Scratch);
		for (size_t i = 0; i < Layout->Bindings.Count; i++) {
			gdi_bind_group_binding* Binding = Layout->Bindings.Ptr + i;

			VkWriteDescriptorSet WriteDescriptorSet = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = Set,
				.dstBinding = (u32)i,
				.dstArrayElement = 0,
				.descriptorCount = Binding->Count,
				.descriptorType = VK_Get_Descriptor_Type(Binding->Type)
			};

			switch (Binding->Type) {
				case GDI_BIND_GROUP_TYPE_SAMPLER: {
					VkDescriptorImageInfo* ImageInfo = Arena_Push_Array(Scratch, Binding->Count, VkDescriptorImageInfo);
					for (size_t i = 0; i < Binding->Count; i++) {
						Assert(SamplerIndex < BindGroupInfo->Samplers.Count);
						gdi_handle Handle = BindGroupInfo->Samplers.Ptr[SamplerIndex++];
						vk_sampler* Sampler = VK_Sampler_Pool_Get(&VkGDI->ResourcePool, Handle);
						Assert(Sampler);
						ImageInfo[i].sampler = Sampler->Sampler;
					}

					WriteDescriptorSet.pImageInfo = ImageInfo;
				} break;

				case GDI_BIND_GROUP_TYPE_TEXTURE: {
					VkDescriptorImageInfo* ImageInfo = Arena_Push_Array(Scratch, Binding->Count, VkDescriptorImageInfo);

					for (size_t i = 0; i < Binding->Count; i++) {
						Assert(TextureIndex < BindGroupInfo->TextureViews.Count);
						gdi_handle Handle = BindGroupInfo->TextureViews.Ptr[TextureIndex++];
						vk_texture_view* TextureView = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, Handle);

						ImageInfo[i].imageView = TextureView ? TextureView->ImageView : VK_NULL_HANDLE;
						ImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					}

					WriteDescriptorSet.pImageInfo = ImageInfo;
				} break;

				case GDI_BIND_GROUP_TYPE_CONSTANT_BUFFER: {
					VkDescriptorBufferInfo* BufferInfo = Arena_Push_Array(Scratch, Binding->Count, VkDescriptorBufferInfo);

					for (size_t i = 0; i < Binding->Count; i++) {
						Assert(BufferIndex < BindGroupInfo->Buffers.Count);
						gdi_bind_group_buffer BindGroupBuffer = BindGroupInfo->Buffers.Ptr[BufferIndex++];
						vk_buffer* Buffer = VK_Buffer_Pool_Get(&VkGDI->ResourcePool, BindGroupBuffer.Buffer);
						Assert(Buffer);

						BufferInfo[i].buffer = Buffer->Buffer;
						BufferInfo[i].offset = BindGroupBuffer.Offset;
						BufferInfo[i].range = BindGroupBuffer.Size == 0 ? VK_WHOLE_SIZE : BindGroupBuffer.Size;
					}

					WriteDescriptorSet.pBufferInfo = BufferInfo;
				} break;
			}

			Dynamic_VK_Write_Descriptor_Set_Array_Add(&DescriptorWrites, WriteDescriptorSet);
		}

		vkUpdateDescriptorSets(VkGDI->Device, (u32)DescriptorWrites.Count, DescriptorWrites.Ptr, 0, VK_NULL_HANDLE);
		Scratch_Release();
	}

	return Result;
}

function GDI_BACKEND_DELETE_BIND_GROUP_DEFINE(VK_Delete_Bind_Group) {
	VK_Bind_Group_Add_To_Delete_Queue((vk_gdi*)GDI, BindGroup);
}

function GDI_BACKEND_WRITE_BIND_GROUP_DEFINE(VK_Write_Bind_Group) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;
	vk_bind_group* BindGroup = VK_Bind_Group_Pool_Get(&VkGDI->ResourcePool, BindGroupHandle);
	if (BindGroup) {
		vk_bind_group_layout* Layout = VK_Bind_Group_Layout_Pool_Get(&VkGDI->ResourcePool, BindGroup->Layout);
		Assert(Layout);
		
		arena* Scratch = Scratch_Get();
		dynamic_vk_write_descriptor_set_array DescriptorWrites = Dynamic_VK_Write_Descriptor_Set_Array_Init((allocator*)Scratch);

		for (size_t i = 0; i < BindGroupWriteInfo->Writes.Count; i++) {
			gdi_bind_group_write* Write = BindGroupWriteInfo->Writes.Ptr + i;
			
			Assert(Write->Binding < Layout->Bindings.Count);
			gdi_bind_group_binding* Binding = Layout->Bindings.Ptr + Write->Binding;

			VkWriteDescriptorSet WriteDescriptorSet = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = BindGroup->Set,
				.dstBinding = Write->Binding,
				.dstArrayElement = Write->Index,
				.descriptorType = VK_Get_Descriptor_Type(Binding->Type)
			};

			switch (Binding->Type) {
				case GDI_BIND_GROUP_TYPE_SAMPLER: {
					WriteDescriptorSet.descriptorCount = (u32)Write->Samplers.Count;
					VkDescriptorImageInfo* ImageInfo = Arena_Push_Array(Scratch, Write->Samplers.Count, VkDescriptorImageInfo);
					for (size_t i = 0; i < Write->Samplers.Count; i++) {
						gdi_handle Handle = Write->Samplers.Ptr[i];
						vk_sampler* Sampler = VK_Sampler_Pool_Get(&VkGDI->ResourcePool, Handle);
						Assert(Sampler);
						ImageInfo[i].sampler = Sampler->Sampler;
					}
					WriteDescriptorSet.pImageInfo = ImageInfo;
				} break;

				case GDI_BIND_GROUP_TYPE_TEXTURE: {
					WriteDescriptorSet.descriptorCount = (u32)Write->TextureViews.Count;
					VkDescriptorImageInfo* ImageInfo = Arena_Push_Array(Scratch, Write->TextureViews.Count, VkDescriptorImageInfo);

					for (size_t i = 0; i < Write->TextureViews.Count; i++) {
						gdi_handle Handle = Write->TextureViews.Ptr[i];
						vk_texture_view* TextureView = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, Handle);

						ImageInfo[i].imageView = TextureView ? TextureView->ImageView : VK_NULL_HANDLE;
						ImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					}

					WriteDescriptorSet.pImageInfo = ImageInfo;
				} break;

				case GDI_BIND_GROUP_TYPE_CONSTANT_BUFFER: {
					WriteDescriptorSet.descriptorCount = (u32)Write->Buffers.Count;
					VkDescriptorBufferInfo* BufferInfo = Arena_Push_Array(Scratch, Write->Buffers.Count, VkDescriptorBufferInfo);

					for (size_t i = 0; i < Write->Buffers.Count; i++) {
						gdi_bind_group_buffer BindGroupBuffer = Write->Buffers.Ptr[i];
						vk_buffer* Buffer = VK_Buffer_Pool_Get(&VkGDI->ResourcePool, BindGroupBuffer.Buffer);
						Assert(Buffer);

						BufferInfo[i].buffer = Buffer->Buffer;
						BufferInfo[i].offset = BindGroupBuffer.Offset;
						BufferInfo[i].range  = BindGroupBuffer.Size == 0 ? VK_WHOLE_SIZE : BindGroupBuffer.Size;
					}

					WriteDescriptorSet.pBufferInfo = BufferInfo;
				} break;

				Invalid_Default_Case;
			}

			Assert(WriteDescriptorSet.dstArrayElement + WriteDescriptorSet.descriptorCount <= Binding->Count);

			Dynamic_VK_Write_Descriptor_Set_Array_Add(&DescriptorWrites, WriteDescriptorSet);
		}

		vkUpdateDescriptorSets(VkGDI->Device, (u32)DescriptorWrites.Count, DescriptorWrites.Ptr, 0, VK_NULL_HANDLE);
		Scratch_Release();
	}
}
 
function GDI_BACKEND_CREATE_SHADER_DEFINE(VK_Create_Shader) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;

	arena* Scratch = Scratch_Get();

	VkDescriptorSetLayout WritableLayout = VK_NULL_HANDLE;
	u32 LayoutCount = 0;
	if (!Buffer_Is_Empty(ShaderInfo->CS)) {
		//Need to save one bind group for the writable bind group
		Assert(ShaderInfo->BindGroupLayouts.Count <= (GDI_MAX_BIND_GROUP_COUNT - 1)); 
		Assert(ShaderInfo->WritableBindings.Count > 0);

		arena* Scratch = Scratch_Get();
		VkDescriptorSetLayoutBinding* Bindings = Arena_Push_Array(Scratch, ShaderInfo->WritableBindings.Count, VkDescriptorSetLayoutBinding);
		for (size_t i = 0; i < ShaderInfo->WritableBindings.Count; i++) {
			Assert(GDI_Is_Bind_Group_Type_Writable(ShaderInfo->WritableBindings.Ptr[i].Type));

			VkDescriptorSetLayoutBinding Binding = {
				.binding = (u32)i,
				.descriptorType = VK_Get_Descriptor_Type(ShaderInfo->WritableBindings.Ptr[i].Type),
				.descriptorCount = ShaderInfo->WritableBindings.Ptr[i].Count,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
			};

			Bindings[i] = Binding;
		}

		VkDescriptorSetLayoutCreateInfo CreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
			.bindingCount = (u32)ShaderInfo->WritableBindings.Count,
			.pBindings = Bindings
		};

		VkDescriptorSetLayout SetLayout;
		VkResult Status = vkCreateDescriptorSetLayout(VkGDI->Device, &CreateInfo, VK_NULL_HANDLE, &SetLayout);
		Scratch_Release();

		if (Status != VK_SUCCESS) {
			Debug_Log("vkCreateDescriptorSetLayout failed!");
			return GDI_Null_Handle();
		}

		WritableLayout = SetLayout;

		LayoutCount++;
	}

	LayoutCount += (u32)ShaderInfo->BindGroupLayouts.Count;
	Assert(LayoutCount <= GDI_MAX_BIND_GROUP_COUNT);

	VkDescriptorSetLayout* SetLayouts = Arena_Push_Array(Scratch, LayoutCount, VkDescriptorSetLayout);
	
	size_t LayoutOffset = 0;
	if (!Buffer_Is_Empty(ShaderInfo->CS)) {
		SetLayouts[0] = WritableLayout;
		LayoutOffset = 1;
	}

	VkPushConstantRange PushRange = {
		.stageFlags = VK_SHADER_STAGE_ALL,
		.size = (u32)(ShaderInfo->PushConstantCount * sizeof(u32))
	};

	for (size_t i = 0; i < ShaderInfo->BindGroupLayouts.Count; i++) {
		vk_bind_group_layout* Layout = VK_Bind_Group_Layout_Pool_Get(&VkGDI->ResourcePool, ShaderInfo->BindGroupLayouts.Ptr[i]);
		Assert(Layout);
		SetLayouts[i+LayoutOffset] = Layout->Layout;
	}

	VkPipelineLayoutCreateInfo LayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = LayoutCount,
		.pSetLayouts = SetLayouts,
		.pushConstantRangeCount = ShaderInfo->PushConstantCount ? 1u : 0u,
		.pPushConstantRanges = ShaderInfo->PushConstantCount ? &PushRange : VK_NULL_HANDLE,
	};

	gdi_handle Result = GDI_Null_Handle();

	VkPipelineLayout PipelineLayout;
	if (vkCreatePipelineLayout(VkGDI->Device, &LayoutCreateInfo, VK_NULL_HANDLE, &PipelineLayout) == VK_SUCCESS) {
		if (Buffer_Is_Empty(ShaderInfo->CS)) {
			Assert(!Buffer_Is_Empty(ShaderInfo->VS) && !Buffer_Is_Empty(ShaderInfo->PS));

			VkShaderModuleCreateInfo VSModuleInfo = {
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = ShaderInfo->VS.Size,
				.pCode = (const u32*)ShaderInfo->VS.Ptr
			};

			VkShaderModuleCreateInfo PSModuleInfo = {
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = ShaderInfo->PS.Size,
				.pCode = (const u32*)ShaderInfo->PS.Ptr
			};

			VkShaderModule VSModule, PSModule;
			vkCreateShaderModule(VkGDI->Device, &VSModuleInfo, VK_NULL_HANDLE, &VSModule);
			vkCreateShaderModule(VkGDI->Device, &PSModuleInfo, VK_NULL_HANDLE, &PSModule);

			u32 RenderTargetFormatCount = 0;
			VkFormat* RenderTargetFormats = Arena_Push_Array(Scratch, GDI_MAX_RENDER_TARGET_COUNT, VkFormat);
			for (u32 i = 0; i < GDI_MAX_RENDER_TARGET_COUNT; i++) {
				if (ShaderInfo->RenderTargetFormats[i] != GDI_FORMAT_NONE) {
					RenderTargetFormats[RenderTargetFormatCount] = VK_Get_Format(ShaderInfo->RenderTargetFormats[i]);
					RenderTargetFormatCount++;
				}
			}

			VkPipelineRenderingCreateInfoKHR RenderingInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
				.colorAttachmentCount = RenderTargetFormatCount,
				.pColorAttachmentFormats = RenderTargetFormats,
				.depthAttachmentFormat = VK_Get_Format(ShaderInfo->DepthState.DepthFormat)
			};

			VkPipelineShaderStageCreateInfo ShaderStages[] = {
				{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_VERTEX_BIT,
					.module = VSModule,
					.pName = "VS_Main"
				},
				{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.module = PSModule,
					.pName = "PS_Main"
				}
			};

			u32 BindingCount = 0;
			u32 AttributeCount = 0;
			for (size_t i = 0; i < ShaderInfo->VtxBindings.Count; i++) {
				gdi_vtx_binding* VtxBinding = ShaderInfo->VtxBindings.Ptr + i;
				for (size_t j = 0; j < VtxBinding->Attributes.Count; j++) {
					AttributeCount++;
				}
				BindingCount++;
			}

			VkVertexInputBindingDescription* VtxInputBindings = Arena_Push_Array(Scratch, BindingCount, VkVertexInputBindingDescription);
			VkVertexInputAttributeDescription* VtxInputAttributes = Arena_Push_Array(Scratch, AttributeCount, VkVertexInputAttributeDescription);

			u32 Location = 0;
			BindingCount = 0;
			AttributeCount = 0;
			for (size_t i = 0; i < ShaderInfo->VtxBindings.Count; i++) {
				gdi_vtx_binding* VtxBinding = ShaderInfo->VtxBindings.Ptr + i;
			
				u32 Offset = 0;
				for (size_t j = 0; j < VtxBinding->Attributes.Count; j++) {
					VkVertexInputAttributeDescription Attribute = {
						.location = Location,
						.binding = (u32)i,
						.format = VK_Get_Format(VtxBinding->Attributes.Ptr[j].Format),
						.offset = Offset
					};

					VtxInputAttributes[AttributeCount] = Attribute;
					Offset += (u32)GDI_Get_Format_Size(VtxBinding->Attributes.Ptr[j].Format);
					AttributeCount++;
					Location++;
				}

				VkVertexInputBindingDescription Binding = {
					.binding = (u32)i,
					.stride = (u32)VtxBinding->Stride,
					.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
				};

				VtxInputBindings[BindingCount] = Binding;

				BindingCount++;
			}

			VkPipelineVertexInputStateCreateInfo VertexInputState = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				.vertexBindingDescriptionCount = BindingCount,
				.pVertexBindingDescriptions = VtxInputBindings,
				.vertexAttributeDescriptionCount = AttributeCount,
				.pVertexAttributeDescriptions = VtxInputAttributes
			};

			VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = VK_Get_Primitive(ShaderInfo->Primitive)
			};

			VkPipelineViewportStateCreateInfo ViewportState = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
				.viewportCount = 1,
				.scissorCount = 1
			};

			VkPipelineRasterizationStateCreateInfo RasterizationState = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_BACK_BIT,
				.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
				.lineWidth = 1.0f
			};

			VkPipelineMultisampleStateCreateInfo MultisampleState = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
			};

			VkPipelineDepthStencilStateCreateInfo DepthStencilState = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable = (VkBool32)ShaderInfo->DepthState.TestEnabled,
				.depthWriteEnable = (VkBool32)ShaderInfo->DepthState.WriteEnabled,
				.depthCompareOp = VK_Get_Compare_Func(ShaderInfo->DepthState.CompareFunc)
			};

			VkPipelineColorBlendAttachmentState ColorBlendAttachments[GDI_MAX_RENDER_TARGET_COUNT] = {};
			
			Assert(ShaderInfo->BlendStates.Count <= GDI_MAX_RENDER_TARGET_COUNT);
			for (u32 i = 0; i < RenderTargetFormatCount; i++) {
				const gdi_blend_state* BlendState = NULL;
				if (i < ShaderInfo->BlendStates.Count) {
					BlendState = ShaderInfo->BlendStates.Ptr + i;
				}

				VkPipelineColorBlendAttachmentState ColorBlendAttachment = {
					.colorWriteMask = VK_COLOR_COMPONENT_ALL
				};

				if (BlendState) {
					ColorBlendAttachment.blendEnable = true;
					ColorBlendAttachment.srcColorBlendFactor = VK_Get_Blend(BlendState->SrcFactor);
					ColorBlendAttachment.dstColorBlendFactor = VK_Get_Blend(BlendState->DstFactor);
					ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
					ColorBlendAttachment.srcAlphaBlendFactor = VK_Get_Blend(BlendState->SrcFactor);
					ColorBlendAttachment.dstAlphaBlendFactor = VK_Get_Blend(BlendState->DstFactor);
					ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
				}

				ColorBlendAttachments[i] = ColorBlendAttachment;
			}

			VkDynamicState DynamicStates[] = {
				VK_DYNAMIC_STATE_SCISSOR,
				VK_DYNAMIC_STATE_VIEWPORT
			};

			VkPipelineColorBlendStateCreateInfo ColorBlendState = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				.attachmentCount = RenderTargetFormatCount,
				.pAttachments = ColorBlendAttachments
			};

			VkPipelineDynamicStateCreateInfo DynamicState = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = Array_Count(DynamicStates),
				.pDynamicStates = DynamicStates
			};

			VkGraphicsPipelineCreateInfo PipelineCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
				.pNext = &RenderingInfo,
				.stageCount = Array_Count(ShaderStages),
				.pStages = ShaderStages,
				.pVertexInputState = &VertexInputState,
				.pInputAssemblyState = &InputAssemblyState,
				.pViewportState = &ViewportState,
				.pRasterizationState = &RasterizationState,
				.pMultisampleState = &MultisampleState,
				.pDepthStencilState = &DepthStencilState,
				.pColorBlendState = &ColorBlendState,
				.pDynamicState = &DynamicState,
				.layout = PipelineLayout
			};

			VkPipeline Pipeline;
			if (vkCreateGraphicsPipelines(VkGDI->Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo, VK_NULL_HANDLE, &Pipeline) == VK_SUCCESS) {
				Result = VK_Shader_Pool_Allocate(&VkGDI->ResourcePool);
				vk_shader* Shader = VK_Shader_Pool_Get(&VkGDI->ResourcePool, Result);
				Shader->BindGroupCount = (u32)ShaderInfo->BindGroupLayouts.Count;
				Shader->Layout = PipelineLayout;
				Shader->Pipeline = Pipeline;

#ifdef DEBUG_BUILD
				Shader->DEBUGType = GDI_PASS_TYPE_RENDER;
#endif
			} else {
				Debug_Log("vkCreateGraphicsPipelines failed!");
				vkDestroyPipelineLayout(VkGDI->Device, PipelineLayout, VK_NULL_HANDLE);
			}
			
			vkDestroyShaderModule(VkGDI->Device, VSModule, VK_NULL_HANDLE);
			vkDestroyShaderModule(VkGDI->Device, PSModule, VK_NULL_HANDLE);
		} else {
			Assert(Buffer_Is_Empty(ShaderInfo->VS) && Buffer_Is_Empty(ShaderInfo->PS));
			Assert(ShaderInfo->BindGroupLayouts.Count > 0); //Must have at least one writable bind group

			VkShaderModuleCreateInfo CSModuleInfo = {
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = ShaderInfo->CS.Size,
				.pCode = (const u32*)ShaderInfo->CS.Ptr
			};

			VkShaderModule CSModule;
			vkCreateShaderModule(VkGDI->Device, &CSModuleInfo, VK_NULL_HANDLE, &CSModule);

			VkComputePipelineCreateInfo PipelineCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.stage = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_COMPUTE_BIT,
					.module = CSModule,
					.pName = "CS_Main"
				},
				.layout = PipelineLayout
			};

			VkPipeline Pipeline;
			if (vkCreateComputePipelines(VkGDI->Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo, VK_NULL_HANDLE, &Pipeline) == VK_SUCCESS) {
				Result = VK_Shader_Pool_Allocate(&VkGDI->ResourcePool);
				vk_shader* Shader = VK_Shader_Pool_Get(&VkGDI->ResourcePool, Result);
				Shader->Layout = PipelineLayout;
				Shader->Pipeline = Pipeline;
				Shader->WritableBindings = GDI_Bind_Group_Binding_Array_Copy(Default_Allocator_Get(), ShaderInfo->WritableBindings.Ptr, ShaderInfo->WritableBindings.Count);
				Shader->WritableLayout = WritableLayout;

#ifdef DEBUG_BUILD
				Shader->DEBUGType = GDI_PASS_TYPE_COMPUTE;
#endif
			} else {
				Debug_Log("vkCreateComputePipelines failed!");
				vkDestroyPipelineLayout(VkGDI->Device, PipelineLayout, VK_NULL_HANDLE);
				if (WritableLayout) {
					vkDestroyDescriptorSetLayout(VkGDI->Device, WritableLayout, VK_NULL_HANDLE);
				}
			}

			vkDestroyShaderModule(VkGDI->Device, CSModule, VK_NULL_HANDLE);
		}
	} else {
		Debug_Log("vkCreatePipelineLayout failed!");
		
		if (WritableLayout) {
			vkDestroyDescriptorSetLayout(VkGDI->Device, WritableLayout, VK_NULL_HANDLE);
		}
	}

	Scratch_Release();

	vk_shader* Shader = VK_Shader_Pool_Get(&VkGDI->ResourcePool, Result);
	if (Shader) {
		VK_Set_Debug_Name(VK_OBJECT_TYPE_PIPELINE, "shader", Shader->Pipeline, ShaderInfo->DebugName);
	}

	return Result;
}

function GDI_BACKEND_DELETE_SHADER_DEFINE(VK_Delete_Shader) {
	VK_Shader_Add_To_Delete_Queue((vk_gdi*)GDI, Shader);
}

function GDI_BACKEND_CREATE_SWAPCHAIN_DEFINE(VK_Create_Swapchain) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;
	gdi_handle Result = VK_Swapchain_Pool_Allocate(&VkGDI->ResourcePool);
	vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&VkGDI->ResourcePool, Result);
	Swapchain->Handle = Result;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
	Swapchain->Surface = VK_Create_Surface(VkGDI, SwapchainInfo->Window, SwapchainInfo->Instance);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT = (PFN_vkCreateMetalSurfaceEXT)vkGetInstanceProcAddr(VkGDI->Instance, "vkCreateMetalSurfaceEXT");
	VkMetalSurfaceCreateInfoEXT SurfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
		.pLayer = SwapchainInfo->Layer
	};

	VkResult Status = vkCreateMetalSurfaceEXT(VkGDI->Instance, &SurfaceCreateInfo, VK_NULL_HANDLE, &Swapchain->Surface);
	if(Status != VK_SUCCESS) {
		Debug_Log("vkCreateMetalSurfaceEXT failed!");
		goto error;
	}
#else
#error "Not Implemented!"
#endif

	if (!Swapchain->Surface) {
		goto error;
	}

	VkSurfaceCapabilitiesKHR SurfaceCaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkGDI->TargetGPU->PhysicalDevice, Swapchain->Surface, &SurfaceCaps);

	if (SurfaceCaps.minImageCount < 2) {
		Debug_Log("Surface must have two or more images! Found %d", SurfaceCaps.maxImageCount);
		goto error;
	}

	{
		Swapchain->ImageCount = SurfaceCaps.minImageCount;
		size_t AllocationSize = Swapchain->ImageCount*(sizeof(VkImage) + sizeof(gdi_handle) + sizeof(gdi_handle) + sizeof(VkSemaphore));
		Swapchain->Images = (VkImage*)Allocator_Allocate_Memory(Default_Allocator_Get(), AllocationSize);
		Swapchain->Textures = (gdi_handle*)(Swapchain->Images + Swapchain->ImageCount);
		Swapchain->TextureViews = (gdi_handle*)(Swapchain->Textures + Swapchain->ImageCount);
		Swapchain->Locks = (VkSemaphore*)(Swapchain->TextureViews + Swapchain->ImageCount);

		arena* Scratch = Scratch_Get();

		VkFormat TargetFormat = VK_Get_Format(SwapchainInfo->Format);

		u32 SurfaceFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(VkGDI->TargetGPU->PhysicalDevice, Swapchain->Surface, &SurfaceFormatCount, VK_NULL_HANDLE);

		VkSurfaceFormatKHR* SurfaceFormats = Arena_Push_Array(Scratch, SurfaceFormatCount, VkSurfaceFormatKHR);
		vkGetPhysicalDeviceSurfaceFormatsKHR(VkGDI->TargetGPU->PhysicalDevice, Swapchain->Surface, &SurfaceFormatCount, SurfaceFormats);

		VkSurfaceFormatKHR SurfaceFormat;
		Memory_Clear(&SurfaceFormat, sizeof(VkSurfaceFormatKHR));

		if (TargetFormat != VK_FORMAT_UNDEFINED) {
			for (u32 i = 0; i < SurfaceFormatCount; i++) {
				if (SurfaceFormats[i].format == TargetFormat) {
					SurfaceFormat = SurfaceFormats[i];
					break;
				}
			}
		}

		if (SurfaceFormat.format == VK_FORMAT_UNDEFINED) {
			SurfaceFormat = SurfaceFormats[0];
		}

		
		VK_Set_Debug_Name(VK_OBJECT_TYPE_SURFACE_KHR, "surface", Swapchain->Surface, String_Concat((allocator*)Scratch, SwapchainInfo->DebugName, String_Lit(" Surface")));

		Scratch_Release();

		Swapchain->SurfaceFormat = SurfaceFormat;
		Swapchain->Format = VK_Get_GDI_Format(SurfaceFormat.format);
	}

	for (size_t i = 0; i < Swapchain->ImageCount; i++) {
		VkSemaphoreCreateInfo SemaphoreLockInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		VkResult Status = vkCreateSemaphore(VkGDI->Device, &SemaphoreLockInfo, VK_NULL_HANDLE, &Swapchain->Locks[i]);
		if (Status != VK_SUCCESS) {
			Debug_Log("vkCreateSemaphore failed!");
			goto error;
		}
	}

	if (!VK_Recreate_Swapchain(VkGDI, Swapchain)) {
		goto error;
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_SWAPCHAIN_KHR, "swapchain", Swapchain->Surface, SwapchainInfo->DebugName);

	return Result;

error:
	VK_Delete_Swapchain_Resources(VkGDI, Swapchain);
	VK_Swapchain_Pool_Free(&VkGDI->ResourcePool, Result);
	return GDI_Null_Handle();
}

function GDI_BACKEND_DELETE_SWAPCHAIN_DEFINE(VK_Delete_Swapchain) {
	VK_Swapchain_Add_To_Delete_Queue((vk_gdi*)GDI, Swapchain);
}

function GDI_BACKEND_GET_SWAPCHAIN_VIEW_DEFINE(VK_Get_Swapchain_View) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;
	gdi_handle Result = GDI_Null_Handle();
	vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&VkGDI->ResourcePool, SwapchainHandle);
	if (Swapchain) {
		Result = Swapchain->TextureViews[Swapchain->ImageIndex];
	}
	return Result;
}

function GDI_BACKEND_GET_SWAPCHAIN_INFO_DEFINE(VK_Get_Swapchain_Info) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;

	gdi_swapchain_info Result;
	Memory_Clear(&Result, sizeof(gdi_swapchain_info));

	vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&VkGDI->ResourcePool, SwapchainHandle);
	if (Swapchain) {
		Result.Format = Swapchain->Format;
		Result.Dim = Swapchain->Dim;
	}
	return Result;
}

function GDI_BACKEND_RENDER_DEFINE(VK_Render) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;

	//Reset transfers for next frame
	vk_transfer_context* TransferContext = NULL;

	{
		OS_RW_Mutex_Write_Lock(VkGDI->TransferLock);
		TransferContext = VkGDI->CurrentTransfer;
		VkGDI->TransferIndex++;
		VkGDI->CurrentTransfer = VkGDI->Transfers + (VkGDI->TransferIndex % VK_MAX_TRANSFER_COUNT);
		vk_transfer_context* NewTransferContext = VkGDI->CurrentTransfer;
#ifdef DEBUG_BUILD
		//Fence should always be signaled since we waited on the render fence already
		Assert(vkGetFenceStatus(VkGDI->Device, NewTransferContext->DEBUGFence) == VK_SUCCESS);
		vkResetFences(VkGDI->Device, 1, &NewTransferContext->DEBUGFence);
#endif
		for (vk_transfer_thread_context* Thread = (vk_transfer_thread_context*)Atomic_Load_Ptr(&NewTransferContext->TopThread); Thread; Thread = Thread->Next) {
			Thread->NeedsReset = true;
		}
		OS_RW_Mutex_Write_Unlock(VkGDI->TransferLock);
	}

	for (vk_transfer_thread_context* Thread = (vk_transfer_thread_context*)Atomic_Load_Ptr(&TransferContext->TopThread); Thread; Thread = Thread->Next) {
		if (!Thread->NeedsReset) {
			vkEndCommandBuffer(Thread->CmdBuffer);
		}
	}

	vk_frame_context* Frame = VkGDI->CurrentFrame;
   
	//Transfer commands
	{
		VkResult Status = vkResetCommandPool(VkGDI->Device, TransferContext->CmdPool, 0);
		if (Status != VK_SUCCESS) {
			Debug_Log("vkResetCommandPool failed");
			return;
		}

		VkCommandBufferBeginInfo BeginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		Status = vkBeginCommandBuffer(TransferContext->CmdBuffer, &BeginInfo);
		if (Status != VK_SUCCESS) {
			Debug_Log("vkBeginCommandBuffer failed");
			return;
		}

		arena* Scratch = Scratch_Get();

		//Accumulate barriers
		vk_barriers PrePassBarriers = VK_Barriers_Init(Scratch, TransferContext->CmdBuffer);
		vk_barriers PostPassBarriers = VK_Barriers_Init(Scratch, TransferContext->CmdBuffer);
		for (vk_transfer_thread_context* Thread = (vk_transfer_thread_context*)Atomic_Load_Ptr(&TransferContext->TopThread); Thread; Thread = Thread->Next) {
			if (!Thread->NeedsReset) {
				for (vk_texture_barrier_cmd* Cmd = Thread->FirstTextureBarrierCmd; Cmd; Cmd = Cmd->Next) {
					vk_texture* Texture = Cmd->Texture;
					Assert(Texture->QueuedBarrier);
					VkImageAspectFlags ImageAspect = GDI_Is_Depth_Format(Texture->Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

					if (Cmd->IsTransfer) {
						VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_NONE, VK_RESOURCE_STATE_TRANSFER_WRITE);
						VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_TRANSFER_WRITE, VK_RESOURCE_STATE_SHADER_READ);
					} else {
						if (Texture->Usage & GDI_TEXTURE_USAGE_SAMPLED) {
							VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_NONE, VK_RESOURCE_STATE_SHADER_READ);
						} else if (Texture->Usage & GDI_TEXTURE_USAGE_DEPTH) {
							VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_NONE, VK_RESOURCE_STATE_DEPTH);
						}
					}
					Texture->QueuedBarrier = false;
				}
			}
		}

		//Submit barriers and execute transfer cmds
		VK_Submit_Memory_Barriers(&PrePassBarriers, VK_RESOURCE_STATE_NONE, VK_RESOURCE_STATE_TRANSFER_WRITE);
		for (vk_transfer_thread_context* Thread = (vk_transfer_thread_context*)Atomic_Load_Ptr(&TransferContext->TopThread); Thread; Thread = Thread->Next) {
			if (!Thread->NeedsReset) {
				vkCmdExecuteCommands(TransferContext->CmdBuffer, 1, &Thread->CmdBuffer);
			}
		}
		VK_Submit_Memory_Barriers(&PostPassBarriers, VK_RESOURCE_STATE_TRANSFER_WRITE, VK_RESOURCE_STATE_MEMORY_READ);

		Status = vkEndCommandBuffer(TransferContext->CmdBuffer);
		Scratch_Release();

		if (Status != VK_SUCCESS) {
			Debug_Log("vkEndCommandBuffer failed!");
			return;
		}

		VkCommandBuffer CmdBuffers[] = { TransferContext->CmdBuffer };
		VkSemaphore SignalSemaphores[] = { Frame->TransferLock };

		VkSubmitInfo SubmitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = Array_Count(CmdBuffers),
			.pCommandBuffers = CmdBuffers,
			.signalSemaphoreCount = Array_Count(SignalSemaphores),
			.pSignalSemaphores = SignalSemaphores,
		};

		VkFence TransferFence = VK_NULL_HANDLE;
#ifdef DEBUG_BUILD
		TransferFence = TransferContext->DEBUGFence;
#endif
		Status = vkQueueSubmit(VkGDI->TransferQueue, 1, &SubmitInfo, TransferFence);
		if (Status != VK_SUCCESS) {
			Assert(false);
			Debug_Log("vkQueueSubmit failed");
			return;
		}
	}

	VkResult Status = VK_SUCCESS;

	//Render commands
	{
		Status = vkResetCommandPool(VkGDI->Device, Frame->CmdPool, 0);
		if (Status != VK_SUCCESS) {
			Debug_Log("vkResetCommandPool failed");
			return;
		}

		VkCommandBufferBeginInfo BeginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		Status = vkBeginCommandBuffer(Frame->CmdBuffer, &BeginInfo);
		if (Status != VK_SUCCESS) {
			Debug_Log("vkBeginCommandBuffer failed");
			return;
		}

		arena* Scratch = Scratch_Get();
		vk_barriers PrePassBarriers = VK_Barriers_Init(Scratch, Frame->CmdBuffer);
		vk_barriers PostPassBarriers = VK_Barriers_Init(Scratch, Frame->CmdBuffer);
		
		VkPipelineStageFlags* WaitStageFlags = Arena_Push_Array(Scratch, RenderParams->Swapchains.Count + 1, VkPipelineStageFlags);
		VkSemaphore* WaitSemaphores = Arena_Push_Array(Scratch, RenderParams->Swapchains.Count + 1, VkSemaphore);

		for (size_t i = 0; i < RenderParams->Swapchains.Count; i++) {
			vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&VkGDI->ResourcePool, RenderParams->Swapchains.Ptr[i]);
			WaitSemaphores[i] = Swapchain->Locks[Frame->Index];
		}

		for (gdi_pass* Pass = GDI->FirstPass; Pass; Pass = Pass->Next) {
			VK_Barriers_Clear(&PrePassBarriers);
			VK_Barriers_Clear(&PostPassBarriers);

			switch (Pass->Type) {
				case GDI_PASS_TYPE_COMPUTE: {

					gdi_compute_pass* ComputePass = &Pass->ComputePass;


					//Image memory barriers
					for (size_t i = 0; i < ComputePass->TextureWrites.Count; i++) {
						vk_texture_view* TextureView = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, ComputePass->TextureWrites.Ptr[i]);
						Assert(TextureView);
						vk_texture* Texture = VK_Texture_Pool_Get(&VkGDI->ResourcePool, TextureView->Texture);
						Assert(Texture);

						if (!GDI_Is_Null(Texture->Swapchain)) {
							size_t Index = (size_t)-1;
							for (size_t i = 0; i < RenderParams->Swapchains.Count; i++) {
								if (GDI_Is_Equal(RenderParams->Swapchains.Ptr[i], Texture->Swapchain)) {
									Index = i;
									break;
								}
							}
							Assert(Index != (size_t)-1); //Swapchain is not used

							if (WaitStageFlags[i] == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
								WaitStageFlags[i] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
							}

							VK_Add_Pre_Swapchain_Barrier(&PrePassBarriers, Texture, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR, VK_RESOURCE_STATE_SHADER_READ);
							VK_Add_Post_Swapchain_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_SHADER_READ);
						} else {
							VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_SHADER_READ, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE);
							VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE, VK_RESOURCE_STATE_SHADER_READ);
						}
					}

					if (ComputePass->BufferWrites.Count) {
						VK_Submit_Memory_Barriers(&PrePassBarriers, VK_RESOURCE_STATE_MEMORY_READ, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE);
					} else {
						VK_Submit_Barriers(&PrePassBarriers);
					}

					vk_shader* CurrentShader = VK_NULL_HANDLE;
					for (size_t i = 0; i < ComputePass->Dispatches.Count; i++) {
						gdi_dispatch* Dispatch = ComputePass->Dispatches.Ptr + i;
						
						vk_shader* Shader = VK_Shader_Pool_Get(&VkGDI->ResourcePool, Dispatch->Shader);
						Assert(Shader->DEBUGType == GDI_PASS_TYPE_COMPUTE);

						if (Shader != CurrentShader) {
							CurrentShader = Shader;

							size_t SamplerIndex = 0;
							size_t TextureIndex = 0;
							size_t BufferIndex = 0;

							VkWriteDescriptorSet* DescriptorWrites = Arena_Push_Array(Scratch, Shader->WritableBindings.Count, VkWriteDescriptorSet);

							for (size_t i = 0; i < Shader->WritableBindings.Count; i++) {
								gdi_bind_group_binding* Binding = Shader->WritableBindings.Ptr + i;

								VkWriteDescriptorSet WriteDescriptorSet = {
									.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
									.dstBinding = (u32)i,
									.dstArrayElement = 0,
									.descriptorCount = Binding->Count,
									.descriptorType = VK_Get_Descriptor_Type(Binding->Type)
								};

								switch (Binding->Type) {
									case GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE: {
										VkDescriptorImageInfo* ImageInfo = Arena_Push_Array(Scratch, Binding->Count, VkDescriptorImageInfo);

										for (size_t i = 0; i < Binding->Count; i++) {
											Assert(TextureIndex < ComputePass->TextureWrites.Count);
											gdi_handle Handle = ComputePass->TextureWrites.Ptr[TextureIndex++];
											vk_texture_view* TextureView = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, Handle);

											ImageInfo[i].imageView = TextureView ? TextureView->ImageView : VK_NULL_HANDLE;
											ImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
										}

										WriteDescriptorSet.pImageInfo = ImageInfo;
									} break;

									case GDI_BIND_GROUP_TYPE_STORAGE_BUFFER: {
										VkDescriptorBufferInfo* BufferInfo = Arena_Push_Array(Scratch, Binding->Count, VkDescriptorBufferInfo);

										for (size_t i = 0; i < Binding->Count; i++) {
											Assert(BufferIndex < ComputePass->BufferWrites.Count);
											gdi_handle Handle = ComputePass->BufferWrites.Ptr[BufferIndex++];
											vk_buffer* Buffer = VK_Buffer_Pool_Get(&VkGDI->ResourcePool, Handle);
											Assert(Buffer);

											BufferInfo[i].buffer = Buffer->Buffer;
											BufferInfo[i].range = VK_WHOLE_SIZE;
										}

										WriteDescriptorSet.pBufferInfo = BufferInfo;
									} break;

									Invalid_Default_Case;
								}

								DescriptorWrites[i] = WriteDescriptorSet;
							}

							vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Shader->Pipeline);
							//Outputs of compute shaders are always the first descriptor set
							vkCmdPushDescriptorSetKHR(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Shader->Layout, 0, (u32)Shader->WritableBindings.Count, DescriptorWrites); 
						}

						u32 DescriptorSetCount = 0;
						VkDescriptorSet DescriptorSets[GDI_MAX_BIND_GROUP_COUNT - 1];

						for (u32 i = 0; i < GDI_MAX_BIND_GROUP_COUNT - 1; i++) {
							vk_bind_group* BindGroup = VK_Bind_Group_Pool_Get(&VkGDI->ResourcePool, Dispatch->BindGroups[i]);
							if (BindGroup) {
								DescriptorSets[DescriptorSetCount++] = BindGroup->Set;
							}
						}

						if (DescriptorSetCount) {
							vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, CurrentShader->Layout, 1, DescriptorSetCount, DescriptorSets, 0, VK_NULL_HANDLE);
						}

						if (Dispatch->PushConstantCount) {
							vkCmdPushConstants(Frame->CmdBuffer, CurrentShader->Layout, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
											   0, Dispatch->PushConstantCount * sizeof(u32), Dispatch->PushConstants);
						}

						vkCmdDispatch(Frame->CmdBuffer, Dispatch->ThreadGroupCount.x, Dispatch->ThreadGroupCount.y, Dispatch->ThreadGroupCount.z);
					}

					if (ComputePass->BufferWrites.Count) {
						VK_Submit_Memory_Barriers(&PostPassBarriers, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE, VK_RESOURCE_STATE_MEMORY_READ);
					} else {
						VK_Submit_Barriers(&PostPassBarriers);
					}
				} break;

				case GDI_PASS_TYPE_RENDER: {
					VkCommandBuffer PassCmdBuffer = VK_NULL_HANDLE;
					Assert(Pass->Type == GDI_PASS_TYPE_RENDER);

					vk_render_pass* RenderPass = (vk_render_pass*)Pass->RenderPass;
					PassCmdBuffer = RenderPass->CmdBuffer;
		
					u32 ColorAttachmentCount = 0;
					VkRenderingAttachmentInfoKHR ColorAttachments[GDI_MAX_RENDER_TARGET_COUNT];
					Memory_Clear(ColorAttachments, sizeof(ColorAttachments));

					VkRenderingAttachmentInfoKHR DepthAttachment;
					Memory_Clear(&DepthAttachment, sizeof(DepthAttachment));

					for (size_t i = 0; i < GDI_MAX_RENDER_TARGET_COUNT; i++) {
						vk_texture_view* View = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, RenderPass->RenderTargetViews[i]);
						if (View) {
							vk_texture* Texture = VK_Texture_Pool_Get(&VkGDI->ResourcePool, View->Texture);
							Assert(Texture);

							gdi_clear_color* ClearState = RenderPass->ClearColors+i;

							VkRenderingAttachmentInfoKHR AttachmentInfo = {
								.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
								.imageView = View->ImageView,
								.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
								.loadOp	= ClearState->ShouldClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
								.storeOp = VK_ATTACHMENT_STORE_OP_STORE
							};

							if (ClearState->ShouldClear) {
								Memory_Copy(AttachmentInfo.clearValue.color.float32, ClearState->F32, sizeof(ClearState->F32));
							}

							ColorAttachments[ColorAttachmentCount++] = AttachmentInfo;

							if (!GDI_Is_Null(Texture->Swapchain)) {
								size_t Index = (size_t)-1;
								for (size_t i = 0; i < RenderParams->Swapchains.Count; i++) {
									if (GDI_Is_Equal(RenderParams->Swapchains.Ptr[i], Texture->Swapchain)) {
										Index = i;
										break;
									}
								}
								Assert(Index != (size_t)-1); //Swapchain is not used

								if (WaitStageFlags[i] == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
									WaitStageFlags[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
								}
																	
								VK_Add_Pre_Swapchain_Barrier(&PrePassBarriers, Texture, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, VK_RESOURCE_STATE_RENDER_TARGET);
								VK_Add_Post_Swapchain_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_RENDER_TARGET);
							} else {
								VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_SHADER_READ, VK_RESOURCE_STATE_RENDER_TARGET);
								VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_RENDER_TARGET, VK_RESOURCE_STATE_SHADER_READ);
							}
						}
					}

					vk_texture_view* DepthView = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, RenderPass->DepthBufferView);
					if (DepthView) {
						vk_texture* DepthTexture = VK_Texture_Pool_Get(&VkGDI->ResourcePool, DepthView->Texture);
						gdi_clear_depth* ClearState = &RenderPass->ClearDepth;

						if (!(DepthTexture->Usage & GDI_TEXTURE_USAGE_SAMPLED)) {
							VK_Add_Texture_Barrier(&PrePassBarriers, DepthTexture, VK_RESOURCE_STATE_DEPTH, VK_RESOURCE_STATE_DEPTH);
							VK_Add_Texture_Barrier(&PrePassBarriers, DepthTexture, VK_RESOURCE_STATE_DEPTH, VK_RESOURCE_STATE_DEPTH);
						} else {
							VK_Add_Texture_Barrier(&PrePassBarriers, DepthTexture, VK_RESOURCE_STATE_SHADER_READ, VK_RESOURCE_STATE_DEPTH);
							VK_Add_Texture_Barrier(&PrePassBarriers, DepthTexture, VK_RESOURCE_STATE_DEPTH, VK_RESOURCE_STATE_SHADER_READ);
						}

						VkRenderingAttachmentInfoKHR DepthAttachmentInfo = {
							.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
							.imageView = DepthView->ImageView,
							.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
							.loadOp	= ClearState->ShouldClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
							.storeOp = VK_ATTACHMENT_STORE_OP_STORE
						};

						if (ClearState->ShouldClear) {
							DepthAttachmentInfo.clearValue.depthStencil.depth = ClearState->Depth;
						}
						DepthAttachment = DepthAttachmentInfo;
					}

					VkRenderingInfoKHR RenderingInfo = {
						.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
						.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT_KHR,
						.renderArea = { { 0 }, { (u32)RenderPass->Dim.x, (u32)RenderPass->Dim.y } },
						.layerCount = 1,
						.colorAttachmentCount = ColorAttachmentCount,
						.pColorAttachments = ColorAttachments,
						.pDepthAttachment = DepthView ? &DepthAttachment : NULL
					};

					VK_Submit_Barriers(&PrePassBarriers);
					vkCmdBeginRenderingKHR(Frame->CmdBuffer, &RenderingInfo);
					vkCmdExecuteCommands(Frame->CmdBuffer, 1, &PassCmdBuffer);
					vkCmdEndRenderingKHR(Frame->CmdBuffer);
					VK_Submit_Barriers(&PostPassBarriers);
				} break;
			}
		}

		if (RenderParams->TextureReadbacks.Count || RenderParams->BufferReadbacks.Count) {
			Frame->TextureReadbacks = VK_Texture_Readback_Array_Alloc((allocator*)Frame->Arena, RenderParams->TextureReadbacks.Count);
			Frame->BufferReadbacks = VK_Buffer_Readback_Array_Alloc((allocator*)Frame->Arena, RenderParams->BufferReadbacks.Count);

			vk_barriers PrePassBarriers = VK_Barriers_Init(Scratch, Frame->CmdBuffer);
			vk_barriers PostPassBarriers = VK_Barriers_Init(Scratch, Frame->CmdBuffer);
		
			for (size_t i = 0; i < RenderParams->TextureReadbacks.Count; i++) {
				const gdi_texture_readback* Readback = RenderParams->TextureReadbacks.Ptr + i;
				vk_texture* Texture = VK_Texture_Pool_Get(&VkGDI->ResourcePool, Readback->Texture);
				if (Texture->Usage & GDI_TEXTURE_USAGE_SAMPLED) {
					VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_SHADER_READ, VK_RESOURCE_STATE_TRANSFER_READ);
					VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_TRANSFER_READ, VK_RESOURCE_STATE_SHADER_READ);
				} else if (Texture->Usage & GDI_TEXTURE_USAGE_DEPTH) {
					VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_DEPTH, VK_RESOURCE_STATE_TRANSFER_READ);
					VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_TRANSFER_READ, VK_RESOURCE_STATE_DEPTH);
				} Invalid_Else;
			}

			if (RenderParams->BufferReadbacks.Count) {
				VK_Submit_Memory_Barriers(&PrePassBarriers, VK_RESOURCE_STATE_MEMORY_READ, VK_RESOURCE_STATE_TRANSFER_READ);
			} else {
				VK_Submit_Barriers(&PrePassBarriers);
			}

			for (size_t i = 0; i < RenderParams->TextureReadbacks.Count; i++) {
				const gdi_texture_readback* Readback = RenderParams->TextureReadbacks.Ptr + i;
				vk_texture* Texture = VK_Texture_Pool_Get(&VkGDI->ResourcePool, Readback->Texture);
				
				v2i Dim = Texture->Dim;
				size_t AllocationSize = 0;
				for (u32 i = 0; i < Texture->MipCount; i++) {
					size_t TextureSize = Texture->Dim.x * Texture->Dim.y * GDI_Get_Format_Size(Texture->Format);
					AllocationSize += TextureSize;
					Dim = V2i_Div_S32(Dim, 2);
				}

				vk_cpu_buffer_push CpuReadback = VK_CPU_Buffer_Push(&Frame->ReadbackBuffer, AllocationSize);

				Dim = Texture->Dim;
				VkBufferImageCopy* Regions = Arena_Push_Array(Scratch, Texture->MipCount, VkBufferImageCopy);

				VkImageAspectFlags ImageAspect = GDI_Is_Depth_Format(Texture->Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

				VkDeviceSize Offset = 0;
				for (u32 i = 0; i < Texture->MipCount; i++) {
					size_t TextureSize = Texture->Dim.x * Texture->Dim.y * GDI_Get_Format_Size(Texture->Format);

					VkBufferImageCopy BufferImageCopy = {
						.bufferOffset = CpuReadback.Offset + Offset,
						.imageSubresource = { ImageAspect, i, 0, 1 },
						.imageExtent = { (u32)Dim.x, (u32)Dim.y, 1 }
					};
					Regions[i] = BufferImageCopy;
					Dim = V2i_Div_S32(Dim, 2);

					Offset += TextureSize;
				}

				vkCmdCopyImageToBuffer(Frame->CmdBuffer, Texture->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
									   CpuReadback.Buffer, Texture->MipCount, Regions);

				vk_texture_readback VkReadback = {
					.Texture = Readback->Texture,
					.CPU = CpuReadback,
					.Func = Readback->ReadbackFunc,
					.UserData = Readback->UserData,
				};

				Frame->TextureReadbacks.Ptr[i] = VkReadback;
			}

			for (size_t i = 0; i < RenderParams->BufferReadbacks.Count; i++) {
				const gdi_buffer_readback* Readback = RenderParams->BufferReadbacks.Ptr + i;
				vk_buffer* Buffer = VK_Buffer_Pool_Get(&VkGDI->ResourcePool, Readback->Buffer);

				vk_cpu_buffer_push CpuReadback = VK_CPU_Buffer_Push(&Frame->ReadbackBuffer, Buffer->Size);

				VkBufferCopy Region = {
					.srcOffset = 0,
					.dstOffset = CpuReadback.Offset,
					.size = Buffer->Size
				};

				vkCmdCopyBuffer(Frame->CmdBuffer, Buffer->Buffer, CpuReadback.Buffer, 1, &Region);

				vk_buffer_readback VkReadback = {
					.Buffer = Readback->Buffer, 
					.CPU = CpuReadback,
					.Func = Readback->ReadbackFunc,
					.UserData = Readback->UserData
				};

				Frame->BufferReadbacks.Ptr[i] = VkReadback;
			}

			if (RenderParams->BufferReadbacks.Count) {
				VK_Submit_Memory_Barriers(&PostPassBarriers, VK_RESOURCE_STATE_TRANSFER_READ, VK_RESOURCE_STATE_MEMORY_READ);
			} else {
				VK_Submit_Barriers(&PostPassBarriers);
			}
		}

		Status = vkEndCommandBuffer(Frame->CmdBuffer);
		Scratch_Release();

		if (Status != VK_SUCCESS) {
			Debug_Log("vkEndCommandBuffer failed");
			return;
		}

		{
			
			VkCommandBuffer CmdBuffers[] = { Frame->CmdBuffer };
			VkSemaphore SignalSemaphores[] = { Frame->RenderLock };
			
			WaitSemaphores[RenderParams->Swapchains.Count] = Frame->TransferLock;
			WaitStageFlags[RenderParams->Swapchains.Count] = VK_PIPELINE_STAGE_TRANSFER_BIT;
	
			VkSubmitInfo SubmitInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = (u32)(RenderParams->Swapchains.Count+1),
				.pWaitSemaphores = WaitSemaphores,
				.pWaitDstStageMask = WaitStageFlags,
				.commandBufferCount = Array_Count(CmdBuffers),
				.pCommandBuffers = CmdBuffers,
				.signalSemaphoreCount = Array_Count(SignalSemaphores),
				.pSignalSemaphores = SignalSemaphores
			};
			Status = vkQueueSubmit(VkGDI->GraphicsQueue, 1, &SubmitInfo, Frame->Fence);
			if (Status != VK_SUCCESS) {
				Assert(false);
				Debug_Log("vkQueueSubmit failed");
				return;
			}
		}
	}

	//Final swapchain present
	{
		arena* Scratch = Scratch_Get();
		VkSwapchainKHR* Swapchains = Arena_Push_Array(Scratch, RenderParams->Swapchains.Count, VkSwapchainKHR);
		u32* ImageIndices = Arena_Push_Array(Scratch, RenderParams->Swapchains.Count, u32);
		VkResult* Statuses = Arena_Push_Array(Scratch, RenderParams->Swapchains.Count, VkResult);

		for (size_t i = 0; i < RenderParams->Swapchains.Count; i++) {
			vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&VkGDI->ResourcePool, RenderParams->Swapchains.Ptr[i]);
			Swapchains[i] = Swapchain->Swapchain;
			ImageIndices[i] = Swapchain->ImageIndex;
		}

		VkSemaphore WaitSemaphores[] = { Frame->RenderLock };

		VkPresentInfoKHR PresentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = Array_Count(WaitSemaphores),
			.pWaitSemaphores = WaitSemaphores,
			.swapchainCount = Array_Count(Swapchains),
			.pSwapchains = Swapchains,
			.pImageIndices = ImageIndices,
			.pResults = Statuses
		};
		Status = vkQueuePresentKHR(VkGDI->PresentQueue, &PresentInfo);
		Scratch_Release();

		if (Status == VK_ERROR_OUT_OF_DATE_KHR || Status == VK_SUBOPTIMAL_KHR) {
			for (size_t i = 0; i < RenderParams->Swapchains.Count; i++) {
				if (Statuses[i] == VK_ERROR_OUT_OF_DATE_KHR || Statuses[i] == VK_SUBOPTIMAL_KHR) {
					vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&VkGDI->ResourcePool, RenderParams->Swapchains.Ptr[i]);
					if (!VK_Recreate_Swapchain(VkGDI, Swapchain)) return;
				}
			}			
		} else if (Status != VK_SUCCESS) {
			Debug_Log("vkQueuePresentKHR failed");
			return;
		}
	}

	//Free renderpasses that were queued for deleting this frame
	for (vk_transfer_thread_context* Thread = (vk_transfer_thread_context*)Atomic_Load_Ptr(&TransferContext->TopThread); Thread; Thread = Thread->Next) {
		vk_render_pass* RenderPass = Thread->RenderPassesToDelete;
		while (RenderPass) {
			vk_render_pass* RenderPassToDelete = RenderPass;
			RenderPass = RenderPass->Next;

			RenderPassToDelete->Next = NULL;
			SLL_Push_Front(Thread->FreeRenderPasses, RenderPassToDelete);
		}
		Thread->RenderPassesToDelete = NULL;
	}

	VkGDI->FrameIndex++;

	u64 FrameIndex = (VkGDI->FrameIndex % VK_FRAME_COUNT);
	VkGDI->CurrentFrame = VkGDI->Frames + FrameIndex;

	Frame = VkGDI->CurrentFrame;

	//Wait on the render fence now. We always have the render frame count less than
	//the transfer, so waiting on the render is also a wait on the transfers
	VkResult FenceStatus = vkGetFenceStatus(VkGDI->Device, Frame->Fence);
	if (FenceStatus == VK_NOT_READY) {
		vkWaitForFences(VkGDI->Device, 1, &Frame->Fence, VK_TRUE, UINT64_MAX);
	} else {
		Assert(FenceStatus == VK_SUCCESS);
	}
	vkResetFences(VkGDI->Device, 1, &Frame->Fence);

	//Delete the entries in the delete now that we have waited on the frame and then
	//safely set that delete queue as the current delete queue atomically
	vk_delete_queue* DeleteQueue = VkGDI->DeleteQueues + FrameIndex;
	VK_Delete_Queued_Resources(VkGDI, DeleteQueue);

	//Readbacks before we reset the arena
	for (size_t i = 0; i < Frame->TextureReadbacks.Count; i++) {
		vk_texture_readback* Readback = Frame->TextureReadbacks.Ptr + i;
		vk_texture* Texture = VK_Texture_Pool_Get(&VkGDI->ResourcePool, Readback->Texture);
		Readback->Func(Readback->Texture, Texture->Dim, Texture->Format, Texture->MipCount, Readback->CPU.Ptr, Readback->UserData);
	}

	for (size_t i = 0; i < Frame->BufferReadbacks.Count; i++) {
		vk_buffer_readback* Readback = Frame->BufferReadbacks.Ptr + i;
		vk_buffer* Buffer = VK_Buffer_Pool_Get(&VkGDI->ResourcePool, Readback->Buffer);
		Readback->Func(Readback->Buffer, Buffer->Size, Readback->CPU.Ptr, Readback->UserData);
	}

	OS_RW_Mutex_Write_Lock(VkGDI->DeleteLock);
	VkGDI->CurrentDeleteQueue = DeleteQueue;
	OS_RW_Mutex_Write_Unlock(VkGDI->DeleteLock);
	
	VK_CPU_Buffer_Clear(&Frame->ReadbackBuffer);
	Arena_Clear(Frame->Arena);

	//Acquire the swapchain image so we know what view to pass to the user land code

	for (size_t i = 0; i < RenderParams->Swapchains.Count; i++) {
		vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&VkGDI->ResourcePool, RenderParams->Swapchains.Ptr[i]);
		Status = vkAcquireNextImageKHR(VkGDI->Device, Swapchain->Swapchain, UINT64_MAX, 
									   Swapchain->Locks[FrameIndex], VK_NULL_HANDLE, &Swapchain->ImageIndex);
		if (Status == VK_ERROR_OUT_OF_DATE_KHR || Status == VK_SUBOPTIMAL_KHR) {
			if (!VK_Recreate_Swapchain(VkGDI, Swapchain)) return;
		} else if (Status != VK_SUCCESS) {
			Debug_Log("vkAcquireNextImageKHR failed");
			return;
		}
	}
}
 
function GDI_BACKEND_BEGIN_RENDER_PASS_DEFINE(VK_Begin_Render_Pass) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;
	
	b32 HasDim = false;
	v2i RenderPassDim = V2i_Zero();
	for (size_t i = 0; i < GDI_MAX_RENDER_TARGET_COUNT; i++) {
		vk_texture_view* View = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, BeginInfo->RenderTargetViews[i]);
		if (View) {
			if (!HasDim) {
				RenderPassDim = View->Dim;
				HasDim = true;
			} else {
				Assert(RenderPassDim.x == View->Dim.x && RenderPassDim.y == View->Dim.y);
			}
		}
	}

	vk_texture_view* DepthView = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, BeginInfo->DepthBufferView);
	if (DepthView) {
		if (DepthView) {
			if (!HasDim) {
				RenderPassDim = DepthView->Dim;
				HasDim = true;
			} else {
				Assert(RenderPassDim.x == DepthView->Dim.x && RenderPassDim.y == DepthView->Dim.y);
			}
		}
	}

	//Doesn't need to acuqire the transfer lock since BeginRenderPass() and EndRenderPass()
	//cannot run concurrently with Render()
	vk_transfer_context* TransferContext = VkGDI->CurrentTransfer;
	vk_transfer_thread_context* ThreadContext = VK_Get_Transfer_Thread_Context(VkGDI, TransferContext);

	vk_render_pass* RenderPass = ThreadContext->FreeRenderPasses;
	if (RenderPass) SLL_Pop_Front(ThreadContext->FreeRenderPasses);
	else {
		RenderPass = Arena_Push_Struct(ThreadContext->Arena, vk_render_pass);
		RenderPass->Base.Memory = Make_Memory_Reserve(GB(1));

		VkCommandBufferAllocateInfo CmdBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = ThreadContext->CmdPool,
			.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			.commandBufferCount = 1
		};

		vkAllocateCommandBuffers(VkGDI->Device, &CmdBufferInfo, &RenderPass->CmdBuffer);
	}
	
	Memory_Copy(RenderPass->ClearColors, BeginInfo->ClearColors, sizeof(BeginInfo->ClearColors));
	RenderPass->ClearDepth = BeginInfo->ClearDepth;
	RenderPass->Dim = RenderPassDim;
	Memory_Copy(RenderPass->RenderTargetViews, BeginInfo->RenderTargetViews, sizeof(BeginInfo->RenderTargetViews));
	RenderPass->DepthBufferView = BeginInfo->DepthBufferView;

	return (gdi_render_pass*)RenderPass;
}

function GDI_BACKEND_END_RENDER_PASS_DEFINE(VK_End_Render_Pass) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;
	vk_transfer_thread_context* ThreadContext = VK_Get_Transfer_Thread_Context(VkGDI, VkGDI->CurrentTransfer);
	vk_render_pass* VkRenderPass = (vk_render_pass*)RenderPass;

	u32 ColorFormatCount = 0;
	VkFormat ColorFormats[GDI_MAX_RENDER_TARGET_COUNT];
	Memory_Clear(ColorFormats, sizeof(ColorFormats));

	VkFormat DepthFormat = VK_FORMAT_UNDEFINED;

	for (u32 i = 0; i < GDI_MAX_RENDER_TARGET_COUNT; i++) {
		vk_texture_view* View = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, VkRenderPass->RenderTargetViews[i]);
		if (View) {
			ColorFormats[ColorFormatCount++] = VK_Get_Format(View->Format);
		}
	}

	vk_texture_view* DepthView = VK_Texture_View_Pool_Get(&VkGDI->ResourcePool, VkRenderPass->DepthBufferView);
	if (DepthView) {
		DepthFormat = VK_Get_Format(DepthView->Format);
	}

	VkCommandBufferInheritanceRenderingInfoKHR DynamicRenderingInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO_KHR,
		.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT_KHR,
		.colorAttachmentCount = ColorFormatCount,
		.pColorAttachmentFormats = ColorFormats,
		.depthAttachmentFormat = DepthFormat,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	};

	VkCommandBufferInheritanceInfo InheritanceInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext = &DynamicRenderingInfo
	};

	VkCommandBufferBeginInfo BeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
		.pInheritanceInfo = &InheritanceInfo
	};

	vkBeginCommandBuffer(VkRenderPass->CmdBuffer, &BeginInfo);

	VkViewport Viewport = {
		.width = (f32)VkRenderPass->Dim.x,
		.height = (f32)VkRenderPass->Dim.y,
		.maxDepth = 1.0f
	};

	vkCmdSetViewport(VkRenderPass->CmdBuffer, 0, 1, &Viewport);

	vk_shader* CurrentShader = NULL;
	vk_buffer* CurrentIdxBuffer = NULL;
	gdi_idx_format CurrentIdxFormat = GDI_IDX_FORMAT_NONE;
	u32 CurrentIdxCount = 0;
	u32 CurrentIdxOffset = 0;
	u32 CurrentVtxOffset = 0;
	u32 PushConstantCount = 0;
	vk_bind_group* BindGroups[GDI_MAX_BIND_GROUP_COUNT] = {0};

	VkRect2D CurrentScissor = {{}, { (u32)VkRenderPass->Dim.x, (u32)VkRenderPass->Dim.y } };
	vkCmdSetScissor(VkRenderPass->CmdBuffer, 0, 1, &CurrentScissor);

	bstream_reader Reader = BStream_Reader_Begin(Make_Buffer(VkRenderPass->Base.Memory.BaseAddress, 
															 VkRenderPass->Base.Offset));

	while (BStream_Reader_Is_Valid(&Reader)) {
		u32 DirtyFlag = BStream_Reader_U32(&Reader);

		if (DirtyFlag & GDI_RENDER_PASS_SHADER_BIT) {
			gdi_handle Handle = BStream_Reader_Struct(&Reader, gdi_handle);
			CurrentShader = VK_Shader_Pool_Get(&VkGDI->ResourcePool, Handle);
			Assert(CurrentShader->DEBUGType == GDI_PASS_TYPE_RENDER);
			vkCmdBindPipeline(VkRenderPass->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, CurrentShader->Pipeline);
			

		}

		for (size_t i = 0; i < GDI_MAX_BIND_GROUP_COUNT; i++) {
			vk_bind_group* BindGroup = NULL;
			if (DirtyFlag & (GDI_RENDER_PASS_BIND_GROUP_BIT << i)) {
				gdi_handle Handle = BStream_Reader_Struct(&Reader, gdi_handle);
				BindGroup = VK_Bind_Group_Pool_Get(&VkGDI->ResourcePool, Handle);
			} else {
				//If the shader changed make sure to update the bind group to the last one bound
				if ((DirtyFlag & GDI_RENDER_PASS_SHADER_BIT) && (i < CurrentShader->BindGroupCount)) {
					BindGroup = BindGroups[i];
				}
			}

			if (BindGroup) {
				vkCmdBindDescriptorSets(VkRenderPass->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, CurrentShader->Layout, (u32)i, 1, &BindGroup->Set, 0, NULL);
				BindGroups[i] = BindGroup;
			}
		}

		for (size_t i = 0; i < GDI_MAX_VTX_BUFFER_COUNT; i++) {
			if (DirtyFlag & (GDI_RENDER_PASS_VTX_BUFFER_BIT << i)) {
				gdi_handle Handle = BStream_Reader_Struct(&Reader, gdi_handle);
				vk_buffer* VtxBuffer = VK_Buffer_Pool_Get(&VkGDI->ResourcePool, Handle);
				
				VkDeviceSize Offset = 0;
				vkCmdBindVertexBuffers(VkRenderPass->CmdBuffer, (u32)i, 1, &VtxBuffer->Buffer, &Offset);
			}
		}

		if ((DirtyFlag & GDI_RENDER_PASS_IDX_BUFFER_BIT) || (DirtyFlag & GDI_RENDER_PASS_IDX_FORMAT_BIT)) {
			if (DirtyFlag & GDI_RENDER_PASS_IDX_BUFFER_BIT) {
				gdi_handle Handle = BStream_Reader_Struct(&Reader, gdi_handle);
				CurrentIdxBuffer = VK_Buffer_Pool_Get(&VkGDI->ResourcePool, Handle);
			}

			if (DirtyFlag & GDI_RENDER_PASS_IDX_FORMAT_BIT) {
				CurrentIdxFormat = (gdi_idx_format)BStream_Reader_U32(&Reader);
			}

			vkCmdBindIndexBuffer(VkRenderPass->CmdBuffer, CurrentIdxBuffer->Buffer, 0, VK_Get_Idx_Type(CurrentIdxFormat));
		}

		if (DirtyFlag & GDI_RENDER_PASS_SCISSOR_MIN_X_BIT) {
			CurrentScissor.offset.x = (s32)BStream_Reader_U32(&Reader);
		}

		if (DirtyFlag & GDI_RENDER_PASS_SCISSOR_MIN_Y_BIT) {
			CurrentScissor.offset.y = (s32)BStream_Reader_U32(&Reader);
		}

		if (DirtyFlag & GDI_RENDER_PASS_SCISSOR_MAX_X_BIT) {
			CurrentScissor.extent.width = (s32)BStream_Reader_U32(&Reader)-CurrentScissor.offset.x;
		}

		if (DirtyFlag & GDI_RENDER_PASS_SCISSOR_MAX_Y_BIT) {
			CurrentScissor.extent.height = (s32)BStream_Reader_U32(&Reader)-CurrentScissor.offset.y;
		}

		if (DirtyFlag & GDI_RENDER_PASS_SCISSOR) {
			vkCmdSetScissor(VkRenderPass->CmdBuffer, 0, 1, &CurrentScissor);
		}

		if (DirtyFlag & GDI_RENDER_PASS_IDX_COUNT_BIT) {
			CurrentIdxCount = BStream_Reader_U32(&Reader);
		}

		if (DirtyFlag & GDI_RENDER_PASS_IDX_OFFSET_BIT) {
			CurrentIdxOffset = BStream_Reader_U32(&Reader);
		}

		if (DirtyFlag & GDI_RENDER_PASS_VTX_OFFSET_BIT) {
			CurrentVtxOffset = BStream_Reader_U32(&Reader);
		}

		if (DirtyFlag & GDI_RENDER_PASS_PUSH_CONSTANT_COUNT) {
			PushConstantCount = BStream_Reader_U32(&Reader);
		}

		if (PushConstantCount) {
			u32 Size = PushConstantCount * sizeof(u32);
			vkCmdPushConstants(VkRenderPass->CmdBuffer, CurrentShader->Layout, VK_SHADER_STAGE_ALL, 0, Size, BStream_Reader_Size(&Reader, Size));
		}

		vkCmdDrawIndexed(VkRenderPass->CmdBuffer, CurrentIdxCount, 1, CurrentIdxOffset, CurrentVtxOffset, 0);
	}
	Assert(Reader.At == Reader.End);

	vkEndCommandBuffer(VkRenderPass->CmdBuffer);

	Memory_Clear(&RenderPass->PrevState, sizeof(gdi_draw_state));
	Memory_Clear(&RenderPass->CurrentState, sizeof(gdi_draw_state));
	RenderPass->Offset = 0;
	SLL_Push_Front(ThreadContext->RenderPassesToDelete, VkRenderPass);
}

global gdi_backend_vtable VK_Backend_VTable = {
	.CreateTextureFunc = VK_Create_Texture,
	.DeleteTextureFunc = VK_Delete_Texture,

	.CreateTextureViewFunc = VK_Create_Texture_View,
	.DeleteTextureViewFunc = VK_Delete_Texture_View,
	
	.CreateBufferFunc = VK_Create_Buffer,
	.DeleteBufferFunc = VK_Delete_Buffer,
	.MapBufferFunc = VK_Map_Buffer,
	.UnmapBufferFunc = VK_Unmap_Buffer,

	.CreateSamplerFunc = VK_Create_Sampler,
	.DeleteSamplerFunc = VK_Delete_Sampler,

	.CreateBindGroupLayoutFunc = VK_Create_Bind_Group_Layout,
	.DeleteBindGroupLayoutFunc = VK_Delete_Bind_Group_Layout,

	.CreateBindGroupFunc = VK_Create_Bind_Group,
	.DeleteBindGroupFunc = VK_Delete_Bind_Group,
	.WriteBindGroupFunc = VK_Write_Bind_Group,
	
	.CreateShaderFunc = VK_Create_Shader,
	.DeleteShaderFunc = VK_Delete_Shader,

	.CreateSwapchainFunc = VK_Create_Swapchain,
	.DeleteSwapchainFunc = VK_Delete_Swapchain,
	.GetSwapchainViewFunc = VK_Get_Swapchain_View,
	.GetSwapchainInfoFunc = VK_Get_Swapchain_Info,

	.BeginRenderPassFunc = VK_Begin_Render_Pass,
	.EndRenderPassFunc = VK_End_Render_Pass,

	.RenderFunc = VK_Render
};

function VkBool32 VK_Debug_Callback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT MessageTypes,
									const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData) {
	Debug_Log("%s", CallbackData->pMessage);
	Assert(false);
	return VK_FALSE;
}

export_function GDI_INIT_DEFINE(GDI_Init) {
	Base_Set(Base);
	arena* Arena = Arena_Create();
	vk_gdi* GDI = Arena_Push_Struct(Arena, vk_gdi);
	GDI->Base.Backend = &VK_Backend_VTable;
	GDI->Base.Arena = Arena;
	GDI->Base.FrameArena = Arena_Create();
	GDI->Base.IMThreadLocalStorage = OS_TLS_Create();

	string LibraryPath = String_Empty();

#if defined(VK_USE_PLATFORM_WIN32_KHR)
	LibraryPath = String_Lit("vulkan-1.dll");
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	LibraryPath = String_Lit("/usr/local/lib/libvulkan.1.dylib");
#else
#error "Not Implemented"
#endif

	GDI->Library = OS_Library_Create(LibraryPath);
	if (!GDI->Library) {
		Debug_Log("Failed to load vulkan-1.dll");
		return NULL;
	}

	VK_Load_Library_Funcs(GDI->Library);
	VK_Load_Global_Funcs();

	arena* Scratch = Scratch_Get();
	dynamic_char_ptr_array InstanceExtensions = Dynamic_Char_Ptr_Array_Init((allocator*)Scratch);
	dynamic_char_ptr_array Layers = Dynamic_Char_Ptr_Array_Init((allocator*)Scratch);

	u32 LayerCount;
	vkEnumerateInstanceLayerProperties(&LayerCount, VK_NULL_HANDLE);

	VkLayerProperties* LayerProperties = Arena_Push_Array(Scratch, LayerCount, VkLayerProperties);
	vkEnumerateInstanceLayerProperties(&LayerCount, LayerProperties);

#ifdef DEBUG_BUILD
	b32 HasValidationLayers = false;
	b32 HasDebugUtil = false;

	for (u32 i = 0; i < LayerCount; i++) {
		string LayerName = String_Null_Term(LayerProperties[i].layerName);
		if (String_Equals(LayerName, String_Lit("VK_LAYER_KHRONOS_validation"))) {
			Dynamic_Char_Ptr_Array_Add(&Layers, LayerProperties[i].layerName);
			HasValidationLayers = true;
		}
	}
#endif

	b32 HasRequiredInstanceExtensions[Array_Count(G_RequiredInstanceExtensions)] = { 0 };

	u32 InstanceExtensionCount;
	vkEnumerateInstanceExtensionProperties(NULL, &InstanceExtensionCount, VK_NULL_HANDLE);

	VkExtensionProperties* InstanceExtensionProperties = Arena_Push_Array(Scratch, InstanceExtensionCount, VkExtensionProperties);
	vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &InstanceExtensionCount, InstanceExtensionProperties);

	for (u32 i = 0; i < InstanceExtensionCount; i++) {
		string ExtensionName = String_Null_Term(InstanceExtensionProperties[i].extensionName);
		for (size_t j = 0; j < Array_Count(G_RequiredInstanceExtensions); j++) {
			if (String_Equals(ExtensionName, G_RequiredInstanceExtensions[j])) {
				HasRequiredInstanceExtensions[j] = true;
				Dynamic_Char_Ptr_Array_Add(&InstanceExtensions, InstanceExtensionProperties[i].extensionName);
			}
		}

#ifdef DEBUG_BUILD
		if (String_Equals(ExtensionName, String_Lit(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))) {
			Dynamic_Char_Ptr_Array_Add(&InstanceExtensions, InstanceExtensionProperties[i].extensionName);
			HasDebugUtil = true;
		}
#endif
	}

	VkInstanceCreateFlags InstanceFlags = 0;
#ifdef VK_USE_PLATFORM_METAL_EXT
	InstanceFlags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	VkApplicationInfo ApplicationInfo = {
		.apiVersion = VK_API_VERSION_1_0
	};

	VkInstanceCreateInfo InstanceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.flags = InstanceFlags,
		.pApplicationInfo = &ApplicationInfo,
		.enabledLayerCount = (u32)Layers.Count,
		.ppEnabledLayerNames = (const char* const*)Layers.Ptr,
		.enabledExtensionCount = (u32)InstanceExtensions.Count,
		.ppEnabledExtensionNames = (const char* const*)InstanceExtensions.Ptr,
	};

	VkResult Status = vkCreateInstance(&InstanceCreateInfo, VK_NULL_HANDLE, &GDI->Instance);
	Scratch_Release();

	if (Status != VK_SUCCESS) {
		Debug_Log("vkCreateInstance failed!");
		return NULL;
	}

#ifdef DEBUG_BUILD
	if (!HasValidationLayers) {
		Debug_Log("Missing vulkan validation layers");
	}

	if (!HasDebugUtil) {
		Debug_Log("Missing vulkan debug utility extension");
	}
#endif

	b32 HasExtensions = true;
	for (size_t i = 0; i < Array_Count(G_RequiredInstanceExtensions); i++) {
		if (!HasRequiredInstanceExtensions[i]) {
			Debug_Log("Missing vulkan instance extension '%.*s'", G_RequiredInstanceExtensions[i].Size, G_RequiredInstanceExtensions[i].Ptr);
			HasExtensions = false;
		}
	}

	if (!HasExtensions) return NULL;

#ifdef DEBUG_BUILD
	if (HasDebugUtil) {
		Vk_Ext_Debug_Utils_Funcs_Load(GDI->Instance);

		VkDebugUtilsMessengerCreateInfoEXT DebugUtilsCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = VK_Debug_Callback,
		};

		if (vkCreateDebugUtilsMessengerEXT(GDI->Instance, &DebugUtilsCreateInfo, VK_NULL_HANDLE, &GDI->DebugUtils) != VK_SUCCESS) {
			Debug_Log("vkCreateDebugUtilsMessengerEXT failed!");
		}
	}
#endif

	Vk_Khr_Surface_Funcs_Load(GDI->Instance);
	Vk_Instance_Funcs_Load(GDI->Instance);
	Vk_Khr_Get_Physical_Device_Properties2_Funcs_Load(GDI->Instance);

	Scratch = Scratch_Get();

	u32 PhysicalDeviceCount;
	vkEnumeratePhysicalDevices(GDI->Instance, &PhysicalDeviceCount, VK_NULL_HANDLE);

	VkPhysicalDevice* PhysicalDevices = Arena_Push_Array(Scratch, PhysicalDeviceCount, VkPhysicalDevice);
	vkEnumeratePhysicalDevices(GDI->Instance, &PhysicalDeviceCount, PhysicalDevices);

	GDI->GPUs = Arena_Push_Array(GDI->Base.Arena, PhysicalDeviceCount, vk_gpu);

	vk_tmp_surface Surface = VK_Create_Tmp_Surface(GDI);
	if (Surface.Surface == VK_NULL_HANDLE) {
		Debug_Log("Failed to create the vulkan tmp surface");
		return NULL;
	}

	for (size_t i = 0; i < PhysicalDeviceCount; i++) {
		vk_gpu GPU;
		if (VK_Fill_GPU(GDI, &GPU, PhysicalDevices[i], Surface.Surface)) {
			GDI->GPUs[GDI->GPUCount++] = GPU;
		}
	}

	VK_Delete_Tmp_Surface(GDI, &Surface);

	if (!GDI->GPUCount) {
		Debug_Log("No valid GPUs");
		return NULL;
	}

	//todo: Is this gpu selection logic really good enough?
	//Priority discrete gpus first
	vk_gpu* TargetGPU = NULL;
	for (size_t i = 0; i < GDI->GPUCount; i++) {
		if (GDI->GPUs[i].Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			TargetGPU = &GDI->GPUs[i];
			break;
		}
	}

	//Just choose the first gpu if nothing else is available
	if (!TargetGPU) {
		TargetGPU = &GDI->GPUs[0];
	}

	//Create the device
	f32 QueuePriority = 1.0f;
	u32 QueueCreateInfoCount = 1;
	VkDeviceQueueCreateInfo QueueCreateInfos[2] = { 
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = TargetGPU->GraphicsQueueFamilyIndex,
			.queueCount = 1,
			.pQueuePriorities = &QueuePriority
		}
	};

	if (TargetGPU->PresentQueueFamilyIndex != TargetGPU->GraphicsQueueFamilyIndex) {
		VkDeviceQueueCreateInfo QueueCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = TargetGPU->PresentQueueFamilyIndex,
			.queueCount = 1,
			.pQueuePriorities = &QueuePriority
		};
		QueueCreateInfos[QueueCreateInfoCount++] = QueueCreateInfo;
	}
					
	VkDeviceCreateInfo DeviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = TargetGPU->Features,
		.queueCreateInfoCount = QueueCreateInfoCount,
		.pQueueCreateInfos = QueueCreateInfos,
		.enabledExtensionCount = (u32)TargetGPU->Extensions.Count,
		.ppEnabledExtensionNames = (const char* const*)TargetGPU->Extensions.Ptr
	};

	VkDevice Device;
	Status = vkCreateDevice(TargetGPU->PhysicalDevice, &DeviceCreateInfo, VK_NULL_HANDLE, &Device);
	if (Status != VK_SUCCESS) {
		Debug_Log("vkCreateDevice failed to create device '%s'", TargetGPU->Properties.deviceName);
		return NULL;
	}

	GDI->TargetGPU = TargetGPU;
	GDI->Device = Device;

	Scratch_Release();
	if (GDI->Device == VK_NULL_HANDLE) {
		Debug_Log("Could not find a valid vulkan device with the required app features");
		return NULL;
	}

	GDI->Base.DeviceName = String_Copy((allocator*)GDI->Base.Arena, String_Null_Term(TargetGPU->Properties.deviceName));

	Debug_Log("GPU: %.*s", GDI->Base.DeviceName.Size, GDI->Base.DeviceName.Ptr);

	Vk_Device_Funcs_Load(GDI->Device);
	Vk_Khr_Swapchain_Funcs_Load(GDI->Device);
	Vk_Khr_Synchronization2_Funcs_Load(GDI->Device);
	Vk_Khr_Dynamic_Rendering_Funcs_Load(GDI->Device);
	Vk_Khr_Push_Descriptor_Funcs_Load(GDI->Device);

	VmaVulkanFunctions VmaFunctions = {
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
		.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
		.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
		.vkAllocateMemory = vkAllocateMemory,
		.vkFreeMemory = vkFreeMemory,
		.vkMapMemory = vkMapMemory,
		.vkUnmapMemory = vkUnmapMemory,
		.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
		.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
		.vkBindBufferMemory = vkBindBufferMemory,
		.vkBindImageMemory = vkBindImageMemory,
		.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
		.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
		.vkCreateBuffer = vkCreateBuffer,
		.vkDestroyBuffer = vkDestroyBuffer,
		.vkCreateImage = vkCreateImage,
		.vkDestroyImage = vkDestroyImage,
		.vkCmdCopyBuffer = vkCmdCopyBuffer
	};

	VmaAllocatorCreateInfo GPUAllocatorInfo = {
		.physicalDevice = GDI->TargetGPU->PhysicalDevice,
		.device = GDI->Device,
		.pVulkanFunctions = &VmaFunctions,
		.instance = GDI->Instance,
		.vulkanApiVersion = VK_API_VERSION_1_0,
	};

	Status = vmaCreateAllocator(&GPUAllocatorInfo, &GDI->GPUAllocator);
	if (Status != VK_SUCCESS) {
		Debug_Log("vmaCreateAllocator failed!");
		return NULL;
	}

	vkGetDeviceQueue(GDI->Device, GDI->TargetGPU->GraphicsQueueFamilyIndex, 0, &GDI->GraphicsQueue);
	if (GDI->TargetGPU->GraphicsQueueFamilyIndex != GDI->TargetGPU->PresentQueueFamilyIndex) {
		vkGetDeviceQueue(GDI->Device, GDI->TargetGPU->PresentQueueFamilyIndex, 0, &GDI->PresentQueue);
	} else {
		GDI->PresentQueue = GDI->GraphicsQueue;
	}
	GDI->TransferQueue = GDI->GraphicsQueue;
	GDI->Base.ConstantBufferAlignment = GDI->TargetGPU->Properties.limits.minUniformBufferOffsetAlignment;

	GDI->DescriptorLock = OS_Mutex_Create();

	u32 EntryCount = Array_Count(GDI->ResourcePool.Bind_GroupPool.Entries);
	VkDescriptorPoolSize DescriptorPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, EntryCount },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, EntryCount },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, EntryCount }
	};

	VkDescriptorPoolCreateInfo DescriptorPoolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT|VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
		.maxSets = EntryCount,
		.poolSizeCount = Array_Count(DescriptorPoolSizes),
		.pPoolSizes = DescriptorPoolSizes
	};

	Status = vkCreateDescriptorPool(GDI->Device, &DescriptorPoolInfo, VK_NULL_HANDLE, &GDI->DescriptorPool);
	if (Status != VK_SUCCESS) {
		Debug_Log("vkCreateDescriptorPool failed!");
		return NULL;
	}

	VK_Resource_Pool_Init(&GDI->ResourcePool, GDI->Base.Arena);

	for (u32 i = 0; i < VK_FRAME_COUNT; i++) {
		vk_frame_context* Frame = GDI->Frames + i;
		Frame->Arena = Arena_Create();
		Frame->Index = i;

		VkCommandPoolCreateInfo CommandPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = GDI->TargetGPU->GraphicsQueueFamilyIndex
		};
		vkCreateCommandPool(GDI->Device, &CommandPoolCreateInfo, VK_NULL_HANDLE, &Frame->CmdPool);

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = Frame->CmdPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		vkAllocateCommandBuffers(GDI->Device, &CommandBufferAllocateInfo, &Frame->CmdBuffer);

		VkFenceCreateInfo FenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		if (i == 0) {
			FenceCreateInfo.flags = 0;
		}

		vkCreateFence(GDI->Device, &FenceCreateInfo, VK_NULL_HANDLE, &Frame->Fence);
		
		VkSemaphoreCreateInfo SemaphoreLockInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		Status = vkCreateSemaphore(GDI->Device, &SemaphoreLockInfo, VK_NULL_HANDLE, &Frame->RenderLock);
		if (Status != VK_SUCCESS) {
			Debug_Log("vkCreateSemaphore failed!");
			return NULL;
		}

		Status = vkCreateSemaphore(GDI->Device, &SemaphoreLockInfo, VK_NULL_HANDLE, &Frame->TransferLock);
		if (Status != VK_SUCCESS) {
			Debug_Log("vkCreateSemaphore failed!");
			return NULL;
		}

		Frame->ReadbackBuffer = VK_CPU_Buffer_Init(GDI, GDI->Base.Arena, VK_CPU_BUFFER_TYPE_READBACK);

		vk_delete_queue* DeleteQueue = GDI->DeleteQueues + i;
		DeleteQueue->ThreadContextTLS = OS_TLS_Create();
	}

	GDI->DeleteLock = OS_RW_Mutex_Create();
	GDI->TransferLock = OS_RW_Mutex_Create();
	for (u64 i = 0; i < VK_MAX_TRANSFER_COUNT; i++) {
		vk_transfer_context* Transfer = GDI->Transfers + i;

		Transfer->ThreadContextTLS = OS_TLS_Create();
		
		VkCommandPoolCreateInfo CommandPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = GDI->TargetGPU->GraphicsQueueFamilyIndex
		};
		vkCreateCommandPool(GDI->Device, &CommandPoolCreateInfo, VK_NULL_HANDLE, &Transfer->CmdPool);

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = Transfer->CmdPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		vkAllocateCommandBuffers(GDI->Device, &CommandBufferAllocateInfo, &Transfer->CmdBuffer);

#ifdef DEBUG_BUILD
		VkFenceCreateInfo FenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		if (i == 0) {
			FenceCreateInfo.flags = 0;
		}

		vkCreateFence(GDI->Device, &FenceCreateInfo, VK_NULL_HANDLE, &Transfer->DEBUGFence);		
#endif
	}
	GDI->CurrentTransfer = &GDI->Transfers[0];
	GDI->CurrentFrame = GDI->Frames + GDI->FrameIndex;

	vk_frame_context* Frame = GDI->CurrentFrame;
	return (gdi*)GDI;
}

//todo: Does this need to even be here anymore?
#ifdef OS_WIN32
#pragma comment(lib, "base.lib")
#endif