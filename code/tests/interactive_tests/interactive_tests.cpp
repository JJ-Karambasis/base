#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#ifdef OS_OSX
#include <SDL3/SDL_metal.h>
#endif

#include <base.h>
#include <gdi/gdi.h>

function GDI_LOG_DEFINE(GDI_Log_Callback) {
    Debug_Log("%.*s", Message.Size, Message.Ptr);
    Assert(Type == GDI_LOG_TYPE_INFO);
}

struct window {
    SDL_Window* Window;
    gdi_handle  Swapchain;
};

function window Create_Window(string Title, v2i Dim, SDL_WindowFlags Flags) {
    window Window = {};

    scratch Scratch;
    Title = String_Copy(&Scratch, Title); //Make sure its null terminated 

    Window.Window = SDL_CreateWindow(Title.Ptr, Dim.x, Dim.y, Flags);

	gdi_swapchain_create_info SwapchainInfo = {
		.Format = GDI_FORMAT_B8G8R8A8_UNORM,
		.DebugName = String_Concat(&Scratch, Title, String_Lit(" Window Swapchain"))
	};

	SDL_PropertiesID WindowProps = SDL_GetWindowProperties(Window.Window);

#if defined(OS_WIN32)
	SwapchainInfo.Window = (HWND)SDL_GetPointerProperty(WindowProps, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	SwapchainInfo.Instance = (HINSTANCE)GetWindowLongPtrA(SwapchainInfo.Window, GWLP_HINSTANCE);
#else
	SwapchainInfo.Layer = SDL_Metal_GetLayer(SDL_Metal_CreateView(Window.Window));
#endif

	Window.Swapchain = GDI_Create_Swapchain(&SwapchainInfo);
    return Window;
}

int main(int ArgumentCount, const char** Args) {
    Base_Init();
    SDL_Init(SDL_INIT_VIDEO);

    gdi_init_info InitInfo = {
        .Base = Base_Get(),
        .LogCallbacks = {
            .LogFunc = GDI_Log_Callback
        }
    };

    if(!GDI_Init(&InitInfo)) {
        return 1;
    }

    if(!GDI_Set_Default_Device_Context()) {
        return 1;
    }

    window Window = Create_Window(String_Lit("Interactive Tests"), V2i(1920, 1080), SDL_WINDOW_RESIZABLE|SDL_WINDOW_MAXIMIZED);
    SDL_MaximizeWindow(Window.Window);

    for(;;) {
        SDL_Event Event;
        while(SDL_PollEvent(&Event)) {
            switch(Event.type) {
                case SDL_EVENT_QUIT: {
                    return 0;
                } break;
            }
        }

        gdi_handle View = GDI_Get_Swapchain_View(Window.Swapchain);
        if(!GDI_Is_Null(View)) {
            gdi_render_pass_begin_info BeginInfo = {
                .RenderTargetViews = { View },
                .ClearColors = { { .ShouldClear = true, .F32 = {1, 0, 0, 1}}}
            };

            gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);
            GDI_End_Render_Pass(RenderPass);

            GDI_Submit_Render_Pass(RenderPass);
        }

        gdi_render_params RenderParams = {
            .Swapchains = { .Ptr = &Window.Swapchain, .Count = 1}
        };
        GDI_Render(&RenderParams);
    }

    return 0;
}