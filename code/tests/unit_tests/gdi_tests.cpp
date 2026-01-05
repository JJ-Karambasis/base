struct gdi_tests {
    gdi*                GDI;
    IDxcCompiler3*      Compiler;
    IDxcUtils*          Utils;
    IDxcIncludeHandler* IncludeHandler;

    gdi_handle BufferLayout;
};

struct vtx_p2_c4 {
    v2 P;
    v4 C;
};

struct vtx_p2_uv2_c4 {
    v2 P;
    v2 UV;
    v4 C;
};

struct texture {
    gdi_handle Handle;
    gdi_handle View;
};

struct texture_create_info {
    gdi_format Format;
    v2i Dim;
    gdi_texture_usage Usage;
    buffer InitialData;
};

struct gpu_buffer {
    gdi_handle Handle;
    gdi_handle BindGroup;
};

struct gpu_buffer_create_info {
    size_t           Size;
    gdi_buffer_usage Usage;
    buffer           InitialData;
};


function b32 Create_Layouts(gdi_tests* Tests) {
    //Buffer layout
    {
        gdi_bind_group_binding Binding = {
            .Type = GDI_BIND_GROUP_TYPE_CONSTANT_BUFFER, .Count = 1
        };

        gdi_bind_group_layout_create_info LayoutInfo = {
            .Bindings = { .Ptr = &Binding, .Count = 1 }
        };

        Tests->BufferLayout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
        if(GDI_Is_Null(Tests->BufferLayout)) return false;
    }

    return true;
}

global gdi_tests* G_GDITests;

function GDI_LOG_DEFINE(GDI_Test_Log) {
	Debug_Log("%.*s", Message.Size, Message.Ptr);
	Assert(Type == GDI_LOG_TYPE_INFO);
}

function b32 GDI_Tests_Setup() {
    gdi_tests* Tests = Allocator_Allocate_Struct(Default_Allocator_Get(), gdi_tests);

	gdi_init_info InitInfo = {
		.Base = Base_Get(),
		.LogCallbacks = {
			.LogFunc = GDI_Test_Log
		}
	};

    Tests->GDI = GDI_Init(&InitInfo);
    if(!Tests->GDI) return false;

    gdi_device_array Devices = GDI_Get_Devices();
    gdi_device* TargetDevice = &Devices.Ptr[0];
    for(size_t i = 0; i < Devices.Count; i++) {
        if(Devices.Ptr[i].Type == GDI_DEVICE_TYPE_DISCRETE) {
            TargetDevice = &Devices.Ptr[i];
            break;
        }
    }

    if(!GDI_Set_Device_Context(TargetDevice)) return false;

    HRESULT Status = DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void**)&Tests->Compiler);
    if(FAILED(Status)) return false;

    Status = DxcCreateInstance(CLSID_DxcUtils, __uuidof(IDxcUtils), (void**)&Tests->Utils);
    if(FAILED(Status)) return false;

    Status = Tests->Utils->CreateDefaultIncludeHandler(&Tests->IncludeHandler);
    if(FAILED(Status)) return false;

    b32 LayoutStatus = Create_Layouts(Tests);
    if(!LayoutStatus) return false;

    G_GDITests = Tests;

    return true;
}

function gdi_tests* GDI_Get_Tests() {
    if(!G_GDITests) {
        GDI_Tests_Setup();
    }
    return G_GDITests;
}

function void GDI_Delete_Tests() {
	if (G_GDITests) {
		GDI_Delete_Bind_Group_Layout(G_GDITests->BufferLayout);

		G_GDITests->IncludeHandler->Release();
		G_GDITests->Utils->Release();
		G_GDITests->Compiler->Release();

		GDI_Shutdown(0);

		Allocator_Free_Memory(Default_Allocator_Get(), G_GDITests);
	}
}

function b32 Texture_Create(texture* Texture, const texture_create_info& CreateInfo) {
    gdi_handle Handle;
    gdi_handle View;

    gdi_texture_create_info TextureInfo = {
        .Format = CreateInfo.Format,
        .Dim = CreateInfo.Dim,
        .Usage = CreateInfo.Usage,
        .MipCount = 1,
        .InitialData = Buffer_Is_Empty(CreateInfo.InitialData) ? NULL : &CreateInfo.InitialData
    };

    Handle = GDI_Create_Texture(&TextureInfo);
    if(GDI_Is_Null(Handle)) return false;

    View = GDI_Create_Texture_View_From_Texture(Handle);
    if(GDI_Is_Null(View)) return false;

    Texture->Handle = Handle;
    Texture->View = View;

    return true;
}

function void Texture_Delete(texture* Texture) {
    GDI_Delete_Texture_View(Texture->View);
    GDI_Delete_Texture(Texture->Handle);
}

function b32 GPU_Buffer_Create(gpu_buffer* Buffer, const gpu_buffer_create_info& CreateInfo) {
    gdi_tests* Tests = GDI_Get_Tests();
    
    gdi_buffer_create_info BufferInfo = {
        .Size = CreateInfo.Size,
        .Usage = CreateInfo.Usage,
        .InitialData = CreateInfo.InitialData
    };
    gdi_handle Handle = GDI_Create_Buffer(&BufferInfo);
    if(GDI_Is_Null(Handle)) return false;

    gdi_bind_group_buffer BindGroupBuffer = { .Buffer = Handle };
	gdi_bind_group_write Write = { .Buffers = { .Ptr = &BindGroupBuffer, .Count = 1 } };
	gdi_bind_group_create_info BindGroupInfo = {
		.Layout = Tests->BufferLayout,
		.Writes = { .Ptr = &Write, .Count = 1 }
    };

    gdi_handle BindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
    if(GDI_Is_Null(BindGroup)) return false;

    Buffer->Handle = Handle;
    Buffer->BindGroup = BindGroup;

    return true;
}

function void GPU_Buffer_Delete(gpu_buffer* Buffer) {
    GDI_Delete_Bind_Group(Buffer->BindGroup);
    GDI_Delete_Buffer(Buffer->Handle);
}



function IDxcBlob* Compile_Shader(string Source, string Target, string EntryPoint, const span<string>& InputArgs) {
    gdi_tests* Tests = GDI_Get_Tests();
    
    IDxcBlobEncoding* SourceBlob;
    HRESULT Status = Tests->Utils->CreateBlob(Source.Ptr, (u32)Source.Size, DXC_CP_UTF8, &SourceBlob);
    if(FAILED(Status)) return NULL;

    scratch Scratch;
    array<LPCWSTR> Args(&Scratch);

    Array_Add(&Args, (LPCWSTR)L"-Zi");
    Array_Add(&Args, (LPCWSTR)L"-Qembed_debug");
    Array_Add(&Args, (LPCWSTR)L"-Od");
    Array_Add(&Args, (LPCWSTR)L"-spirv");
    Array_Add(&Args, (LPCWSTR)L"-T");
    Array_Add(&Args, WString_From_String(&Scratch, Target).Ptr);
    Array_Add(&Args, (LPCWSTR)L"-E");
    Array_Add(&Args, WString_From_String(&Scratch, EntryPoint).Ptr);

    for(size_t i = 0; i < InputArgs.Count; i++) {
        Array_Add(&Args, WString_From_String(&Scratch, InputArgs[i]).Ptr);
    }

    DxcBuffer SourceBuffer = {
        .Ptr = SourceBlob->GetBufferPointer(),
        .Size = SourceBlob->GetBufferSize(),
        .Encoding = DXC_CP_UTF8
    };

    IDxcResult* CompilerResult;
    Status = Tests->Compiler->Compile(&SourceBuffer, Args.Ptr, (u32)Args.Count, Tests->IncludeHandler, __uuidof(IDxcResult), (void**)&CompilerResult);
    if(FAILED(Status)) return NULL;

    IDxcBlobUtf8* Errors;
    CompilerResult->GetOutput(DXC_OUT_ERRORS, __uuidof(IDxcBlobUtf8), (void**)&Errors, NULL);
    if(Errors && Errors->GetStringLength()) {
        Debug_Log("%.*s", Errors->GetStringLength(), Errors->GetStringPointer());
		Errors->Release();
	}

    CompilerResult->GetStatus(&Status);
    if(FAILED(Status)) return NULL;

    IDxcBlob* Result;
    CompilerResult->GetOutput(DXC_OUT_OBJECT, __uuidof(IDxcBlob), (void**)&Result, NULL);

	SourceBlob->Release();
	CompilerResult->Release();
	
	return Result;
}

struct simple_test_context {
    u8*        Texels;
    v2i        Dim;
    gdi_format Format;
    os_event*  Event;
};

function GDI_TEXTURE_READBACK_DEFINE(Simple_Texture_Readback) {
    simple_test_context* SimpleTestContext = (simple_test_context*)UserData;
    Memory_Copy(SimpleTestContext->Texels, Texels, Dim.x*Dim.y*GDI_Get_Format_Size(Format));
    SimpleTestContext->Dim = Dim;
    SimpleTestContext->Format = Format;
    OS_Event_Signal(SimpleTestContext->Event);
}

UTEST(gdi, SimpleTest) {
    scratch Scratch;

    //First build the shader
    gdi_handle Shader;
    {
        const char* ShaderCode = R"(

        struct vs_input {
            float2 Position : POSITION0;
            float4 Color : COLOR0;
        };

        struct vs_output {
            float4 Position : SV_POSITION;
            float4 Color : COLOR0;
        };

        vs_output VS_Main(vs_input Input) {
            vs_output Result;
            Result.Position = float4(Input.Position, 0.0f, 1.0f);
            Result.Color = Input.Color;
            return Result;
        }

        float4 PS_Main(vs_output Pxl) : SV_TARGET0 {
            return Pxl.Color;
        };

        )";

        IDxcBlob* VtxShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
        IDxcBlob* PxlShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
        ASSERT_TRUE(VtxShader && PxlShader);

        gdi_vtx_attribute Attributes[] = {
            { String_Lit("POSITION"), GDI_FORMAT_R32G32_FLOAT},
            { String_Lit("COLOR"), GDI_FORMAT_R32G32B32A32_FLOAT}
        };

        gdi_vtx_binding VtxBinding = {
            .Stride = sizeof(vtx_p2_c4),
            .Attributes = { .Ptr = Attributes, .Count = Array_Count(Attributes)}
        };

        gdi_shader_create_info CreateInfo = {
            .VS = { .Ptr = (u8*)VtxShader->GetBufferPointer(), .Size = VtxShader->GetBufferSize()},
            .PS = { .Ptr = (u8*)PxlShader->GetBufferPointer(), .Size = PxlShader->GetBufferSize()},
            .VtxBindings = { .Ptr = &VtxBinding, .Count = 1},
            .RenderTargetFormats = {GDI_FORMAT_R8G8B8A8_UNORM}, 
            .DebugName = String_Lit("Simple Shader")
        };

        Shader = GDI_Create_Shader(&CreateInfo);
        ASSERT_FALSE(GDI_Is_Null(Shader));

        VtxShader->Release();
        PxlShader->Release();
    }

    //Create the render target
    texture RenderTarget;
    ASSERT_TRUE(Texture_Create(&RenderTarget, {
        .Format = GDI_FORMAT_R8G8B8A8_UNORM,
        .Dim = V2i(64, 64),
        .Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_READBACK,
    }));

    //Event to synchronize between the readback and test assertions
    os_event* Event = OS_Event_Create();

    //Iterate, render, and test
    for(u32 i = 0; i < 10; i++) {
        gdi_texture_info TextureInfo = GDI_Get_Texture_Info(RenderTarget.Handle);

        simple_test_context TestContext = {
            .Texels = (u8*)Arena_Push(Scratch.Arena, TextureInfo.Dim.x*TextureInfo.Dim.y*GDI_Get_Format_Size(TextureInfo.Format)),
            .Event = Event
        };
        
        gdi_render_pass_begin_info BeginInfo = {
            .RenderTargetViews = { RenderTarget.View },
            .ClearColors = { { .ShouldClear = true, .F32 = {1, 0, 0, 1}}}
        };

        gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);

        Render_Set_Shader(RenderPass, Shader);

        v2i Min = (TextureInfo.Dim/2)-4;
        v2i Max = (TextureInfo.Dim/2)+4;

        m4 Orthographic = M4_Orthographic(0, (f32)TextureInfo.Dim.x, (f32)TextureInfo.Dim.y, 0, -10, 10);

        v4 MinV4 = V4_From_V3(V3_From_V2(V2_From_V2i(Min)), 1.0f)*Orthographic;
        v4 MaxV4 = V4_From_V3(V3_From_V2(V2_From_V2i(Max)), 1.0f)*Orthographic;

        IM_Push_Rect2D_Color(MinV4.xy, MaxV4.xy, V4(0.0f, 0.0f, 1.0f, 1.0f));
        IM_Flush(RenderPass);

        GDI_End_Render_Pass(RenderPass);
        GDI_Submit_Render_Pass(RenderPass);

        gdi_texture_readback TextureReadback = {
            .Texture = RenderTarget.Handle,
            .UserData = &TestContext, 
            .ReadbackFunc = Simple_Texture_Readback
        };

        gdi_render_params RenderParams = {
            .TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1}
        };

        GDI_Render(&RenderParams);

        OS_Event_Wait(TestContext.Event);
        OS_Event_Reset(TestContext.Event);

        ASSERT_EQ(TestContext.Format, TextureInfo.Format);
        ASSERT_EQ(TestContext.Dim.x, TextureInfo.Dim.x);
        ASSERT_EQ(TestContext.Dim.y, TextureInfo.Dim.y);

        u32* Texels = (u32*)TestContext.Texels;
        for(s32 y = 0; y < TestContext.Dim.y; y++) {
            for(s32 x = 0; x < TestContext.Dim.x; x++) {
                if(x >= Min.x && y >= Min.y && x < Max.x && y < Max.y) {
                    ASSERT_EQ(*Texels, 0xFFFF0000);
                } else {
                    ASSERT_EQ(*Texels, 0xFF0000FF);
                }
                Texels++;
            }
        }
    }

    //Cleanup final resources
    OS_Event_Delete(Event);
    Texture_Delete(&RenderTarget);
    GDI_Delete_Shader(Shader);
}

UTEST(gdi, SimpleBindGroupTest) {
    scratch Scratch;

    //First create the shader
    gdi_handle Shader;
    {
        gdi_tests* Tests = GDI_Get_Tests();

        const char* ShaderCode = R"(
        struct vs_input {
            float2 Position : POSITION0;
            float4 Color : COLOR0;
        };

        struct vs_output {
            float4 Position : SV_POSITION;
            float4 Color : COLOR0;
        };

        struct draw_data {
            float4x4 Projection;
        };

        ConstantBuffer<draw_data> DrawData : register(b0, space0);

        vs_output VS_Main(vs_input Input) {
            vs_output Result;
            Result.Position = mul(float4(Input.Position, 0.0f, 1.0f), DrawData.Projection);
            Result.Color = Input.Color;
            return Result;
        }

        float4 PS_Main(vs_output Pxl) : SV_TARGET0 {
            return Pxl.Color;
        };

        )";

        IDxcBlob* VtxShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
        IDxcBlob* PxlShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
        ASSERT_TRUE(VtxShader && PxlShader);

        gdi_vtx_attribute Attributes[] = {
            { String_Lit("POSITION"), GDI_FORMAT_R32G32_FLOAT},
            { String_Lit("COLOR"), GDI_FORMAT_R32G32B32A32_FLOAT}
        };

        gdi_vtx_binding VtxBinding = {
            .Stride = sizeof(vtx_p2_c4),
            .Attributes = { .Ptr = Attributes, .Count = Array_Count(Attributes)}
        };

        gdi_shader_create_info CreateInfo = {
            .VS = { .Ptr = (u8*)VtxShader->GetBufferPointer(), .Size = VtxShader->GetBufferSize()},
            .PS = { .Ptr = (u8*)PxlShader->GetBufferPointer(), .Size = PxlShader->GetBufferSize()},
            .BindGroupLayouts = { .Ptr = &Tests->BufferLayout, .Count = 1},
            .VtxBindings = { .Ptr = &VtxBinding, .Count = 1},
            .RenderTargetFormats = {GDI_FORMAT_R8G8B8A8_UNORM}, 
            .DebugName = String_Lit("Simple Bind Group Shader")
        };

        Shader = GDI_Create_Shader(&CreateInfo);
        ASSERT_FALSE(GDI_Is_Null(Shader));

        VtxShader->Release();
        PxlShader->Release();
    }

    //Create the render target and ortho matrix
    texture RenderTarget;
    ASSERT_TRUE(Texture_Create(&RenderTarget, {
        .Format = GDI_FORMAT_R8G8B8A8_UNORM,
        .Dim = V2i(64, 64),
        .Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_READBACK,
    }));
    gdi_texture_info TextureInfo = GDI_Get_Texture_Info(RenderTarget.Handle);

    m4 Orthographic = M4_Orthographic(0, (f32)TextureInfo.Dim.x, (f32)TextureInfo.Dim.y, 0, -10, 10);
    Orthographic = M4_Transpose(&Orthographic);
    
    gpu_buffer MatrixBuffer;
    ASSERT_TRUE(GPU_Buffer_Create(&MatrixBuffer, {
        .Size = sizeof(m4),
        .Usage = GDI_BUFFER_USAGE_CONSTANT,
        .InitialData = Make_Buffer(&Orthographic, sizeof(m4))
    }));

    os_event* Event = OS_Event_Create();

    for(u32 i = 0; i < 10; i++) {
        simple_test_context TestContext = {
            .Texels = (u8*)Arena_Push(Scratch.Arena, TextureInfo.Dim.x*TextureInfo.Dim.y*GDI_Get_Format_Size(TextureInfo.Format)),
            .Event = Event
        };
        
        gdi_render_pass_begin_info BeginInfo = {
            .RenderTargetViews = { RenderTarget.View },
            .ClearColors = { { .ShouldClear = true, .F32 = {1, 0, 0, 1}}}
        };
        gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);

        Render_Set_Shader(RenderPass, Shader);
        Render_Set_Bind_Group(RenderPass, 0, MatrixBuffer.BindGroup);
        
        v2i Min = (TextureInfo.Dim/2)-4;
        v2i Max = (TextureInfo.Dim/2)+4;

        IM_Push_Rect2D_Color(V2_From_V2i(Min), V2_From_V2i(Max), V4(0.0f, 0.0f, 1.0f, 1.0f));
        IM_Flush(RenderPass);

        GDI_End_Render_Pass(RenderPass);
        GDI_Submit_Render_Pass(RenderPass);

        gdi_texture_readback TextureReadback = {
            .Texture = RenderTarget.Handle,
            .UserData = &TestContext, 
            .ReadbackFunc = Simple_Texture_Readback
        };

        gdi_render_params RenderParams = {
            .TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1}
        };

        GDI_Render(&RenderParams);

        OS_Event_Wait(TestContext.Event);
        OS_Event_Reset(TestContext.Event);

        ASSERT_EQ(TestContext.Format, TextureInfo.Format);
        ASSERT_EQ(TestContext.Dim.x, TextureInfo.Dim.x);
        ASSERT_EQ(TestContext.Dim.y, TextureInfo.Dim.y);

        u32* Texels = (u32*)TestContext.Texels;
        for(s32 y = 0; y < TestContext.Dim.y; y++) {
            for(s32 x = 0; x < TestContext.Dim.x; x++) {
                if(x >= Min.x && y >= Min.y && x < Max.x && y < Max.y) {
                    ASSERT_EQ(*Texels, 0xFFFF0000);
                } else {
                    ASSERT_EQ(*Texels, 0xFF0000FF);
                }
                Texels++;
            }
        }
    }

    OS_Event_Delete(Event);
    GPU_Buffer_Delete(&MatrixBuffer);
    Texture_Delete(&RenderTarget);
    GDI_Delete_Shader(Shader);
}

UTEST(gdi, BindlessTextureTest) {
    scratch Scratch;

    struct const_data {
        s32 TextureIndex;
        s32 SamplerIndex;
    };

    //First create the layout
    gdi_handle Layout;
    {
        gdi_bind_group_binding Bindings[] = {
            { .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 64*5 },
            { .Type = GDI_BIND_GROUP_TYPE_SAMPLER, .Count = 1 }
        };

        gdi_bind_group_layout_create_info LayoutInfo = {
            .Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings)}
        };

        Layout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
        ASSERT_FALSE(GDI_Is_Null(Layout));
    }

    //Then create the shader
    gdi_handle Shader;
    {
        gdi_tests* Tests = GDI_Get_Tests();

        const char* ShaderCode = R"(
        struct vs_input {
            float2 Position : POSITION0;
            float2 UV : TEXCOORD0;
            float4 Color : COLOR0;
        };

        struct vs_output {
            float4 Position : SV_POSITION;
            float2 UV : TEXCOORD0;
            float4 Color : COLOR0;
        };

        struct draw_data {
            float4x4 Projection;
        };

        struct const_data {
            int TextureIndex;
            int SamplerIndex;
        };

        ConstantBuffer<draw_data> DrawData : register(b0, space0);
        Texture2D Textures[] : register(t0, space1);
        SamplerState Samplers[] : register(s1, space1);

        [[vk::push_constant]]
        ConstantBuffer<const_data> ConstData : register(b0, space2);

        vs_output VS_Main(vs_input Input) {
            vs_output Result = (vs_output)0;
            Result.Position = mul(float4(Input.Position, 0.0f, 1.0f), DrawData.Projection);
            Result.UV = Input.UV;
            Result.Color = Input.Color;
            return Result;
        }

        float4 PS_Main(vs_output Pxl) : SV_TARGET0 {
            float4 Result = Pxl.Color*Textures[ConstData.TextureIndex].Sample(Samplers[ConstData.SamplerIndex], Pxl.UV);
            return Result;
        }

        )";

        IDxcBlob* VtxShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
        IDxcBlob* PxlShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
        ASSERT_TRUE(VtxShader && PxlShader);

        gdi_vtx_attribute Attributes[] = {
            { String_Lit("POSITION"), GDI_FORMAT_R32G32_FLOAT},
            { String_Lit("TEXCOORD"), GDI_FORMAT_R32G32_FLOAT},
            { String_Lit("COLOR"), GDI_FORMAT_R32G32B32A32_FLOAT}
        };

        gdi_vtx_binding VtxBinding = {
            .Stride = sizeof(vtx_p2_uv2_c4),
            .Attributes = { .Ptr = Attributes, .Count = Array_Count(Attributes)}
        };

        gdi_handle Layouts[] = {
            Tests->BufferLayout, Layout
        };

        gdi_shader_create_info CreateInfo = {
            .VS = { .Ptr = (u8*)VtxShader->GetBufferPointer(), .Size = VtxShader->GetBufferSize()},
            .PS = { .Ptr = (u8*)PxlShader->GetBufferPointer(), .Size = PxlShader->GetBufferSize()},
            .BindGroupLayouts = { .Ptr = Layouts, .Count = Array_Count(Layouts)},
            .PushConstantCount = sizeof(const_data)/4,
            .VtxBindings = { .Ptr = &VtxBinding, .Count = 1},
            .RenderTargetFormats = {GDI_FORMAT_R8G8B8A8_UNORM}, 
            .DebugName = String_Lit("Bindless Texture Shader")
        };

        Shader = GDI_Create_Shader(&CreateInfo);
        ASSERT_FALSE(GDI_Is_Null(Shader));

        VtxShader->Release();
        PxlShader->Release();
    }

    //Next we need the render target and matrix buffer
    texture RenderTarget;
    ASSERT_TRUE(Texture_Create(&RenderTarget, {
        .Format = GDI_FORMAT_R8G8B8A8_UNORM,
        .Dim = V2i(64, 64),
        .Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_READBACK
    }));

    gdi_texture_info TextureInfo = GDI_Get_Texture_Info(RenderTarget.Handle);

    m4 Orthographic = M4_Orthographic(0, (f32)TextureInfo.Dim.x, (f32)TextureInfo.Dim.y, 0, -10, 10);
    Orthographic = M4_Transpose(&Orthographic);
    
    gpu_buffer MatrixBuffer;
    ASSERT_TRUE(GPU_Buffer_Create(&MatrixBuffer, {
        .Size = sizeof(m4),
        .Usage = GDI_BUFFER_USAGE_CONSTANT,
        .InitialData = Make_Buffer(&Orthographic, sizeof(m4))
    }));

    //Setup the bindless texture bind group
    gdi_handle BindlessTextureBindGroup;
    texture BindlessTextures[64];
    gdi_handle BindlessSamplers[1];
    u32 BindlessColors[64];
    u32 BindlessTextureIndices[64];

    //First build the textures and colors. Keep the colors so we can
    //compare against them later
    {
        gdi_sampler_create_info SamplerInfo = {
            .Filter = GDI_FILTER_LINEAR,
            .AddressModeU = GDI_ADDRESS_MODE_CLAMP,
            .AddressModeV = GDI_ADDRESS_MODE_CLAMP
        };

        BindlessSamplers[0] = GDI_Create_Sampler(&SamplerInfo);
        ASSERT_FALSE(GDI_Is_Null(BindlessSamplers[0]));

        for(u32 i = 0; i < 64; i++) {
            v4 Color = V4(Random32_UNorm(), Random32_UNorm(), Random32_UNorm(), 1.0f);
            BindlessColors[i] = U32_Color_From_V4(Color);

            u32 Data[] = {
                BindlessColors[i], BindlessColors[i],
                BindlessColors[i], BindlessColors[i]
            };

            ASSERT_TRUE(Texture_Create(&BindlessTextures[i], {
                .Format = GDI_FORMAT_R8G8B8A8_UNORM,
                .Dim = V2i(2, 2),
                .Usage = GDI_TEXTURE_USAGE_SAMPLED,
                .InitialData = Make_Buffer(Data, sizeof(Data))
            }));

            BindlessTextureIndices[i] = i*5;
        }
    }

	//Then write to the bindless bind group
	{
		gdi_bind_group_create_info BindGroupInfo = {
			.Layout = Layout
		};
		BindlessTextureBindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
		ASSERT_FALSE(GDI_Is_Null(BindlessTextureBindGroup));

		gdi_bind_group_write Writes[Array_Count(BindlessTextures) + 1] = {};
		for (u32 i = 0; i < Array_Count(BindlessTextures); i++) {
			gdi_bind_group_write Write = {
				.DstBindGroup = BindlessTextureBindGroup,
				.DstBinding = 0,
				.DstIndex = BindlessTextureIndices[i],
				.TextureViews = { .Ptr = &BindlessTextures[i].View, .Count = 1 }
			};
			Writes[i] = Write;
		}

		Writes[Array_Count(BindlessTextures)] = {
			.DstBindGroup = BindlessTextureBindGroup,
			.DstBinding = 1,
			.DstIndex = 0,
			.Samplers = { .Ptr = &BindlessSamplers[0], .Count = 1 }
		};

		GDI_Update_Bind_Groups( { .Ptr = Writes, .Count = Array_Count(Writes) }, {});
    }
    
    //Iterate, render, and test
    os_event* Event = OS_Event_Create();

    for(size_t iteration = 0; iteration < 10; iteration++) {
        simple_test_context TestContext = {
            .Texels = (u8*)Arena_Push(Scratch.Arena, TextureInfo.Dim.x*TextureInfo.Dim.y*GDI_Get_Format_Size(GDI_FORMAT_R8G8B8A8_UNORM)),
            .Event = Event
        };

        gdi_render_pass_begin_info BeginInfo = {
            .RenderTargetViews = { RenderTarget.View },
            .ClearColors = { { .ShouldClear = true, .F32 = {1, 0, 0, 1}}}
        };
        gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);

        Render_Set_Shader(RenderPass, Shader);
        
        gdi_handle BindGroups[] = { MatrixBuffer.BindGroup, BindlessTextureBindGroup};
        Render_Set_Bind_Groups(RenderPass, 0, BindGroups, Array_Count(BindGroups));

        v2i P = V2i(0, -8);
        for(size_t i = 0; i < Array_Count(BindlessTextures); i++) {
            if((i % 8) == 0) {
                P.y += 8;
                P.x = 0;
            }

            const_data ConstData = {
                .TextureIndex = (s32)BindlessTextureIndices[i],
                .SamplerIndex = 0
            };
            Render_Set_Push_Constants(RenderPass, &ConstData, sizeof(const_data));

            v2i Min = P;
            v2i Max = Min+4;
            IM_Push_Rect2D_Color_UV_Norm(V2_From_V2i(Min), V2_From_V2i(Max), V4(1.0f, 1.0f, 1.0f, 1.0f));
            IM_Flush(RenderPass);

            P.x += 8;
        }

        GDI_End_Render_Pass(RenderPass);
        GDI_Submit_Render_Pass(RenderPass);

        gdi_texture_readback TextureReadback = {
            .Texture = RenderTarget.Handle,
            .UserData = &TestContext, 
            .ReadbackFunc = Simple_Texture_Readback
        };

        gdi_render_params RenderParams = {
            .TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1}
        };

        GDI_Render(&RenderParams);

        OS_Event_Wait(TestContext.Event);
        OS_Event_Reset(TestContext.Event);

        ASSERT_EQ(TestContext.Format, TextureInfo.Format);
        ASSERT_EQ(TestContext.Dim.x, TextureInfo.Dim.x);
        ASSERT_EQ(TestContext.Dim.y, TextureInfo.Dim.y);

        u32* Texels = (u32*)TestContext.Texels;
        for(s32 y = 0; y < TestContext.Dim.y; y++) {
            for(s32 x = 0; x < TestContext.Dim.x; x++) {

                b32 HitRect = false;
                u32 HitColor = 0;
                v2i P = V2i(0, -8);
                for(size_t i = 0; i < Array_Count(BindlessTextures); i++) {
                    if((i % 8) == 0) {
                        P.y += 8;
                        P.x = 0;
                    }
                    
                    v2i Min = P;
                    v2i Max = Min+4;

                    if(x >= Min.x && y >= Min.y && x < Max.x && y < Max.y) {
                        HitRect = true;
                        HitColor = BindlessColors[i];
                        break;
                    }

                    P.x += 8;
                }

                if(HitRect) {
                    ASSERT_EQ(*Texels, HitColor);
                } else {
                    ASSERT_EQ(*Texels, 0xFF0000FF);
                }

                Texels++;
            }
        }
    }

    OS_Event_Delete(Event);

    GDI_Delete_Bind_Group(BindlessTextureBindGroup);

    for(size_t i = 0; i < Array_Count(BindlessTextures); i++) {
        Texture_Delete(&BindlessTextures[i]);
    }

    for(size_t i = 0; i < Array_Count(BindlessSamplers); i++) {
        GDI_Delete_Sampler(BindlessSamplers[i]);
    }

    GPU_Buffer_Delete(&MatrixBuffer);
    Texture_Delete(&RenderTarget);
    GDI_Delete_Shader(Shader);
    GDI_Delete_Bind_Group_Layout(Layout);
}

const char* G_BoxRectShader = R"(

	struct box_data {
	int4   MinMax;
	float4 Color;
	};

	[[vk::image_format("rgba8")]]
	RWTexture2D<float4> Texture : register(u0, space0);

	StructuredBuffer<box_data> Boxes : register(t0, space1);

	struct const_data {
	int BoxCount;
	};

	[[vk::push_constant]]
	ConstantBuffer<const_data> ConstData : register(b1, space1);


	[numthreads(8, 8, 1)]
	void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
	uint2 Coords = ThreadID.xy;

	int HitBox = false;
	float4 BoxColor = float4(0, 0, 0, 0);

	for(int i = 0; i < ConstData.BoxCount; i++) {
	box_data BoxData = Boxes[i];

	int2 Min = BoxData.MinMax.xy;
	int2 Max = BoxData.MinMax.zw;

	if(Coords.x >= Min.x && Coords.y >= Min.y &&
	Coords.x < Max.x && Coords.y < Max.y) {
                        
	HitBox = true;
	BoxColor = BoxData.Color;
	break;
	}
	}

	if(HitBox) {
	Texture[Coords] = BoxColor;
	} else {
	Texture[Coords] = float4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	}

)";

UTEST(gdi, SimpleComputeTest) {
    struct const_data {
        v2i Min;
        v2i Max;
    };

    scratch Scratch;

    //First create the shader
    gdi_handle Shader;
    {
        const char* ShaderCode = R"(
		struct const_data {
			int2 Min;
			int2 Max;
		};

		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Texture : register(u0, space0);

		[[vk::push_constant]]
		ConstantBuffer<const_data> ConstData : register(b0, space1);

		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			uint2 Coords = ThreadID.xy;
			if(Coords.x >= ConstData.Min.x && Coords.y >= ConstData.Min.y &&
			Coords.x < ConstData.Max.x && Coords.y < ConstData.Max.y) {
				Texture[Coords] = float4(0.0f, 0.0f, 1.0f, 1.0f);
			} else {
				Texture[Coords] = float4(1.0f, 0.0f, 0.0f, 1.0f);
			}
		}
		)";
    
        IDxcBlob* ComputeShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
        ASSERT_TRUE(ComputeShader);

        gdi_bind_group_binding Binding = {
            .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE,
            .Count = 1
        };

        gdi_shader_create_info CreateInfo = {
            .CS = { .Ptr = (u8*)ComputeShader->GetBufferPointer(), .Size = ComputeShader->GetBufferSize()},
            .WritableBindings = { .Ptr = &Binding, .Count = 1},
            .PushConstantCount = sizeof(const_data)/sizeof(u32),
            .DebugName = String_Lit("Simple Compute Shader")
        };

        Shader = GDI_Create_Shader(&CreateInfo);
        ASSERT_FALSE(GDI_Is_Null(Shader));

        ComputeShader->Release();
    }

    texture Texture;
    ASSERT_TRUE(Texture_Create(&Texture, {
        .Format = GDI_FORMAT_R8G8B8A8_UNORM,
        .Dim = V2i(64, 64),
        .Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_READBACK
    }));

    gdi_texture_info TextureInfo = GDI_Get_Texture_Info(Texture.Handle);

    os_event* Event = OS_Event_Create();

    for(u32 i = 0; i < 10; i++) {
        simple_test_context TestContext = {
            .Texels = (u8*)Arena_Push(Scratch.Arena, TextureInfo.Dim.x*TextureInfo.Dim.y*GDI_Get_Format_Size(TextureInfo.Format)),
            .Event = Event
        };

        v2i Min = (TextureInfo.Dim/2)-4;
        v2i Max = (TextureInfo.Dim/2)+4;
        
        const_data ComputeRect = {
            Min, Max
        };

        gdi_dispatch Dispatch = {
            .Shader = Shader,
            .PushConstantCount = sizeof(ComputeRect)/sizeof(u32),
            .ThreadGroupCount = V3i(TextureInfo.Dim.x/8, TextureInfo.Dim.y/8, 1)
        };
        Memory_Copy(Dispatch.PushConstants, &ComputeRect, sizeof(const_data));
        GDI_Submit_Compute_Pass({ .Ptr = &Texture.View, .Count = 1}, {}, { .Ptr = &Dispatch, .Count = 1});

        gdi_texture_readback TextureReadback = {
            .Texture = Texture.Handle,
            .UserData = &TestContext, 
            .ReadbackFunc = Simple_Texture_Readback
        };

        gdi_render_params RenderParams = {
            .TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1}
        };

        GDI_Render(&RenderParams);

        OS_Event_Wait(TestContext.Event);
        OS_Event_Reset(TestContext.Event);

        ASSERT_EQ(TestContext.Format, TextureInfo.Format);
        ASSERT_EQ(TestContext.Dim.x, TextureInfo.Dim.x);
        ASSERT_EQ(TestContext.Dim.y, TextureInfo.Dim.y);

        u32* Texels = (u32*)TestContext.Texels;
        for(s32 y = 0; y < TestContext.Dim.y; y++) {
            for(s32 x = 0; x < TestContext.Dim.x; x++) {
                if(x >= Min.x && y >= Min.y && x < Max.x && y < Max.y) {
                    ASSERT_EQ(*Texels, 0xFFFF0000);
                } else {
                    ASSERT_EQ(*Texels, 0xFF0000FF);
                }
                Texels++;
            }
        }
    }

    OS_Event_Delete(Event);
    Texture_Delete(&Texture);
    GDI_Delete_Shader(Shader);
}

struct box_data {
	v4i MinMax;
	v4 Color;
};

UTEST(gdi, ComputeRenderTest) {
    gdi_tests* Tests = GDI_Get_Tests();
    ASSERT_TRUE(Tests);

    struct const_data {
        s32 BoxCount;
    };

    scratch Scratch;

    //First build the box data
    box_data BoxData[64] = {};
    {
        v2i P = V2i(0, -8);
        for(size_t i = 0; i < Array_Count(BoxData); i++) {
            if((i % 8) == 0) {
                P.y += 8;
                P.x = 0;
            }
            
            v2i Min = P;
            v2i Max = Min+4;

            BoxData[i] = {
                .MinMax = V4i(Min.x, Min.y, Max.x, Max.y),
                .Color = V4(Random32_UNorm(), Random32_UNorm(), Random32_UNorm(), 1.0f)
            };

            P.x += 8;
        }
    }
    

    //Then create the layouts
    gdi_handle ComputeLayout;
    gdi_handle RenderLayout;
    {
        {
            gdi_bind_group_binding Bindings[] = {
                { .Type = GDI_BIND_GROUP_TYPE_STORAGE_BUFFER, .Count = 1}
            };

            gdi_bind_group_layout_create_info CreateInfo = {
                .Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings)}
            };

            ComputeLayout = GDI_Create_Bind_Group_Layout(&CreateInfo);
            ASSERT_FALSE(GDI_Is_Null(ComputeLayout));
        }

        {
            gdi_bind_group_binding Bindings[] = {
                { .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1}
            };

            gdi_bind_group_layout_create_info CreateInfo = {
                .Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings)}
            };

            RenderLayout = GDI_Create_Bind_Group_Layout(&CreateInfo);
            ASSERT_FALSE(GDI_Is_Null(RenderLayout));
        }
    }

    //Build the shaders
    gdi_handle ComputeShader;
    gdi_handle RenderShader;
    {        
		{
			const char* ShaderCode = G_BoxRectShader;

			IDxcBlob* CompShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
			ASSERT_TRUE(CompShader);

			gdi_bind_group_binding Binding = {
				.Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE,
				.Count = 1
			};

			gdi_shader_create_info CreateInfo = {
				.CS = { .Ptr = (u8*)CompShader->GetBufferPointer(), .Size = CompShader->GetBufferSize()},
				.BindGroupLayouts = { .Ptr = &ComputeLayout, .Count = 1 },
				.WritableBindings = { .Ptr = &Binding, .Count = 1},
				.PushConstantCount = sizeof(const_data)/sizeof(u32),
				.DebugName = String_Lit("Compute Shader")
			};

			ComputeShader = GDI_Create_Shader(&CreateInfo);
            
			ASSERT_FALSE(GDI_Is_Null(ComputeShader));

			CompShader->Release();
		}


        {
            const char* ShaderCode = R"(

            Texture2D InputTexture : register(t0, space0);
            float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
                float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
                float4 Result = float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
                return Result;
            }

            float4 PS_Main(float4 Position : SV_POSITION) : SV_TARGET0 {
                return InputTexture.Load(int3(Position.xy, 0));
            }

            )";


            IDxcBlob* VtxShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
            IDxcBlob* PxlShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
            ASSERT_TRUE(VtxShader && PxlShader);

            gdi_shader_create_info CreateInfo = {
                .VS = { .Ptr = (u8*)VtxShader->GetBufferPointer(), .Size = VtxShader->GetBufferSize()},
                .PS = { .Ptr = (u8*)PxlShader->GetBufferPointer(), .Size = PxlShader->GetBufferSize()},
                .BindGroupLayouts = { .Ptr = &RenderLayout, .Count = 1},
                .RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM}
            };

            RenderShader = GDI_Create_Shader(&CreateInfo);
            ASSERT_FALSE(GDI_Is_Null(RenderShader));

            VtxShader->Release();
            PxlShader->Release();
        }
    }

    //Build the bindless buffer and bind group
    gdi_handle BindlessBuffer;
    gdi_handle BindlessBindGroup;
    {
        gdi_buffer_create_info BufferInfo = {
            .Size = sizeof(BoxData),
            .Usage = GDI_BUFFER_USAGE_STORAGE,
            .InitialData = Make_Buffer(BoxData, sizeof(BoxData))
        };

        BindlessBuffer = GDI_Create_Buffer(&BufferInfo);
        ASSERT_FALSE(GDI_Is_Null(BindlessBuffer));

        gdi_bind_group_buffer BindGroupBuffer = { .Buffer = BindlessBuffer };
		gdi_bind_group_write Writes = { .Buffers = { .Ptr = &BindGroupBuffer, .Count = 1 } };
		gdi_bind_group_create_info BindGroupInfo = {
            .Layout = ComputeLayout,
            .Writes = { .Ptr = &Writes, .Count = 1}
        };

        BindlessBindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
        ASSERT_FALSE(GDI_Is_Null(BindlessBindGroup));
    }

    //Build the compute texture
    texture ComputeTexture;
    ASSERT_TRUE(Texture_Create(&ComputeTexture, {
        .Format = GDI_FORMAT_R8G8B8A8_UNORM,
        .Dim = V2i(64, 64),
        .Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_READBACK|GDI_TEXTURE_USAGE_SAMPLED
    }));

    texture RenderTexture;
    ASSERT_TRUE(Texture_Create(&RenderTexture, {
        .Format = GDI_FORMAT_R8G8B8A8_UNORM,
        .Dim = V2i(64, 64),
        .Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_READBACK|GDI_TEXTURE_USAGE_SAMPLED
    }));

    gdi_handle TextureBindGroup;
    gdi_handle TextureBindGroupSwitch;
    {
		gdi_bind_group_write Writes = { .TextureViews = { .Ptr = &ComputeTexture.View, .Count = 1 } };
        gdi_bind_group_create_info BindGroupInfo = {
			.Layout = RenderLayout,
			.Writes = { .Ptr = &Writes, .Count = 1 }
        };
        TextureBindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
        ASSERT_FALSE(GDI_Is_Null(TextureBindGroup));

		Writes.TextureViews.Ptr = &RenderTexture.View;
        TextureBindGroupSwitch = GDI_Create_Bind_Group(&BindGroupInfo);
        ASSERT_FALSE(GDI_Is_Null(TextureBindGroup));
    }

    gdi_texture_info ComputeTextureInfo = GDI_Get_Texture_Info(ComputeTexture.Handle);

    //Dispatch the compute shader
    {
        const_data ConstData = {
            .BoxCount = Array_Count(BoxData)
        };
    
        gdi_dispatch Dispatch = {
            .Shader = ComputeShader,
            .BindGroups = { BindlessBindGroup },
            .PushConstantCount = sizeof(const_data)/sizeof(u32),
            .ThreadGroupCount = V3i(ComputeTextureInfo.Dim.x/8, ComputeTextureInfo.Dim.y/8, 1)
        };
        Memory_Copy(Dispatch.PushConstants, &ConstData, sizeof(const_data));
        GDI_Submit_Compute_Pass({.Ptr = &ComputeTexture.View, .Count = 1}, {}, { .Ptr = &Dispatch, .Count = 1});
    }

    //Copy the compute shader result into the render target
    {
        gdi_render_pass_begin_info BeginInfo = {
            .RenderTargetViews = { RenderTexture.View }
        };

        gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);

        Render_Set_Shader(RenderPass, RenderShader);
        Render_Set_Bind_Group(RenderPass, 0, TextureBindGroup);

        Render_Draw(RenderPass, 3, 0);

        GDI_End_Render_Pass(RenderPass);
        GDI_Submit_Render_Pass(RenderPass);
    }

    //Do it again but this time switch the textures
    {
        gdi_render_pass_begin_info BeginInfo = {
            .RenderTargetViews = { ComputeTexture.View }
        };

        gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);

        Render_Set_Shader(RenderPass, RenderShader);
        Render_Set_Bind_Group(RenderPass, 0, TextureBindGroupSwitch);

        Render_Draw(RenderPass, 3, 0);

        GDI_End_Render_Pass(RenderPass);
        GDI_Submit_Render_Pass(RenderPass);
    }

    os_event* Event = OS_Event_Create();

    simple_test_context TestContext = {
        .Texels = (u8*)Arena_Push(Scratch.Arena, ComputeTextureInfo.Dim.x*ComputeTextureInfo.Dim.y*GDI_Get_Format_Size(GDI_FORMAT_R8G8B8A8_UNORM)),
        .Event = Event
    };

    gdi_texture_readback TextureReadback = {
        .Texture = ComputeTexture.Handle,
        .UserData = &TestContext, 
        .ReadbackFunc = Simple_Texture_Readback
    };

    gdi_render_params RenderParams = {
        .TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1}
    };

    GDI_Render(&RenderParams);

    OS_Event_Wait(TestContext.Event);
    OS_Event_Reset(TestContext.Event);
	OS_Event_Delete(TestContext.Event);

    ASSERT_EQ(TestContext.Format, ComputeTextureInfo.Format);
    ASSERT_EQ(TestContext.Dim.x, ComputeTextureInfo.Dim.x);
    ASSERT_EQ(TestContext.Dim.y, ComputeTextureInfo.Dim.y);

    u32* Texels = (u32*)TestContext.Texels;
    for(s32 y = 0; y < TestContext.Dim.y; y++) {
        for(s32 x = 0; x < TestContext.Dim.x; x++) {

            b32 HitRect = false;
            v4 HitColor = V4_Zero();
            for(size_t i = 0; i < Array_Count(BoxData); i++) {
                box_data Box = BoxData[i];

                v2i Min = Box.MinMax.xy;
                v2i Max = Box.MinMax.zw;

                if(x >= Min.x && y >= Min.y && x < Max.x && y < Max.y) {
                    HitRect = true;
                    HitColor = Box.Color;
                    break;
                }
            }

            if(HitRect) {
                f32 Epsilon = 1.0f/255.0f;
                v4 TexelColor = V4_Color_From_U32(*Texels);
                ASSERT_NEAR(TexelColor.x, HitColor.x, Epsilon);
                ASSERT_NEAR(TexelColor.y, HitColor.y, Epsilon);
                ASSERT_NEAR(TexelColor.z, HitColor.z, Epsilon);
                ASSERT_NEAR(TexelColor.w, HitColor.w, Epsilon);
            } else {
                ASSERT_EQ(*Texels, 0xFF0000FF);
            }

            Texels++;
        }
    }

	GDI_Delete_Bind_Group(TextureBindGroupSwitch);
	GDI_Delete_Bind_Group(TextureBindGroup);
	Texture_Delete(&RenderTexture);
	Texture_Delete(&ComputeTexture);
	GDI_Delete_Bind_Group(BindlessBindGroup);
	GDI_Delete_Buffer(BindlessBuffer);
	GDI_Delete_Shader(RenderShader);
	GDI_Delete_Shader(ComputeShader);
	GDI_Delete_Bind_Group_Layout(RenderLayout);
	GDI_Delete_Bind_Group_Layout(ComputeLayout);
}

struct simple_dynamic_buffer_test_context {
	box_data* BoxData;
	size_t    BoxCount;

	size_t FrameIndex;
	size_t FrameCount;

	os_event* Event;
};

function GDI_TEXTURE_READBACK_DEFINE(Simple_Dynamic_Buffer_Readback) {
	simple_dynamic_buffer_test_context* Context = (simple_dynamic_buffer_test_context *)UserData;

	u32* TexelsAt = (u32*)Texels;
    for(s32 y = 0; y < Dim.y; y++) {
        for(s32 x = 0; x < Dim.x; x++) {

			u32 Texel = *TexelsAt++;

            b32 HitRect = false;
            v4 HitColor = V4_Zero();
            for(size_t i = 0; i < Context->BoxCount; i++) {
                box_data Box = Context->BoxData[i];

                v2i Min = Box.MinMax.xy;
                v2i Max = Box.MinMax.zw;

                if(x >= Min.x && y >= Min.y && x < Max.x && y < Max.y) {
                    HitRect = true;
                    HitColor = Box.Color;
                    break;
                }
            }

            if(HitRect) {
                f32 Epsilon = 1.0f/255.0f;
                v4 TexelColor = V4_Color_From_U32(Texel);
                Assert(Equal_Approx_F32(TexelColor.x, HitColor.x, Epsilon));
                Assert(Equal_Approx_F32(TexelColor.y, HitColor.y, Epsilon));
                Assert(Equal_Approx_F32(TexelColor.z, HitColor.z, Epsilon));
                Assert(Equal_Approx_F32(TexelColor.w, HitColor.w, Epsilon));
            } else {
                Assert(Texel == 0xFF0000FF);
            }
        }
    }

	if (Context->FrameIndex == (Context->FrameCount-1)) {
		OS_Event_Signal(Context->Event);
	}
}

UTEST(gdi, DynamicBufferTest) {
	struct const_data {
        s32 BoxCount;
    };

	scratch Scratch;

	//First create the layout
	gdi_handle Layout;
	{
		gdi_bind_group_binding Bindings[] = {
			{ .Type = GDI_BIND_GROUP_TYPE_STORAGE_BUFFER, .Count = 1}
		};

		gdi_bind_group_layout_create_info CreateInfo = {
			.Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings)}
		};

		Layout = GDI_Create_Bind_Group_Layout(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(Layout));
	}

    //Then create the shader
    gdi_handle Shader;
	{
		const char* ShaderCode = G_BoxRectShader;

		IDxcBlob* CompShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(CompShader);

		gdi_bind_group_binding Binding = {
			.Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE,
			.Count = 1
		};

		gdi_shader_create_info CreateInfo = {
			.CS = { .Ptr = (u8*)CompShader->GetBufferPointer(), .Size = CompShader->GetBufferSize()},
			.BindGroupLayouts = { .Ptr = &Layout, .Count = 1 },
			.WritableBindings = { .Ptr = &Binding, .Count = 1},
			.PushConstantCount = sizeof(const_data)/sizeof(u32),
			.DebugName = String_Lit("Compute Shader")
		};

		Shader = GDI_Create_Shader(&CreateInfo);
            
		ASSERT_FALSE(GDI_Is_Null(Shader));

		CompShader->Release();
	}

	texture Texture;
    ASSERT_TRUE(Texture_Create(&Texture, {
        .Format = GDI_FORMAT_R8G8B8A8_UNORM,
        .Dim = V2i(128, 128),
        .Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_READBACK
    }));

    gdi_texture_info TextureInfo = GDI_Get_Texture_Info(Texture.Handle);

	//Build the bindless buffer and bind group
	u32 BoxDataCount = 64;
    gdi_handle BindlessBuffer;
    gdi_handle BindlessBindGroup;
    {
        gdi_buffer_create_info BufferInfo = {
            .Size = BoxDataCount*sizeof(box_data),
            .Usage = GDI_BUFFER_USAGE_STORAGE|GDI_BUFFER_USAGE_DYNAMIC,
        };

        BindlessBuffer = GDI_Create_Buffer(&BufferInfo);
        ASSERT_FALSE(GDI_Is_Null(BindlessBuffer));

        gdi_bind_group_buffer BindGroupBuffer = { .Buffer = BindlessBuffer };
		gdi_bind_group_write Write = { .Buffers = { .Ptr = &BindGroupBuffer, .Count = 1 } };
		gdi_bind_group_create_info BindGroupInfo = {
            .Layout = Layout,
            .Writes = { .Ptr = &Write, .Count = 1}
        };

        BindlessBindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
        ASSERT_FALSE(GDI_Is_Null(BindlessBindGroup));
    }

	size_t FrameCount = 10;
	os_event* Event = OS_Event_Create();

	for (size_t i = 0; i < FrameCount; i++) {

		v2i Offset = V2i((s32)((i % 2 == 0) ? i * 3 : 0), (s32)((i % 2 == 1) ? i * 3 : 0));
		box_data* BoxData = Arena_Push_Array(Scratch.Arena, BoxDataCount, box_data);

		v2i P = V2i(0, -8)+Offset;
        for(size_t j = 0; j < BoxDataCount; j++) {
            if((j % 8) == 0) {
                P.y += 8;
                P.x = 0;
            }
            
            v2i Min = P;
            v2i Max = Min+4;

            BoxData[j] = {
                .MinMax = V4i(Min.x, Min.y, Max.x, Max.y),
                .Color = V4(Random32_UNorm(), Random32_UNorm(), Random32_UNorm(), 1.0f)
            };

            P.x += 8;
        }

		void* MappedData = GDI_Map_Buffer(BindlessBuffer, 0, 0);
		Memory_Copy(MappedData, BoxData, BoxDataCount * sizeof(box_data));
		GDI_Unmap_Buffer(BindlessBuffer);

		const_data ConstData = {
			.BoxCount = (s32)BoxDataCount
		};

		gdi_dispatch Dispatch = {
			.Shader = Shader,
			.BindGroups = { BindlessBindGroup },
			.PushConstantCount = sizeof(const_data)/sizeof(u32),
			.ThreadGroupCount = V3i(Ceil_U32((f32)TextureInfo.Dim.x/8.0f), Ceil_U32((f32)TextureInfo.Dim.y/8.0f), 1)
		};
		Memory_Copy(Dispatch.PushConstants, &ConstData, sizeof(const_data));
		GDI_Submit_Compute_Pass({.Ptr = &Texture.View, .Count = 1}, {}, { .Ptr = &Dispatch, .Count = 1});
	
		simple_dynamic_buffer_test_context* TestContext = Arena_Push_Struct(Scratch.Arena, simple_dynamic_buffer_test_context);
		TestContext->BoxData = BoxData;
		TestContext->BoxCount = BoxDataCount;
		TestContext->FrameIndex = i;
		TestContext->FrameCount = FrameCount;
		TestContext->Event = Event;

		gdi_texture_readback TextureReadback = {
			.Texture = Texture.Handle,
			.UserData = TestContext,
			.ReadbackFunc = Simple_Dynamic_Buffer_Readback
		};

		gdi_render_params RenderParams = {
			.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1}
		};

		GDI_Render(&RenderParams);
	}

	OS_Event_Wait(Event);
	OS_Event_Delete(Event);

	GDI_Delete_Buffer(BindlessBuffer);
	GDI_Delete_Bind_Group(BindlessBindGroup);
	Texture_Delete(&Texture);
	GDI_Delete_Shader(Shader);
	GDI_Delete_Bind_Group_Layout(Layout);
}

UTEST(gdi, DynamicBindGroupCopyTest) {
	struct const_data {
        s32 BoxCount;
    };

	scratch Scratch;

	//First create the layout
	gdi_handle Layout;
	{
		gdi_bind_group_binding Bindings[] = {
			{ .Type = GDI_BIND_GROUP_TYPE_STORAGE_BUFFER, .Count = 1}
		};

		gdi_bind_group_layout_create_info CreateInfo = {
			.Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings)}
		};

		Layout = GDI_Create_Bind_Group_Layout(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(Layout));
	}

    //Then create the shader
    gdi_handle Shader;
	{
		const char* ShaderCode = G_BoxRectShader;

		IDxcBlob* CompShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(CompShader);

		gdi_bind_group_binding Binding = {
			.Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE,
			.Count = 1
		};

		gdi_shader_create_info CreateInfo = {
			.CS = { .Ptr = (u8*)CompShader->GetBufferPointer(), .Size = CompShader->GetBufferSize()},
			.BindGroupLayouts = { .Ptr = &Layout, .Count = 1 },
			.WritableBindings = { .Ptr = &Binding, .Count = 1},
			.PushConstantCount = sizeof(const_data)/sizeof(u32),
			.DebugName = String_Lit("Compute Shader")
		};

		Shader = GDI_Create_Shader(&CreateInfo);
            
		ASSERT_FALSE(GDI_Is_Null(Shader));

		CompShader->Release();
	}

	texture Texture;
    ASSERT_TRUE(Texture_Create(&Texture, {
        .Format = GDI_FORMAT_R8G8B8A8_UNORM,
        .Dim = V2i(64, 64),
        .Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_READBACK
    }));

    gdi_texture_info TextureInfo = GDI_Get_Texture_Info(Texture.Handle);


	static const size_t FrameCount  = 5;
	static const size_t BufferCount = 5;
	static const size_t BufferIterations = 5;

	u32    BoxDataCount = 64;
	size_t BufferSize = BoxDataCount * sizeof(box_data);

	gdi_handle Buffer;
	gdi_handle BufferBindGroup[BufferCount];
	gdi_handle ShaderBindGroup;
	{
		gdi_buffer_create_info BufferInfo = {
			.Size = BufferSize*BufferCount,
			.Usage = GDI_BUFFER_USAGE_STORAGE|GDI_BUFFER_USAGE_DYNAMIC
		};

		Buffer = GDI_Create_Buffer(&BufferInfo);
		ASSERT_FALSE(GDI_Is_Null(Buffer));

		for (size_t i = 0; i < BufferCount; i++) {
			gdi_bind_group_buffer BindGroupBuffer = {
				.Buffer = Buffer,
				.Offset = BufferSize * i,
				.Size = BufferSize
			};

			gdi_bind_group_write Write = { .Buffers = { .Ptr = &BindGroupBuffer, .Count = 1 } };
			gdi_bind_group_create_info BindGroupInfo = {
				.Layout = Layout,
				.Writes = { .Ptr = &Write, .Count = 1 }
			};

			BufferBindGroup[i] = GDI_Create_Bind_Group(&BindGroupInfo);
			ASSERT_FALSE(GDI_Is_Null(BufferBindGroup[i]));
		}

		gdi_bind_group_create_info ShaderGroupInfo = {
			.Layout = Layout
		};
		ShaderBindGroup = GDI_Create_Bind_Group(&ShaderGroupInfo);
		ASSERT_FALSE(GDI_Is_Null(ShaderBindGroup));
	}

	os_event* Event = OS_Event_Create();

	//First check writes
	for (size_t i = 0; i < FrameCount; i++) {
		for (size_t j = 0; j < BufferCount; j++) {
			gdi_bind_group_buffer BindGroupBuffer = {
				.Buffer = Buffer,
				.Offset = BufferSize * j,
				.Size = BufferSize
			};

			gdi_bind_group_write Write = {
				.DstBindGroup = ShaderBindGroup,
				.Buffers = { .Ptr = &BindGroupBuffer, .Count = 1 }
			};

			GDI_Update_Bind_Groups({ .Ptr = &Write, .Count = 1 }, { });

			for (size_t k = 0; k < BufferIterations; k++) {
				v2i Offset = V2i((s32)((k % 2 == 0) ? k * 3 : 0), (s32)((k % 2 == 1) ? k * 3 : 0));
				box_data* BoxData = Arena_Push_Array(Scratch.Arena, BoxDataCount, box_data);

				v2i P = V2i(0, -8)+Offset;
				for(size_t b = 0; b < BoxDataCount; b++) {
					if((b % 8) == 0) {
						P.y += 8;
						P.x = 0;
					}
            
					v2i Min = P;
					v2i Max = Min+4;

					BoxData[b] = {
						.MinMax = V4i(Min.x, Min.y, Max.x, Max.y),
						.Color = V4(Random32_UNorm(), Random32_UNorm(), Random32_UNorm(), 1.0f)
					};

					P.x += 8;
				}

				void* MappedData = GDI_Map_Buffer(Buffer, BufferSize*j, BufferSize);
				Memory_Copy(MappedData, BoxData, BoxDataCount * sizeof(box_data));
				GDI_Unmap_Buffer(Buffer);

				const_data ConstData = {
					.BoxCount = (s32)BoxDataCount
				};

				gdi_dispatch Dispatch = {
					.Shader = Shader,
					.BindGroups = { ShaderBindGroup },
					.PushConstantCount = sizeof(const_data)/sizeof(u32),
					.ThreadGroupCount = V3i(Ceil_U32((f32)TextureInfo.Dim.x/8.0f), Ceil_U32((f32)TextureInfo.Dim.y/8.0f), 1)
				};
				Memory_Copy(Dispatch.PushConstants, &ConstData, sizeof(const_data));
				GDI_Submit_Compute_Pass({.Ptr = &Texture.View, .Count = 1}, {}, { .Ptr = &Dispatch, .Count = 1});
	
				simple_dynamic_buffer_test_context* TestContext = Arena_Push_Struct(Scratch.Arena, simple_dynamic_buffer_test_context);
				TestContext->BoxData = BoxData;
				TestContext->BoxCount = BoxDataCount;
				TestContext->FrameIndex = j*BufferCount+k;
				TestContext->FrameCount = BufferCount*BufferIterations;
				TestContext->Event = Event;

				gdi_texture_readback TextureReadback = {
					.Texture = Texture.Handle,
					.UserData = TestContext,
					.ReadbackFunc = Simple_Dynamic_Buffer_Readback
				};

				gdi_render_params RenderParams = {
					.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1}
				};

				GDI_Render(&RenderParams);
			}
		}
		
		OS_Event_Wait(Event);
		OS_Event_Reset(Event);
	}

	//Then check copies
	for (size_t i = 0; i < FrameCount; i++) {
		for (size_t j = 0; j < BufferCount; j++) {
			gdi_bind_group_copy Copy = {
				.DstBindGroup = ShaderBindGroup,
				.SrcBindGroup = BufferBindGroup[j],
				.Count = 1
			};

			GDI_Update_Bind_Groups({}, { .Ptr = &Copy, .Count = 1 });

			for (size_t k = 0; k < BufferIterations; k++) {
				v2i Offset = V2i((s32)((k % 2 == 0) ? k * 3 : 0), (s32)((k % 2 == 1) ? k * 3 : 0));
				box_data* BoxData = Arena_Push_Array(Scratch.Arena, BoxDataCount, box_data);

				v2i P = V2i(0, -8)+Offset;
				for(size_t b = 0; b < BoxDataCount; b++) {
					if((b % 8) == 0) {
						P.y += 8;
						P.x = 0;
					}
            
					v2i Min = P;
					v2i Max = Min+4;

					BoxData[b] = {
						.MinMax = V4i(Min.x, Min.y, Max.x, Max.y),
						.Color = V4(Random32_UNorm(), Random32_UNorm(), Random32_UNorm(), 1.0f)
					};

					P.x += 8;
				}

				void* MappedData = GDI_Map_Buffer(Buffer, BufferSize*j, BufferSize);
				Memory_Copy(MappedData, BoxData, BoxDataCount * sizeof(box_data));
				GDI_Unmap_Buffer(Buffer);

				const_data ConstData = {
					.BoxCount = (s32)BoxDataCount
				};

				gdi_dispatch Dispatch = {
					.Shader = Shader,
					.BindGroups = { ShaderBindGroup },
					.PushConstantCount = sizeof(const_data)/sizeof(u32),
					.ThreadGroupCount = V3i(Ceil_U32((f32)TextureInfo.Dim.x/8.0f), Ceil_U32((f32)TextureInfo.Dim.y/8.0f), 1)
				};
				Memory_Copy(Dispatch.PushConstants, &ConstData, sizeof(const_data));
				GDI_Submit_Compute_Pass({.Ptr = &Texture.View, .Count = 1}, {}, { .Ptr = &Dispatch, .Count = 1});
	
				simple_dynamic_buffer_test_context* TestContext = Arena_Push_Struct(Scratch.Arena, simple_dynamic_buffer_test_context);
				TestContext->BoxData = BoxData;
				TestContext->BoxCount = BoxDataCount;
				TestContext->FrameIndex = j*BufferCount+k;
				TestContext->FrameCount = BufferCount*BufferIterations;
				TestContext->Event = Event;

				gdi_texture_readback TextureReadback = {
					.Texture = Texture.Handle,
					.UserData = TestContext,
					.ReadbackFunc = Simple_Dynamic_Buffer_Readback
				};

				gdi_render_params RenderParams = {
					.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1}
				};

				GDI_Render(&RenderParams);
			}
		}
		
		OS_Event_Wait(Event);
		OS_Event_Reset(Event);
	}

	OS_Event_Delete(Event);

	for (size_t i = 0; i < Array_Count(BufferBindGroup); i++) {
		GDI_Delete_Bind_Group(BufferBindGroup[i]);
	}
	GDI_Delete_Bind_Group(ShaderBindGroup);
	GDI_Delete_Buffer(Buffer);
	Texture_Delete(&Texture);
	GDI_Delete_Shader(Shader);
	GDI_Delete_Bind_Group_Layout(Layout);
}