#ifndef IMGUI_IMPL_GDI_H
#define IMGUI_IMPL_GDI_H

struct ImGui_ImplGDI_Texture {
	gdi_handle Handle;
	gdi_handle View;
	gdi_handle BindGroup;
	ImGui_ImplGDI_Texture* Next;
};

struct ImGui_ImplGDI_Data {
	arena* Arena;
	ImGui_ImplGDI_Texture* FirstFreeTexture;

    gdi_handle Sampler;
    gdi_handle BindGroupLayout;
    gdi_handle Shader;

    size_t VtxBufferSize;
    gdi_handle VtxBuffer;

    size_t IdxBufferSize;
    gdi_handle IdxBuffer;
};

export_function gdi_handle ImGui_ImplGDI_GetBindGroup(ImTextureID TexID);
export_function void ImGui_ImplGDI_Init(gdi_handle Swapchain);
export_function void ImGui_ImplGDI_NewFrame();
export_function void ImGui_ImplGDI_RenderDrawData(gdi_render_pass* RenderPass, ImDrawData* DrawData);
export_function void ImGui_ImplGDI_Shutdown();

#endif