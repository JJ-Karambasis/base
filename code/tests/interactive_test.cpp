#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <base.h>
#include <gdi/gdi.h>

int main(int ArgumentCount, char** Arguments) {
    Base_Init();
    SDL_Init(SDL_INIT_VIDEO);

    GDI_Init(Base_Get());

    SDL_Window* Window = SDL_CreateWindow("Test App", 1280, 720, 0);
    SDL_PropertiesID Props = SDL_GetWindowProperties(Window);

    gdi_swapchain_create_info SwapchainInfo = {
#ifdef OS_LINUX
        .Display = (Display*)SDL_GetPointerProperty(Props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL),
        .Window = (XID)SDL_GetNumberProperty(Props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0),
#else
#error "Not Implemented
#endif
        .DebugName = String_Lit("Test App Swapchain")
    };

    gdi_handle Swapchain = GDI_Create_Swapchain(&SwapchainInfo);

    bool StopRunning = false;
    while(!StopRunning) {
        SDL_Event Event;
        while(SDL_PollEvent(&Event)) {
            switch(Event.type) {
                case SDL_EVENT_QUIT: {
                    StopRunning = true;
                } break;
            }
        }

        if(!StopRunning) {
            gdi_render_pass_begin_info BeginInfo = {
                .RenderTargetViews = { GDI_Get_Swapchain_View(Swapchain) },
                .ClearColors = { { .ShouldClear = true, .F32 = {1, 0, 0, 1}}}
            };

            gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);
            GDI_End_Render_Pass(RenderPass);

            GDI_Submit_Render_Pass(RenderPass);

            gdi_render_params RenderParams = {
                .Swapchains = { .Ptr = &Swapchain, .Count = 1}
            };
            GDI_Render(&RenderParams);
        }
    }

    SDL_Quit();
    return 0;
}