struct gdi_tests {
    gdi*                GDI;
    IDxcCompiler3*      Compiler;
    IDxcUtils*          Utils;
    IDxcIncludeHandler* IncludeHandler;

    gdi_layout BufferLayout;
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
    gdi_texture      Handle;
    gdi_texture_view View;
};

struct texture_create_info {
    gdi_format Format;
    v2i Dim;
    gdi_texture_usage Usage;
    buffer InitialData;
};

struct gpu_buffer {
    gdi_buffer     Handle;
    gdi_bind_group BindGroup;
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
    gdi_texture Handle;
    gdi_texture_view View;

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
    gdi_buffer Handle = GDI_Create_Buffer(&BufferInfo);
    if(GDI_Is_Null(Handle)) return false;

    gdi_bind_group_buffer BindGroupBuffer = { .Buffer = Handle };
	gdi_bind_group_write Write = { .Buffers = { .Ptr = &BindGroupBuffer, .Count = 1 } };
	gdi_bind_group_create_info BindGroupInfo = {
		.Layout = Tests->BufferLayout,
		.Writes = { .Ptr = &Write, .Count = 1 }
    };

    gdi_bind_group BindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
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
};

function GDI_TEXTURE_READBACK_DEFINE(Simple_Texture_Readback) {
    simple_test_context* SimpleTestContext = (simple_test_context*)UserData;
    Memory_Copy(SimpleTestContext->Texels, Texels, Dim.x*Dim.y*GDI_Get_Format_Size(Format));
    SimpleTestContext->Dim = Dim;
    SimpleTestContext->Format = Format;
}

UTEST(gdi, SimpleTest) {
    scratch Scratch;

    //First build the shader
    gdi_shader Shader;
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

    //Iterate, render, and test
    for(u32 i = 0; i < 10; i++) {
        gdi_texture_info TextureInfo = GDI_Get_Texture_Info(RenderTarget.Handle);

        simple_test_context TestContext = {
            .Texels = (u8*)Arena_Push(Scratch.Arena, TextureInfo.Dim.x*TextureInfo.Dim.y*GDI_Get_Format_Size(TextureInfo.Format)),
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
        GDI_Flush();

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

    Texture_Delete(&RenderTarget);
    GDI_Delete_Shader(Shader);
}

UTEST(gdi, SimpleBindGroupTest) {
    scratch Scratch;

    //First create the shader
    gdi_shader Shader;
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

    for(u32 i = 0; i < 10; i++) {
        simple_test_context TestContext = {
            .Texels = (u8*)Arena_Push(Scratch.Arena, TextureInfo.Dim.x*TextureInfo.Dim.y*GDI_Get_Format_Size(TextureInfo.Format)),
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
        GDI_Flush();

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
    gdi_layout Layout;
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
    gdi_shader Shader;
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

        gdi_layout Layouts[] = {
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
    gdi_bind_group BindlessTextureBindGroup;
    texture BindlessTextures[64];
    gdi_sampler BindlessSamplers[1];
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
    for(size_t iteration = 0; iteration < 10; iteration++) {
        simple_test_context TestContext = {
            .Texels = (u8*)Arena_Push(Scratch.Arena, TextureInfo.Dim.x*TextureInfo.Dim.y*GDI_Get_Format_Size(GDI_FORMAT_R8G8B8A8_UNORM)),
        };

        gdi_render_pass_begin_info BeginInfo = {
            .RenderTargetViews = { RenderTarget.View },
            .ClearColors = { { .ShouldClear = true, .F32 = {1, 0, 0, 1}}}
        };
        gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);

        Render_Set_Shader(RenderPass, Shader);
        
        gdi_bind_group BindGroups[] = { MatrixBuffer.BindGroup, BindlessTextureBindGroup};
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
        GDI_Flush();

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
    gdi_shader Shader;
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

    for(u32 i = 0; i < 10; i++) {
        simple_test_context TestContext = {
            .Texels = (u8*)Arena_Push(Scratch.Arena, TextureInfo.Dim.x*TextureInfo.Dim.y*GDI_Get_Format_Size(TextureInfo.Format)),
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
        GDI_Flush();

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
    gdi_layout ComputeLayout;
    gdi_layout RenderLayout;
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
    gdi_shader ComputeShader;
    gdi_shader RenderShader;
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
    gdi_buffer BindlessBuffer;
    gdi_bind_group BindlessBindGroup;
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

    gdi_bind_group TextureBindGroup;
    gdi_bind_group TextureBindGroupSwitch;
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

    simple_test_context TestContext = {
        .Texels = (u8*)Arena_Push(Scratch.Arena, ComputeTextureInfo.Dim.x*ComputeTextureInfo.Dim.y*GDI_Get_Format_Size(GDI_FORMAT_R8G8B8A8_UNORM)),
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
    GDI_Flush();

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
}

UTEST(gdi, DynamicBufferTest) {
	struct const_data {
        s32 BoxCount;
    };

	scratch Scratch;

	//First create the layout
	gdi_layout Layout;
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
    gdi_shader Shader;
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
    gdi_buffer BindlessBuffer;
    gdi_bind_group BindlessBindGroup;
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

	GDI_Flush();

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
	gdi_layout Layout;
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
    gdi_shader Shader;
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

	gdi_buffer Buffer;
	gdi_bind_group BufferBindGroup[BufferCount];
	gdi_bind_group ShaderBindGroup;
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
		
		GDI_Flush();
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
		
		GDI_Flush();
	}

	for (size_t i = 0; i < Array_Count(BufferBindGroup); i++) {
		GDI_Delete_Bind_Group(BufferBindGroup[i]);
	}
	GDI_Delete_Bind_Group(ShaderBindGroup);
	GDI_Delete_Buffer(Buffer);
	Texture_Delete(&Texture);
	GDI_Delete_Shader(Shader);
	GDI_Delete_Bind_Group_Layout(Layout);
}

UTEST(gdi, AsyncComputeOverlapTest) {
    if(!GDI_Is_Async_Compute_Supported()) return;

	scratch Scratch;

	enum {
		QUERY_COMPUTE_BEGIN = 0,
		QUERY_COMPUTE_END   = 1,
		QUERY_RENDER_BEGIN  = 2,
		QUERY_RENDER_END    = 3,
		QUERY_COUNT         = 4
	};

	gdi_query_pool_create_info QueryPoolInfo = {
		.Count = QUERY_COUNT,
		.DebugName = String_Lit("Async Overlap Query Pool")
	};
	gdi_query_pool QueryPool = GDI_Create_Query_Pool(&QueryPoolInfo);
	ASSERT_FALSE(GDI_Is_Null(QueryPool));

	//
	// Heavy compute shader: tight loop per thread to burn GPU time
	//
	gdi_shader ComputeShader;
	{
		const char* ShaderCode = R"(
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);

		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			float4 Accum = float4(0, 0, 0, 0);
			for (int i = 0; i < 100000; i++) {
				Accum += float4(sin((float)i * 0.001f), cos((float)i * 0.001f), 0, 0);
			}
			Output[ThreadID.xy] = Accum * 0.00001f;
		}
		)";

		IDxcBlob* CSBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(CSBlob);

		gdi_bind_group_binding Binding = {
			.Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE,
			.Count = 1
		};

		gdi_shader_create_info CreateInfo = {
			.CS = { .Ptr = (u8*)CSBlob->GetBufferPointer(), .Size = CSBlob->GetBufferSize() },
			.WritableBindings = { .Ptr = &Binding, .Count = 1 },
			.DebugName = String_Lit("Heavy Compute Shader")
		};

		ComputeShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeShader));
		CSBlob->Release();
	}

	//
	// Heavy render shader: fullscreen quad with expensive fragment shader
	//
	gdi_shader RenderShader;
	{
		const char* ShaderCode = R"(
		float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
			float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
			return float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
		}

		float4 PS_Main(float4 Position : SV_POSITION) : SV_TARGET0 {
			float4 Accum = float4(0, 0, 0, 0);
			for (int i = 0; i < 100000; i++) {
				Accum += float4(sin((float)i * 0.001f), cos((float)i * 0.001f), 0, 0);
			}
			return Accum * 0.00001f;
		}
		)";

		IDxcBlob* VtxBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
		IDxcBlob* PxlBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VtxBlob && PxlBlob);

		gdi_shader_create_info CreateInfo = {
			.VS = { .Ptr = (u8*)VtxBlob->GetBufferPointer(), .Size = VtxBlob->GetBufferSize() },
			.PS = { .Ptr = (u8*)PxlBlob->GetBufferPointer(), .Size = PxlBlob->GetBufferSize() },
			.RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
			.DebugName = String_Lit("Heavy Render Shader")
		};

		RenderShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(RenderShader));
		VtxBlob->Release();
		PxlBlob->Release();
	}

	// Compute output texture (storage)
	texture ComputeTexture;
	ASSERT_TRUE(Texture_Create(&ComputeTexture, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_STORAGE
	}));

	// Render target
	texture RenderTarget;
	ASSERT_TRUE(Texture_Create(&RenderTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET
	}));

	gdi_texture_info ComputeTextureInfo = GDI_Get_Texture_Info(ComputeTexture.Handle);

	//
	// Submit: timestamp -> compute -> timestamp -> timestamp -> render -> timestamp -> execute
	//
	GDI_Write_Timestamp(QueryPool, QUERY_COMPUTE_BEGIN);

	{
		gdi_dispatch Dispatch = {
			.Shader = ComputeShader,
			.ThreadGroupCount = V3i(ComputeTextureInfo.Dim.x / 8, ComputeTextureInfo.Dim.y / 8, 1)
		};
		GDI_Submit_Async_Compute_Pass({ .Ptr = &ComputeTexture.View, .Count = 1 }, {}, { .Ptr = &Dispatch, .Count = 1 });
	}

	GDI_Write_Timestamp(QueryPool, QUERY_COMPUTE_END);
	GDI_Write_Timestamp(QueryPool, QUERY_RENDER_BEGIN);

	{
		gdi_render_pass_begin_info BeginInfo = {
			.RenderTargetViews = { RenderTarget.View },
			.ClearColors = { { .ShouldClear = true, .F32 = { 0, 0, 0, 1 } } }
		};

		gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);
		Render_Set_Shader(RenderPass, RenderShader);
		Render_Draw(RenderPass, 3, 0);
		GDI_End_Render_Pass(RenderPass);
		GDI_Submit_Render_Pass(RenderPass);
	}

	GDI_Write_Timestamp(QueryPool, QUERY_RENDER_END);

	gdi_render_params RenderParams = {};
	GDI_Render(&RenderParams);
	GDI_Flush();

	//
	// Read timestamps and validate overlap
	//
	gdi_timestamp_result Results[QUERY_COUNT];
	b32 GotResults = GDI_Get_Query_Results(QueryPool, 0, QUERY_COUNT, Results);
	ASSERT_TRUE(GotResults);

	for (u32 i = 0; i < QUERY_COUNT; i++) {
		ASSERT_TRUE(Results[i].Available);
	}

	f64 Period = GDI_Get_Timestamp_Period();
	ASSERT_GT(Period, 0.0);

	f64 ComputeMs = (f64)(Results[QUERY_COMPUTE_END].Ticks - Results[QUERY_COMPUTE_BEGIN].Ticks) * Period / 1000000.0;
	f64 RenderMs  = (f64)(Results[QUERY_RENDER_END].Ticks - Results[QUERY_RENDER_BEGIN].Ticks) * Period / 1000000.0;

	// Both workloads must be heavy enough to measure (> 0.1ms)
	ASSERT_GT(ComputeMs, 0.1);
	ASSERT_GT(RenderMs, 0.1);

	// Overlap check: compute and render time ranges must overlap.
	// On a single queue (no async compute), these will be serial:
	//   COMPUTE_BEGIN < COMPUTE_END <= RENDER_BEGIN < RENDER_END
	// With async compute, they overlap:
	//   COMPUTE_BEGIN < RENDER_END AND RENDER_BEGIN < COMPUTE_END
	b32 HasOverlap = Results[QUERY_COMPUTE_END].Ticks > Results[QUERY_RENDER_BEGIN].Ticks &&
					 Results[QUERY_COMPUTE_BEGIN].Ticks < Results[QUERY_RENDER_END].Ticks;

	ASSERT_TRUE(HasOverlap);

	// Cleanup
	Texture_Delete(&RenderTarget);
	Texture_Delete(&ComputeTexture);
	GDI_Delete_Shader(RenderShader);
	GDI_Delete_Shader(ComputeShader);
	GDI_Delete_Query_Pool(QueryPool);
}

UTEST(gdi, AsyncComputeSignalWaitTest) {
	scratch Scratch;

	gdi_shader ComputeShader;
	{
		const char* ShaderCode = R"(
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);

		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			Output[ThreadID.xy] = float4(0, 0, 1, 1);
		}
		)";

		IDxcBlob* CSBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(CSBlob);

		gdi_bind_group_binding Binding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CreateInfo = {
			.CS = { .Ptr = (u8*)CSBlob->GetBufferPointer(), .Size = CSBlob->GetBufferSize() },
			.WritableBindings = { .Ptr = &Binding, .Count = 1 },
			.DebugName = String_Lit("Async Write Blue Shader")
		};

		ComputeShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeShader));
		CSBlob->Release();
	}

	gdi_shader RenderShader;
	gdi_layout RenderLayout;
	{
		gdi_bind_group_binding Bindings[] = {
			{ .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1 }
		};
		gdi_bind_group_layout_create_info LayoutInfo = {
			.Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings) }
		};
		RenderLayout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
		ASSERT_FALSE(GDI_Is_Null(RenderLayout));

		const char* ShaderCode = R"(
		Texture2D InputTexture : register(t0, space0);
		float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
			float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
			return float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
		}
		float4 PS_Main(float4 Position : SV_POSITION) : SV_TARGET0 {
			return InputTexture.Load(int3(Position.xy, 0));
		}
		)";

		IDxcBlob* VtxBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
		IDxcBlob* PxlBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VtxBlob && PxlBlob);

		gdi_shader_create_info CreateInfo = {
			.VS = { .Ptr = (u8*)VtxBlob->GetBufferPointer(), .Size = VtxBlob->GetBufferSize() },
			.PS = { .Ptr = (u8*)PxlBlob->GetBufferPointer(), .Size = PxlBlob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &RenderLayout, .Count = 1 },
			.RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
			.DebugName = String_Lit("Async Blit Shader")
		};

		RenderShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(RenderShader));
		VtxBlob->Release();
		PxlBlob->Release();
	}

	texture ComputeTexture;
	ASSERT_TRUE(Texture_Create(&ComputeTexture, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));

	texture RenderTarget;
	ASSERT_TRUE(Texture_Create(&RenderTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_READBACK
	}));

	gdi_bind_group TextureBindGroup;
	{
		gdi_bind_group_write Write = { .TextureViews = { .Ptr = &ComputeTexture.View, .Count = 1 } };
		gdi_bind_group_create_info BindGroupInfo = {
			.Layout = RenderLayout,
			.Writes = { .Ptr = &Write, .Count = 1 }
		};
		TextureBindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
		ASSERT_FALSE(GDI_Is_Null(TextureBindGroup));
	}

	gdi_texture_info TexInfo = GDI_Get_Texture_Info(RenderTarget.Handle);

	// Async compute writes blue to ComputeTexture
	gdi_dispatch Dispatch = {
		.Shader = ComputeShader,
		.ThreadGroupCount = V3i(64/8, 64/8, 1)
	};
	gdi_signal ComputeSignal = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &ComputeTexture.View, .Count = 1 }, {}, { .Ptr = &Dispatch, .Count = 1 });

	// Wait for async compute to finish before reading its output
	GDI_Wait_Signal(ComputeSignal);

	// Render pass blits compute result into render target
	{
		gdi_render_pass_begin_info BeginInfo = { .RenderTargetViews = { RenderTarget.View } };
		gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);
		Render_Set_Shader(RenderPass, RenderShader);
		Render_Set_Bind_Group(RenderPass, 0, TextureBindGroup);
		Render_Draw(RenderPass, 3, 0);
		GDI_End_Render_Pass(RenderPass);
		GDI_Submit_Render_Pass(RenderPass);
	}

	simple_test_context TestContext = {
		.Texels = (u8*)Arena_Push(Scratch.Arena, TexInfo.Dim.x * TexInfo.Dim.y * GDI_Get_Format_Size(TexInfo.Format)),
	};

	gdi_texture_readback TextureReadback = {
		.Texture = RenderTarget.Handle,
		.UserData = &TestContext,
		.ReadbackFunc = Simple_Texture_Readback
	};

	gdi_render_params RenderParams = {
		.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 }
	};

	GDI_Render(&RenderParams);
	GDI_Flush();

	ASSERT_EQ(TestContext.Dim.x, TexInfo.Dim.x);
	ASSERT_EQ(TestContext.Dim.y, TexInfo.Dim.y);

	u32* Texels = (u32*)TestContext.Texels;
	for(s32 y = 0; y < TestContext.Dim.y; y++) {
		for(s32 x = 0; x < TestContext.Dim.x; x++) {
			ASSERT_EQ(*Texels, 0xFFFF0000);
			Texels++;
		}
	}

	GDI_Delete_Bind_Group(TextureBindGroup);
	Texture_Delete(&RenderTarget);
	Texture_Delete(&ComputeTexture);
	GDI_Delete_Shader(RenderShader);
	GDI_Delete_Shader(ComputeShader);
	GDI_Delete_Bind_Group_Layout(RenderLayout);
}

UTEST(gdi, GraphicsToAsyncComputeWaitTest) {
	scratch Scratch;

	gdi_shader RenderShader;
	{
		const char* ShaderCode = R"(
		float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
			float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
			return float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
		}
		float4 PS_Main(float4 Position : SV_POSITION) : SV_TARGET0 {
			return float4(0, 1, 0, 1);
		}
		)";

		IDxcBlob* VtxBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
		IDxcBlob* PxlBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VtxBlob && PxlBlob);

		gdi_shader_create_info CreateInfo = {
			.VS = { .Ptr = (u8*)VtxBlob->GetBufferPointer(), .Size = VtxBlob->GetBufferSize() },
			.PS = { .Ptr = (u8*)PxlBlob->GetBufferPointer(), .Size = PxlBlob->GetBufferSize() },
			.RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
			.DebugName = String_Lit("G2C Green Shader")
		};

		RenderShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(RenderShader));
		VtxBlob->Release();
		PxlBlob->Release();
	}

	gdi_shader ComputeShader;
	gdi_layout ComputeLayout;
	{
		gdi_bind_group_binding Bindings[] = {
			{ .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1 }
		};
		gdi_bind_group_layout_create_info LayoutInfo = {
			.Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings) }
		};
		ComputeLayout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeLayout));

		const char* ShaderCode = R"(
		Texture2D<float4> InputTexture : register(t0, space1);

		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);

		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			float4 Color = InputTexture.Load(int3(ThreadID.xy, 0));
			Output[ThreadID.xy] = Color;
		}
		)";

		IDxcBlob* CSBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(CSBlob);

		gdi_bind_group_binding WBinding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CreateInfo = {
			.CS = { .Ptr = (u8*)CSBlob->GetBufferPointer(), .Size = CSBlob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &ComputeLayout, .Count = 1 },
			.WritableBindings = { .Ptr = &WBinding, .Count = 1 },
			.DebugName = String_Lit("G2C Copy Compute Shader")
		};

		ComputeShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeShader));
		CSBlob->Release();
	}

	texture RenderTarget;
	ASSERT_TRUE(Texture_Create(&RenderTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_SAMPLED
	}));

	texture ComputeOutput;
	ASSERT_TRUE(Texture_Create(&ComputeOutput, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_READBACK
	}));

	gdi_bind_group InputBindGroup;
	{
		gdi_bind_group_write Write = { .TextureViews = { .Ptr = &RenderTarget.View, .Count = 1 } };
		gdi_bind_group_create_info BindGroupInfo = {
			.Layout = ComputeLayout,
			.Writes = { .Ptr = &Write, .Count = 1 }
		};
		InputBindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
		ASSERT_FALSE(GDI_Is_Null(InputBindGroup));
	}

	gdi_texture_info TexInfo = GDI_Get_Texture_Info(ComputeOutput.Handle);

	// Step 1: Render pass fills render target with green
	gdi_render_pass_begin_info BeginInfo = {
		.RenderTargetViews = { RenderTarget.View },
		.ClearColors = { { .ShouldClear = true, .F32 = { 0, 0, 0, 1 } } }
	};
	gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);
	Render_Set_Shader(RenderPass, RenderShader);
	Render_Draw(RenderPass, 3, 0);
	GDI_End_Render_Pass(RenderPass);
	gdi_signal GraphicsSignal = GDI_Submit_Render_Pass(RenderPass);

	// Step 2: Async compute waits on render pass, then copies render target to compute output
	GDI_Wait_Signal(GraphicsSignal);

	gdi_dispatch Dispatch = {
		.Shader = ComputeShader,
		.BindGroups = { InputBindGroup },
		.ThreadGroupCount = V3i(64/8, 64/8, 1)
	};
	gdi_signal ComputeSignal = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &ComputeOutput.View, .Count = 1 }, {}, { .Ptr = &Dispatch, .Count = 1 });

	simple_test_context TestContext = {
		.Texels = (u8*)Arena_Push(Scratch.Arena, TexInfo.Dim.x * TexInfo.Dim.y * GDI_Get_Format_Size(TexInfo.Format)),
	};

	gdi_texture_readback TextureReadback = {
		.Texture = ComputeOutput.Handle,
		.UserData = &TestContext,
		.ReadbackFunc = Simple_Texture_Readback
	};

	gdi_render_params RenderParams = {
		.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 }
	};

	GDI_Render(&RenderParams);
	GDI_Flush();

	ASSERT_EQ(TestContext.Dim.x, TexInfo.Dim.x);
	ASSERT_EQ(TestContext.Dim.y, TexInfo.Dim.y);

	u32* Texels = (u32*)TestContext.Texels;
	for(s32 y = 0; y < TestContext.Dim.y; y++) {
		for(s32 x = 0; x < TestContext.Dim.x; x++) {
			ASSERT_EQ(*Texels, 0xFF00FF00);
			Texels++;
		}
	}

	GDI_Delete_Bind_Group(InputBindGroup);
	Texture_Delete(&ComputeOutput);
	Texture_Delete(&RenderTarget);
	GDI_Delete_Shader(ComputeShader);
	GDI_Delete_Shader(RenderShader);
	GDI_Delete_Bind_Group_Layout(ComputeLayout);
}

UTEST(gdi, MultipleAsyncComputeSignalTest) {
	scratch Scratch;

	gdi_shader RedShader;
	gdi_shader BlueShader;
	{
		auto MakeColorShader = [](const char* ShaderCode) -> gdi_shader {
			IDxcBlob* CSBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
			if(!CSBlob) return GDI_Null_Handle(gdi_shader);

			gdi_bind_group_binding Binding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
			gdi_shader_create_info CreateInfo = {
				.CS = { .Ptr = (u8*)CSBlob->GetBufferPointer(), .Size = CSBlob->GetBufferSize() },
				.WritableBindings = { .Ptr = &Binding, .Count = 1 },
			};
			gdi_shader Result = GDI_Create_Shader(&CreateInfo);
			CSBlob->Release();
			return Result;
		};

		const char* RedCode = R"(
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) { Output[ThreadID.xy] = float4(1, 0, 0, 1); }
		)";

		const char* BlueCode = R"(
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) { Output[ThreadID.xy] = float4(0, 0, 1, 1); }
		)";

		RedShader = MakeColorShader(RedCode);
		ASSERT_FALSE(GDI_Is_Null(RedShader));
		BlueShader = MakeColorShader(BlueCode);
		ASSERT_FALSE(GDI_Is_Null(BlueShader));
	}

	gdi_shader BlitShader;
	gdi_layout BlitLayout;
	{
		gdi_bind_group_binding Bindings[] = {
			{ .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1 }
		};
		gdi_bind_group_layout_create_info LayoutInfo = {
			.Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings) }
		};
		BlitLayout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
		ASSERT_FALSE(GDI_Is_Null(BlitLayout));

		const char* ShaderCode = R"(
		Texture2D InputTexture : register(t0, space0);
		float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
			float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
			return float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
		}
		float4 PS_Main(float4 Position : SV_POSITION) : SV_TARGET0 {
			return InputTexture.Load(int3(Position.xy, 0));
		}
		)";

		IDxcBlob* VtxBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
		IDxcBlob* PxlBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VtxBlob && PxlBlob);

		gdi_shader_create_info CreateInfo = {
			.VS = { .Ptr = (u8*)VtxBlob->GetBufferPointer(), .Size = VtxBlob->GetBufferSize() },
			.PS = { .Ptr = (u8*)PxlBlob->GetBufferPointer(), .Size = PxlBlob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &BlitLayout, .Count = 1 },
			.RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
		};

		BlitShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(BlitShader));
		VtxBlob->Release();
		PxlBlob->Release();
	}

	texture TexA, TexB;
	ASSERT_TRUE(Texture_Create(&TexA, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));
	ASSERT_TRUE(Texture_Create(&TexB, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));

	texture FinalTarget;
	ASSERT_TRUE(Texture_Create(&FinalTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_READBACK
	}));

	gdi_bind_group BindGroupB;
	{
		gdi_bind_group_write Write = { .TextureViews = { .Ptr = &TexB.View, .Count = 1 } };
		gdi_bind_group_create_info Info = { .Layout = BlitLayout, .Writes = { .Ptr = &Write, .Count = 1 } };
		BindGroupB = GDI_Create_Bind_Group(&Info);
		ASSERT_FALSE(GDI_Is_Null(BindGroupB));
	}

	v3i TGC = V3i(64/8, 64/8, 1);

	// Two independent async compute passes writing different textures
	gdi_dispatch DispatchA = { .Shader = RedShader, .ThreadGroupCount = TGC };
	gdi_signal SignalA = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &TexA.View, .Count = 1 }, {}, { .Ptr = &DispatchA, .Count = 1 });

	gdi_dispatch DispatchB = { .Shader = BlueShader, .ThreadGroupCount = TGC };
	gdi_signal SignalB = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &TexB.View, .Count = 1 }, {}, { .Ptr = &DispatchB, .Count = 1 });

	// Wait on both before rendering
	GDI_Wait_Signal(SignalA);
	GDI_Wait_Signal(SignalB);

	// Blit TexB (blue) to final target to verify it completed
	{
		gdi_render_pass_begin_info BeginInfo = { .RenderTargetViews = { FinalTarget.View } };
		gdi_render_pass* RP = GDI_Begin_Render_Pass(&BeginInfo);
		Render_Set_Shader(RP, BlitShader);
		Render_Set_Bind_Group(RP, 0, BindGroupB);
		Render_Draw(RP, 3, 0);
		GDI_End_Render_Pass(RP);
		GDI_Submit_Render_Pass(RP);
	}

	gdi_texture_info TexInfo = GDI_Get_Texture_Info(FinalTarget.Handle);
	simple_test_context TestContext = {
		.Texels = (u8*)Arena_Push(Scratch.Arena, TexInfo.Dim.x * TexInfo.Dim.y * GDI_Get_Format_Size(TexInfo.Format)),
	};

	gdi_texture_readback TextureReadback = {
		.Texture = FinalTarget.Handle,
		.UserData = &TestContext,
		.ReadbackFunc = Simple_Texture_Readback
	};

	gdi_render_params RenderParams = {
		.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 }
	};

	GDI_Render(&RenderParams);
	GDI_Flush();

	u32* Texels = (u32*)TestContext.Texels;
	for(s32 y = 0; y < TestContext.Dim.y; y++) {
		for(s32 x = 0; x < TestContext.Dim.x; x++) {
			ASSERT_EQ(*Texels, 0xFFFF0000);
			Texels++;
		}
	}

	GDI_Delete_Bind_Group(BindGroupB);
	Texture_Delete(&FinalTarget);
	Texture_Delete(&TexB);
	Texture_Delete(&TexA);
	GDI_Delete_Shader(BlitShader);
	GDI_Delete_Shader(BlueShader);
	GDI_Delete_Shader(RedShader);
	GDI_Delete_Bind_Group_Layout(BlitLayout);
}

UTEST(gdi, ChainedAsyncComputeTest) {
	scratch Scratch;

	gdi_shader WriteRedShader;
	{
		const char* ShaderCode = R"(
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			Output[ThreadID.xy] = float4(1, 0, 0, 1);
		}
		)";

		IDxcBlob* CSBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(CSBlob);
		gdi_bind_group_binding Binding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CreateInfo = {
			.CS = { .Ptr = (u8*)CSBlob->GetBufferPointer(), .Size = CSBlob->GetBufferSize() },
			.WritableBindings = { .Ptr = &Binding, .Count = 1 },
			.DebugName = String_Lit("Chain Stage A")
		};
		WriteRedShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(WriteRedShader));
		CSBlob->Release();
	}

	gdi_shader CopyShader;
	gdi_layout CopyLayout;
	{
		gdi_bind_group_binding Bindings[] = {
			{ .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1 }
		};
		gdi_bind_group_layout_create_info LayoutInfo = {
			.Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings) }
		};
		CopyLayout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
		ASSERT_FALSE(GDI_Is_Null(CopyLayout));

		const char* ShaderCode = R"(
		Texture2D<float4> InputTexture : register(t0, space1);
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			Output[ThreadID.xy] = InputTexture.Load(int3(ThreadID.xy, 0));
		}
		)";

		IDxcBlob* CSBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(CSBlob);
		gdi_bind_group_binding WBinding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CreateInfo = {
			.CS = { .Ptr = (u8*)CSBlob->GetBufferPointer(), .Size = CSBlob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &CopyLayout, .Count = 1 },
			.WritableBindings = { .Ptr = &WBinding, .Count = 1 },
			.DebugName = String_Lit("Chain Stage B")
		};
		CopyShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(CopyShader));
		CSBlob->Release();
	}

	texture TexA;
	ASSERT_TRUE(Texture_Create(&TexA, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));

	texture TexB;
	ASSERT_TRUE(Texture_Create(&TexB, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_READBACK
	}));

	gdi_bind_group InputBindGroup;
	{
		gdi_bind_group_write Write = { .TextureViews = { .Ptr = &TexA.View, .Count = 1 } };
		gdi_bind_group_create_info Info = { .Layout = CopyLayout, .Writes = { .Ptr = &Write, .Count = 1 } };
		InputBindGroup = GDI_Create_Bind_Group(&Info);
		ASSERT_FALSE(GDI_Is_Null(InputBindGroup));
	}

	v3i TGC = V3i(64/8, 64/8, 1);

	// Stage A: async compute writes red to TexA
	gdi_dispatch DispatchA = { .Shader = WriteRedShader, .ThreadGroupCount = TGC };
	gdi_signal SignalA = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &TexA.View, .Count = 1 }, {}, { .Ptr = &DispatchA, .Count = 1 });

	// Stage B must wait on Stage A, then copy TexA -> TexB
	GDI_Wait_Signal(SignalA);

	gdi_dispatch DispatchB = {
		.Shader = CopyShader,
		.BindGroups = { InputBindGroup },
		.ThreadGroupCount = TGC
	};
	gdi_signal SignalB = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &TexB.View, .Count = 1 }, {}, { .Ptr = &DispatchB, .Count = 1 });

	gdi_texture_info TexInfo = GDI_Get_Texture_Info(TexB.Handle);
	simple_test_context TestContext = {
		.Texels = (u8*)Arena_Push(Scratch.Arena, TexInfo.Dim.x * TexInfo.Dim.y * GDI_Get_Format_Size(TexInfo.Format)),
	};

	gdi_texture_readback TextureReadback = {
		.Texture = TexB.Handle,
		.UserData = &TestContext,
		.ReadbackFunc = Simple_Texture_Readback
	};

	gdi_render_params RenderParams = {
		.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 }
	};

	GDI_Render(&RenderParams);
	GDI_Flush();

	u32* Texels = (u32*)TestContext.Texels;
	for(s32 y = 0; y < TestContext.Dim.y; y++) {
		for(s32 x = 0; x < TestContext.Dim.x; x++) {
			ASSERT_EQ(*Texels, 0xFF0000FF);
			Texels++;
		}
	}

	GDI_Delete_Bind_Group(InputBindGroup);
	Texture_Delete(&TexB);
	Texture_Delete(&TexA);
	GDI_Delete_Shader(CopyShader);
	GDI_Delete_Shader(WriteRedShader);
	GDI_Delete_Bind_Group_Layout(CopyLayout);
}

UTEST(gdi, MultipleWaitsSameSignalTest) {
	scratch Scratch;

	gdi_shader ComputeShader;
	{
		const char* ShaderCode = R"(
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			Output[ThreadID.xy] = float4(1, 0, 1, 1);
		}
		)";

		IDxcBlob* CSBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(CSBlob);
		gdi_bind_group_binding Binding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CreateInfo = {
			.CS = { .Ptr = (u8*)CSBlob->GetBufferPointer(), .Size = CSBlob->GetBufferSize() },
			.WritableBindings = { .Ptr = &Binding, .Count = 1 },
			.DebugName = String_Lit("Multi Wait Shader")
		};
		ComputeShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeShader));
		CSBlob->Release();
	}

	texture Tex;
	ASSERT_TRUE(Texture_Create(&Tex, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_READBACK
	}));

	v3i TGC = V3i(64/8, 64/8, 1);
	gdi_dispatch Dispatch = { .Shader = ComputeShader, .ThreadGroupCount = TGC };

	gdi_signal Signal = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &Tex.View, .Count = 1 }, {}, { .Ptr = &Dispatch, .Count = 1 });

	// Backend must handle redundant waits on the same signal gracefully
	GDI_Wait_Signal(Signal);
	GDI_Wait_Signal(Signal);
	GDI_Wait_Signal(Signal);

	gdi_texture_info TexInfo = GDI_Get_Texture_Info(Tex.Handle);
	simple_test_context TestContext = {
		.Texels = (u8*)Arena_Push(Scratch.Arena, TexInfo.Dim.x * TexInfo.Dim.y * GDI_Get_Format_Size(TexInfo.Format)),
	};

	gdi_texture_readback TextureReadback = {
		.Texture = Tex.Handle,
		.UserData = &TestContext,
		.ReadbackFunc = Simple_Texture_Readback
	};

	gdi_render_params RenderParams = {
		.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 }
	};

	GDI_Render(&RenderParams);
	GDI_Flush();

	u32* Texels = (u32*)TestContext.Texels;
	for(s32 y = 0; y < TestContext.Dim.y; y++) {
		for(s32 x = 0; x < TestContext.Dim.x; x++) {
			ASSERT_EQ(*Texels, 0xFFFF00FF);
			Texels++;
		}
	}

	Texture_Delete(&Tex);
	GDI_Delete_Shader(ComputeShader);
}

UTEST(gdi, InterleavedAsyncAndGraphicsTest) {
	scratch Scratch;

	gdi_shader ComputeShader;
	{
		const char* ShaderCode = R"(
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			Output[ThreadID.xy] = float4(0, 1, 1, 1);
		}
		)";

		IDxcBlob* CSBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(CSBlob);
		gdi_bind_group_binding Binding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CreateInfo = {
			.CS = { .Ptr = (u8*)CSBlob->GetBufferPointer(), .Size = CSBlob->GetBufferSize() },
			.WritableBindings = { .Ptr = &Binding, .Count = 1 },
			.DebugName = String_Lit("Interleaved Compute Shader")
		};
		ComputeShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeShader));
		CSBlob->Release();
	}

	gdi_shader BlitShader;
	gdi_layout BlitLayout;
	{
		gdi_bind_group_binding Bindings[] = {
			{ .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1 }
		};
		gdi_bind_group_layout_create_info LayoutInfo = {
			.Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings) }
		};
		BlitLayout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
		ASSERT_FALSE(GDI_Is_Null(BlitLayout));

		const char* ShaderCode = R"(
		Texture2D InputTexture : register(t0, space0);
		float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
			float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
			return float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
		}
		float4 PS_Main(float4 Position : SV_POSITION) : SV_TARGET0 {
			return InputTexture.Load(int3(Position.xy, 0));
		}
		)";

		IDxcBlob* VtxBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
		IDxcBlob* PxlBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VtxBlob && PxlBlob);

		gdi_shader_create_info CreateInfo = {
			.VS = { .Ptr = (u8*)VtxBlob->GetBufferPointer(), .Size = VtxBlob->GetBufferSize() },
			.PS = { .Ptr = (u8*)PxlBlob->GetBufferPointer(), .Size = PxlBlob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &BlitLayout, .Count = 1 },
			.RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
		};
		BlitShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(BlitShader));
		VtxBlob->Release();
		PxlBlob->Release();
	}

	texture ComputeTexA, ComputeTexB, RenderTargetA, FinalTarget;
	ASSERT_TRUE(Texture_Create(&ComputeTexA, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));
	ASSERT_TRUE(Texture_Create(&ComputeTexB, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));
	ASSERT_TRUE(Texture_Create(&RenderTargetA, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET
	}));
	ASSERT_TRUE(Texture_Create(&FinalTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_READBACK
	}));

	gdi_bind_group BindGroupB;
	{
		gdi_bind_group_write Write = { .TextureViews = { .Ptr = &ComputeTexB.View, .Count = 1 } };
		gdi_bind_group_create_info Info = { .Layout = BlitLayout, .Writes = { .Ptr = &Write, .Count = 1 } };
		BindGroupB = GDI_Create_Bind_Group(&Info);
		ASSERT_FALSE(GDI_Is_Null(BindGroupB));
	}

	v3i TGC = V3i(64/8, 64/8, 1);

	// Pattern: Async Compute A -> Render A -> Wait A -> Async Compute B -> Wait B -> Final Render
	gdi_dispatch DispatchA = { .Shader = ComputeShader, .ThreadGroupCount = TGC };
	gdi_signal SignalA = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &ComputeTexA.View, .Count = 1 }, {}, { .Ptr = &DispatchA, .Count = 1 });

	// Unrelated render work while async compute A is in flight
	{
		gdi_render_pass_begin_info BeginInfo = {
			.RenderTargetViews = { RenderTargetA.View },
			.ClearColors = { { .ShouldClear = true, .F32 = { 1, 0, 0, 1 } } }
		};
		gdi_render_pass* RP = GDI_Begin_Render_Pass(&BeginInfo);
		GDI_End_Render_Pass(RP);
		GDI_Submit_Render_Pass(RP);
	}

	GDI_Wait_Signal(SignalA);

	// Second async compute after the first completed
	gdi_dispatch DispatchB = { .Shader = ComputeShader, .ThreadGroupCount = TGC };
	gdi_signal SignalB = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &ComputeTexB.View, .Count = 1 }, {}, { .Ptr = &DispatchB, .Count = 1 });

	GDI_Wait_Signal(SignalB);

	// Final render: blit ComputeTexB (cyan) to final target
	{
		gdi_render_pass_begin_info BeginInfo = { .RenderTargetViews = { FinalTarget.View } };
		gdi_render_pass* RP = GDI_Begin_Render_Pass(&BeginInfo);
		Render_Set_Shader(RP, BlitShader);
		Render_Set_Bind_Group(RP, 0, BindGroupB);
		Render_Draw(RP, 3, 0);
		GDI_End_Render_Pass(RP);
		GDI_Submit_Render_Pass(RP);
	}

	gdi_texture_info TexInfo = GDI_Get_Texture_Info(FinalTarget.Handle);
	simple_test_context TestContext = {
		.Texels = (u8*)Arena_Push(Scratch.Arena, TexInfo.Dim.x * TexInfo.Dim.y * GDI_Get_Format_Size(TexInfo.Format)),
	};

	gdi_texture_readback TextureReadback = {
		.Texture = FinalTarget.Handle,
		.UserData = &TestContext,
		.ReadbackFunc = Simple_Texture_Readback
	};

	gdi_render_params RenderParams = {
		.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 }
	};

	GDI_Render(&RenderParams);
	GDI_Flush();

	u32* Texels = (u32*)TestContext.Texels;
	for(s32 y = 0; y < TestContext.Dim.y; y++) {
		for(s32 x = 0; x < TestContext.Dim.x; x++) {
			ASSERT_EQ(*Texels, 0xFFFFFF00);
			Texels++;
		}
	}

	GDI_Delete_Bind_Group(BindGroupB);
	Texture_Delete(&FinalTarget);
	Texture_Delete(&RenderTargetA);
	Texture_Delete(&ComputeTexB);
	Texture_Delete(&ComputeTexA);
	GDI_Delete_Shader(BlitShader);
	GDI_Delete_Shader(ComputeShader);
	GDI_Delete_Bind_Group_Layout(BlitLayout);
}

UTEST(gdi, AsyncComputePipelinedFrameTest) {
	scratch Scratch;

	//
	// Pipelined frame pattern:
	//
	// Frame 0:  AsyncCompute(TexA, color0)
	// Frame 1:  Wait(compute0) -> Render(TexA -> FinalTarget) + AsyncCompute(TexB, color1)
	// Frame 2:  Wait(compute1) -> Render(TexB -> FinalTarget) + AsyncCompute(TexA, color2)
	// Frame 3:  Wait(compute2) -> Render(TexA -> FinalTarget) + AsyncCompute(TexB, color3)
	// ...
	// Frame N (final): Wait(computeN-1) -> Render(Tex -> FinalTarget), no new compute
	//
	// Each frame renders last frame's compute result while kicking off new compute.
	// Textures ping-pong between A and B. The render pass consumes the "old" texture
	// while async compute writes the "new" one, so they can truly run in parallel.
	//
	// Then we do the same thing again in reverse order (descending colors).
	//

	gdi_shader ComputeShader;
	{
		const char* ShaderCode = R"(
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		struct push_data { float R; float G; float B; };
		[[vk::push_constant]]
		ConstantBuffer<push_data> PushData : register(b0, space1);
		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			Output[ThreadID.xy] = float4(PushData.R, PushData.G, PushData.B, 1);
		}
		)";
		IDxcBlob* Blob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(Blob);
		gdi_bind_group_binding Binding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CI = {
			.CS = { .Ptr = (u8*)Blob->GetBufferPointer(), .Size = Blob->GetBufferSize() },
			.WritableBindings = { .Ptr = &Binding, .Count = 1 },
			.PushConstantCount = 3,
			.DebugName = String_Lit("Pipelined Compute Shader")
		};
		ComputeShader = GDI_Create_Shader(&CI);
		ASSERT_FALSE(GDI_Is_Null(ComputeShader));
		Blob->Release();
	}

	gdi_layout BlitLayout;
	gdi_shader BlitShader;
	{
		gdi_bind_group_binding Bindings[] = { { .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1 } };
		gdi_bind_group_layout_create_info LI = { .Bindings = { .Ptr = Bindings, .Count = 1 } };
		BlitLayout = GDI_Create_Bind_Group_Layout(&LI);
		ASSERT_FALSE(GDI_Is_Null(BlitLayout));

		const char* ShaderCode = R"(
		Texture2D InputTexture : register(t0, space0);
		float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
			float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
			return float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
		}
		float4 PS_Main(float4 Position : SV_POSITION) : SV_TARGET0 {
			return InputTexture.Load(int3(Position.xy, 0));
		}
		)";
		IDxcBlob* VBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
		IDxcBlob* PBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VBlob && PBlob);
		gdi_shader_create_info CI = {
			.VS = { .Ptr = (u8*)VBlob->GetBufferPointer(), .Size = VBlob->GetBufferSize() },
			.PS = { .Ptr = (u8*)PBlob->GetBufferPointer(), .Size = PBlob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &BlitLayout, .Count = 1 },
			.RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
			.DebugName = String_Lit("Pipelined Blit Shader")
		};
		BlitShader = GDI_Create_Shader(&CI);
		ASSERT_FALSE(GDI_Is_Null(BlitShader));
		VBlob->Release();
		PBlob->Release();
	}

	v2i TexDim = V2i(64, 64);
	v3i TGC = V3i(TexDim.x/8, TexDim.y/8, 1);

	texture TexA, TexB;
	ASSERT_TRUE(Texture_Create(&TexA, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = TexDim,
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));
	ASSERT_TRUE(Texture_Create(&TexB, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = TexDim,
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));

	texture FinalTarget;
	ASSERT_TRUE(Texture_Create(&FinalTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = TexDim,
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_READBACK
	}));

	gdi_bind_group BindGroupA, BindGroupB;
	{
		gdi_bind_group_write WA = { .TextureViews = { .Ptr = &TexA.View, .Count = 1 } };
		gdi_bind_group_create_info IA = { .Layout = BlitLayout, .Writes = { .Ptr = &WA, .Count = 1 } };
		BindGroupA = GDI_Create_Bind_Group(&IA);
		ASSERT_FALSE(GDI_Is_Null(BindGroupA));

		gdi_bind_group_write WB = { .TextureViews = { .Ptr = &TexB.View, .Count = 1 } };
		gdi_bind_group_create_info IB = { .Layout = BlitLayout, .Writes = { .Ptr = &WB, .Count = 1 } };
		BindGroupB = GDI_Create_Bind_Group(&IB);
		ASSERT_FALSE(GDI_Is_Null(BindGroupB));
	}

	struct color3 { f32 R, G, B; };

	auto RunPipelinedSequence = [&](color3* Colors, u32 Count) {
		texture* PingPong[2] = { &TexA, &TexB };
		gdi_bind_group PingPongBG[2] = { BindGroupA, BindGroupB };

		gdi_signal PrevComputeSignal = {};
		u32 PrevIdx = 0;
		b32 HasPrevCompute = false;

		for(u32 i = 0; i < Count; i++) {
			u32 CurIdx = i % 2;

			if(HasPrevCompute) {
				GDI_Wait_Signal(PrevComputeSignal);

				gdi_render_pass_begin_info BeginInfo = { .RenderTargetViews = { FinalTarget.View } };
				gdi_render_pass* RP = GDI_Begin_Render_Pass(&BeginInfo);
				Render_Set_Shader(RP, BlitShader);
				Render_Set_Bind_Group(RP, 0, PingPongBG[PrevIdx]);
				Render_Draw(RP, 3, 0);
				GDI_End_Render_Pass(RP);
				GDI_Submit_Render_Pass(RP);
			}

			color3 C = Colors[i];
			gdi_dispatch Dispatch = { .Shader = ComputeShader, .PushConstantCount = 3, .ThreadGroupCount = TGC };
			Memory_Copy(Dispatch.PushConstants, &C, sizeof(color3));
			PrevComputeSignal = GDI_Submit_Async_Compute_Pass(
				{ .Ptr = &PingPong[CurIdx]->View, .Count = 1 }, {},
				{ .Ptr = &Dispatch, .Count = 1 });

			HasPrevCompute = true;
			PrevIdx = CurIdx;

			gdi_render_params Params = {};
			GDI_Render(&Params);
		}

		// Drain frame: wait on last compute, render its result, readback and validate
		GDI_Wait_Signal(PrevComputeSignal);
		{
			gdi_render_pass_begin_info BeginInfo = { .RenderTargetViews = { FinalTarget.View } };
			gdi_render_pass* RP = GDI_Begin_Render_Pass(&BeginInfo);
			Render_Set_Shader(RP, BlitShader);
			Render_Set_Bind_Group(RP, 0, PingPongBG[PrevIdx]);
			Render_Draw(RP, 3, 0);
			GDI_End_Render_Pass(RP);
			GDI_Submit_Render_Pass(RP);
		}

		gdi_texture_info TexInfo = GDI_Get_Texture_Info(FinalTarget.Handle);
		simple_test_context TestContext = {
			.Texels = (u8*)Arena_Push(Scratch.Arena, TexInfo.Dim.x * TexInfo.Dim.y * GDI_Get_Format_Size(TexInfo.Format)),
		};

		gdi_texture_readback Readback = {
			.Texture = FinalTarget.Handle,
			.UserData = &TestContext,
			.ReadbackFunc = Simple_Texture_Readback
		};

		gdi_render_params Params = { .TextureReadbacks = { .Ptr = &Readback, .Count = 1 } };
		GDI_Render(&Params);
		GDI_Flush();

		color3 Last = Colors[Count - 1];
		u8 ExpR = (u8)(Last.R * 255.0f + 0.5f);
		u8 ExpG = (u8)(Last.G * 255.0f + 0.5f);
		u8 ExpB = (u8)(Last.B * 255.0f + 0.5f);

		u32* Texels = (u32*)TestContext.Texels;
		for(s32 y = 0; y < TestContext.Dim.y; y++) {
			for(s32 x = 0; x < TestContext.Dim.x; x++) {
				u8 TexelR = (*Texels >> 0)  & 0xFF;
				u8 TexelG = (*Texels >> 8)  & 0xFF;
				u8 TexelB = (*Texels >> 16) & 0xFF;
				ASSERT_NEAR((f32)TexelR, (f32)ExpR, 2.0f);
				ASSERT_NEAR((f32)TexelG, (f32)ExpG, 2.0f);
				ASSERT_NEAR((f32)TexelB, (f32)ExpB, 2.0f);
				Texels++;
			}
		}
	};

	// Forward pass: 10 frames with ascending colors
	{
		color3 Colors[10];
		for(u32 i = 0; i < 10; i++) {
			f32 T = (f32)i / 9.0f;
			Colors[i] = { T, 0.0f, 1.0f - T };
		}
		RunPipelinedSequence(Colors, 10);
	}

	// Reverse pass: 10 frames with descending colors
	{
		color3 Colors[10];
		for(u32 i = 0; i < 10; i++) {
			f32 T = 1.0f - (f32)i / 9.0f;
			Colors[i] = { T, 0.0f, 1.0f - T };
		}
		RunPipelinedSequence(Colors, 10);
	}

	GDI_Delete_Bind_Group(BindGroupB);
	GDI_Delete_Bind_Group(BindGroupA);
	Texture_Delete(&FinalTarget);
	Texture_Delete(&TexB);
	Texture_Delete(&TexA);
	GDI_Delete_Shader(BlitShader);
	GDI_Delete_Shader(ComputeShader);
	GDI_Delete_Bind_Group_Layout(BlitLayout);
}

UTEST(gdi, AsyncComputeBufferOwnershipTest) {
	gdi_tests* Tests = GDI_Get_Tests();
	ASSERT_TRUE(Tests);

	scratch Scratch;

	struct buf_data {
		v4 Values[16];
	};

	gdi_shader ComputeShader;
	gdi_layout ComputeBufferLayout;
	{
		gdi_bind_group_binding Bindings[] = {
			{ .Type = GDI_BIND_GROUP_TYPE_STORAGE_BUFFER, .Count = 1 }
		};
		gdi_bind_group_layout_create_info LayoutInfo = {
			.Bindings = { .Ptr = Bindings, .Count = Array_Count(Bindings) }
		};
		ComputeBufferLayout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeBufferLayout));

		const char* ShaderCode = R"(
		struct buf_data {
			float4 Values[16];
		};

		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		StructuredBuffer<buf_data> InputBuffer : register(t0, space1);

		[numthreads(4, 4, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			uint Idx = ThreadID.y * 4 + ThreadID.x;
			if(Idx < 16) {
				float4 Color = InputBuffer[0].Values[Idx];
				uint2 Base = ThreadID.xy * 4;
				for(uint y = 0; y < 4; y++) {
					for(uint x = 0; x < 4; x++) {
						Output[Base + uint2(x, y)] = Color;
					}
				}
			}
		}
		)";

		IDxcBlob* CSBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(CSBlob);
		gdi_bind_group_binding WBinding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CreateInfo = {
			.CS = { .Ptr = (u8*)CSBlob->GetBufferPointer(), .Size = CSBlob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &ComputeBufferLayout, .Count = 1 },
			.WritableBindings = { .Ptr = &WBinding, .Count = 1 },
			.DebugName = String_Lit("Buffer Ownership Compute Shader")
		};
		ComputeShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeShader));
		CSBlob->Release();
	}

	buf_data BufferData;
	for(u32 i = 0; i < 16; i++) {
		BufferData.Values[i] = V4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	gdi_buffer Buffer;
	gdi_bind_group BufferBindGroup;
	{
		gdi_buffer_create_info BufferInfo = {
			.Size = sizeof(buf_data),
			.Usage = GDI_BUFFER_USAGE_STORAGE,
			.InitialData = Make_Buffer(&BufferData, sizeof(BufferData))
		};
		Buffer = GDI_Create_Buffer(&BufferInfo);
		ASSERT_FALSE(GDI_Is_Null(Buffer));

		gdi_bind_group_buffer BGBuffer = { .Buffer = Buffer };
		gdi_bind_group_write Write = { .Buffers = { .Ptr = &BGBuffer, .Count = 1 } };
		gdi_bind_group_create_info Info = {
			.Layout = ComputeBufferLayout,
			.Writes = { .Ptr = &Write, .Count = 1 }
		};
		BufferBindGroup = GDI_Create_Bind_Group(&Info);
		ASSERT_FALSE(GDI_Is_Null(BufferBindGroup));
	}

	texture OutputTexture;
	ASSERT_TRUE(Texture_Create(&OutputTexture, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = V2i(16, 16),
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_READBACK
	}));

	gdi_dispatch Dispatch = {
		.Shader = ComputeShader,
		.BindGroups = { BufferBindGroup },
		.ThreadGroupCount = V3i(1, 1, 1)
	};

	gdi_signal Signal = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &OutputTexture.View, .Count = 1 }, { .Ptr = &Buffer, .Count = 1 },
		{ .Ptr = &Dispatch, .Count = 1 });

	gdi_texture_info TexInfo = GDI_Get_Texture_Info(OutputTexture.Handle);
	simple_test_context TestContext = {
		.Texels = (u8*)Arena_Push(Scratch.Arena, TexInfo.Dim.x * TexInfo.Dim.y * GDI_Get_Format_Size(TexInfo.Format)),
	};

	gdi_texture_readback TextureReadback = {
		.Texture = OutputTexture.Handle,
		.UserData = &TestContext,
		.ReadbackFunc = Simple_Texture_Readback
	};

	gdi_render_params RenderParams = {
		.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 }
	};

	GDI_Render(&RenderParams);
	GDI_Flush();

	u32* Texels = (u32*)TestContext.Texels;
	for(s32 y = 0; y < TestContext.Dim.y; y++) {
		for(s32 x = 0; x < TestContext.Dim.x; x++) {
			ASSERT_EQ(*Texels, 0xFF0000FF);
			Texels++;
		}
	}

	GDI_Delete_Bind_Group(BufferBindGroup);
	GDI_Delete_Buffer(Buffer);
	Texture_Delete(&OutputTexture);
	GDI_Delete_Shader(ComputeShader);
	GDI_Delete_Bind_Group_Layout(ComputeBufferLayout);
}

UTEST(gdi, AsyncComputeMultiFrameStressTest) {
	//
	// Per-frame dependency graph (repeated over multiple frames):
	//
	//   AsyncCompute_Red  (writes TexRed)  ----+
	//                                          |
	//   AsyncCompute_Blue (writes TexBlue) -+  |
	//                                       |  |
	//   Render_GreenBG (writes GreenRT) -+  |  |
	//                                    |  |  |
	//                          wait(Green)  |  |
	//                                    |  |  |
	//   AsyncCompute_Copy (reads GreenRT, writes TexGreen)
	//                                       |  |
	//                           wait(Blue) -+  |
	//                            wait(Red) ----+
	//                          wait(Copy)
	//                                    |
	//   Render_Composite (reads TexRed, TexBlue, TexGreen -> FinalTarget)
	//                                    |
	//                              readback FinalTarget
	//
	// This exercises:
	// - Multiple async computes running in parallel (Red and Blue)
	// - Graphics-to-async-compute dependency (Green render -> Copy compute)
	// - Multiple waits converging before a single render pass
	// - All of the above repeated across frames with changing data
	//

	scratch Scratch;

	v2i TexDim = V2i(64, 64);
	v3i TGC = V3i(TexDim.x/8, TexDim.y/8, 1);

	// --- Shaders ---

	gdi_shader RedShader;
	{
		const char* ShaderCode = R"(
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		struct push_data { float Intensity; };
		[[vk::push_constant]]
		ConstantBuffer<push_data> PushData : register(b0, space1);
		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			Output[ThreadID.xy] = float4(PushData.Intensity, 0, 0, 1);
		}
		)";
		IDxcBlob* Blob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(Blob);
		gdi_bind_group_binding Binding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CI = {
			.CS = { .Ptr = (u8*)Blob->GetBufferPointer(), .Size = Blob->GetBufferSize() },
			.WritableBindings = { .Ptr = &Binding, .Count = 1 },
			.PushConstantCount = 1,
			.DebugName = String_Lit("Stress Red CS")
		};
		RedShader = GDI_Create_Shader(&CI);
		ASSERT_FALSE(GDI_Is_Null(RedShader));
		Blob->Release();
	}

	gdi_shader BlueShader;
	{
		const char* ShaderCode = R"(
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		struct push_data { float Intensity; };
		[[vk::push_constant]]
		ConstantBuffer<push_data> PushData : register(b0, space1);
		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			Output[ThreadID.xy] = float4(0, 0, PushData.Intensity, 1);
		}
		)";
		IDxcBlob* Blob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(Blob);
		gdi_bind_group_binding Binding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CI = {
			.CS = { .Ptr = (u8*)Blob->GetBufferPointer(), .Size = Blob->GetBufferSize() },
			.WritableBindings = { .Ptr = &Binding, .Count = 1 },
			.PushConstantCount = 1,
			.DebugName = String_Lit("Stress Blue CS")
		};
		BlueShader = GDI_Create_Shader(&CI);
		ASSERT_FALSE(GDI_Is_Null(BlueShader));
		Blob->Release();
	}

	gdi_layout CopyLayout;
	gdi_shader CopyShader;
	{
		gdi_bind_group_binding Bindings[] = {
			{ .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1 }
		};
		gdi_bind_group_layout_create_info LI = { .Bindings = { .Ptr = Bindings, .Count = 1 } };
		CopyLayout = GDI_Create_Bind_Group_Layout(&LI);
		ASSERT_FALSE(GDI_Is_Null(CopyLayout));

		const char* ShaderCode = R"(
		Texture2D<float4> InputTexture : register(t0, space1);
		[[vk::image_format("rgba8")]]
		RWTexture2D<float4> Output : register(u0, space0);
		[numthreads(8, 8, 1)]
		void CS_Main(uint3 ThreadID : SV_DispatchThreadID) {
			Output[ThreadID.xy] = InputTexture.Load(int3(ThreadID.xy, 0));
		}
		)";
		IDxcBlob* Blob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(Blob);
		gdi_bind_group_binding WBinding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_TEXTURE, .Count = 1 };
		gdi_shader_create_info CI = {
			.CS = { .Ptr = (u8*)Blob->GetBufferPointer(), .Size = Blob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &CopyLayout, .Count = 1 },
			.WritableBindings = { .Ptr = &WBinding, .Count = 1 },
			.DebugName = String_Lit("Stress Copy CS")
		};
		CopyShader = GDI_Create_Shader(&CI);
		ASSERT_FALSE(GDI_Is_Null(CopyShader));
		Blob->Release();
	}

	gdi_shader GreenRenderShader;
	{
		const char* ShaderCode = R"(
		struct push_data { float Intensity; };
		[[vk::push_constant]] ConstantBuffer<push_data> PushData : register(b0, space1);
		float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
			float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
			return float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
		}
		float4 PS_Main(float4 Position : SV_POSITION) : SV_TARGET0 {
			return float4(0, PushData.Intensity, 0, 1);
		}
		)";
		IDxcBlob* VBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
		IDxcBlob* PBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VBlob && PBlob);
		gdi_shader_create_info CI = {
			.VS = { .Ptr = (u8*)VBlob->GetBufferPointer(), .Size = VBlob->GetBufferSize() },
			.PS = { .Ptr = (u8*)PBlob->GetBufferPointer(), .Size = PBlob->GetBufferSize() },
            .PushConstantCount = 1,
            .RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
			.DebugName = String_Lit("Stress Green Render")
		};
		GreenRenderShader = GDI_Create_Shader(&CI);
		ASSERT_FALSE(GDI_Is_Null(GreenRenderShader));
		VBlob->Release();
		PBlob->Release();
	}

	gdi_layout CompositeLayout;
	gdi_shader CompositeShader;
	{
		gdi_bind_group_binding Bindings[] = {
			{ .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1 },
			{ .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1 },
			{ .Type = GDI_BIND_GROUP_TYPE_TEXTURE, .Count = 1 }
		};
		gdi_bind_group_layout_create_info LI = { .Bindings = { .Ptr = Bindings, .Count = 3 } };
		CompositeLayout = GDI_Create_Bind_Group_Layout(&LI);
		ASSERT_FALSE(GDI_Is_Null(CompositeLayout));

		const char* ShaderCode = R"(
		Texture2D RedTex   : register(t0, space0);
		Texture2D BlueTex  : register(t1, space0);
		Texture2D GreenTex : register(t2, space0);
		float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
			float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
			return float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
		}
		float4 PS_Main(float4 Position : SV_POSITION) : SV_TARGET0 {
			int3 Coord = int3(Position.xy, 0);
			float R = RedTex.Load(Coord).r;
			float G = GreenTex.Load(Coord).g;
			float B = BlueTex.Load(Coord).b;
			return float4(R, G, B, 1);
		}
		)";
		IDxcBlob* VBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
		IDxcBlob* PBlob = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VBlob && PBlob);
		gdi_shader_create_info CI = {
			.VS = { .Ptr = (u8*)VBlob->GetBufferPointer(), .Size = VBlob->GetBufferSize() },
			.PS = { .Ptr = (u8*)PBlob->GetBufferPointer(), .Size = PBlob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &CompositeLayout, .Count = 1 },
			.RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
			.DebugName = String_Lit("Stress Composite Render")
		};
		CompositeShader = GDI_Create_Shader(&CI);
		ASSERT_FALSE(GDI_Is_Null(CompositeShader));
		VBlob->Release();
		PBlob->Release();
	}

	// --- Resources ---

	texture TexRed, TexBlue, TexGreen, GreenRT, FinalTarget;
	ASSERT_TRUE(Texture_Create(&TexRed, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = TexDim,
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));
	ASSERT_TRUE(Texture_Create(&TexBlue, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = TexDim,
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));
	ASSERT_TRUE(Texture_Create(&TexGreen, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = TexDim,
		.Usage = GDI_TEXTURE_USAGE_STORAGE|GDI_TEXTURE_USAGE_SAMPLED
	}));
	ASSERT_TRUE(Texture_Create(&GreenRT, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = TexDim,
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_SAMPLED
	}));
	ASSERT_TRUE(Texture_Create(&FinalTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM, .Dim = TexDim,
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET|GDI_TEXTURE_USAGE_READBACK
	}));

	gdi_bind_group CopyBindGroup;
	{
		gdi_bind_group_write Write = { .TextureViews = { .Ptr = &GreenRT.View, .Count = 1 } };
		gdi_bind_group_create_info Info = { .Layout = CopyLayout, .Writes = { .Ptr = &Write, .Count = 1 } };
		CopyBindGroup = GDI_Create_Bind_Group(&Info);
		ASSERT_FALSE(GDI_Is_Null(CopyBindGroup));
	}

	gdi_bind_group CompositeBindGroup;
	{
		gdi_bind_group_write Writes[] = {
			{ .DstBinding = 0, .TextureViews = { .Ptr = &TexRed.View, .Count = 1 } },
			{ .DstBinding = 1, .TextureViews = { .Ptr = &TexBlue.View, .Count = 1 } },
			{ .DstBinding = 2, .TextureViews = { .Ptr = &TexGreen.View, .Count = 1 } }
		};
		gdi_bind_group_create_info Info = { .Layout = CompositeLayout, .Writes = { .Ptr = Writes, .Count = 3 } };
		CompositeBindGroup = GDI_Create_Bind_Group(&Info);
		ASSERT_FALSE(GDI_Is_Null(CompositeBindGroup));
	}

	// --- Overlap timing (only verified when async compute is supported) ---

	enum {
		QUERY_ASYNC_BEGIN  = 0,
		QUERY_ASYNC_END    = 1,
		QUERY_RENDER_BEGIN = 2,
		QUERY_RENDER_END   = 3,
		QUERY_SLOT_COUNT   = 4
	};

	gdi_query_pool QueryPool = {};
	if(GDI_Is_Async_Compute_Supported()) {
		gdi_query_pool_create_info QPI = {
			.Count = QUERY_SLOT_COUNT,
			.DebugName = String_Lit("Stress Overlap Query Pool")
		};
		QueryPool = GDI_Create_Query_Pool(&QPI);
		ASSERT_FALSE(GDI_Is_Null(QueryPool));
	}

	u32 OverlapCount = 0;

	// --- Frame loop ---

	static const u32 FrameCount = 10;
	for(u32 Frame = 0; Frame < FrameCount; Frame++) {
		f32 T = (f32)Frame / (f32)(FrameCount - 1);
		f32 RedIntensity   = T;
		f32 BlueIntensity  = 1.0f - T;
		f32 GreenIntensity = 0.5f + 0.5f * T;

		//
		// 1) Two async computes fire in parallel: Red and Blue
		//    These have no dependencies and should overlap with each other
		//    AND with the green render pass below.
		//
		if(GDI_Is_Async_Compute_Supported()) {
			GDI_Write_Timestamp(QueryPool, QUERY_ASYNC_BEGIN);
		}

		gdi_dispatch RedDispatch = { .Shader = RedShader, .PushConstantCount = 1, .ThreadGroupCount = TGC };
		Memory_Copy(RedDispatch.PushConstants, &RedIntensity, sizeof(f32));
		gdi_signal RedSignal = GDI_Submit_Async_Compute_Pass(
			{ .Ptr = &TexRed.View, .Count = 1 }, {}, { .Ptr = &RedDispatch, .Count = 1 });

		gdi_dispatch BlueDispatch = { .Shader = BlueShader, .PushConstantCount = 1, .ThreadGroupCount = TGC };
		Memory_Copy(BlueDispatch.PushConstants, &BlueIntensity, sizeof(f32));
		gdi_signal BlueSignal = GDI_Submit_Async_Compute_Pass(
			{ .Ptr = &TexBlue.View, .Count = 1 }, {}, { .Ptr = &BlueDispatch, .Count = 1 });

		if(GDI_Is_Async_Compute_Supported()) {
			GDI_Write_Timestamp(QueryPool, QUERY_ASYNC_END);
		}

		//
		// 2) Graphics: render green to GreenRT (runs in parallel with Red/Blue async)
		//
		if(GDI_Is_Async_Compute_Supported()) {
			GDI_Write_Timestamp(QueryPool, QUERY_RENDER_BEGIN);
		}

		gdi_signal GreenSignal;
		{
			gdi_render_pass_begin_info BeginInfo = {
				.RenderTargetViews = { GreenRT.View },
				.ClearColors = { { .ShouldClear = true, .F32 = { 0, 0, 0, 1 } } }
			};
			gdi_render_pass* RP = GDI_Begin_Render_Pass(&BeginInfo);
			Render_Set_Shader(RP, GreenRenderShader);
			Render_Set_Push_Constants(RP, &GreenIntensity, sizeof(f32));
			Render_Draw(RP, 3, 0);
			GDI_End_Render_Pass(RP);
			GreenSignal = GDI_Submit_Render_Pass(RP);
		}

		if(GDI_Is_Async_Compute_Supported()) {
			GDI_Write_Timestamp(QueryPool, QUERY_RENDER_END);
		}

		//
		// 3) Async compute copies GreenRT -> TexGreen.
		//    Must wait on the green render pass first (graphics -> compute dependency).
		//
		GDI_Wait_Signal(GreenSignal);

		gdi_dispatch CopyDispatch = {
			.Shader = CopyShader,
			.BindGroups = { CopyBindGroup },
			.ThreadGroupCount = TGC
		};
		gdi_signal CopySignal = GDI_Submit_Async_Compute_Pass(
			{ .Ptr = &TexGreen.View, .Count = 1 }, {}, { .Ptr = &CopyDispatch, .Count = 1 });

		//
		// 4) Composite render: reads TexRed, TexBlue, TexGreen -> FinalTarget.
		//    Must wait on ALL three producers: Red, Blue, and Copy.
		//
		GDI_Wait_Signal(RedSignal);
		GDI_Wait_Signal(BlueSignal);
		GDI_Wait_Signal(CopySignal);

		{
			gdi_render_pass_begin_info BeginInfo = { .RenderTargetViews = { FinalTarget.View } };
			gdi_render_pass* RP = GDI_Begin_Render_Pass(&BeginInfo);
			Render_Set_Shader(RP, CompositeShader);
			Render_Set_Bind_Group(RP, 0, CompositeBindGroup);
			Render_Draw(RP, 3, 0);
			GDI_End_Render_Pass(RP);
			GDI_Submit_Render_Pass(RP);
		}

		//
		// 5) Readback and validate
		//
		gdi_texture_info TexInfo = GDI_Get_Texture_Info(FinalTarget.Handle);
		simple_test_context TestContext = {
			.Texels = (u8*)Arena_Push(Scratch.Arena, TexInfo.Dim.x * TexInfo.Dim.y * GDI_Get_Format_Size(TexInfo.Format)),
		};

		gdi_texture_readback TextureReadback = {
			.Texture = FinalTarget.Handle,
			.UserData = &TestContext,
			.ReadbackFunc = Simple_Texture_Readback
		};

		gdi_render_params RenderParams = {
			.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 }
		};

		GDI_Render(&RenderParams);
		GDI_Flush();

		u8 ExpR = (u8)(RedIntensity   * 255.0f + 0.5f);
		u8 ExpG = (u8)(GreenIntensity * 255.0f + 0.5f);
		u8 ExpB = (u8)(BlueIntensity  * 255.0f + 0.5f);

		u32* Texels = (u32*)TestContext.Texels;
		for(s32 y = 0; y < TestContext.Dim.y; y++) {
			for(s32 x = 0; x < TestContext.Dim.x; x++) {
				u8 TexR = (*Texels >> 0)  & 0xFF;
				u8 TexG = (*Texels >> 8)  & 0xFF;
				u8 TexB = (*Texels >> 16) & 0xFF;
				ASSERT_NEAR((f32)TexR, (f32)ExpR, 2.0f);
				ASSERT_NEAR((f32)TexG, (f32)ExpG, 2.0f);
				ASSERT_NEAR((f32)TexB, (f32)ExpB, 2.0f);
				Texels++;
			}
		}

		//
		// 6) Check overlap when async compute is supported
		//
		if(GDI_Is_Async_Compute_Supported()) {
			gdi_timestamp_result Results[QUERY_SLOT_COUNT];
			if(GDI_Get_Query_Results(QueryPool, 0, QUERY_SLOT_COUNT, Results)) {
				b32 AllAvailable = true;
				for(u32 i = 0; i < QUERY_SLOT_COUNT; i++) {
					AllAvailable = AllAvailable && Results[i].Available;
				}

				if(AllAvailable) {
					b32 HasOverlap =
						Results[QUERY_ASYNC_END].Ticks > Results[QUERY_RENDER_BEGIN].Ticks &&
						Results[QUERY_ASYNC_BEGIN].Ticks < Results[QUERY_RENDER_END].Ticks;

					if(HasOverlap) OverlapCount++;
				}
			}
		}
	}

	// When async compute is supported, at least some frames should show overlap
	if(GDI_Is_Async_Compute_Supported()) {
		ASSERT_GT(OverlapCount, (u32)0);
	}

	// Cleanup
	if(!GDI_Is_Null(QueryPool)) GDI_Delete_Query_Pool(QueryPool);
	GDI_Delete_Bind_Group(CompositeBindGroup);
	GDI_Delete_Bind_Group(CopyBindGroup);
	Texture_Delete(&FinalTarget);
	Texture_Delete(&GreenRT);
	Texture_Delete(&TexGreen);
	Texture_Delete(&TexBlue);
	Texture_Delete(&TexRed);
	GDI_Delete_Shader(CompositeShader);
	GDI_Delete_Shader(GreenRenderShader);
	GDI_Delete_Shader(CopyShader);
	GDI_Delete_Shader(BlueShader);
	GDI_Delete_Shader(RedShader);
	GDI_Delete_Bind_Group_Layout(CompositeLayout);
	GDI_Delete_Bind_Group_Layout(CopyLayout);
}
