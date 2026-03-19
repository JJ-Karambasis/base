#ifndef IMGUI_IMPL_GDI_H
#define IMGUI_IMPL_GDI_H

#ifdef IMGUI_HAS_VIEWPORT
typedef void (*ImGui_ImplGDI_FillSwapchainInfoFunc)(ImGuiViewport* Viewport, gdi_swapchain_create_info* OutInfo, void** OutPlatformData);
typedef void (*ImGui_ImplGDI_CleanupSwapchainFunc)(ImGuiViewport* Viewport, void* PlatformData);
#endif

struct ImGui_ImplGDI_Texture {
	gdi_texture Handle;
	gdi_texture_view View;
	gdi_bind_group BindGroup;
	ImGui_ImplGDI_Texture* Next;
};

struct ImGui_ImplGDI_RenderBuffers {
    size_t VtxBufferSize;
    gdi_buffer VtxBuffer;
    size_t IdxBufferSize;
    gdi_buffer IdxBuffer;
};

struct ImGui_ImplGDI_Data {
	arena* Arena;
	ImGui_ImplGDI_Texture* FirstFreeTexture;
    
    gdi_sampler Sampler;
    gdi_layout BindGroupLayout;
    gdi_shader Shader;
    
    ImGui_ImplGDI_RenderBuffers MainRenderBuffers;
    
#ifdef IMGUI_HAS_VIEWPORT
    gdi_format SwapchainFormat;
    struct ImGui_ImplGDI_ViewportData* FirstFreeViewportData;
    ImGui_ImplGDI_FillSwapchainInfoFunc FillSwapchainInfoFunc;
    ImGui_ImplGDI_CleanupSwapchainFunc CleanupSwapchainFunc;
#endif
};

export_function gdi_bind_group ImGui_ImplGDI_GetBindGroup(ImTextureID TexID);
export_function void ImGui_ImplGDI_Init(gdi_swapchain Swapchain);
export_function void ImGui_ImplGDI_NewFrame();
export_function void ImGui_ImplGDI_RenderDrawData(gdi_render_pass* RenderPass, ImDrawData* DrawData);
export_function void ImGui_ImplGDI_Shutdown();

#ifdef IMGUI_HAS_VIEWPORT
export_function void ImGui_ImplGDI_SetPlatformSwapchainFuncs(ImGui_ImplGDI_FillSwapchainInfoFunc FillFunc, ImGui_ImplGDI_CleanupSwapchainFunc CleanupFunc);
export_function size_t ImGui_ImplGDI_GetSwapchainCount();
export_function void ImGui_ImplGDI_GetSwapchains(gdi_swapchain* Swapchains);
#endif

#endif