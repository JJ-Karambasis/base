#define VK_NO_PROTOTYPES

#include <meta_program/meta_defines.h>

#ifdef VK_USE_PLATFORM_METAL_EXT
#define VK_ENABLE_BETA_EXTENSIONS
#endif

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#ifdef VK_USE_PLATFORM_XLIB_KHR
#undef Status
#endif

#include <base.h>

#include <gdi/gdi.h>

#include "vk_gdi.h"

#include <gdi/gdi.c>

Dynamic_Array_Implement(char_ptr, char*, Char_Ptr);
Dynamic_Array_Implement_Type(u32, U32);
Dynamic_Array_Implement(vk_image_memory_barrier, VkImageMemoryBarrier, VK_Image_Memory_Barrier);
Dynamic_Array_Implement(vk_image_memory_barrier2, VkImageMemoryBarrier2KHR, VK_Image_Memory_Barrier2);
Dynamic_Array_Implement(vk_write_descriptor_set, VkWriteDescriptorSet, VK_Write_Descriptor_Set);
Dynamic_Array_Implement(vk_copy_descriptor_set, VkCopyDescriptorSet, VK_Copy_Descriptor_Set);
Array_Implement(vk_texture_readback, VK_Texture_Readback);
Array_Implement(vk_buffer_readback, VK_Buffer_Readback);
Array_Implement(gdi_bind_group_buffer, GDI_Bind_Group_Buffer);
Array_Implement(vk_frame_context, VK_Frame_Context);

global string G_RequiredInstanceExtensions[] = {
	String_Expand(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME),
	String_Expand(VK_KHR_SURFACE_EXTENSION_NAME),
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	String_Expand(VK_KHR_WIN32_SURFACE_EXTENSION_NAME)
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	String_Expand(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME),
	String_Expand(VK_EXT_METAL_SURFACE_EXTENSION_NAME)
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	String_Expand(VK_KHR_XLIB_SURFACE_EXTENSION_NAME)
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
	String_Expand(VK_KHR_SWAPCHAIN_EXTENSION_NAME),
	String_Expand(VK_KHR_MAINTENANCE3_EXTENSION_NAME),
	String_Expand(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME),
#ifdef VK_USE_PLATFORM_METAL_EXT
	String_Expand(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
#endif
};

global gdi_log_func* G_LogFunc;
global void* G_LogUserData;

function GDI_LOG_DEFINE(GDI_Default_Log_Callback) {
	string LogTypeStr = GDI_Get_Log_Type_Str(Type);
	Debug_Log("GDI %.*s: %.*s", LogTypeStr.Size, LogTypeStr.Ptr, Message.Size, Message.Ptr);
}

function void GDI_Log(gdi_log_type Type, const char* Format, ...) {
	arena* Scratch = Scratch_Get();
	va_list List;
	va_start(List, Format);
	string Message = String_FormatV((allocator*)Scratch, Format, List);
	va_end(List);

	G_LogFunc(Type, Message, G_LogUserData);
}

#define GDI_Log_Warning(...) GDI_Log(GDI_LOG_TYPE_WARNING, __VA_ARGS__)
#define GDI_Log_Error(...) GDI_Log(GDI_LOG_TYPE_ERROR, __VA_ARGS__)

typedef struct {
    size_t ByteSize;
} vk_aligned_block;

function void* VKAPI_PTR VK_Alloc(void* UserData, size_t ByteSize, size_t Alignment, VkSystemAllocationScope AllocationScope) {
    Assert(Alignment > 0 && Is_Pow2(Alignment));

    size_t Offset = Alignment - 1 + sizeof(void*);
    vk_aligned_block* Block = (vk_aligned_block*)Allocator_Allocate_Memory(Default_Allocator_Get(), ByteSize+Offset+sizeof(vk_aligned_block));
    if(!Block) return NULL;

    Block->ByteSize = ByteSize;

    void* P1  = Block+1;
    void** P2 = (void**)(((size_t)(P1) + Offset) & ~(Alignment - 1));
    P2[-1] = P1;
        
    return P2;
}

function void* VKAPI_PTR VK_Realloc(void* UserData, void* Original, size_t Size, size_t Alignment, VkSystemAllocationScope AllocationScope) {
    if(!Original) return VK_Alloc(UserData, Size, Alignment, AllocationScope);
    Assert(Alignment > 0 && Is_Pow2(Alignment));
        
    void* OriginalUnaligned = ((void**)Original)[-1];
    vk_aligned_block* OriginalBlock = ((vk_aligned_block*)OriginalUnaligned)-1;

    size_t Offset = Alignment - 1 + sizeof(void*);

    vk_aligned_block* NewBlock = (vk_aligned_block*)Allocator_Allocate_Memory(Default_Allocator_Get(), Size+Offset+sizeof(vk_aligned_block));
    NewBlock->ByteSize = Size;

    void* P1  = NewBlock+1;
    void** P2 = (void**)(((size_t)(P1) + Offset) & ~(Alignment - 1));
    P2[-1] = P1;

    Memory_Copy(P2, Original, Min(NewBlock->ByteSize, OriginalBlock->ByteSize));

    Allocator_Free_Memory(Default_Allocator_Get(), OriginalBlock);

    return P2;
}

function void VKAPI_PTR VK_Free(void* UserData, void* Memory) {
    if(Memory) {
        void* OriginalUnaligned = ((void**)Memory)[-1];
        vk_aligned_block* Block = ((vk_aligned_block*)OriginalUnaligned)-1;
		Allocator_Free_Memory(Default_Allocator_Get(), Block);
    }
}

global VkAllocationCallbacks G_AllocationCallback;

void VK_Set_Allocator() {
	G_AllocationCallback.pfnAllocation = VK_Alloc;
	G_AllocationCallback.pfnReallocation = VK_Realloc;
	G_AllocationCallback.pfnFree = VK_Free;
}

const VkAllocationCallbacks* VK_Get_Allocator() {
    return &G_AllocationCallback;
}

#if defined(VK_USE_PLATFORM_WIN32_KHR)

typedef LRESULT win32_def_window_proc_a(HWND, UINT, WPARAM, LPARAM);
typedef ATOM win32_register_class_a(const WNDCLASSA*);
typedef BOOL win32_unregister_class_a(LPCSTR, HINSTANCE);
typedef HWND win32_create_window_ex_a(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef BOOL win32_destroy_window(HWND);

typedef struct {
	HWND Window;
	VkSurfaceKHR Surface;

	HMODULE User32Library;
	win32_def_window_proc_a* DefWindowProcA;
	win32_register_class_a* RegisterClassA;
	win32_unregister_class_a* UnregisterClassA;
	win32_create_window_ex_a* CreateWindowExA;
	win32_destroy_window* DestroyWindow;
} vk_tmp_surface;

VkSurfaceKHR VK_Create_Surface(vk_gdi* GDI, HWND Window, HINSTANCE Instance) {
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(GDI->Instance, "vkCreateWin32SurfaceKHR");
	
	VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = Instance,
		.hwnd = Window
	};

	VkSurfaceKHR Surface;
	VkResult Status = vkCreateWin32SurfaceKHR(GDI->Instance, &SurfaceCreateInfo, VK_Get_Allocator(), &Surface);
	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vkCreateWin32SurfaceKHR failed!");
		return VK_NULL_HANDLE;
	}

	return Surface;
}

function vk_tmp_surface VK_Create_Tmp_Surface(vk_gdi* GDI) {
	vk_tmp_surface Result;
	Memory_Clear(&Result, sizeof(vk_tmp_surface));

	Result.User32Library = LoadLibraryA("user32.dll");
	if (!Result.User32Library) {
		GDI_Log_Error("Failed to load user32.dll");
		return Result;
	}

	Result.DefWindowProcA = (win32_def_window_proc_a*)GetProcAddress(Result.User32Library, "DefWindowProcA");
	Result.RegisterClassA = (win32_register_class_a*)GetProcAddress(Result.User32Library, "RegisterClassA");
	Result.UnregisterClassA = (win32_unregister_class_a*)GetProcAddress(Result.User32Library, "UnregisterClassA");
	Result.CreateWindowExA = (win32_create_window_ex_a*)GetProcAddress(Result.User32Library, "CreateWindowExA");
	Result.DestroyWindow = (win32_destroy_window*)GetProcAddress(Result.User32Library, "DestroyWindow");

	// Create a minimal window class
    WNDCLASSA WindowClass = {
		.lpfnWndProc = Result.DefWindowProcA,
		.hInstance = GetModuleHandle(NULL),
		.lpszClassName = "GDI_DummyVulkanWindow"
	};
    
    if (!Result.RegisterClassA(&WindowClass)) {
		GDI_Log_Error("Failed to register window class");
		return Result;
    }

	HWND Window = Result.CreateWindowA(WindowClass.lpszClassName, "Dummy", 
                              	   	   WS_POPUP, 0, 0, 1, 1, 
								   	   NULL, NULL, WindowClass.hInstance, NULL);
    
    if (!Window) {
		//todo: Need to cleanup resources
        GDI_Log_Error("Failed to create window");
        return Result;
    }

	VkSurfaceKHR Surface = VK_Create_Surface(GDI, Window, WindowClass.hInstance);
	if (Surface != VK_NULL_HANDLE) {
		Result.Window = Window;
		Result.Surface = Surface;
	} else {
		//todo: Need to cleanup resources
	}

	return Result;
}

function void VK_Delete_Tmp_Surface(vk_gdi* GDI, vk_tmp_surface* Surface) {
	vkDestroySurfaceKHR(GDI->Instance, Surface->Surface, VK_Get_Allocator());
	Surface->DestroyWindow(Surface->Window);
	Surface->UnregisterClassA("GDI_DummyVulkanWindow", GetModuleHandle(NULL));
	FreeLibrary(Surface->User32Library);
}

#elif defined(VK_USE_PLATFORM_METAL_EXT)

typedef struct {
	NSWindow* Window;
	CAMetalLayer* Layer;
	VkSurfaceKHR Surface;
} vk_tmp_surface;

function VkSurfaceKHR VK_Create_Surface(vk_gdi* GDI, CAMetalLayer* Layer) {
    PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT = (PFN_vkCreateMetalSurfaceEXT)vkGetInstanceProcAddr(GDI->Instance, "vkCreateMetalSurfaceEXT");
	VkMetalSurfaceCreateInfoEXT SurfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
		.pLayer = Layer
	};

    VkSurfaceKHR Surface;
	VkResult Status = vkCreateMetalSurfaceEXT(GDI->Instance, &SurfaceCreateInfo, VK_Get_Allocator(), &Surface);
	if(Status != VK_SUCCESS) {
		GDI_Log_Error("vkCreateMetalSurfaceEXT failed!");
		return VK_NULL_HANDLE;
	}

    return Surface;
}

function vk_tmp_surface VK_Create_Tmp_Surface(vk_gdi* GDI) {
	vk_tmp_surface Result;
	Memory_Clear(&Result, sizeof(vk_tmp_surface));

    NSRect Rect = NSMakeRect(0, 0, 1, 1);
    NSWindow* Window = [[NSWindow alloc] initWithContentRect:Rect
                                         styleMask:NSWindowStyleMaskBorderless
                                         backing:NSBackingStoreBuffered
                                         defer:NO];
    if(Window == nil) {
        GDI_Log_Error("Failed to create window");
        return Result;
    }

    NSView* View = [Window contentView];
    
    // Create a CAMetalLayer and attach to view
    CAMetalLayer* Layer = [CAMetalLayer layer];
    if(Layer == nil) {
        //todo: Need to cleanup resources
        GDI_Log_Error("Failed to create the metal layer");
        return Result;
    }
    
    [View setLayer:Layer];
    [View setWantsLayer:YES];

	VkSurfaceKHR Surface = VK_Create_Surface(GDI, Layer);
	if (Surface != VK_NULL_HANDLE) {
		Result.Window = Window;
        Result.Layer = Layer;
		Result.Surface = Surface;
	}

	return Result;
}

function void VK_Delete_Tmp_Surface(vk_gdi* GDI, vk_tmp_surface* Surface) {
	vkDestroySurfaceKHR(GDI->Instance, Surface->Surface, VK_Get_Allocator());
	[Surface->Layer release];
    [Surface->Window release];
}

#elif defined(VK_USE_PLATFORM_XLIB_KHR)

typedef struct {
	Display* Display;
	Window Window;
	VkSurfaceKHR Surface;
} vk_tmp_surface;

function VkSurfaceKHR VK_Create_Surface(vk_gdi* GDI, Display* Display, Window Window) {
    PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(GDI->Instance, "vkCreateXlibSurfaceKHR");
	VkXlibSurfaceCreateInfoKHR SurfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
		.dpy = Display,
		.window = Window
	};

    VkSurfaceKHR Surface;
	VkResult Status = vkCreateXlibSurfaceKHR(GDI->Instance, &SurfaceCreateInfo, VK_Get_Allocator(), &Surface);
	if(Status != VK_SUCCESS) {
		GDI_Log_Error("vkCreateXlibSurfaceKHR failed!");
		return VK_NULL_HANDLE;
	}

    return Surface;
}

function vk_tmp_surface VK_Create_Tmp_Surface(vk_gdi* GDI) {
	vk_tmp_surface Result;
	Memory_Clear(&Result, sizeof(vk_tmp_surface));

	Display* Display = XOpenDisplay(NULL);
	if(!Display) {
		GDI_Log_Error("Failed to open X Display");
		return Result;
	}

	int Screen = DefaultScreen(Display);
	Window Window = RootWindow(Display, Screen);

	VkSurfaceKHR Surface = VK_Create_Surface(GDI, Display, Window);
	if (Surface != VK_NULL_HANDLE) {
		Result.Display = Display;
		Result.Window = Window;
		Result.Surface = Surface;
	}

	return Result;
}

function void VK_Delete_Tmp_Surface(vk_gdi* GDI, vk_tmp_surface* Surface) {
	vkDestroySurfaceKHR(GDI->Instance, Surface->Surface, VK_Get_Allocator());
	XCloseDisplay(Surface->Display);
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

function inline vk_cpu_buffer VK_CPU_Buffer_Init(vk_device_context* Context, arena* Arena, vk_cpu_buffer_type Type) {
	vk_cpu_buffer Result = {
		.BlockArena = Arena,
		.Context = Context,
		.Type = Type
	};
	return Result;
}

function void VK_CPU_Buffer_Release(vk_cpu_buffer* CpuBuffer) {
	vk_cpu_buffer_block* Block = CpuBuffer->FirstBlock;
	while (Block) {
		vk_cpu_buffer_block* BlockToDelete = Block;
		Block = Block->Next;

		if (BlockToDelete->Buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(CpuBuffer->Context->GPUAllocator, BlockToDelete->Buffer, BlockToDelete->Allocation);

			BlockToDelete->Allocation = VK_NULL_HANDLE;
			BlockToDelete->Buffer = VK_NULL_HANDLE;

			BlockToDelete->Start = NULL;
			BlockToDelete->End = NULL;
			BlockToDelete->Current = NULL;
		}
	}

	if (CpuBuffer->BlockArena) {
		CpuBuffer->BlockArena = NULL;
	}

	CpuBuffer->FirstBlock = NULL;
	CpuBuffer->LastBlock = NULL;
	CpuBuffer->CurrentBlock = NULL;
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
	VkResult Status = vmaCreateBuffer(CpuBuffer->Context->GPUAllocator, &BufferInfo, &AllocateInfo, &Buffer, &Allocation, &AllocInfo);
	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vmaCreateBuffer failed!");
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

function vk_frame_thread_context* VK_Get_Frame_Thread_Context(vk_device_context* Context, vk_frame_context* Frame) {
	vk_frame_thread_context* ThreadContext = (vk_frame_thread_context*)OS_TLS_Get(Frame->ThreadContextTLS);
	if (!ThreadContext) {
		arena* Arena = Arena_Create(String_Lit("VK Thread Context"));
		
		ThreadContext = Arena_Push_Struct(Arena, vk_frame_thread_context);
		ThreadContext->Arena = Arena;
		ThreadContext->TempArena = Arena_Create(String_Lit("VK Thread Context Temp"));
		ThreadContext->NeedsReset = true;

		
		VkCommandPoolCreateInfo CommandPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = Context->GPU->GraphicsQueueFamilyIndex
		};
		VkResult Status = vkCreateCommandPool(Context->Device, &CommandPoolCreateInfo, VK_Get_Allocator(), &ThreadContext->CmdPool);
		if (Status != VK_SUCCESS) {
			GDI_Log_Error("vkCreateCommandPool failed!");
			return NULL;
		}

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = ThreadContext->CmdPool,
			.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			.commandBufferCount = 1
		};

		Status = vkAllocateCommandBuffers(Context->Device, &CommandBufferAllocateInfo, &ThreadContext->CmdBuffer);
		if (Status != VK_SUCCESS) {
			GDI_Log_Error("vkAllocateCommandBuffers failed!");
			return NULL;
		}

		//todo: Improvement
		//These upload buffers can eat a lot of memory since there is one
		//upload buffer per frame/per thread. On some system with many
		//cores this can consume a lot of additional megabytes
		ThreadContext->UploadBuffer = VK_CPU_Buffer_Init(Context, ThreadContext->Arena, VK_CPU_BUFFER_TYPE_UPLOAD);

		/*Append to link list atomically*/
		for(;;) {
			vk_frame_thread_context* OldTop = (vk_frame_thread_context*)Atomic_Load_Ptr(&Frame->TopThread);
			ThreadContext->Next = OldTop;
			if(Atomic_Compare_Exchange_Ptr(&Frame->TopThread, OldTop, ThreadContext) == OldTop) {
				break;
			}
		}

		OS_TLS_Set(Frame->ThreadContextTLS, ThreadContext);
	}

	if (ThreadContext->NeedsReset) {
		Arena_Clear(ThreadContext->TempArena);

		vkResetCommandPool(Context->Device, ThreadContext->CmdPool, 0);
		
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
		ThreadContext->FirstBindGroupWriteCmd = NULL;
		ThreadContext->LastBindGroupWriteCmd = NULL;
		ThreadContext->FirstBindGroupCopyCmd = NULL;
		ThreadContext->LastBindGroupCopyCmd = NULL;
	}

	return ThreadContext;
}

function void VK_Delete_Texture_Resources(vk_device_context* Context, vk_texture* Texture) {
	if (Texture->Image != VK_NULL_HANDLE) {
		vmaDestroyImage(Context->GPUAllocator, Texture->Image, Texture->Allocation);
		Texture->Image = VK_NULL_HANDLE;
	}
}

function void VK_Delete_Texture_View_Resources(vk_device_context* Context, vk_texture_view* TextureView) {
	if (TextureView->ImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(Context->Device, TextureView->ImageView, VK_Get_Allocator());
		TextureView->ImageView = VK_NULL_HANDLE;
	}
}

function void VK_Delete_Buffer_Resources(vk_device_context* Context, vk_buffer* Buffer) {
	
	if (Buffer->MappedPtr != NULL) {
		Assert(Buffer->Usage & GDI_BUFFER_USAGE_DYNAMIC);
		vmaUnmapMemory(Context->GPUAllocator, Buffer->Allocation);
		Buffer->MappedPtr = NULL;
	}

	if (Buffer->Buffer != VK_NULL_HANDLE) {
		vmaDestroyBuffer(Context->GPUAllocator, Buffer->Buffer, Buffer->Allocation);
		Buffer->Buffer = VK_NULL_HANDLE;
	}
}

function void VK_Delete_Sampler_Resources(vk_device_context* Context, vk_sampler* Sampler) {
	if (Sampler->Sampler != VK_NULL_HANDLE) {
		vkDestroySampler(Context->Device, Sampler->Sampler, VK_Get_Allocator());
		Sampler->Sampler = VK_NULL_HANDLE;
	}
}

function void VK_Delete_Bind_Group_Layout_Resources(vk_device_context* Context, vk_bind_group_layout* BindGroupLayout) {
	if (BindGroupLayout->AllocationData) {
		Allocator_Free_Memory(Default_Allocator_Get(), BindGroupLayout->AllocationData);
	}

	if (BindGroupLayout->Layout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(Context->Device, BindGroupLayout->Layout, VK_Get_Allocator());
		BindGroupLayout->Layout = VK_NULL_HANDLE;
	}
}

function void VK_Delete_Bind_Group_Resources(vk_device_context* Context, vk_bind_group* BindGroup) {
	arena* Scratch = Scratch_Get();

	u32 SetCount = 0;
	VkDescriptorSet* Sets = Arena_Push_Array(Scratch, Context->Frames.Count, VkDescriptorSet);
	for (size_t i = 0; i < Context->Frames.Count; i++) {
		if (BindGroup->Sets[i]) {
			Sets[SetCount++] = BindGroup->Sets[i];
		}
	}

	if (SetCount) {
		OS_Mutex_Lock(Context->DescriptorLock);
		vkFreeDescriptorSets(Context->Device, Context->DescriptorPool, SetCount, Sets);
		OS_Mutex_Unlock(Context->DescriptorLock);
	}

	Scratch_Release();

	if (BindGroup->AllocationData) {
		Allocator_Free_Memory(Default_Allocator_Get(), BindGroup->AllocationData);
		BindGroup->AllocationData = NULL;
		BindGroup->Sets = NULL;
		BindGroup->Bindings = NULL;
	}
}

function void VK_Delete_Shader_Resources(vk_device_context* Context, vk_shader* Shader) {
	if (Shader->Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(Context->Device, Shader->Pipeline, VK_Get_Allocator());
		Shader->Pipeline = VK_NULL_HANDLE;
	}

	if (Shader->Layout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(Context->Device, Shader->Layout, VK_Get_Allocator());
		Shader->Layout = VK_NULL_HANDLE;
	}

	if (Shader->WritableLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(Context->Device, Shader->WritableLayout, VK_Get_Allocator());
		Shader->WritableLayout = VK_NULL_HANDLE;
	}

	if (!GDI_Bind_Group_Binding_Array_Is_Empty(Shader->WritableBindings)) {
		Allocator_Free_Memory(Default_Allocator_Get(), Shader->WritableBindings.Ptr);
	}
}

function void VK_Delete_Semaphore_Resources(vk_device_context* Context, vk_semaphore* Semaphore) {
	if (Semaphore->Handle != VK_NULL_HANDLE) {
		vkDestroySemaphore(Context->Device, Semaphore->Handle, VK_Get_Allocator());
		Semaphore->Handle = VK_NULL_HANDLE;
	}
}

function void VK_Delete_Swapchain_Resources(vk_device_context* Context, vk_swapchain* Swapchain) {
	vk_gdi* GDI = (vk_gdi*)Context->Base.GDI;

	for (size_t i = 0; i < Context->Frames.Count; i++) {
		VK_Delete_Semaphore_Resources(Context, &Swapchain->Locks[i]);
	}

	for (size_t i = 0; i < Swapchain->ImageCount; i++) {
		vk_texture_view* TextureView = VK_Texture_View_Pool_Get(&Context->ResourcePool, Swapchain->TextureViews[i]);
		if (TextureView) {
			VK_Delete_Texture_View_Resources(Context, TextureView);
			VK_Texture_View_Pool_Free(&Context->ResourcePool, Swapchain->TextureViews[i]);
		}
	}

	for (size_t i = 0; i < Swapchain->ImageCount; i++) {
		VK_Texture_Pool_Free(&Context->ResourcePool, Swapchain->Textures[i]);
	}

	if (Swapchain->Images != VK_NULL_HANDLE) {
		Allocator_Free_Memory(Default_Allocator_Get(), Swapchain->Images);
		Swapchain->Images = NULL;
		Swapchain->Textures = NULL;
		Swapchain->TextureViews = NULL;
		Swapchain->Locks = NULL;
	}

	if (Swapchain->Swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(Context->Device, Swapchain->Swapchain, VK_Get_Allocator());
		Swapchain->Swapchain = VK_NULL_HANDLE;
	}
		
	if (Swapchain->Surface) {
		vkDestroySurfaceKHR(GDI->Instance, Swapchain->Surface, VK_Get_Allocator());
		Swapchain->Surface = VK_NULL_HANDLE;
	}
}

function b32 VK_Fill_GPU(vk_gdi* GDI, vk_gpu* GPU, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface) {
	VkPhysicalDeviceProperties DeviceProperties;
	vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT* DescriptorIndexingFeature = Arena_Push_Struct(GDI->Base.Arena, VkPhysicalDeviceDescriptorIndexingFeaturesEXT);
	VkPhysicalDeviceSynchronization2FeaturesKHR* Synchronization2Feature = Arena_Push_Struct(GDI->Base.Arena, VkPhysicalDeviceSynchronization2FeaturesKHR);
	VkPhysicalDeviceDynamicRenderingFeaturesKHR* DynamicRenderingFeature = Arena_Push_Struct(GDI->Base.Arena, VkPhysicalDeviceDynamicRenderingFeaturesKHR);
	VkPhysicalDeviceFeatures2KHR* Features = Arena_Push_Struct(GDI->Base.Arena, VkPhysicalDeviceFeatures2KHR);

	DescriptorIndexingFeature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	Synchronization2Feature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
	DynamicRenderingFeature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	Features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;

	Synchronization2Feature->pNext = DescriptorIndexingFeature;
	DynamicRenderingFeature->pNext = Synchronization2Feature;
	Features->pNext = DynamicRenderingFeature;
	
	vkGetPhysicalDeviceFeatures2KHR(PhysicalDevice, Features);

	b32 HasFeatures = true;
	if (!DescriptorIndexingFeature->descriptorBindingSampledImageUpdateAfterBind ||
		!DescriptorIndexingFeature->descriptorBindingStorageBufferUpdateAfterBind ||
		!DescriptorIndexingFeature->descriptorBindingStorageImageUpdateAfterBind) {
		GDI_Log_Warning("Missing vulkan feature 'Descriptor Indexing' for device '%s'", DeviceProperties.deviceName);
		HasFeatures = false;
	}

	if (!Synchronization2Feature->synchronization2) {
		GDI_Log_Warning("Missing vulkan feature 'Synchronization 2' for device '%s'", DeviceProperties.deviceName);
		HasFeatures = false;
	}

	if (!DynamicRenderingFeature->dynamicRendering) {
		GDI_Log_Warning("Missing vulkan feature 'Dynamic Rendering' for device '%s'", DeviceProperties.deviceName);
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
			GDI_Log_Warning("Missing vulkan device extension '%.*s' for device '%s'", G_RequiredDeviceExtensions[j].Size, G_RequiredDeviceExtensions[j].Ptr, DeviceProperties.deviceName);
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
				GDI_Log_Warning("Could not find a valid presentation queue family index for device '%s'", DeviceProperties.deviceName);
			}
		} else {
			GDI_Log_Warning("Could not find a valid graphics queue family index for device '%s'", DeviceProperties.deviceName);
		}
	}

	Scratch_Release();
	return Result;
}

function vk_frame_context* VK_Lock_Frame(vk_device_context* Context) {
	OS_RW_Mutex_Read_Lock(Context->FrameLock);
	u64 FrameIndex = Atomic_Load_U64(&Context->FrameCount);
	vk_frame_context* Result = Context->Frames.Ptr + (FrameIndex % Context->Frames.Count);
	return Result;
}

function void VK_Unlock_Frame(vk_device_context* Context) {
	OS_RW_Mutex_Read_Unlock(Context->FrameLock);
}

function void VK_Semaphore_Add_To_Delete_Queue(vk_device_context* Context, vk_semaphore* Semaphore) {
	vk_frame_context* Frame = VK_Lock_Frame(Context);
	vk_frame_thread_context* ThreadContext = VK_Get_Frame_Thread_Context(Context, Frame);
	vk_semaphore_delete_queue_entry* Entry = Arena_Push_Struct(ThreadContext->TempArena, vk_semaphore_delete_queue_entry);
	Entry->Entry = *Semaphore;
	SLL_Push_Back(ThreadContext->DeleteQueue.SemaphoreDeleteQueue.First, ThreadContext->DeleteQueue.SemaphoreDeleteQueue.Last, Entry);
	VK_Unlock_Frame(Context);
	Memory_Clear(Semaphore, sizeof(vk_semaphore));
}

function VkImageView VK_Create_Image_View(vk_device_context* Context, const VkImageViewCreateInfo* TextureViewInfo);
function GDI_BACKEND_CREATE_TEXTURE_VIEW_DEFINE(VK_Create_Texture_View);
function b32 VK_Recreate_Swapchain(vk_device_context* Context, vk_swapchain* Swapchain) {	
	VkSurfaceCapabilitiesKHR SurfaceCaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Context->GPU->PhysicalDevice, Swapchain->Surface, &SurfaceCaps);

	//Check if we even need to resize
	if (Swapchain->Dim.x == (s32)SurfaceCaps.currentExtent.width && Swapchain->Dim.y == (s32)SurfaceCaps.currentExtent.height) {
		return true;
	}

	Swapchain->Dim = V2i(SurfaceCaps.currentExtent.width, SurfaceCaps.currentExtent.height);

	if (Swapchain->Dim.x == 0 || Swapchain->Dim.y == 0) {
		return false;
	}

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

	VkResult Status = vkCreateSwapchainKHR(Context->Device, &SwapchainCreateInfo, VK_Get_Allocator(), &Swapchain->Swapchain);

	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vkCreateSwapchainKHR failed!");
		return false;
	}

	vkGetSwapchainImagesKHR(Context->Device, Swapchain->Swapchain, &Swapchain->ImageCount, Swapchain->Images);
	
	for (u32 i = 0; i < Swapchain->ImageCount; i++) {
		if (GDI_Is_Null(Swapchain->Textures[i])) {
			Swapchain->Textures[i] = VK_Texture_Pool_Allocate(&Context->ResourcePool);

			vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, Swapchain->Textures[i]);
			Texture->Image = Swapchain->Images[i];
			Texture->Dim = Swapchain->Dim;
			Texture->Format = Swapchain->Format;
			Texture->MipCount = 1;
			Texture->Swapchain = Swapchain->Handle;

			gdi_texture_view_create_info TextureViewCreateInfo = {
				.Texture = Swapchain->Textures[i]
			};
			Swapchain->TextureViews[i] = VK_Create_Texture_View(Context->Base.GDI, &TextureViewCreateInfo);
		} else {
			vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, Swapchain->Textures[i]);
			Texture->Image = Swapchain->Images[i];
			Texture->Dim = Swapchain->Dim;
			Texture->Format = Swapchain->Format;
			Texture->MipCount = 1;
			Texture->Swapchain = Swapchain->Handle;

			vk_texture_view* TextureView = VK_Texture_View_Pool_Get(&Context->ResourcePool, Swapchain->TextureViews[i]);
			VK_Texture_View_Add_Data_To_Delete_Queue(Context, TextureView);

			VkImageViewCreateInfo ViewInfo = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = Texture->Image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = VK_Get_Format(TextureView->Format),
				.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
			};

			TextureView->ImageView = VK_Create_Image_View(Context, &ViewInfo);
			TextureView->Texture = Swapchain->Textures[i];
			TextureView->Format = Swapchain->Format;
			TextureView->Dim = Swapchain->Dim;
		}
	}

	for (size_t i = 0; i < Context->Frames.Count; i++) {
		if (Swapchain->Locks[i].Handle != VK_NULL_HANDLE) {
			VK_Semaphore_Add_To_Delete_Queue(Context, &Swapchain->Locks[i]);
		}

		VkSemaphoreCreateInfo SemaphoreLockInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		VkResult Status = vkCreateSemaphore(Context->Device, &SemaphoreLockInfo, VK_Get_Allocator(), &Swapchain->Locks[i].Handle);
		if (Status != VK_SUCCESS) {
			GDI_Log_Error("vkCreateSemaphore failed!");
			return false;
		}
	}

	Swapchain->ImageIndex = -1;
	return true;
}

#include "meta/vk_meta.c"

#ifdef DEBUG_BUILD
#define VK_Set_Debug_Name(type, type_str, handle, name_str) \
if (!String_Is_Empty(name_str)) { \
VkDebugUtilsObjectNameInfoEXT NameInfo = { \
.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, \
.objectType = type, \
.objectHandle = (u64)handle, \
.pObjectName = name_str.Ptr \
}; \
if (vkSetDebugUtilsObjectNameEXT(Context->Device, &NameInfo) != VK_SUCCESS) { \
GDI_Log_Error("WARNING: Could not set the vulkan debug name for " type_str " %.*s", name_str.Size, name_str.Ptr); \
} \
}
#else
#define VK_Set_Debug_Name(...)
#endif

function OS_THREAD_CALLBACK_DEFINE(VK_Readback_Thread) {
	vk_device_context* Context = (vk_device_context*)UserData;

	for (;;) {
		OS_Semaphore_Decrement(Context->ReadbackSignalSemaphore);

		//Wait on the frame fence before we readback to the cpu
		if (Context->ReadbackFrameCount < Atomic_Load_U64(&Context->FrameCount)) {
			vk_frame_context* Frame = Context->Frames.Ptr + (Context->ReadbackFrameCount % Context->Frames.Count);

			OS_Mutex_Lock(Frame->FenceLock);
			VkResult FenceStatus = vkGetFenceStatus(Context->Device, Frame->Fence);
			if (FenceStatus == VK_NOT_READY) {
				vkWaitForFences(Context->Device, 1, &Frame->Fence, VK_TRUE, UINT64_MAX);
			} else {
				Assert(FenceStatus == VK_SUCCESS);
			}
			OS_Mutex_Unlock(Frame->FenceLock);
		
			for (size_t i = 0; i < Frame->TextureReadbacks.Count; i++) {
				vk_texture_readback* Readback = Frame->TextureReadbacks.Ptr + i;
				vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, Readback->Texture);
				Readback->Func(Readback->Texture, Texture->Dim, Texture->Format, Readback->CPU.Ptr, Readback->UserData);
			}

			for (size_t i = 0; i < Frame->BufferReadbacks.Count; i++) {
				vk_buffer_readback* Readback = Frame->BufferReadbacks.Ptr + i;
				vk_buffer* Buffer = VK_Buffer_Pool_Get(&Context->ResourcePool, Readback->Buffer);
				Readback->Func(Readback->Buffer, Buffer->Size, Readback->CPU.Ptr, Readback->UserData);
			}

			Context->ReadbackFrameCount++;
			OS_Semaphore_Increment(Context->ReadbackFinishedSemaphore);
		} else {
			Assert(Context->ReadbackFrameCount == Atomic_Load_U64(&Context->FrameCount));
			break;
		}
	}
}

function GDI_BACKEND_DELETE_BUFFER_DEFINE(VK_Delete_Buffer);
function void VK_Delete_Device_Context(vk_gdi* GDI, vk_device_context* Context, gdi_shutdown_flags Flags) {
	//Wait for the readback thread to finish and then delete
	vkDeviceWaitIdle(Context->Device);
	
	if (Context->ReadbackThread) {		
		//Signal the readback submit event to stop the readback thread from waiting
		OS_Semaphore_Increment(Context->ReadbackSignalSemaphore);

		OS_Thread_Join(Context->ReadbackThread);
		Context->ReadbackThread = NULL;
	}

	im_gdi* ImGDI = (im_gdi*)Atomic_Load_Ptr(&Context->Base.TopIM);
	while (ImGDI) {
		im_gdi* ImGDIToDelete = ImGDI;
		ImGDI = ImGDI->Next;

		if (!GDI_Is_Null(ImGDIToDelete->VtxBuffer)) {
			VK_Delete_Buffer((gdi*)GDI, ImGDIToDelete->VtxBuffer);
			ImGDIToDelete->VtxBuffer = GDI_Null_Handle();
		}

		if (!GDI_Is_Null(ImGDIToDelete->IdxBuffer)) {
			VK_Delete_Buffer((gdi*)GDI, ImGDIToDelete->IdxBuffer);
			ImGDIToDelete->IdxBuffer = GDI_Null_Handle();
		}

		if (ImGDIToDelete->Arena) {
			Arena_Delete(ImGDIToDelete->Arena);
		}
	}
	Atomic_Store_Ptr(&Context->Base.TopIM, NULL);

	for (u32 i = 0; i < Context->Frames.Count; i++) {
		vk_frame_context* Frame = Context->Frames.Ptr + i;

		vk_frame_thread_context* ThreadContext = (vk_frame_thread_context*)Atomic_Load_Ptr(&Frame->TopThread);
		while (ThreadContext) {
			vk_frame_thread_context* ThreadContextToDelete = ThreadContext;
			ThreadContext = ThreadContext->Next;

			VK_Delete_Queued_Thread_Resources(Context, ThreadContextToDelete);
			
			Assert(!ThreadContextToDelete->RenderPassesToDelete);
			vk_render_pass* RenderPass = ThreadContextToDelete->FreeRenderPasses;
			while (RenderPass) {
				vk_render_pass* RenderPassToDelete = RenderPass;
				RenderPass = RenderPass->Next;

				Delete_Memory_Reserve(&RenderPassToDelete->Base.Memory);
			}

			VK_CPU_Buffer_Release(&ThreadContextToDelete->UploadBuffer);

			if (ThreadContextToDelete->CmdPool != VK_NULL_HANDLE) {
				vkDestroyCommandPool(Context->Device, ThreadContextToDelete->CmdPool, VK_Get_Allocator());
				ThreadContextToDelete->CmdPool = VK_NULL_HANDLE;
			}
			
			if (ThreadContextToDelete->TempArena) {
				Arena_Delete(ThreadContextToDelete->TempArena);
				ThreadContextToDelete->TempArena = NULL;
			}

			if (ThreadContextToDelete->Arena) {
				Arena_Delete(ThreadContextToDelete->Arena);
			}
		}

		if (Frame->ThreadContextTLS) {
			OS_TLS_Delete(Frame->ThreadContextTLS);
			Frame->ThreadContextTLS = NULL;
		}

		VK_CPU_Buffer_Release(&Frame->ReadbackBuffer);

		if (Frame->RenderLock != VK_NULL_HANDLE) {
			vkDestroySemaphore(Context->Device, Frame->RenderLock, VK_Get_Allocator());
			Frame->RenderLock = VK_NULL_HANDLE;
		}

		if (Frame->FenceLock) {
			OS_Mutex_Delete(Frame->FenceLock);
			Frame->FenceLock = NULL;
		}

		if (Frame->Fence != VK_NULL_HANDLE) {
			vkDestroyFence(Context->Device, Frame->Fence, VK_Get_Allocator());
			Frame->Fence = VK_NULL_HANDLE;
		}

		if (Frame->CmdPool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(Context->Device, Frame->CmdPool, VK_Get_Allocator());
			Frame->CmdPool = VK_NULL_HANDLE;
		}

		if (Frame->TempArena) {
			Arena_Delete(Frame->TempArena);
			Frame->TempArena = NULL;
		}
	}

	if (Context->ReadbackSignalSemaphore) {
		OS_Semaphore_Delete(Context->ReadbackSignalSemaphore);
		Context->ReadbackSignalSemaphore = NULL;
	}

	if (Context->ReadbackFinishedSemaphore) {
		OS_Semaphore_Delete(Context->ReadbackFinishedSemaphore);
		Context->ReadbackFinishedSemaphore = NULL;
	}

	if (Context->FrameLock) {
		OS_RW_Mutex_Delete(Context->FrameLock);
		Context->FrameLock = NULL;
	}

	if (Flags & GDI_SHUTDOWN_FLAG_FREE_RESOURCES) {
		VK_Resource_Pool_Free_All(Context, &Context->ResourcePool);
	}

	if (Context->DescriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(Context->Device, Context->DescriptorPool, VK_Get_Allocator());
		Context->DescriptorPool = VK_NULL_HANDLE;
	}

	if (Context->DescriptorLock) {
		OS_Mutex_Delete(Context->DescriptorLock);
		Context->DescriptorLock = NULL;
	}

	vmaDestroyAllocator(Context->GPUAllocator);

	if (Context->Base.IMThreadLocalStorage) {
		OS_TLS_Delete(Context->Base.IMThreadLocalStorage);
		Context->Base.IMThreadLocalStorage = NULL;
	}

	if (Context->Base.FrameArena) {
		Arena_Delete(Context->Base.FrameArena);
		Context->Base.FrameArena = NULL;
	}

	if (Context->Device != VK_NULL_HANDLE) {
		vkDestroyDevice(Context->Device, VK_Get_Allocator());
		Context->Device = VK_NULL_HANDLE;
	}

	if (Context->Arena) {
		Arena_Delete(Context->Arena);
	}
}

function vk_device_context* VK_Create_Device_Context(vk_gdi* GDI, gdi_device* Device) {
	//Allocate the device context memory
	arena* Arena = Arena_Create(String_Lit("VK Device Context"));
	vk_device_context* Context = Arena_Push_Struct(Arena, vk_device_context);
	
	//Create the device
	vk_gpu* TargetGPU = GDI->GPUs + Device->DeviceIndex;
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

	VkResult Status = vkCreateDevice(TargetGPU->PhysicalDevice, &DeviceCreateInfo, VK_Get_Allocator(), &Context->Device);
	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vkCreateDevice failed to create device '%s'", TargetGPU->Properties.deviceName);
		return NULL;
	}

	//Initialize the context base variables
	Context->Base.GDI = (gdi*)GDI;
	Context->Base.Device = Device;
	Context->Base.FrameArena = Arena_Create(String_Lit("GDI Frame"));
	Context->Base.ConstantBufferAlignment = TargetGPU->Properties.limits.minUniformBufferOffsetAlignment;
	Context->Base.IMThreadLocalStorage = OS_TLS_Create();
	Context->Arena = Arena;
	Context->GPU = TargetGPU;


	//Load the extension functions
	Vk_Device_Funcs_Load(Context->Device);
	Vk_Khr_Swapchain_Funcs_Load(Context->Device);
	Vk_Khr_Synchronization2_Funcs_Load(Context->Device);
	Vk_Khr_Dynamic_Rendering_Funcs_Load(Context->Device);
	Vk_Khr_Push_Descriptor_Funcs_Load(Context->Device);

	//Create a vma allocator
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
		.physicalDevice = TargetGPU->PhysicalDevice,
		.device = Context->Device,
		.pAllocationCallbacks = VK_Get_Allocator(),
		.pVulkanFunctions = &VmaFunctions,
		.instance = GDI->Instance,
		.vulkanApiVersion = VK_API_VERSION_1_0,
	};

	Status = vmaCreateAllocator(&GPUAllocatorInfo, &Context->GPUAllocator);
	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vmaCreateAllocator failed!");
		return NULL;
	}

	//Get the command queues
	vkGetDeviceQueue(Context->Device, TargetGPU->GraphicsQueueFamilyIndex, 0, &Context->GraphicsQueue);
	if (TargetGPU->GraphicsQueueFamilyIndex != TargetGPU->PresentQueueFamilyIndex) {
		vkGetDeviceQueue(Context->Device, TargetGPU->PresentQueueFamilyIndex, 0, &Context->PresentQueue);
	} else {
		Context->PresentQueue = Context->GraphicsQueue;
	}
	Context->TransferQueue = Context->GraphicsQueue;

	//Initiaize resources (descriptors and the resource pools)
	Context->DescriptorLock = OS_Mutex_Create();

	u32 EntryCount = Array_Count(Context->ResourcePool.Bind_GroupPool.Entries);
	VkDescriptorPoolSize DescriptorPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, EntryCount },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, EntryCount },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, EntryCount },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, EntryCount },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, EntryCount },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, EntryCount }
	};

	VkDescriptorPoolCreateInfo DescriptorPoolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT|VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
		.maxSets = EntryCount,
		.poolSizeCount = Array_Count(DescriptorPoolSizes),
		.pPoolSizes = DescriptorPoolSizes
	};

	Status = vkCreateDescriptorPool(Context->Device, &DescriptorPoolInfo, VK_Get_Allocator(), &Context->DescriptorPool);
	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vkCreateDescriptorPool failed!");
		return NULL;
	}

	VK_Resource_Pool_Init(&Context->ResourcePool, Context->Arena);


	//Create the frame contexts
	Context->FrameLock = OS_RW_Mutex_Create();
	vk_frame_context_array Frames = VK_Frame_Context_Array_Alloc((allocator*)Context->Arena, GDI->Base.FramesInFlight + 1);

	for (u32 i = 0; i < Frames.Count; i++) {
		vk_frame_context* Frame = Frames.Ptr + i;

		arena* Scratch = Scratch_Get();
		string FrameArenaName = String_Format((allocator*)Scratch, "VK Frame Temp %u", i);
		Frame->TempArena = Arena_Create(FrameArenaName);
		Scratch_Release();

		Frame->Index = i;

		VkCommandPoolCreateInfo CommandPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = TargetGPU->GraphicsQueueFamilyIndex
		};
		vkCreateCommandPool(Context->Device, &CommandPoolCreateInfo, VK_Get_Allocator(), &Frame->CmdPool);

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = Frame->CmdPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		vkAllocateCommandBuffers(Context->Device, &CommandBufferAllocateInfo, &Frame->CmdBuffer);

		VkFenceCreateInfo FenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		if (i == 0) {
			FenceCreateInfo.flags = 0;
		}

		vkCreateFence(Context->Device, &FenceCreateInfo, VK_Get_Allocator(), &Frame->Fence);

		Frame->FenceLock = OS_Mutex_Create();
		
		VkSemaphoreCreateInfo SemaphoreLockInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		Status = vkCreateSemaphore(Context->Device, &SemaphoreLockInfo, VK_Get_Allocator(), &Frame->RenderLock);
		if (Status != VK_SUCCESS) {
			GDI_Log_Error("vkCreateSemaphore failed!");
			return NULL;
		}

		Frame->ThreadContextTLS = OS_TLS_Create();

		Frame->ReadbackBuffer = VK_CPU_Buffer_Init(Context, GDI->Base.Arena, VK_CPU_BUFFER_TYPE_READBACK);
	}
	Context->Frames = Frames;

	Context->ReadbackSignalSemaphore = OS_Semaphore_Create(0);
	Context->ReadbackFinishedSemaphore = OS_Semaphore_Create((u32)Context->Frames.Count-1);

	//Initialize the readback thread
	Atomic_Store_B32(&Context->ReadbackIsInitialized, true);
	Context->ReadbackThread = OS_Thread_Create(String_Lit("Vulkan Readback"), VK_Readback_Thread, Context);

	return Context;
}

function GDI_BACKEND_SET_DEVICE_CONTEXT_DEFINE(VK_Set_Device_Context) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;
	vk_gdi* VkGDI = (vk_gdi*)GDI;
	if(Context) {
		VK_Delete_Device_Context(VkGDI, Context, GDI_SHUTDOWN_FLAG_FREE_RESOURCES);
		GDI->DeviceContext = NULL;
	}

	if(Device) {
		Context = VK_Create_Device_Context(VkGDI, Device);
		GDI->DeviceContext = (gdi_device_context*)Context;
		if(!GDI->DeviceContext) {
			return false;
		}
	}

	return true;
}

function GDI_BACKEND_CREATE_TEXTURE_DEFINE(VK_Create_Texture) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	VkImageUsageFlags ImageUsage = 0;
	if (TextureInfo->Usage & GDI_TEXTURE_USAGE_RENDER_TARGET) {
		ImageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if (TextureInfo->Usage & GDI_TEXTURE_USAGE_DEPTH) {
		ImageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}

	if (TextureInfo->Usage & GDI_TEXTURE_USAGE_SAMPLED) {
		ImageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if (TextureInfo->Usage & GDI_TEXTURE_USAGE_STORAGE) {
		ImageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if (TextureInfo->Usage & GDI_TEXTURE_USAGE_READBACK) {
		ImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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
	VkResult Status = vmaCreateImage(Context->GPUAllocator, &ImageCreateInfo, &AllocateInfo, &Image, &Allocation, NULL);
	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vmaCreateImage failed");
		return GDI_Null_Handle();
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_IMAGE, "texture", Image, TextureInfo->DebugName);

	gdi_handle Result = VK_Texture_Pool_Allocate(&Context->ResourcePool);	
	vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, Result);

	Texture->Image = Image;
	Texture->Allocation = Allocation;
	Texture->Format = TextureInfo->Format;
	Texture->MipCount = TextureInfo->MipCount;
	Texture->Dim = TextureInfo->Dim;
	Texture->Usage = TextureInfo->Usage;
	
	vk_frame_context* Frame = VK_Lock_Frame(Context);
	vk_frame_thread_context* ThreadContext = VK_Get_Frame_Thread_Context(Context, Frame);

	vk_texture_barrier_cmd* Cmd = Arena_Push_Struct(ThreadContext->TempArena, vk_texture_barrier_cmd);
	Cmd->Texture = Result;
	Cmd->QueueFlag = VK_GDI_QUEUE_FLAG_CREATION;
	SLL_Push_Back(ThreadContext->FirstTextureBarrierCmd, ThreadContext->LastTextureBarrierCmd, Cmd);

	VK_Unlock_Frame(Context);

	return Result;
}

function GDI_BACKEND_DELETE_TEXTURE_DEFINE(VK_Delete_Texture) {
	VK_Texture_Add_To_Delete_Queue((vk_device_context*)GDI->DeviceContext, Texture);
}

function GDI_BACKEND_UPDATE_TEXTURES_DEFINE(VK_Update_Textures) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	vk_frame_context* Frame = VK_Lock_Frame(Context);
	vk_frame_thread_context* ThreadContext = VK_Get_Frame_Thread_Context(Context, Frame);

	for (size_t i = 0; i < Updates.Count; i++) {
		const gdi_texture_update* Update = Updates.Ptr + i;

		//If we are updating multiple mips in one update, make sure they do not have
		//offsets.
		Assert(Update->MipCount);
		Assert((Update->Offset.x == 0 && Update->Offset.y == 0) || (Update->MipCount == 1));

		vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, Update->Texture);
		if (Texture) {

			size_t TotalSize = 0;
			for (size_t i = 0; i < Update->MipCount; i++) {
				TotalSize += Update->UpdateData[i].Size;
			}

			vk_cpu_buffer_push Upload = VK_CPU_Buffer_Push(&ThreadContext->UploadBuffer, TotalSize);

			VkImageAspectFlags ImageAspect = GDI_Is_Depth_Format(Texture->Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

			arena* Scratch = Scratch_Get();
			VkBufferImageCopy* Regions = Arena_Push_Array(Scratch, Update->MipCount, VkBufferImageCopy);
			v2i TextureDim = Update->Dim;

			VkDeviceSize Offset = 0;
			for (u32 i = 0; i < Update->MipCount; i++) {
				VkBufferImageCopy BufferImageCopy = {
					.bufferOffset = Upload.Offset + Offset,
					.imageSubresource = { ImageAspect, Update->MipOffset + i, 0, 1 },
					.imageOffset = { Update->Offset.x, Update->Offset.y, 0 },
					.imageExtent = { (u32)TextureDim.x, (u32)TextureDim.y, 1 }
				};
				Regions[i] = BufferImageCopy;
				TextureDim = V2i_Div_S32(TextureDim, 2);

				Memory_Copy(Upload.Ptr + Offset, Update->UpdateData[i].Ptr, Update->UpdateData[i].Size);
				Offset += Update->UpdateData[i].Size;
			}
			vkCmdCopyBufferToImage(ThreadContext->CmdBuffer, Upload.Buffer, Texture->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Update->MipCount, Regions);
			Scratch_Release();

			vk_texture_barrier_cmd* Cmd = Arena_Push_Struct(ThreadContext->TempArena, vk_texture_barrier_cmd);
			Cmd->Texture = Update->Texture;
			Cmd->QueueFlag = VK_GDI_QUEUE_FLAG_TRANSFER;
			SLL_Push_Back(ThreadContext->FirstTextureBarrierCmd, ThreadContext->LastTextureBarrierCmd, Cmd);
		} Invalid_Else;
	}

	VK_Unlock_Frame(Context);
}

function GDI_BACKEND_GET_TEXTURE_INFO_DEFINE(VK_Get_Texture_Info) {
	gdi_texture_info TextureInfo;
	Memory_Clear(&TextureInfo, sizeof(gdi_texture_info));

	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	vk_texture* VkTexture = VK_Texture_Pool_Get(&Context->ResourcePool, Texture);
	if(!VkTexture) return TextureInfo;

	TextureInfo.Dim = VkTexture->Dim;
	TextureInfo.Format = VkTexture->Format;
	TextureInfo.MipCount = VkTexture->MipCount;
	TextureInfo.Usage = VkTexture->Usage;

	return TextureInfo;
}

function VkImageView VK_Create_Image_View(vk_device_context* Context, const VkImageViewCreateInfo* TextureViewInfo) {
	VkImageView ImageView;
	VkResult Status = vkCreateImageView(Context->Device, TextureViewInfo, VK_Get_Allocator(), &ImageView);
	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vkCreateImageView failed!");
		return VK_NULL_HANDLE;
	}

	return ImageView;
}

function GDI_BACKEND_CREATE_TEXTURE_VIEW_DEFINE(VK_Create_Texture_View) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, TextureViewInfo->Texture);
	
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

	VkImageView ImageView = VK_Create_Image_View(Context, &ViewInfo);
	if(!ImageView) {
		return GDI_Null_Handle();
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_IMAGE_VIEW, "texture view", ImageView, TextureViewInfo->DebugName);

	gdi_handle Result = VK_Texture_View_Pool_Allocate(&Context->ResourcePool);
	vk_texture_view* View = VK_Texture_View_Pool_Get(&Context->ResourcePool, Result);

	View->ImageView = ImageView;
	View->Texture = TextureViewInfo->Texture;
	View->Format = Format;
	View->Dim = Texture->Dim;
	
	return Result;
}

function GDI_BACKEND_DELETE_TEXTURE_VIEW_DEFINE(VK_Delete_Texture_View) {
	VK_Texture_View_Add_To_Delete_Queue((vk_device_context*)GDI->DeviceContext, TextureView);
}

function GDI_BACKEND_GET_TEXTURE_VIEW_TEXTURE_DEFINE(VK_Get_Texture_View_Texture) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;
	vk_texture_view* VkTextureView = VK_Texture_View_Pool_Get(&Context->ResourcePool, TextureView);
	if(!VkTextureView) return GDI_Null_Handle();
	return VkTextureView->Texture;
}

function GDI_BACKEND_MAP_BUFFER_DEFINE(VK_Map_Buffer) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;
	vk_buffer* VkBuffer = VK_Buffer_Pool_Get(&Context->ResourcePool, Buffer);

	if (Size == 0) {
		Size = VkBuffer->Size - Offset;
	}
	Assert(Offset + Size <= VkBuffer->Size);

	if (VkBuffer) {
		vk_frame_context* Frame = VK_Lock_Frame(Context);
		VkBuffer->LockedFrame = Frame;
		if (VkBuffer->Usage & GDI_BUFFER_USAGE_DYNAMIC) {
			return VkBuffer->MappedPtr + (VkBuffer->Size * Frame->Index) + Offset;
		} else {
			vk_frame_thread_context* ThreadContext = VK_Get_Frame_Thread_Context(Context, Frame);
			vk_cpu_buffer_push Upload = VK_CPU_Buffer_Push(&ThreadContext->UploadBuffer, Size);
			VkBuffer->MappedUpload = Upload;
			VkBuffer->MappedOffset = Offset;
			VkBuffer->MappedSize = Size;
			return Upload.Ptr;
		}
	}

	return NULL;
}

function GDI_BACKEND_UNMAP_BUFFER_DEFINE(VK_Unmap_Buffer) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;
	vk_buffer* VkBuffer = VK_Buffer_Pool_Get(&Context->ResourcePool, Buffer);
	if (VkBuffer) {
		Assert(VkBuffer->LockedFrame);

		if (VkBuffer->Usage & GDI_BUFFER_USAGE_DYNAMIC) {
			//Does nothing
		} else {
			if (VkBuffer->MappedUpload.Ptr) {
				vk_frame_thread_context* ThreadContext = VK_Get_Frame_Thread_Context(Context, VkBuffer->LockedFrame);

				vk_cpu_buffer_push Upload = VkBuffer->MappedUpload;
				VkBufferCopy Region = {
					.srcOffset = Upload.Offset,
					.dstOffset = VkBuffer->MappedOffset,
					.size = VkBuffer->MappedSize
				};

				vkCmdCopyBuffer(ThreadContext->CmdBuffer, Upload.Buffer, VkBuffer->Buffer, 1, &Region);
				Memory_Clear(&VkBuffer->MappedUpload, sizeof(vk_cpu_buffer_push));
			}
		}

		VK_Unlock_Frame(Context);
		VkBuffer->LockedFrame = NULL;
	}
}
 
function GDI_BACKEND_CREATE_BUFFER_DEFINE(VK_Create_Buffer) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	VkBufferUsageFlags BufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if (BufferInfo->Usage & GDI_BUFFER_USAGE_VTX) {
		BufferUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}

	if (BufferInfo->Usage & GDI_BUFFER_USAGE_IDX) {
		BufferUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}

	if (BufferInfo->Usage & GDI_BUFFER_USAGE_CONSTANT) {
		BufferUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}

	if (BufferInfo->Usage & GDI_BUFFER_USAGE_READBACK) {
		BufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	if (BufferInfo->Usage & GDI_BUFFER_USAGE_STORAGE) {
		BufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}

	//If we have a dynamic buffer, we need to allocate space for all 
	//frames that will store data in the buffer

	size_t TotalSize = BufferInfo->Size;

	VmaAllocationCreateFlags AllocationFlags = 0;
	if (BufferInfo->Usage & GDI_BUFFER_USAGE_DYNAMIC) {
		TotalSize *= Context->Frames.Count;
		AllocationFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT|VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	}

	VkBufferCreateInfo BufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = TotalSize,
		.usage = BufferUsage
	};

	VmaAllocationCreateInfo AllocateInfo = {
		.flags = AllocationFlags,
		.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	};

	VkBuffer Handle;
	VmaAllocation Allocation;
	VkResult Status = vmaCreateBuffer(Context->GPUAllocator, &BufferCreateInfo, &AllocateInfo, &Handle, &Allocation, NULL);
	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vmaCreateBuffer failed");
		return GDI_Null_Handle();
	}

	u8* MappedPtr = NULL;
	if (BufferInfo->Usage & GDI_BUFFER_USAGE_DYNAMIC) {
		//Dynamic buffers can map their buffers and persist them throughout the entire buffers lifecycle
		Status = vmaMapMemory(Context->GPUAllocator, Allocation, (void**)&MappedPtr);
		if (Status != VK_SUCCESS) {
			GDI_Log_Error("vmaMapMemory failed!");
			vmaDestroyBuffer(Context->GPUAllocator, Handle, Allocation);
			return GDI_Null_Handle();
		}
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_BUFFER, "buffer", Handle, BufferInfo->DebugName);

	gdi_handle Result = VK_Buffer_Pool_Allocate(&Context->ResourcePool);
	vk_buffer* Buffer = VK_Buffer_Pool_Get(&Context->ResourcePool, Result);
	Buffer->Buffer = Handle;
	Buffer->Allocation = Allocation;
	Buffer->Size = BufferInfo->Size;
	Buffer->TotalSize = TotalSize;
	Buffer->Usage = BufferInfo->Usage;
	Buffer->MappedPtr = MappedPtr;

	return Result;
}

function GDI_BACKEND_DELETE_BUFFER_DEFINE(VK_Delete_Buffer) {
	VK_Buffer_Add_To_Delete_Queue((vk_device_context*)GDI->DeviceContext, Buffer);
}
 
function GDI_BACKEND_CREATE_SAMPLER_DEFINE(VK_Create_Sampler) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

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
	VkResult Status = vkCreateSampler(Context->Device, &CreateInfo, VK_Get_Allocator(), &Handle);
	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vkCreateSampler failed!");
		return GDI_Null_Handle();
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_SAMPLER, "sampler", Handle, SamplerInfo->DebugName);

	gdi_handle Result = VK_Sampler_Pool_Allocate(&Context->ResourcePool);
	vk_sampler* Sampler = VK_Sampler_Pool_Get(&Context->ResourcePool, Result);
	Sampler->Sampler = Handle;
	return Result;
}

function GDI_BACKEND_DELETE_SAMPLER_DEFINE(VK_Delete_Sampler) {
	VK_Sampler_Add_To_Delete_Queue((vk_device_context*)GDI->DeviceContext, Sampler);
}
 
function GDI_BACKEND_CREATE_BIND_GROUP_LAYOUT_DEFINE(VK_Create_Bind_Group_Layout) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	arena* Scratch = Scratch_Get();

	VkDescriptorSetLayoutBinding* Bindings = Arena_Push_Array(Scratch, BindGroupLayoutInfo->Bindings.Count, VkDescriptorSetLayoutBinding);
	VkDescriptorBindingFlagsEXT* BindingFlags = Arena_Push_Array(Scratch, BindGroupLayoutInfo->Bindings.Count, VkDescriptorBindingFlagsEXT);
	
	size_t ImmutableSamplerCount = 0;
	for (size_t i = 0; i < BindGroupLayoutInfo->Bindings.Count; i++) {
		gdi_bind_group_binding* Binding = BindGroupLayoutInfo->Bindings.Ptr + i;

		VkSampler* ImmutableSamplers = NULL;
		if(Binding->StaticSamplers != NULL) {
			Assert(Binding->Type == GDI_BIND_GROUP_TYPE_SAMPLER);
			ImmutableSamplers = Arena_Push_Array(Scratch, Binding->Count, VkSampler);
			for(size_t j = 0; j < Binding->Count; j++) {
				vk_sampler* Sampler = VK_Sampler_Pool_Get(&Context->ResourcePool, Binding->StaticSamplers[j]);
				Assert(Sampler);
				ImmutableSamplers[j] = Sampler->Sampler;
			}

			ImmutableSamplerCount += Binding->Count;
		}

		VkDescriptorSetLayoutBinding LayoutBinding = {
			.binding = (u32)i,
			.descriptorType = VK_Get_Descriptor_Type(Binding->Type),
			.descriptorCount = Binding->Count,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = ImmutableSamplers
		};

		Bindings[i] = LayoutBinding;

		//For some reason, the feature descriptorBindingUniformBufferUpdateAfterBind in the
		//VK_EXT_descriptor_indexing extension has significantly smaller support than the other
		//types, thus we will not be able to update descriptor bindings concurrently with the
		//constant buffer bind group type 
		VkDescriptorBindingFlagsEXT BindingFlag = Binding->Type == GDI_BIND_GROUP_TYPE_CONSTANT_BUFFER ? 0 : VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
		BindingFlags[i] = BindingFlag;
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
	VkResult Status = vkCreateDescriptorSetLayout(Context->Device, &CreateInfo, VK_Get_Allocator(), &SetLayout);
	Scratch_Release();
	
	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vkCreateDescriptorSetLayout failed!");
		return GDI_Null_Handle();
	}

	VK_Set_Debug_Name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "bind group layout", SetLayout, BindGroupLayoutInfo->DebugName);

	gdi_handle Result = VK_Bind_Group_Layout_Pool_Allocate(&Context->ResourcePool);
	vk_bind_group_layout* Layout = VK_Bind_Group_Layout_Pool_Get(&Context->ResourcePool, Result);
	Layout->Layout = SetLayout;

	size_t AllocationSize = ImmutableSamplerCount*sizeof(gdi_handle) + BindGroupLayoutInfo->Bindings.Count*sizeof(gdi_bind_group_binding);
	Layout->AllocationData = Allocator_Allocate_Memory(Default_Allocator_Get(), AllocationSize);

	Layout->Bindings.Ptr = (gdi_bind_group_binding*)Layout->AllocationData;
	Layout->Bindings.Count = BindGroupLayoutInfo->Bindings.Count;

	gdi_handle* StaticSamplersAt = (gdi_handle*)(Layout->Bindings.Ptr + Layout->Bindings.Count);
	for (size_t i = 0; i < BindGroupLayoutInfo->Bindings.Count; i++) {
		gdi_bind_group_binding* Binding = BindGroupLayoutInfo->Bindings.Ptr + i;
		Layout->Bindings.Ptr[i] = *Binding;
		if(Binding->StaticSamplers) {
			Layout->Bindings.Ptr[i].StaticSamplers = StaticSamplersAt;
			StaticSamplersAt += Layout->Bindings.Ptr[i].Count;
		}
	}
	Assert(((size_t)StaticSamplersAt-(size_t)Layout->AllocationData) == AllocationSize);

	return Result;
}

function GDI_BACKEND_DELETE_BIND_GROUP_LAYOUT_DEFINE(VK_Delete_Bind_Group_Layout) {
	VK_Bind_Group_Layout_Add_To_Delete_Queue((vk_device_context*)GDI->DeviceContext, BindGroupLayout);
}

function GDI_BACKEND_UPDATE_BIND_GROUPS_DEFINE(VK_Update_Bind_Groups);
function GDI_BACKEND_CREATE_BIND_GROUP_DEFINE(VK_Create_Bind_Group) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	vk_bind_group_layout* Layout = VK_Bind_Group_Layout_Pool_Get(&Context->ResourcePool, BindGroupInfo->Layout);
	Assert(Layout);

	arena* Scratch = Scratch_Get();
	VkDescriptorSetLayout* Layouts = Arena_Push_Array(Scratch, Context->Frames.Count, VkDescriptorSetLayout);
	for (size_t i = 0; i < Context->Frames.Count; i++) {
		Layouts[i] = Layout->Layout;
	}

	VkDescriptorSetAllocateInfo AllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = Context->DescriptorPool,
		.descriptorSetCount = (u32)Context->Frames.Count,
		.pSetLayouts = Layouts
	};

	VkDescriptorSet* Sets = Arena_Push_Array(Scratch, Context->Frames.Count, VkDescriptorSet);

	OS_Mutex_Lock(Context->DescriptorLock);
	VkResult Status = vkAllocateDescriptorSets(Context->Device, &AllocateInfo, Sets);
	OS_Mutex_Unlock(Context->DescriptorLock);

	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vkAllocateDescriptorSets failed!");
		Scratch_Release();
		return GDI_Null_Handle();
	}

#ifdef DEBUG_BUILD
	if (!String_Is_Empty(BindGroupInfo->DebugName)) {
		arena* Scratch = Scratch_Get();
		for (size_t i = 0; i < Context->Frames.Count; i++) {
			string DebugName = String_Format((allocator*)Scratch, "%.*s %d", BindGroupInfo->DebugName.Size, BindGroupInfo->DebugName.Ptr, i);
			VK_Set_Debug_Name(VK_OBJECT_TYPE_DESCRIPTOR_SET, "bind group", Sets[i], DebugName);
		}
		Scratch_Release();
	}
#endif

	gdi_handle Result = VK_Bind_Group_Pool_Allocate(&Context->ResourcePool);
	vk_bind_group* BindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, Result);

	BindGroup->Layout = BindGroupInfo->Layout;
	
	size_t AllocationSize = sizeof(VkDescriptorSet)*Context->Frames.Count + sizeof(vk_bind_group_binding)*Layout->Bindings.Count;
	BindGroup->AllocationData = Allocator_Allocate_Memory(Default_Allocator_Get(), AllocationSize);
	BindGroup->Sets = (VkDescriptorSet*)BindGroup->AllocationData;
	BindGroup->Bindings = (vk_bind_group_binding*)(BindGroup->Sets + Context->Frames.Count);

	Memory_Copy(BindGroup->Sets, Sets, sizeof(VkDescriptorSet)*Context->Frames.Count);

	Scratch_Release();

	if (BindGroupInfo->Writes.Count || BindGroupInfo->Copies.Count) {
		//Make sure to add the bind group into the destinations for the updates
		for (size_t i = 0; i < BindGroupInfo->Writes.Count; i++) {
			BindGroupInfo->Writes.Ptr[i].DstBindGroup = Result;
		}

		for (size_t i = 0; i < BindGroupInfo->Copies.Count; i++) {
			BindGroupInfo->Copies.Ptr[i].DstBindGroup = Result;
		}

		VK_Update_Bind_Groups(GDI, BindGroupInfo->Writes, BindGroupInfo->Copies);
	}

	return Result;
}

function GDI_BACKEND_DELETE_BIND_GROUP_DEFINE(VK_Delete_Bind_Group) {
	VK_Bind_Group_Add_To_Delete_Queue((vk_device_context*)GDI->DeviceContext, BindGroup);
}

function GDI_BACKEND_UPDATE_BIND_GROUPS_DEFINE(VK_Update_Bind_Groups) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	arena* Scratch = Scratch_Get();

	dynamic_vk_write_descriptor_set_array DescriptorWrites = Dynamic_VK_Write_Descriptor_Set_Array_Init((allocator*)Scratch);
	dynamic_vk_copy_descriptor_set_array DescriptorCopies = Dynamic_VK_Copy_Descriptor_Set_Array_Init((allocator*)Scratch);

	//For both writes and copies, make sure all dst bind groups are 
	//recorded in the thread contexts update cmd list so the remaining
	//frame descriptor sets are updated
	vk_frame_context* Frame = VK_Lock_Frame(Context);
	vk_frame_thread_context* ThreadContext = VK_Get_Frame_Thread_Context(Context, Frame);

	//Start with writes
	for (size_t i = 0; i < Writes.Count; i++) {
		gdi_bind_group_write* Write = Writes.Ptr + i;
		vk_bind_group* DstBindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, Write->DstBindGroup);
		vk_bind_group_layout* Layout = VK_Bind_Group_Layout_Pool_Get(&Context->ResourcePool, DstBindGroup->Layout);

		if (DstBindGroup && Layout) {
			Assert(Write->DstBinding < Layout->Bindings.Count);
			gdi_bind_group_binding* Binding = Layout->Bindings.Ptr + Write->DstBinding;

			VkWriteDescriptorSet DescriptorWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = DstBindGroup->Sets[Frame->Index],
				.dstBinding = Write->DstBinding,
				.dstArrayElement = Write->DstIndex,
				.descriptorType = VK_Get_Descriptor_Type(Binding->Type)
			};

			vk_bind_group_dynamic_descriptor* FirstDynamicDescriptor = NULL;
			vk_bind_group_dynamic_descriptor* LastDynamicDescriptor = NULL;

			if (Binding->Type == GDI_BIND_GROUP_TYPE_SAMPLER) {
				Assert(!Write->TextureViews.Count && !Write->Buffers.Count);
				Assert(Write->DstIndex + Write->Samplers.Count <= Binding->Count);

				VkDescriptorImageInfo* ImageInfos = Arena_Push_Array(Scratch, Write->Samplers.Count, VkDescriptorImageInfo);
				for (size_t i = 0; i < Write->Samplers.Count; i++) {
					vk_sampler* Sampler = VK_Sampler_Pool_Get(&Context->ResourcePool, Write->Samplers.Ptr[i]);
					VkDescriptorImageInfo ImageInfo = {
						.sampler = Sampler ? Sampler->Sampler : VK_NULL_HANDLE
					};
					ImageInfos[DescriptorWrite.descriptorCount++] = ImageInfo;
				}
				DescriptorWrite.pImageInfo = ImageInfos;

			} else if (GDI_Is_Bind_Group_Type_Texture(Binding->Type)) {
				Assert(!Write->Samplers.Count && !Write->Buffers.Count);
				Assert(Write->DstIndex + Write->TextureViews.Count <= Binding->Count);

				VkDescriptorImageInfo* ImageInfos = Arena_Push_Array(Scratch, Write->TextureViews.Count, VkDescriptorImageInfo);
				for (size_t i = 0; i < Write->TextureViews.Count; i++) {
					vk_texture_view* TextureView = VK_Texture_View_Pool_Get(&Context->ResourcePool, Write->TextureViews.Ptr[i]);
					VkDescriptorImageInfo ImageInfo = {
						.imageView = TextureView ? TextureView->ImageView : VK_NULL_HANDLE,
						.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					};
					ImageInfos[DescriptorWrite.descriptorCount++] = ImageInfo;
				}
				DescriptorWrite.pImageInfo = ImageInfos;

			} else if (GDI_Is_Bind_Group_Type_Buffer(Binding->Type)) {
				Assert(!Write->Samplers.Count && !Write->TextureViews.Count);
				Assert(Write->DstIndex + Write->Buffers.Count <= Binding->Count);

				VkDescriptorBufferInfo* BufferInfos = Arena_Push_Array(Scratch, Write->Buffers.Count, VkDescriptorBufferInfo);
				for (size_t i = 0; i < Write->Buffers.Count; i++) {
					gdi_bind_group_buffer BindGroupBuffer = Write->Buffers.Ptr[i];
					vk_buffer* Buffer = VK_Buffer_Pool_Get(&Context->ResourcePool, BindGroupBuffer.Buffer);

					size_t Offset = BindGroupBuffer.Offset;
					if (Buffer->Usage & GDI_BUFFER_USAGE_DYNAMIC) {
						Offset += Buffer->Size * Frame->Index;
					}

					size_t Size = BindGroupBuffer.Size;
					if (Size == 0) {
						Size = Buffer->Size - BindGroupBuffer.Offset;
					}

					VkDescriptorBufferInfo BufferInfo = {
						.buffer = Buffer ? Buffer->Buffer : VK_NULL_HANDLE,
						.offset = Offset,
						.range = Size
					};

					if (Buffer->Usage & GDI_BUFFER_USAGE_DYNAMIC) {
						vk_bind_group_dynamic_descriptor* DynamicDescriptor = Arena_Push_Struct(ThreadContext->TempArena, vk_bind_group_dynamic_descriptor);
						DynamicDescriptor->Buffer = BindGroupBuffer.Buffer;
						DynamicDescriptor->Index = DescriptorWrite.dstArrayElement+DescriptorWrite.descriptorCount;
						DynamicDescriptor->Offset = BindGroupBuffer.Offset;
						DynamicDescriptor->Size = BindGroupBuffer.Size;
						SLL_Push_Back(FirstDynamicDescriptor, LastDynamicDescriptor, DynamicDescriptor);
					}

					BufferInfos[DescriptorWrite.descriptorCount++] = BufferInfo;
				}
				DescriptorWrite.pBufferInfo = BufferInfos;

			} Invalid_Else;

			Dynamic_VK_Write_Descriptor_Set_Array_Add(&DescriptorWrites, DescriptorWrite);
			
			vk_bind_group_write_cmd* WriteCmd = Arena_Push_Struct(ThreadContext->TempArena, vk_bind_group_write_cmd);
			WriteCmd->BindGroup = Write->DstBindGroup;
			WriteCmd->Binding = Write->DstBinding;
			WriteCmd->FirstDynamicDescriptor = FirstDynamicDescriptor;
			WriteCmd->LastDynamicDescriptor  = LastDynamicDescriptor;
			SLL_Push_Back(ThreadContext->FirstBindGroupWriteCmd, ThreadContext->LastBindGroupWriteCmd, WriteCmd);
		}
	}

	//Now copies
	for (size_t i = 0; i < Copies.Count; i++) {
		gdi_bind_group_copy* Copy = Copies.Ptr + i;
		vk_bind_group* DstBindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, Copy->DstBindGroup);
		vk_bind_group* SrcBindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, Copy->SrcBindGroup);

		if (DstBindGroup && SrcBindGroup) {
			vk_bind_group_layout* DstLayout = VK_Bind_Group_Layout_Pool_Get(&Context->ResourcePool, DstBindGroup->Layout);
			vk_bind_group_layout* SrcLayout = VK_Bind_Group_Layout_Pool_Get(&Context->ResourcePool, SrcBindGroup->Layout);

			if (DstLayout && SrcLayout) {
				Assert(Copy->DstBinding < DstLayout->Bindings.Count);
				Assert(Copy->SrcBinding < SrcLayout->Bindings.Count);

				gdi_bind_group_binding* DstBinding = DstLayout->Bindings.Ptr + Copy->DstBinding;
				gdi_bind_group_binding* SrcBinding = SrcLayout->Bindings.Ptr + Copy->SrcBinding;

				Assert(DstBinding->Type == SrcBinding->Type);

				Assert(Copy->SrcIndex + Copy->Count <= SrcBinding->Count);
				Assert(Copy->DstIndex + Copy->Count <= DstBinding->Count);

				VkCopyDescriptorSet DescriptorCopy = {
					.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
					.srcSet = SrcBindGroup->Sets[Frame->Index],
					.srcBinding = Copy->SrcBinding,
					.srcArrayElement = Copy->SrcIndex,
					.dstSet = DstBindGroup->Sets[Frame->Index],
					.dstBinding = Copy->DstBinding,
					.dstArrayElement = Copy->DstIndex,
					.descriptorCount = Copy->Count
				};
				
				Dynamic_VK_Copy_Descriptor_Set_Array_Add(&DescriptorCopies, DescriptorCopy);

				vk_bind_group_copy_cmd* CopyCmd = Arena_Push_Struct(ThreadContext->TempArena, vk_bind_group_copy_cmd);
				CopyCmd->DstBindGroup = Copy->DstBindGroup;
				CopyCmd->DstBinding   = Copy->DstBinding;
				CopyCmd->DstIndex     = Copy->DstIndex;
				CopyCmd->SrcBindGroup = Copy->SrcBindGroup;
				CopyCmd->SrcBinding   = Copy->SrcBinding;
				CopyCmd->SrcIndex     = Copy->SrcIndex;
				CopyCmd->Count        = Copy->Count;
				SLL_Push_Back(ThreadContext->FirstBindGroupCopyCmd, ThreadContext->LastBindGroupCopyCmd, CopyCmd);
			}
		}
	}

	if (DescriptorWrites.Count || DescriptorCopies.Count) {
		vkUpdateDescriptorSets(Context->Device, (u32)DescriptorWrites.Count, DescriptorWrites.Ptr, (u32)DescriptorCopies.Count, DescriptorCopies.Ptr);
	}
	Scratch_Release();

	VK_Unlock_Frame(Context);
}
 
function GDI_BACKEND_CREATE_SHADER_DEFINE(VK_Create_Shader) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

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
		VkResult Status = vkCreateDescriptorSetLayout(Context->Device, &CreateInfo, VK_Get_Allocator(), &SetLayout);
		Scratch_Release();

		if (Status != VK_SUCCESS) {
			GDI_Log_Error("vkCreateDescriptorSetLayout failed!");
			return GDI_Null_Handle();
		}

		WritableLayout = SetLayout;

		LayoutCount++;
	}

	LayoutCount += (u32)ShaderInfo->BindGroupLayouts.Count;
	Assert(LayoutCount <= GDI_MAX_BIND_GROUP_COUNT);

	VkDescriptorSetLayout* SetLayouts = Arena_Push_Array(Scratch, LayoutCount, VkDescriptorSetLayout);
	
	size_t LayoutOffset = 0;

	VkShaderStageFlags StageFlags = VK_SHADER_STAGE_ALL;
	if (!Buffer_Is_Empty(ShaderInfo->CS)) {
		SetLayouts[0] = WritableLayout;
		LayoutOffset = 1;
		StageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	}

	VkPushConstantRange PushRange = {
		.stageFlags = StageFlags,
		.size = (u32)(ShaderInfo->PushConstantCount * sizeof(u32))
	};

	for (size_t i = 0; i < ShaderInfo->BindGroupLayouts.Count; i++) {
		vk_bind_group_layout* Layout = VK_Bind_Group_Layout_Pool_Get(&Context->ResourcePool, ShaderInfo->BindGroupLayouts.Ptr[i]);
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
	if (vkCreatePipelineLayout(Context->Device, &LayoutCreateInfo, VK_Get_Allocator(), &PipelineLayout) == VK_SUCCESS) {
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
			vkCreateShaderModule(Context->Device, &VSModuleInfo, VK_Get_Allocator(), &VSModule);
			vkCreateShaderModule(Context->Device, &PSModuleInfo, VK_Get_Allocator(), &PSModule);

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
				.polygonMode = ShaderInfo->IsWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,
				.cullMode = VK_Get_Cull_Mode(ShaderInfo->CullMode),
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
			if (vkCreateGraphicsPipelines(Context->Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo, VK_Get_Allocator(), &Pipeline) == VK_SUCCESS) {
				Result = VK_Shader_Pool_Allocate(&Context->ResourcePool);
				vk_shader* Shader = VK_Shader_Pool_Get(&Context->ResourcePool, Result);
				Shader->BindGroupCount = (u32)ShaderInfo->BindGroupLayouts.Count;
				Shader->Layout = PipelineLayout;
				Shader->Pipeline = Pipeline;

#ifdef DEBUG_BUILD
				Shader->DEBUGType = GDI_PASS_TYPE_RENDER;
#endif
			} else {
				GDI_Log_Error("vkCreateGraphicsPipelines failed!");
				vkDestroyPipelineLayout(Context->Device, PipelineLayout, VK_Get_Allocator());
			}
			
			vkDestroyShaderModule(Context->Device, VSModule, VK_Get_Allocator());
			vkDestroyShaderModule(Context->Device, PSModule, VK_Get_Allocator());
		} else {
			Assert(Buffer_Is_Empty(ShaderInfo->VS) && Buffer_Is_Empty(ShaderInfo->PS));

			VkShaderModuleCreateInfo CSModuleInfo = {
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = ShaderInfo->CS.Size,
				.pCode = (const u32*)ShaderInfo->CS.Ptr
			};

			VkShaderModule CSModule;
			vkCreateShaderModule(Context->Device, &CSModuleInfo, VK_Get_Allocator(), &CSModule);

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
			if (vkCreateComputePipelines(Context->Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo, VK_Get_Allocator(), &Pipeline) == VK_SUCCESS) {
				Result = VK_Shader_Pool_Allocate(&Context->ResourcePool);
				vk_shader* Shader = VK_Shader_Pool_Get(&Context->ResourcePool, Result);
				Shader->Layout = PipelineLayout;
				Shader->Pipeline = Pipeline;
				Shader->WritableBindings = GDI_Bind_Group_Binding_Array_Copy(Default_Allocator_Get(), ShaderInfo->WritableBindings.Ptr, ShaderInfo->WritableBindings.Count);
				Shader->WritableLayout = WritableLayout;

#ifdef DEBUG_BUILD
				Shader->DEBUGType = GDI_PASS_TYPE_COMPUTE;
#endif
			} else {
				GDI_Log_Error("vkCreateComputePipelines failed!");
				vkDestroyPipelineLayout(Context->Device, PipelineLayout, VK_Get_Allocator());
				if (WritableLayout) {
					vkDestroyDescriptorSetLayout(Context->Device, WritableLayout, VK_Get_Allocator());
				}
			}

			vkDestroyShaderModule(Context->Device, CSModule, VK_Get_Allocator());
		}
	} else {
		GDI_Log_Error("vkCreatePipelineLayout failed!");
		
		if (WritableLayout) {
			vkDestroyDescriptorSetLayout(Context->Device, WritableLayout, VK_Get_Allocator());
		}
	}

	Scratch_Release();

	vk_shader* Shader = VK_Shader_Pool_Get(&Context->ResourcePool, Result);
	if (Shader) {
		VK_Set_Debug_Name(VK_OBJECT_TYPE_PIPELINE, "shader", Shader->Pipeline, ShaderInfo->DebugName);
	}

	return Result;
}

function GDI_BACKEND_DELETE_SHADER_DEFINE(VK_Delete_Shader) {
	VK_Shader_Add_To_Delete_Queue((vk_device_context*)GDI->DeviceContext, Shader);
}

function GDI_BACKEND_CREATE_SWAPCHAIN_DEFINE(VK_Create_Swapchain) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;
	gdi_handle Result = VK_Swapchain_Pool_Allocate(&Context->ResourcePool);
	vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&Context->ResourcePool, Result);
	Swapchain->Handle = Result;

	//VkResult Status;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	Swapchain->Surface = VK_Create_Surface((vk_gdi*)GDI, SwapchainInfo->Window, SwapchainInfo->Instance);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	Swapchain->Surface = VK_Create_Surface((vk_gdi*)GDI, SwapchainInfo->Layer);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	Swapchain->Surface = VK_Create_Surface((vk_gdi*)GDI, SwapchainInfo->Display, SwapchainInfo->Window);
#else
#error "Not Implemented!"
#endif

	if (!Swapchain->Surface) {
		goto error;
	}

	VkSurfaceCapabilitiesKHR SurfaceCaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Context->GPU->PhysicalDevice, Swapchain->Surface, &SurfaceCaps);

	if (SurfaceCaps.minImageCount < 2) {
		GDI_Log_Error("Surface must have two or more images! Found %d", SurfaceCaps.maxImageCount);
		goto error;
	}

	{
		Swapchain->ImageCount = SurfaceCaps.minImageCount;
		Swapchain->ImageIndex = -1;
		size_t AllocationSize = Swapchain->ImageCount*(sizeof(VkImage) + sizeof(gdi_handle) + sizeof(gdi_handle)) + Context->Frames.Count*sizeof(vk_semaphore);
		Swapchain->Images = (VkImage*)Allocator_Allocate_Memory(Default_Allocator_Get(), AllocationSize);
		Swapchain->Textures = (gdi_handle*)(Swapchain->Images + Swapchain->ImageCount);
		Swapchain->TextureViews = (gdi_handle*)(Swapchain->Textures + Swapchain->ImageCount);
		Swapchain->Locks = (vk_semaphore*)(Swapchain->TextureViews + Swapchain->ImageCount);

		arena* Scratch = Scratch_Get();

		VkFormat TargetFormat = VK_Get_Format(SwapchainInfo->Format);

		u32 SurfaceFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(Context->GPU->PhysicalDevice, Swapchain->Surface, &SurfaceFormatCount, VK_NULL_HANDLE);

		VkSurfaceFormatKHR* SurfaceFormats = Arena_Push_Array(Scratch, SurfaceFormatCount, VkSurfaceFormatKHR);
		vkGetPhysicalDeviceSurfaceFormatsKHR(Context->GPU->PhysicalDevice, Swapchain->Surface, &SurfaceFormatCount, SurfaceFormats);

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

	VK_Recreate_Swapchain(Context, Swapchain);

	return Result;

error:
	VK_Delete_Swapchain_Resources(Context, Swapchain);
	VK_Swapchain_Pool_Free(&Context->ResourcePool, Result);
	return GDI_Null_Handle();
}

function GDI_BACKEND_DELETE_SWAPCHAIN_DEFINE(VK_Delete_Swapchain) {
	VK_Swapchain_Add_To_Delete_Queue((vk_device_context*)GDI->DeviceContext, Swapchain);
}

function GDI_BACKEND_RESIZE_SWAPCHAIN_DEFINE(VK_Resize_Swapchain);
function GDI_BACKEND_GET_SWAPCHAIN_VIEW_DEFINE(VK_Get_Swapchain_View) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;
	gdi_handle Result = GDI_Null_Handle();
	vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&Context->ResourcePool, SwapchainHandle);
	if (Swapchain && Swapchain->Dim.x != 0 && Swapchain->Dim.y != 0) {
		if (Swapchain->ImageIndex == -1) {
			vk_frame_context* Frame = VK_Lock_Frame(Context);
			VkResult Status = vkAcquireNextImageKHR(Context->Device, Swapchain->Swapchain, UINT64_MAX,
													Swapchain->Locks[Frame->Index].Handle, VK_NULL_HANDLE, (u32*)&Swapchain->ImageIndex);
			VK_Unlock_Frame(Context);

			if (Status == VK_ERROR_OUT_OF_DATE_KHR || Status == VK_SUBOPTIMAL_KHR) {
				VK_Resize_Swapchain(GDI, SwapchainHandle);

				Frame = VK_Lock_Frame(Context);
				Status = vkAcquireNextImageKHR(Context->Device, Swapchain->Swapchain, UINT64_MAX,
											   Swapchain->Locks[Frame->Index].Handle, VK_NULL_HANDLE, (u32*)&Swapchain->ImageIndex);
				VK_Unlock_Frame(Context);
			}

			if (Status != VK_SUCCESS) {
				GDI_Log_Error("vkAcquireNextImageKHR failed");
				return Result;
			}
		}
		
		Result = Swapchain->TextureViews[Swapchain->ImageIndex];
	}
	return Result;
}

function GDI_BACKEND_RESIZE_SWAPCHAIN_DEFINE(VK_Resize_Swapchain) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;
	vk_swapchain* VkSwapchain = VK_Swapchain_Pool_Get(&Context->ResourcePool, Swapchain);
	if (VkSwapchain) {
		VK_Recreate_Swapchain(Context, VkSwapchain);
	}
}

function GDI_BACKEND_GET_SWAPCHAIN_INFO_DEFINE(VK_Get_Swapchain_Info) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	gdi_swapchain_info Result;
	Memory_Clear(&Result, sizeof(gdi_swapchain_info));

	vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&Context->ResourcePool, SwapchainHandle);
	if (Swapchain) {
		Result.Format = Swapchain->Format;
		Result.Dim = Swapchain->Dim;
	}
	return Result;
}

 
function GDI_BACKEND_BEGIN_RENDER_PASS_DEFINE(VK_Begin_Render_Pass) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;
	
	b32 HasDim = false;
	v2i RenderPassDim = V2i_Zero();
	for (size_t i = 0; i < GDI_MAX_RENDER_TARGET_COUNT; i++) {
		vk_texture_view* View = VK_Texture_View_Pool_Get(&Context->ResourcePool, BeginInfo->RenderTargetViews[i]);
		if (View) {
			if (!HasDim) {
				RenderPassDim = View->Dim;
				HasDim = true;
			} else {
				Assert(RenderPassDim.x == View->Dim.x && RenderPassDim.y == View->Dim.y);
			}
		}
	}

	vk_texture_view* DepthView = VK_Texture_View_Pool_Get(&Context->ResourcePool, BeginInfo->DepthBufferView);
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

	//Doesn't need to acuqire the frame lock since BeginRenderPass() and EndRenderPass()
	//cannot run concurrently with Render()
	u64 FrameIndex = Atomic_Load_U64(&Context->FrameCount);
	vk_frame_context* FrameContext = Context->Frames.Ptr + (FrameIndex % Context->Frames.Count);
	vk_frame_thread_context* ThreadContext = VK_Get_Frame_Thread_Context(Context, FrameContext);

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

		vkAllocateCommandBuffers(Context->Device, &CmdBufferInfo, &RenderPass->CmdBuffer);
	}
	
	Memory_Copy(RenderPass->ClearColors, BeginInfo->ClearColors, sizeof(BeginInfo->ClearColors));
	RenderPass->ClearDepth = BeginInfo->ClearDepth;
	RenderPass->Dim = RenderPassDim;
	Memory_Copy(RenderPass->RenderTargetViews, BeginInfo->RenderTargetViews, sizeof(BeginInfo->RenderTargetViews));
	RenderPass->DepthBufferView = BeginInfo->DepthBufferView;

	return (gdi_render_pass*)RenderPass;
}

function GDI_BACKEND_END_RENDER_PASS_DEFINE(VK_End_Render_Pass) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	u64 FrameIndex = Atomic_Load_U64(&Context->FrameCount);
	vk_frame_context* Frame = Context->Frames.Ptr + (FrameIndex % Context->Frames.Count);
	vk_frame_thread_context* ThreadContext = VK_Get_Frame_Thread_Context(Context, Frame);

	vk_render_pass* VkRenderPass = (vk_render_pass*)RenderPass;

	u32 ColorFormatCount = 0;
	VkFormat ColorFormats[GDI_MAX_RENDER_TARGET_COUNT];
	Memory_Clear(ColorFormats, sizeof(ColorFormats));

	VkFormat DepthFormat = VK_FORMAT_UNDEFINED;

	for (u32 i = 0; i < GDI_MAX_RENDER_TARGET_COUNT; i++) {
		vk_texture_view* View = VK_Texture_View_Pool_Get(&Context->ResourcePool, VkRenderPass->RenderTargetViews[i]);
		if (View) {
			ColorFormats[ColorFormatCount++] = VK_Get_Format(View->Format);
		}
	}

	vk_texture_view* DepthView = VK_Texture_View_Pool_Get(&Context->ResourcePool, VkRenderPass->DepthBufferView);
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
	gdi_draw_type CurrentDrawType = GDI_DRAW_TYPE_NONE;
	u32 CurrentPrimitiveCount = 0;
	u32 CurrentPrimitiveOffset = 0;
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
			CurrentShader = VK_Shader_Pool_Get(&Context->ResourcePool, Handle);
			Assert(CurrentShader->DEBUGType == GDI_PASS_TYPE_RENDER);
			vkCmdBindPipeline(VkRenderPass->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, CurrentShader->Pipeline);
			

		}

		for (size_t i = 0; i < GDI_MAX_BIND_GROUP_COUNT; i++) {
			vk_bind_group* BindGroup = NULL;
			if (DirtyFlag & (GDI_RENDER_PASS_BIND_GROUP_BIT << i)) {
				gdi_handle Handle = BStream_Reader_Struct(&Reader, gdi_handle);
				BindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, Handle);
			} else {
				//If the shader changed make sure to update the bind group to the last one bound
				if ((DirtyFlag & GDI_RENDER_PASS_SHADER_BIT) && (i < CurrentShader->BindGroupCount)) {
					BindGroup = BindGroups[i];
				}
			}

			if (BindGroup) {
				vkCmdBindDescriptorSets(VkRenderPass->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, CurrentShader->Layout, (u32)i, 1, &BindGroup->Sets[Frame->Index], 0, NULL);
				BindGroups[i] = BindGroup;
			}
		}

		for (size_t i = 0; i < GDI_MAX_VTX_BUFFER_COUNT; i++) {
			if (DirtyFlag & (GDI_RENDER_PASS_VTX_BUFFER_BIT << i)) {
				gdi_handle Handle = BStream_Reader_Struct(&Reader, gdi_handle);
				vk_buffer* VtxBuffer = VK_Buffer_Pool_Get(&Context->ResourcePool, Handle);
				
				Assert(VtxBuffer->Usage & GDI_BUFFER_USAGE_VTX);

				VkDeviceSize Offset = 0;
				if (VtxBuffer->Usage & GDI_BUFFER_USAGE_DYNAMIC) {
					Offset = Frame->Index * VtxBuffer->Size;
				}
				
				vkCmdBindVertexBuffers(VkRenderPass->CmdBuffer, (u32)i, 1, &VtxBuffer->Buffer, &Offset);
			}
		}

		if ((DirtyFlag & GDI_RENDER_PASS_IDX_BUFFER_BIT) || (DirtyFlag & GDI_RENDER_PASS_IDX_FORMAT_BIT)) {
			if (DirtyFlag & GDI_RENDER_PASS_IDX_BUFFER_BIT) {
				gdi_handle Handle = BStream_Reader_Struct(&Reader, gdi_handle);
				CurrentIdxBuffer = VK_Buffer_Pool_Get(&Context->ResourcePool, Handle);
			}

			if (DirtyFlag & GDI_RENDER_PASS_IDX_FORMAT_BIT) {
				CurrentIdxFormat = (gdi_idx_format)BStream_Reader_U32(&Reader);
			}

			Assert(CurrentIdxBuffer->Usage & GDI_BUFFER_USAGE_IDX);

			VkDeviceSize Offset = 0;
			if (CurrentIdxBuffer->Usage & GDI_BUFFER_USAGE_DYNAMIC) {
				Offset = Frame->Index * CurrentIdxBuffer->Size;
			}

			vkCmdBindIndexBuffer(VkRenderPass->CmdBuffer, CurrentIdxBuffer->Buffer, Offset, VK_Get_Idx_Type(CurrentIdxFormat));
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

		if (DirtyFlag & GDI_RENDER_PASS_DRAW_TYPE_BIT) {
			CurrentDrawType = (gdi_draw_type)BStream_Reader_U32(&Reader);
		}

		if (DirtyFlag & GDI_RENDER_PASS_PRIMITIVE_COUNT_BIT) {
			CurrentPrimitiveCount = BStream_Reader_U32(&Reader);
		}

		if (DirtyFlag & GDI_RENDER_PASS_PRIMITIVE_OFFSET_BIT) {
			CurrentPrimitiveOffset = BStream_Reader_U32(&Reader);
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

		switch(CurrentDrawType) {
			case GDI_DRAW_TYPE_IDX: {
				vkCmdDrawIndexed(VkRenderPass->CmdBuffer, CurrentPrimitiveCount, 1, CurrentPrimitiveOffset, CurrentVtxOffset, 0);
			} break;

			case GDI_DRAW_TYPE_VTX: {
				vkCmdDraw(VkRenderPass->CmdBuffer, CurrentPrimitiveCount, 1, CurrentPrimitiveOffset, 0);
			} break;
		}
	}
	Assert(Reader.At == Reader.End);

	vkEndCommandBuffer(VkRenderPass->CmdBuffer);

	Memory_Clear(&RenderPass->PrevState, sizeof(gdi_draw_state));
	Memory_Clear(&RenderPass->CurrentState, sizeof(gdi_draw_state));
	RenderPass->Offset = 0;
	SLL_Push_Front(ThreadContext->RenderPassesToDelete, VkRenderPass);
}

function void VK_Update_Frame_Bind_Groups(vk_device_context* Context, u64 OldFrameIndex) {
	arena* Scratch = Scratch_Get();

	vk_frame_context* OldFrame = Context->Frames.Ptr + (OldFrameIndex % Context->Frames.Count);

	//Accumulate all the bind group copies and writes from all the old frame threads 
	//and add them to the bind group bindings to write them into the next frames descriptor sets
	for (vk_frame_thread_context* Thread = (vk_frame_thread_context*)Atomic_Load_Ptr(&OldFrame->TopThread); 
		Thread; Thread = Thread->Next) {
		
		if (!Thread->NeedsReset) {
			for (vk_bind_group_write_cmd* ThreadWriteCmd = Thread->FirstBindGroupWriteCmd;
				ThreadWriteCmd; ThreadWriteCmd = ThreadWriteCmd->Next) {
				vk_bind_group* DstBindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, ThreadWriteCmd->BindGroup);
				if (DstBindGroup) {
					DstBindGroup->ShouldUpdate = true;
					DstBindGroup->LastUpdatedFrame = OldFrameIndex;

					if (ThreadWriteCmd->FirstDynamicDescriptor) {
						vk_bind_group_write_cmd* BindGroupWriteCmd = Arena_Push_Struct(OldFrame->TempArena, vk_bind_group_write_cmd);
						BindGroupWriteCmd->LastUpdatedFrame = OldFrameIndex;
						
						for (vk_bind_group_dynamic_descriptor* ThreadDescriptor = ThreadWriteCmd->FirstDynamicDescriptor;
							ThreadDescriptor; ThreadDescriptor = ThreadDescriptor->Next) {
						
							vk_bind_group_dynamic_descriptor* BindGroupThreadDescriptor = Arena_Push_Struct(OldFrame->TempArena, vk_bind_group_dynamic_descriptor);
							*BindGroupThreadDescriptor= *ThreadDescriptor;
							BindGroupThreadDescriptor->Next = NULL;

							SLL_Push_Back(BindGroupWriteCmd->FirstDynamicDescriptor, BindGroupWriteCmd->LastDynamicDescriptor, BindGroupThreadDescriptor);
						}

						SLL_Push_Back(DstBindGroup->Bindings[ThreadWriteCmd->Binding].FirstBindGroupWriteCmd,
									  DstBindGroup->Bindings[ThreadWriteCmd->Binding].LastBindGroupWriteCmd,
									  BindGroupWriteCmd);
					}
				}
			}

			for (vk_bind_group_copy_cmd* ThreadCopyCmd = Thread->FirstBindGroupCopyCmd;
				ThreadCopyCmd; ThreadCopyCmd = ThreadCopyCmd->Next) {
				
				vk_bind_group* DstBindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, ThreadCopyCmd->DstBindGroup);
				vk_bind_group* SrcBindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, ThreadCopyCmd->SrcBindGroup);

				if (DstBindGroup && SrcBindGroup) {
					vk_bind_group_copy_cmd* BindGroupCopyCmd = Arena_Push_Struct(OldFrame->TempArena, vk_bind_group_copy_cmd);
					DstBindGroup->ShouldUpdate = true;
					DstBindGroup->LastUpdatedFrame = OldFrameIndex;

					*BindGroupCopyCmd = *ThreadCopyCmd;
					BindGroupCopyCmd->Next = NULL;
					BindGroupCopyCmd->LastUpdatedFrame = OldFrameIndex;
					
					SLL_Push_Back(DstBindGroup->Bindings[ThreadCopyCmd->DstBinding].FirstBindGroupCopyCmd,
								  DstBindGroup->Bindings[ThreadCopyCmd->DstBinding].LastBindGroupCopyCmd,
								  BindGroupCopyCmd);
				}
			}
		}
	}

	//Now write the new frames bind groups
	u64 NewFrameIndex = OldFrameIndex + 1;
	vk_frame_context* NewFrame = Context->Frames.Ptr + (NewFrameIndex % Context->Frames.Count);

	for (size_t i = 0; i < Array_Count(Context->ResourcePool.Bind_GroupPool.Entries); i++) {
		vk_bind_group* BindGroup = Context->ResourcePool.Bind_GroupPool.Entries + i;
		
		if (BindGroup->ShouldUpdate) {
			u64 FrameDiff = NewFrameIndex - BindGroup->LastUpdatedFrame;
			vk_bind_group_layout* Layout = VK_Bind_Group_Layout_Pool_Get(&Context->ResourcePool, BindGroup->Layout);

			if (FrameDiff >= Context->Frames.Count || !Layout) {
				BindGroup->ShouldUpdate = false;

				if (Layout) {
					for (size_t i = 0; i < Layout->Bindings.Count; i++) {
						vk_bind_group_binding* Binding = BindGroup->Bindings + i;
						Binding->FirstBindGroupWriteCmd = NULL;
						Binding->LastBindGroupWriteCmd = NULL;
						Binding->FirstBindGroupCopyCmd = NULL;
						Binding->LastBindGroupCopyCmd = NULL;
					}
				}

			} else {
				for (size_t i = 0; i < Layout->Bindings.Count; i++) {
					if(!Layout->Bindings.Ptr[i].StaticSamplers) {
						vk_bind_group_binding* Binding = BindGroup->Bindings + i;
						vk_bind_group_write_cmd* NewFirstWriteCmd = NULL;
						vk_bind_group_write_cmd* NewLastWriteCmd = NULL;

						for (vk_bind_group_write_cmd* WriteCmd = Binding->FirstBindGroupWriteCmd; 
							WriteCmd; WriteCmd = WriteCmd->Next) {

							FrameDiff = NewFrameIndex - WriteCmd->LastUpdatedFrame;
							if (FrameDiff < Context->Frames.Count) {

								vk_bind_group_write_cmd* NewWriteCmd = Arena_Push_Struct(NewFrame->TempArena, vk_bind_group_write_cmd);
								for (vk_bind_group_dynamic_descriptor* Descriptor = WriteCmd->FirstDynamicDescriptor;
									Descriptor; Descriptor = Descriptor->Next) {
										
									vk_buffer* Buffer = VK_Buffer_Pool_Get(&Context->ResourcePool, Descriptor->Buffer);
									if (Buffer) {
										Assert(Buffer->Usage & GDI_BUFFER_USAGE_DYNAMIC);

										vk_bind_group_dynamic_descriptor* NewDescriptor = Arena_Push_Struct(NewFrame->TempArena, vk_bind_group_dynamic_descriptor);
										*NewDescriptor = *Descriptor;
										NewDescriptor->Next = NULL;

										SLL_Push_Back(NewWriteCmd->FirstDynamicDescriptor, NewWriteCmd->LastDynamicDescriptor, NewDescriptor);
									}
								}

								if (NewWriteCmd->FirstDynamicDescriptor) {
									NewWriteCmd->LastUpdatedFrame = WriteCmd->LastUpdatedFrame;
									SLL_Push_Back(NewFirstWriteCmd, NewLastWriteCmd, NewWriteCmd);
								}
							}
						}

						Binding->FirstBindGroupWriteCmd = NewFirstWriteCmd;
						Binding->LastBindGroupWriteCmd = NewLastWriteCmd;

						vk_bind_group_copy_cmd* NewFirstCopyCmd = NULL;
						vk_bind_group_copy_cmd* NewLastCopyCmd = NULL;

						for (vk_bind_group_copy_cmd* CopyCmd = Binding->FirstBindGroupCopyCmd;
							CopyCmd; CopyCmd = CopyCmd->Next) {
								
							vk_bind_group* SrcBindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, CopyCmd->SrcBindGroup);
							vk_bind_group_layout* SrcLayout = VK_Bind_Group_Layout_Pool_Get(&Context->ResourcePool, SrcBindGroup->Layout);

							FrameDiff = NewFrameIndex - CopyCmd->LastUpdatedFrame;
							if (FrameDiff < Context->Frames.Count && SrcBindGroup && SrcLayout) {
									
								vk_bind_group_copy_cmd* NewCopyCmd = Arena_Push_Struct(NewFrame->TempArena, vk_bind_group_copy_cmd);
								*NewCopyCmd = *CopyCmd;

								SLL_Push_Back(NewFirstCopyCmd, NewLastCopyCmd, NewCopyCmd);
							}
						}

						Binding->FirstBindGroupCopyCmd = NewFirstCopyCmd;
						Binding->LastBindGroupCopyCmd = NewLastCopyCmd;
					}
				}
			}
		}
	}


	dynamic_vk_copy_descriptor_set_array Copies = Dynamic_VK_Copy_Descriptor_Set_Array_Init((allocator*)Scratch);
	dynamic_vk_write_descriptor_set_array Writes = Dynamic_VK_Write_Descriptor_Set_Array_Init((allocator*)Scratch);
		
	for (size_t i = 0; i < Array_Count(Context->ResourcePool.Bind_GroupPool.Entries); i++) {
		vk_bind_group* BindGroup = Context->ResourcePool.Bind_GroupPool.Entries + i;
		
		if (BindGroup->ShouldUpdate) {
			Assert((NewFrameIndex - BindGroup->LastUpdatedFrame) < Context->Frames.Count);
			vk_bind_group_layout* Layout = VK_Bind_Group_Layout_Pool_Get(&Context->ResourcePool, BindGroup->Layout);
			if (Layout) {
				VkDescriptorSet SrcSet = BindGroup->Sets[OldFrame->Index];
				VkDescriptorSet DstSet = BindGroup->Sets[NewFrame->Index];

				for (size_t j = 0; j < Layout->Bindings.Count; j++) {							
					if(!Layout->Bindings.Ptr[j].StaticSamplers) {
						vk_bind_group_binding* Binding = BindGroup->Bindings + j;
						//Check if the descriptor is in use by a current copy or write
						//If not, we just copy from the prev state to the newest state
						u32* DescriptorCounts = Arena_Push_Array(Scratch, Layout->Bindings.Ptr[j].Count, u32);

						for (vk_bind_group_write_cmd* WriteCmd = Binding->FirstBindGroupWriteCmd;
							WriteCmd; WriteCmd = WriteCmd->Next) {

							Assert((NewFrameIndex - WriteCmd->LastUpdatedFrame) < Context->Frames.Count);

							for (vk_bind_group_dynamic_descriptor* Descriptor = WriteCmd->FirstDynamicDescriptor;
								Descriptor; Descriptor = Descriptor->Next) {

								vk_buffer* Buffer = VK_Buffer_Pool_Get(&Context->ResourcePool, Descriptor->Buffer);
								if (Buffer) {
									Assert(Buffer->Usage & GDI_BUFFER_USAGE_DYNAMIC);
									
									VkDescriptorBufferInfo* BufferInfo = Arena_Push_Struct(Scratch, VkDescriptorBufferInfo);
									BufferInfo->buffer = Buffer->Buffer;
									BufferInfo->offset = Descriptor->Offset + NewFrame->Index * Buffer->Size;
									BufferInfo->range = Descriptor->Size;

									if (BufferInfo->range == 0) {
										BufferInfo->range = Buffer->Size - Descriptor->Offset;
									}

									VkDescriptorType Type = VK_Get_Descriptor_Type(Layout->Bindings.Ptr[j].Type);

									VkWriteDescriptorSet WriteDescriptor = {
										.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
										.dstSet = DstSet,
										.dstBinding = (u32)j,
										.dstArrayElement = Descriptor->Index,
										.descriptorCount = 1,
										.descriptorType = Type,
										.pBufferInfo = BufferInfo
									};

									Dynamic_VK_Write_Descriptor_Set_Array_Add(&Writes, WriteDescriptor);

									DescriptorCounts[Descriptor->Index] = 1;
								}
							}
						}

						for (vk_bind_group_copy_cmd* CopyCmd = Binding->FirstBindGroupCopyCmd;
							CopyCmd; CopyCmd = CopyCmd->Next) {
								
							Assert((NewFrameIndex - CopyCmd->LastUpdatedFrame) < Context->Frames.Count);


							vk_bind_group* SrcBindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, CopyCmd->SrcBindGroup);
							if (SrcBindGroup) {
									
								VkCopyDescriptorSet CopyDescriptor = {
									.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
									.srcSet = SrcBindGroup->Sets[NewFrame->Index],
									.srcBinding = CopyCmd->SrcBinding,
									.srcArrayElement = CopyCmd->SrcIndex,
									.dstSet = DstSet,
									.dstBinding = (u32)j,
									.dstArrayElement = CopyCmd->DstIndex,
									.descriptorCount = CopyCmd->Count
								};

								Dynamic_VK_Copy_Descriptor_Set_Array_Add(&Copies, CopyDescriptor);
									
								DescriptorCounts[CopyCmd->DstIndex] = CopyCmd->Count;
							}
						}

						u32 StartIndex = 0;
						for (u32 k = 0; k < Layout->Bindings.Ptr[j].Count; k++) {
							if (DescriptorCounts[k] > 0) {
								u32 CopyCount = k - StartIndex;
								if (CopyCount) {
									VkCopyDescriptorSet CopyDescriptor = {
										.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
										.srcSet = SrcSet,
										.srcBinding = (u32)j,
										.srcArrayElement = StartIndex,
										.dstSet = DstSet,
										.dstBinding = (u32)j,
										.dstArrayElement = StartIndex,
										.descriptorCount = CopyCount
									};

									Dynamic_VK_Copy_Descriptor_Set_Array_Add(&Copies, CopyDescriptor);
								}

								u32 DescriptorsToSkip = DescriptorCounts[k]-1;
								for (k = k+1; k < Layout->Bindings.Ptr[j].Count; k++) {
									if (!DescriptorsToSkip) break;

									DescriptorsToSkip--;

									if (DescriptorCounts[k]) {
										DescriptorsToSkip = Max(DescriptorCounts[k], DescriptorsToSkip);
									}

								}

								StartIndex = k;
							}
						}

						if (StartIndex < Layout->Bindings.Ptr[j].Count) {
							VkCopyDescriptorSet CopyDescriptor = {
								.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
								.srcSet = SrcSet,
								.srcBinding = (u32)j,
								.srcArrayElement = StartIndex,
								.dstSet = DstSet,
								.dstBinding = (u32)j,
								.dstArrayElement = StartIndex,
								.descriptorCount = Layout->Bindings.Ptr[j].Count-StartIndex
							};

							Dynamic_VK_Copy_Descriptor_Set_Array_Add(&Copies, CopyDescriptor);
						}
					}
				}
			}
		}
	}

	if (Writes.Count || Copies.Count) {
		vkUpdateDescriptorSets(Context->Device, (u32)Writes.Count, Writes.Ptr, (u32)Copies.Count, Copies.Ptr);
	}

	Scratch_Release();
}

function vk_frame_context* VK_Begin_Next_Frame(vk_device_context* Context, u64* OutFrameIndex) {
	vk_frame_context* Result = NULL;

	u64 FrameIndex = Atomic_Load_U64(&Context->FrameCount);
	vk_frame_context* NewFrame = Context->Frames.Ptr + ((FrameIndex + 1) % Context->Frames.Count);

	//Wait on the new frame before switching frames. We can still add resources
	//to the frame while waiting for rendering to finish on the next frame
	OS_Mutex_Lock(NewFrame->FenceLock);
	VkResult FenceStatus = vkGetFenceStatus(Context->Device, NewFrame->Fence);
	if (FenceStatus == VK_NOT_READY) {
		vkWaitForFences(Context->Device, 1, &NewFrame->Fence, VK_TRUE, UINT64_MAX);
	} else {
		Assert(FenceStatus == VK_SUCCESS);
	}
	OS_Mutex_Unlock(NewFrame->FenceLock);

	//Wait for readbacks before we reset the arena
	OS_Semaphore_Decrement(Context->ReadbackFinishedSemaphore);

	vkResetFences(Context->Device, 1, &NewFrame->Fence);

	for (vk_frame_thread_context* ThreadContext = (vk_frame_thread_context*)Atomic_Load_Ptr(&NewFrame->TopThread); 
		ThreadContext; ThreadContext = ThreadContext->Next) {
		VK_Delete_Queued_Thread_Resources(Context, ThreadContext);
	}

	VK_CPU_Buffer_Clear(&NewFrame->ReadbackBuffer);
	Arena_Clear(NewFrame->TempArena);

	VK_Update_Frame_Bind_Groups(Context, FrameIndex);

	OS_RW_Mutex_Write_Lock(Context->FrameLock);
	Atomic_Increment_U64(&Context->FrameCount);
	Result = Context->Frames.Ptr + (FrameIndex % Context->Frames.Count);

	for (vk_frame_thread_context* ThreadContext = (vk_frame_thread_context*)Atomic_Load_Ptr(&NewFrame->TopThread); 
		ThreadContext; ThreadContext = ThreadContext->Next) {
		ThreadContext->NeedsReset = true;
	}

	OS_RW_Mutex_Write_Unlock(Context->FrameLock);

	*OutFrameIndex = FrameIndex;

	return Result;
}

function b32 VK_Render_Internal(vk_device_context* Context, const gdi_handle_array* Swapchains, const gdi_buffer_readback_array* BufferReadbacks, const gdi_texture_readback_array* TextureReadbacks) {

	u64 FrameIndex;
	vk_frame_context* FrameContext = VK_Begin_Next_Frame(Context, &FrameIndex);

	//End all the thread command buffers
	for (vk_frame_thread_context* Thread = (vk_frame_thread_context*)Atomic_Load_Ptr(&FrameContext->TopThread); 
		 Thread; Thread = Thread->Next) {
		
		if (!Thread->NeedsReset) {
			vkEndCommandBuffer(Thread->CmdBuffer);
		}

	}

	//Start the command buffer
	{
		VkResult Status = vkResetCommandPool(Context->Device, FrameContext->CmdPool, 0);
		if (Status != VK_SUCCESS) {
			GDI_Log_Error("vkResetCommandPool failed");
			return false;
		}

		VkCommandBufferBeginInfo BeginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		Status = vkBeginCommandBuffer(FrameContext->CmdBuffer, &BeginInfo);
		if (Status != VK_SUCCESS) {
			GDI_Log_Error("vkBeginCommandBuffer failed");
			return false;
		}
	}
   
	//Execute transfer commands
	{

		arena* Scratch = Scratch_Get();

		//Accumulate barriers
		vk_barriers PrePassBarriers = VK_Barriers_Init(Scratch, FrameContext->CmdBuffer);
		vk_barriers PostPassBarriers = VK_Barriers_Init(Scratch, FrameContext->CmdBuffer);
		for (vk_frame_thread_context* Thread = (vk_frame_thread_context*)Atomic_Load_Ptr(&FrameContext->TopThread); 
			 Thread; Thread = Thread->Next) {
			
			if (!Thread->NeedsReset) {
				for (vk_texture_barrier_cmd* Cmd = Thread->FirstTextureBarrierCmd; Cmd; Cmd = Cmd->Next) {
					vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, Cmd->Texture);
					if (Texture) {
						Texture->QueueFlags |= Cmd->QueueFlag;
					}
				}
			}
		}

		for (size_t i = 0; i < Array_Count(Context->ResourcePool.TexturePool.Entries); i++) {
			vk_texture* Texture = Context->ResourcePool.TexturePool.Entries + i;
			if (Texture->QueueFlags) {
				if (Are_Bits_Set(Texture->QueueFlags, VK_GDI_QUEUE_FLAG_TRANSFER|VK_GDI_QUEUE_FLAG_CREATION)) {
					if (Texture->Usage & GDI_TEXTURE_USAGE_SAMPLED) {
						VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_NONE, VK_RESOURCE_STATE_TRANSFER_WRITE);
						VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_TRANSFER_WRITE, VK_RESOURCE_STATE_SHADER_READ);
					} Invalid_Else;
				} else if(Texture->QueueFlags == VK_GDI_QUEUE_FLAG_CREATION) {
					if (Texture->Usage & GDI_TEXTURE_USAGE_SAMPLED) {
						VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_NONE, VK_RESOURCE_STATE_SHADER_READ);
					} else if (Texture->Usage & GDI_TEXTURE_USAGE_DEPTH) {
						VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_NONE, VK_RESOURCE_STATE_DEPTH);
					} else if(Texture->Usage & GDI_TEXTURE_USAGE_RENDER_TARGET) {
						VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_NONE, VK_RESOURCE_STATE_RENDER_TARGET);
					} else if(Texture->Usage & GDI_TEXTURE_USAGE_STORAGE) {
						VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_NONE, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE);
					} Invalid_Else;
				} else if (Texture->QueueFlags == VK_GDI_QUEUE_FLAG_TRANSFER) {
					if (Texture->Usage & GDI_TEXTURE_USAGE_SAMPLED) {
						VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_SHADER_READ, VK_RESOURCE_STATE_TRANSFER_WRITE);
						VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_TRANSFER_WRITE, VK_RESOURCE_STATE_SHADER_READ);
					} Invalid_Else;
				} Invalid_Else;

				Texture->QueueFlags = 0;
			}
		}

		//Submit barriers and execute transfer cmds
		VK_Submit_Memory_Barriers(&PrePassBarriers, VK_RESOURCE_STATE_NONE, VK_RESOURCE_STATE_TRANSFER_WRITE);
		for (vk_frame_thread_context* Thread = (vk_frame_thread_context*)Atomic_Load_Ptr(&FrameContext->TopThread); 
			 Thread; Thread = Thread->Next) {
			if (!Thread->NeedsReset) {
				vkCmdExecuteCommands(FrameContext->CmdBuffer, 1, &Thread->CmdBuffer);
			}
		}
		VK_Submit_Memory_Barriers(&PostPassBarriers, VK_RESOURCE_STATE_TRANSFER_WRITE, VK_RESOURCE_STATE_MEMORY_READ);
		Scratch_Release();
	}

	//Initialize the wait semaphore state (flags and locks)

	arena* Scratch = Scratch_Get();

	u32 WaitSemaphoreCount = 0;
	VkPipelineStageFlags* WaitStageFlags = VK_NULL_HANDLE;
	VkSemaphore* WaitSemaphores = VK_NULL_HANDLE;
	{
		WaitStageFlags = Arena_Push_Array(Scratch, Swapchains->Count, VkPipelineStageFlags);
		WaitSemaphores = Arena_Push_Array(Scratch, Swapchains->Count, VkSemaphore);

		for (size_t i = 0; i < Swapchains->Count; i++) {
			vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&Context->ResourcePool, Swapchains->Ptr[i]);
			WaitSemaphores[WaitSemaphoreCount] = Swapchain->Locks[FrameContext->Index].Handle;
			WaitStageFlags[WaitSemaphoreCount] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			WaitSemaphoreCount++;
		}
	}

	//Execute compute and render commands
	{
		vk_barriers PrePassBarriers = VK_Barriers_Init(Scratch, FrameContext->CmdBuffer);
		vk_barriers PostPassBarriers = VK_Barriers_Init(Scratch, FrameContext->CmdBuffer);

		for (gdi_pass* Pass = Context->Base.FirstPass; Pass; Pass = Pass->Next) {
			VK_Barriers_Clear(&PrePassBarriers);
			VK_Barriers_Clear(&PostPassBarriers);

			switch (Pass->Type) {
				case GDI_PASS_TYPE_COMPUTE: {

					gdi_compute_pass* ComputePass = &Pass->ComputePass;


					//Image memory barriers
					for (size_t i = 0; i < ComputePass->TextureWrites.Count; i++) {
						Assert(GDI_Is_Type(ComputePass->TextureWrites.Ptr[i], TEXTURE_VIEW));
						vk_texture_view* TextureView = VK_Texture_View_Pool_Get(&Context->ResourcePool, ComputePass->TextureWrites.Ptr[i]);
						vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, TextureView->Texture);
						Assert(Texture);

						if (!GDI_Is_Null(Texture->Swapchain)) {
							size_t Index = (size_t)-1;
							for (size_t j = 0; j < Swapchains->Count; j++) {
								if (GDI_Is_Equal(Swapchains->Ptr[j], Texture->Swapchain)) {
									Index = j;
									break;
								}
							}

							if (Index != (size_t)-1) {
								if (WaitStageFlags[Index] == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
									WaitStageFlags[Index] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
									VK_Add_Pre_Swapchain_Barrier(&PrePassBarriers, Texture, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE);
								} else {
									VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE);
								}
							} else {
								//todo: Need to figure out what to do in this case
								Not_Implemented;
							}
						} else {
							if (Texture->Usage & GDI_TEXTURE_USAGE_SAMPLED) {
								VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_SHADER_READ, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE);
								VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE, VK_RESOURCE_STATE_SHADER_READ);
							} else if (Texture->Usage & GDI_TEXTURE_USAGE_STORAGE) {
								VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE);
								VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE);
							}
							Invalid_Else;
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
						
						vk_shader* Shader = VK_Shader_Pool_Get(&Context->ResourcePool, Dispatch->Shader);
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
											vk_texture_view* TextureView = VK_Texture_View_Pool_Get(&Context->ResourcePool, Handle);
											vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, TextureView->Texture);

											ImageInfo[i].imageView = TextureView ? TextureView->ImageView : VK_NULL_HANDLE;
											ImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
										}

										WriteDescriptorSet.pImageInfo = ImageInfo;
									} break;

									case GDI_BIND_GROUP_TYPE_STORAGE_BUFFER: {
										VkDescriptorBufferInfo* BufferInfo = Arena_Push_Array(Scratch, Binding->Count, VkDescriptorBufferInfo);

										for (size_t i = 0; i < Binding->Count; i++) {
											Assert(BufferIndex < ComputePass->BufferWrites.Count);
											gdi_handle Handle = ComputePass->BufferWrites.Ptr[BufferIndex++];
											Assert(GDI_Is_Type(Handle, BUFFER));
											vk_buffer* Buffer = VK_Buffer_Pool_Get(&Context->ResourcePool, Handle);
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

							vkCmdBindPipeline(FrameContext->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Shader->Pipeline);
							//Outputs of compute shaders are always the first descriptor set
							vkCmdPushDescriptorSetKHR(FrameContext->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Shader->Layout, 0, (u32)Shader->WritableBindings.Count, DescriptorWrites);
						}

						u32 DescriptorSetCount = 0;
						VkDescriptorSet DescriptorSets[GDI_MAX_BIND_GROUP_COUNT - 1];

						for (u32 i = 0; i < GDI_MAX_BIND_GROUP_COUNT - 1; i++) {
							vk_bind_group* BindGroup = VK_Bind_Group_Pool_Get(&Context->ResourcePool, Dispatch->BindGroups[i]);
							if (BindGroup) {
								DescriptorSets[DescriptorSetCount++] = BindGroup->Sets[FrameContext->Index];
							}
						}

						if (DescriptorSetCount) {
							vkCmdBindDescriptorSets(FrameContext->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, CurrentShader->Layout, 1, DescriptorSetCount, DescriptorSets, 0, NULL);
						}

						if (Dispatch->PushConstantCount) {
							vkCmdPushConstants(FrameContext->CmdBuffer, CurrentShader->Layout, VK_SHADER_STAGE_COMPUTE_BIT,
											   0, Dispatch->PushConstantCount * sizeof(u32), Dispatch->PushConstants);
						}

						vkCmdDispatch(FrameContext->CmdBuffer, Dispatch->ThreadGroupCount.x, Dispatch->ThreadGroupCount.y, Dispatch->ThreadGroupCount.z);
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
						vk_texture_view* View = VK_Texture_View_Pool_Get(&Context->ResourcePool, RenderPass->RenderTargetViews[i]);
						if (View) {
							vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, View->Texture);
							Assert(Texture);

							if (!GDI_Is_Null(Texture->Swapchain)) {
								size_t Index = (size_t)-1;
								for (size_t j = 0; j < Swapchains->Count; j++) {
									if (GDI_Is_Equal(Swapchains->Ptr[j], Texture->Swapchain)) {
										Index = j;
										break;
									}
								}

								if (Index != (size_t)-1) {
									if (WaitStageFlags[Index] == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
										WaitStageFlags[Index] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
										VK_Add_Pre_Swapchain_Barrier(&PrePassBarriers, Texture, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, VK_RESOURCE_STATE_RENDER_TARGET);
									} else {
										VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_RENDER_TARGET, VK_RESOURCE_STATE_RENDER_TARGET);
									}
								} else {
									//todo: Need to figure out what to do in this case
									Not_Implemented;
								}
							} else {
								if (!(Texture->Usage & GDI_TEXTURE_USAGE_SAMPLED)) {
									VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_RENDER_TARGET, VK_RESOURCE_STATE_RENDER_TARGET);
									VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_RENDER_TARGET, VK_RESOURCE_STATE_RENDER_TARGET);
								} else {
									VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_SHADER_READ, VK_RESOURCE_STATE_RENDER_TARGET);
									VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_RENDER_TARGET, VK_RESOURCE_STATE_SHADER_READ);
								}
							}

							gdi_clear_color* ClearState = RenderPass->ClearColors + i;

							VkRenderingAttachmentInfoKHR AttachmentInfo = {
								.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
								.imageView = View->ImageView,
								.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
								.loadOp = ClearState->ShouldClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
								.storeOp = VK_ATTACHMENT_STORE_OP_STORE
							};

							if (ClearState->ShouldClear) {
								Memory_Copy(AttachmentInfo.clearValue.color.float32, ClearState->F32, sizeof(ClearState->F32));
							}

							ColorAttachments[ColorAttachmentCount++] = AttachmentInfo;
						}
					}

					vk_texture_view* DepthView = VK_Texture_View_Pool_Get(&Context->ResourcePool, RenderPass->DepthBufferView);
					if (DepthView) {
						vk_texture* DepthTexture = VK_Texture_Pool_Get(&Context->ResourcePool, DepthView->Texture);
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
							.loadOp = ClearState->ShouldClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
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
					vkCmdBeginRenderingKHR(FrameContext->CmdBuffer, &RenderingInfo);
					vkCmdExecuteCommands(FrameContext->CmdBuffer, 1, &PassCmdBuffer);
					vkCmdEndRenderingKHR(FrameContext->CmdBuffer);
					VK_Submit_Barriers(&PostPassBarriers);
				} break;
			}
		}
	}

	//Submit the buffer and texture readbacks
	if (TextureReadbacks->Count || BufferReadbacks->Count) {
		FrameContext->TextureReadbacks = VK_Texture_Readback_Array_Alloc((allocator*)FrameContext->TempArena, TextureReadbacks->Count);
		FrameContext->BufferReadbacks = VK_Buffer_Readback_Array_Alloc((allocator*)FrameContext->TempArena, BufferReadbacks->Count);

		vk_barriers PrePassBarriers = VK_Barriers_Init(Scratch, FrameContext->CmdBuffer);
		vk_barriers PostPassBarriers = VK_Barriers_Init(Scratch, FrameContext->CmdBuffer);
		
		for (size_t i = 0; i < TextureReadbacks->Count; i++) {
			const gdi_texture_readback* Readback = TextureReadbacks->Ptr + i;
			vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, Readback->Texture);
			Assert(GDI_Is_Null(Texture->Swapchain)); //Swapchains cannot be transferred
			if (Texture->Usage & GDI_TEXTURE_USAGE_SAMPLED) {
				VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_SHADER_READ, VK_RESOURCE_STATE_TRANSFER_READ);
				VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_TRANSFER_READ, VK_RESOURCE_STATE_SHADER_READ);
			} else if (Texture->Usage & GDI_TEXTURE_USAGE_DEPTH) {
				VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_DEPTH, VK_RESOURCE_STATE_TRANSFER_READ);
				VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_TRANSFER_READ, VK_RESOURCE_STATE_DEPTH);
			} else if (Texture->Usage & GDI_TEXTURE_USAGE_RENDER_TARGET) {
				VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_RENDER_TARGET, VK_RESOURCE_STATE_TRANSFER_READ);
				VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_TRANSFER_READ, VK_RESOURCE_STATE_RENDER_TARGET);
			} else if (Texture->Usage & GDI_TEXTURE_USAGE_STORAGE) {
				VK_Add_Texture_Barrier(&PrePassBarriers, Texture, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE, VK_RESOURCE_STATE_TRANSFER_READ);
				VK_Add_Texture_Barrier(&PostPassBarriers, Texture, VK_RESOURCE_STATE_TRANSFER_READ, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE);
			} Invalid_Else;
		}

		if (BufferReadbacks->Count) {
			VK_Submit_Memory_Barriers(&PrePassBarriers, VK_RESOURCE_STATE_MEMORY_READ, VK_RESOURCE_STATE_TRANSFER_READ);
		} else {
			VK_Submit_Barriers(&PrePassBarriers);
		}

		for (size_t i = 0; i < TextureReadbacks->Count; i++) {
			const gdi_texture_readback* Readback = TextureReadbacks->Ptr + i;
			vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, Readback->Texture);
				
			v2i Dim = Texture->Dim;
			size_t AllocationSize = 0;
			for (u32 i = 0; i < Texture->MipCount; i++) {
				size_t TextureSize = Dim.x * Dim.y * GDI_Get_Format_Size(Texture->Format);
				AllocationSize += TextureSize;
				Dim = V2i_Div_S32(Dim, 2);
			}

			vk_cpu_buffer_push CpuReadback = VK_CPU_Buffer_Push(&FrameContext->ReadbackBuffer, AllocationSize);

			Dim = Texture->Dim;
			VkBufferImageCopy* Regions = Arena_Push_Array(Scratch, Texture->MipCount, VkBufferImageCopy);

			VkImageAspectFlags ImageAspect = GDI_Is_Depth_Format(Texture->Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

			VkDeviceSize Offset = 0;
			for (u32 i = 0; i < Texture->MipCount; i++) {
				size_t TextureSize = Dim.x * Dim.y * GDI_Get_Format_Size(Texture->Format);

				VkBufferImageCopy BufferImageCopy = {
					.bufferOffset = CpuReadback.Offset + Offset,
					.imageSubresource = { ImageAspect, i, 0, 1 },
					.imageExtent = { (u32)Dim.x, (u32)Dim.y, 1 }
				};
				Regions[i] = BufferImageCopy;
				Dim = V2i_Div_S32(Dim, 2);

				Offset += TextureSize;
			}

			vkCmdCopyImageToBuffer(FrameContext->CmdBuffer, Texture->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								   CpuReadback.Buffer, Texture->MipCount, Regions);

			vk_texture_readback VkReadback = {
				.Texture = Readback->Texture,
				.CPU = CpuReadback,
				.Func = Readback->ReadbackFunc,
				.UserData = Readback->UserData,
			};

			FrameContext->TextureReadbacks.Ptr[i] = VkReadback;
		}

		for (size_t i = 0; i < BufferReadbacks->Count; i++) {
			const gdi_buffer_readback* Readback = BufferReadbacks->Ptr + i;
			vk_buffer* Buffer = VK_Buffer_Pool_Get(&Context->ResourcePool, Readback->Buffer);

			vk_cpu_buffer_push CpuReadback = VK_CPU_Buffer_Push(&FrameContext->ReadbackBuffer, Buffer->Size);

			VkBufferCopy Region = {
				.srcOffset = 0,
				.dstOffset = CpuReadback.Offset,
				.size = Buffer->Size
			};

			vkCmdCopyBuffer(FrameContext->CmdBuffer, Buffer->Buffer, CpuReadback.Buffer, 1, &Region);

			vk_buffer_readback VkReadback = {
				.Buffer = Readback->Buffer,
				.CPU = CpuReadback,
				.Func = Readback->ReadbackFunc,
				.UserData = Readback->UserData
			};

			FrameContext->BufferReadbacks.Ptr[i] = VkReadback;
		}

		if (BufferReadbacks->Count) {
			VK_Submit_Memory_Barriers(&PostPassBarriers, VK_RESOURCE_STATE_TRANSFER_READ, VK_RESOURCE_STATE_MEMORY_READ);
		} else {
			VK_Submit_Barriers(&PostPassBarriers);
		}
	}
	
	//Final barriers for the swapchain and submit command buffer
	{
		vk_barriers FinalBarriers = VK_Barriers_Init(Scratch, FrameContext->CmdBuffer);
		for (size_t i = 0; i < Swapchains->Count; i++) {
			vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&Context->ResourcePool, Swapchains->Ptr[i]);
			vk_texture* Texture = VK_Texture_Pool_Get(&Context->ResourcePool, Swapchain->Textures[Swapchain->ImageIndex]);
			if (WaitStageFlags[i] == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
				VK_Add_Post_Swapchain_Barrier(&FinalBarriers, Texture, VK_RESOURCE_STATE_NONE);
			} else if (WaitStageFlags[i] == VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) {
				VK_Add_Post_Swapchain_Barrier(&FinalBarriers, Texture, VK_RESOURCE_STATE_RENDER_TARGET);
			} else if(WaitStageFlags[i] == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
				VK_Add_Post_Swapchain_Barrier(&FinalBarriers, Texture, VK_RESOURCE_STATE_COMPUTE_SHADER_WRITE);
			} Invalid_Else;
		}
		VK_Submit_Barriers(&FinalBarriers);
		VkResult Status = vkEndCommandBuffer(FrameContext->CmdBuffer);

		if (Status != VK_SUCCESS) {
			Assert(false);
			GDI_Log_Error("vkEndCommandBuffer failed");
			Scratch_Release();
			return false;
		}

		VkCommandBuffer CmdBuffers[] = { FrameContext->CmdBuffer };			
	
		VkSubmitInfo SubmitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = WaitSemaphoreCount,
			.pWaitSemaphores = WaitSemaphores,
			.pWaitDstStageMask = WaitStageFlags,
			.commandBufferCount = Array_Count(CmdBuffers),
			.pCommandBuffers = CmdBuffers,
			.signalSemaphoreCount = Swapchains->Count ? 1u : 0u,
			.pSignalSemaphores = Swapchains->Count ? &FrameContext->RenderLock : VK_NULL_HANDLE
		};
		Status = vkQueueSubmit(Context->GraphicsQueue, 1, &SubmitInfo, FrameContext->Fence);
		if (Status != VK_SUCCESS) {
			Assert(false);
			GDI_Log_Error("vkQueueSubmit failed");
			Scratch_Release();
			return false;
		}
	}

	OS_Semaphore_Increment(Context->ReadbackSignalSemaphore);

	//Final swapchain present
	{
		u32 SwapchainCount = 0;
		VkSwapchainKHR* SwapchainHandles = Arena_Push_Array(Scratch, Swapchains->Count, VkSwapchainKHR);
		u32* ImageIndices = Arena_Push_Array(Scratch, Swapchains->Count, u32);
		VkResult* Statuses = Arena_Push_Array(Scratch, Swapchains->Count, VkResult);

		for (size_t i = 0; i < Swapchains->Count; i++) {
			vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&Context->ResourcePool, Swapchains->Ptr[i]);
			SwapchainHandles[SwapchainCount] = Swapchain->Swapchain;
			ImageIndices[SwapchainCount] = Swapchain->ImageIndex;
			SwapchainCount++;
		}

		if (Swapchains->Count) {
			VkSemaphore WaitSemaphores[] = { FrameContext->RenderLock };
			VkPresentInfoKHR PresentInfo = {
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.waitSemaphoreCount = Array_Count(WaitSemaphores),
				.pWaitSemaphores = WaitSemaphores,
				.swapchainCount = SwapchainCount,
				.pSwapchains = SwapchainHandles,
				.pImageIndices = ImageIndices,
				.pResults = Statuses
			};
			VkResult Status = vkQueuePresentKHR(Context->PresentQueue, &PresentInfo);

			if (Status != VK_SUCCESS && Status != VK_ERROR_OUT_OF_DATE_KHR && Status != VK_SUBOPTIMAL_KHR) {
				GDI_Log_Error("vkQueuePresentKHR failed");
			}

			for (size_t i = 0; i < SwapchainCount; i++) {
				vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&Context->ResourcePool, Swapchains->Ptr[i]);
				if (Statuses[i] == VK_ERROR_OUT_OF_DATE_KHR || Statuses[i] == VK_SUBOPTIMAL_KHR) {
					VK_Recreate_Swapchain(Context, Swapchain);
				} else {
					Swapchain->ImageIndex = -1;
				}
			}
		}
	}
	Scratch_Release();

	//Free renderpasses that were queued for deleting this frame
	for (vk_frame_thread_context* Thread = (vk_frame_thread_context*)Atomic_Load_Ptr(&FrameContext->TopThread); 
		 Thread; Thread = Thread->Next) {
		
		vk_render_pass* RenderPass = Thread->RenderPassesToDelete;
		while (RenderPass) {
			vk_render_pass* RenderPassToDelete = RenderPass;
			RenderPass = RenderPass->Next;

			RenderPassToDelete->Next = NULL;
			SLL_Push_Front(Thread->FreeRenderPasses, RenderPassToDelete);
		}
		Thread->RenderPassesToDelete = NULL;
	}

	return true;
}

function GDI_BACKEND_RENDER_DEFINE(VK_Render) {
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;

	arena* Scratch = Scratch_Get();

	u32 SwapchainCount = 0;
	gdi_handle* Swapchains = Arena_Push_Array(Scratch, RenderParams->Swapchains.Count, gdi_handle);
	for (size_t i = 0; i < RenderParams->Swapchains.Count; i++) {
		vk_swapchain* Swapchain = VK_Swapchain_Pool_Get(&Context->ResourcePool, RenderParams->Swapchains.Ptr[i]);
		if (Swapchain) {
			if (Swapchain->Dim.x != 0 && Swapchain->Dim.y != 0) {
				Swapchains[SwapchainCount++] = RenderParams->Swapchains.Ptr[i];
			}
		}
	}

	gdi_handle_array Array = { .Ptr = Swapchains, .Count = SwapchainCount };
	VK_Render_Internal(Context, &Array, &RenderParams->BufferReadbacks, &RenderParams->TextureReadbacks);
	Scratch_Release();
}

function GDI_BACKEND_SHUTDOWN_DEFINE(VK_Shutdown) {
	vk_gdi* VkGDI = (vk_gdi*)GDI;
	vk_device_context* Context = (vk_device_context*)GDI->DeviceContext;
	if (Context) {
		VK_Delete_Device_Context(VkGDI, Context, Flags);
		GDI->DeviceContext = NULL;
	}

#ifdef DEBUG_BUILD
	if (VkGDI->DebugUtils != VK_NULL_HANDLE) {
		vkDestroyDebugUtilsMessengerEXT(VkGDI->Instance, VkGDI->DebugUtils, VK_Get_Allocator());
		VkGDI->DebugUtils = VK_NULL_HANDLE;
	}
#endif

	if (VkGDI->Instance != VK_NULL_HANDLE) {
		vkDestroyInstance(VkGDI->Instance, VK_Get_Allocator());
		VkGDI->Instance = VK_NULL_HANDLE;
	}

	if (VkGDI->Library) {
		OS_Library_Delete(VkGDI->Library);
		VkGDI->Library = NULL;
	}

	if (GDI->Arena) {
		Arena_Delete(GDI->Arena);
	}
}

global gdi_backend_vtable VK_Backend_VTable = {
	.SetDeviceContextFunc = VK_Set_Device_Context,

	.CreateTextureFunc = VK_Create_Texture,
	.DeleteTextureFunc = VK_Delete_Texture,
	.UpdateTexturesFunc = VK_Update_Textures,
	.GetTextureInfoFunc = VK_Get_Texture_Info,

	.CreateTextureViewFunc = VK_Create_Texture_View,
	.DeleteTextureViewFunc = VK_Delete_Texture_View,
	.GetTextureViewTextureFunc = VK_Get_Texture_View_Texture,
	
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
	.UpdateBindGroupsFunc = VK_Update_Bind_Groups,	
	
	.CreateShaderFunc = VK_Create_Shader,
	.DeleteShaderFunc = VK_Delete_Shader,

	.CreateSwapchainFunc = VK_Create_Swapchain,
	.DeleteSwapchainFunc = VK_Delete_Swapchain,
	.GetSwapchainViewFunc = VK_Get_Swapchain_View,
	.ResizeSwapchainFunc = VK_Resize_Swapchain,
	.GetSwapchainInfoFunc = VK_Get_Swapchain_Info,

	.BeginRenderPassFunc = VK_Begin_Render_Pass,
	.EndRenderPassFunc = VK_End_Render_Pass,

	.RenderFunc = VK_Render,
	.ShutdownFunc = VK_Shutdown
};

function VkBool32 VK_Debug_Callback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT MessageTypes,
									const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData) {
	gdi_log_type LogType = GDI_LOG_TYPE_INFO;
	if (MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		LogType = GDI_LOG_TYPE_WARNING;
	} else if (MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		LogType = GDI_LOG_TYPE_ERROR;
	}

	GDI_Log(LogType, "%s", CallbackData->pMessage);
	return VK_FALSE;
}

export_function GDI_INIT_DEFINE(GDI_Init) {
	Base_Set(InitInfo->Base);
	VK_Set_Allocator();

	G_LogFunc = InitInfo->LogCallbacks.LogFunc;
	G_LogUserData = InitInfo->LogCallbacks.UserData;
	if (!G_LogFunc) {
		G_LogFunc = GDI_Default_Log_Callback;
	}

	arena* Arena = Arena_Create(String_Lit("GDI"));
	vk_gdi* GDI = Arena_Push_Struct(Arena, vk_gdi);
	GDI->Base.Backend = &VK_Backend_VTable;
	GDI->Base.Arena = Arena;

	GDI->Base.FramesInFlight = InitInfo->FramesInFlight;
	if (GDI->Base.FramesInFlight == 0) {
		GDI->Base.FramesInFlight = 2;
	}

	string LibraryPath = String_Empty();

#if defined(VK_USE_PLATFORM_WIN32_KHR)
	LibraryPath = String_Lit("vulkan-1.dll");
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	LibraryPath = String_Lit("/usr/local/lib/libvulkan.1.dylib");
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	LibraryPath = String_Lit("libvulkan.so.1");
#else
#error "Not Implemented"
#endif

	GDI->Library = OS_Library_Create(LibraryPath);
	if (!GDI->Library) {
		GDI_Log_Error("Failed to load %.*s", LibraryPath.Size, LibraryPath.Ptr);
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

	VkResult Status = vkCreateInstance(&InstanceCreateInfo, VK_Get_Allocator(), &GDI->Instance);
	Scratch_Release();

	if (Status != VK_SUCCESS) {
		GDI_Log_Error("vkCreateInstance failed!");
		return NULL;
	}

#ifdef DEBUG_BUILD
	if (!HasValidationLayers) {
		GDI_Log_Warning("Missing vulkan validation layers");
	}

	if (!HasDebugUtil) {
		GDI_Log_Warning("Missing vulkan debug utility extension");
	}
#endif

	b32 HasExtensions = true;
	for (size_t i = 0; i < Array_Count(G_RequiredInstanceExtensions); i++) {
		if (!HasRequiredInstanceExtensions[i]) {
			GDI_Log_Error("Missing vulkan instance extension '%.*s'", G_RequiredInstanceExtensions[i].Size, G_RequiredInstanceExtensions[i].Ptr);
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

		if (vkCreateDebugUtilsMessengerEXT(GDI->Instance, &DebugUtilsCreateInfo, VK_Get_Allocator(), &GDI->DebugUtils) != VK_SUCCESS) {
			GDI_Log_Warning("vkCreateDebugUtilsMessengerEXT failed!");
		}
	}
#endif

	Vk_Khr_Surface_Funcs_Load(GDI->Instance);
	Vk_Instance_Funcs_Load(GDI->Instance);
	Vk_Khr_Get_Physical_Device_Properties2_Funcs_Load(GDI->Instance);

	vk_tmp_surface Surface = VK_Create_Tmp_Surface(GDI);
	if (Surface.Surface == VK_NULL_HANDLE) {
		GDI_Log_Error("Failed to create the vulkan tmp surface");
		return NULL;
	}

	Scratch = Scratch_Get();

	u32 PhysicalDeviceCount;
	vkEnumeratePhysicalDevices(GDI->Instance, &PhysicalDeviceCount, VK_NULL_HANDLE);

	VkPhysicalDevice* PhysicalDevices = Arena_Push_Array(Scratch, PhysicalDeviceCount, VkPhysicalDevice);
	vkEnumeratePhysicalDevices(GDI->Instance, &PhysicalDeviceCount, PhysicalDevices);

	GDI->GPUs = Arena_Push_Array(GDI->Base.Arena, PhysicalDeviceCount, vk_gpu);

	for (size_t i = 0; i < PhysicalDeviceCount; i++) {
		vk_gpu GPU;
		if (VK_Fill_GPU(GDI, &GPU, PhysicalDevices[i], Surface.Surface)) {
			GDI->GPUs[GDI->GPUCount++] = GPU;
		}
	}

	Scratch_Release();
	VK_Delete_Tmp_Surface(GDI, &Surface);

	if (!GDI->GPUCount) {
		GDI_Log_Error("No valid GPUs");
		return NULL;
	}

	gdi_device_array Devices = {
		.Ptr = Arena_Push_Array(GDI->Base.Arena, GDI->GPUCount, gdi_device),
		.Count = GDI->GPUCount
	};

	for(size_t i = 0; i < GDI->GPUCount; i++) {
		gdi_device Device = {
			.Type = VK_Get_Physical_Device_Type(GDI->GPUs[i].Properties.deviceType),
			.DeviceIndex = (u32)i,
			.DeviceName = String_Copy((allocator*)GDI->Base.Arena, String_Null_Term(GDI->GPUs[i].Properties.deviceName))
		};
		Devices.Ptr[i] = Device;
	}

	GDI->Base.Devices = Devices;

	GDI_Set((gdi*)GDI);
	return (gdi*)GDI;
}

//todo: Does this need to even be here anymore?
#ifdef OS_WIN32
#pragma comment(lib, "base.lib")
#endif