#ifndef IMGUI_IMPL_GDI_H
#define IMGUI_IMPL_GDI_H

struct ImGui_ImplGDI_Texture {
	gdi_texture Handle;
	gdi_texture_view View;
	gdi_bind_group BindGroup;
	ImGui_ImplGDI_Texture* Next;
};

struct ImGui_ImplGDI_Data {
	arena* Arena;
	ImGui_ImplGDI_Texture* FirstFreeTexture;
    
    gdi_sampler Sampler;
    gdi_layout BindGroupLayout;
    gdi_shader Shader;
    
    size_t VtxBufferSize;
    gdi_buffer VtxBuffer;
    
    size_t IdxBufferSize;
    gdi_buffer IdxBuffer;
};

export_function gdi_bind_group ImGui_ImplGDI_GetBindGroup(ImTextureID TexID);
export_function void ImGui_ImplGDI_Init(gdi_swapchain Swapchain);
export_function void ImGui_ImplGDI_NewFrame();
export_function void ImGui_ImplGDI_RenderDrawData(gdi_render_pass* RenderPass, ImDrawData* DrawData);
export_function void ImGui_ImplGDI_Shutdown();

#endif