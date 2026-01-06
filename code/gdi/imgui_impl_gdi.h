#ifndef IMGUI_IMPL_GDI_H
#define IMGUI_IMPL_GDI_H

struct ImGui_ImplGDI_Data {
    gdi_handle Sampler;
    gdi_handle BindGroupLayout;
    gdi_handle Shader;

    size_t VtxBufferSize;
    gdi_handle VtxBuffer;

    size_t IdxBufferSize;
    gdi_handle IdxBuffer;
};

export_function void ImGui_ImplGDI_Init(gdi_handle Swapchain);
export_function void ImGui_ImplGDI_NewFrame();
export_function void ImGui_ImplGDI_RenderDrawData(gdi_render_pass* RenderPass, ImDrawData* DrawData);
export_function void ImGui_ImplGDI_Shutdown();

#endif