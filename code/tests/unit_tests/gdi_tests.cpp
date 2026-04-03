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

struct dynamic_indirect_frame_readback_ctx {
	v4 ExpectedColor;
};

function GDI_TEXTURE_READBACK_DEFINE(Dynamic_Indirect_Frame_Texture_Readback) {
	(void)Texture;
	(void)Format;
	dynamic_indirect_frame_readback_ctx* Ctx = (dynamic_indirect_frame_readback_ctx*)UserData;
	const u32* Pixels = (const u32*)Texels;
	f32 Eps = 1.0f / 255.0f;
	s32 cx = Dim.x / 2;
	s32 cy = Dim.y / 2;
	u32 CenterSample = Pixels[cx + cy * Dim.x];
	v4 GotColor = V4_Color_From_U32(CenterSample);
	Assert(Equal_Approx_F32(GotColor.x, Ctx->ExpectedColor.x, Eps));
	Assert(Equal_Approx_F32(GotColor.y, Ctx->ExpectedColor.y, Eps));
	Assert(Equal_Approx_F32(GotColor.z, Ctx->ExpectedColor.z, Eps));
	Assert(Equal_Approx_F32(GotColor.w, Ctx->ExpectedColor.w, Eps));
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

struct vtx_mesh_p2 {
    v2 P;
};

/* One float4 per instance — matches a single COLOR attribute (no multi-attribute offset drift vs DXC). */
struct vtx_inst_color {
    v4 Color;
};

UTEST(gdi, InstancedDrawTest) {
    scratch Scratch;

    gdi_shader Shader;
    {
        gdi_tests* Tests = GDI_Get_Tests();

        const char* ShaderCode = R"(
        struct mesh_vtx {
            float2 Position : POSITION0;
        };

        struct inst_vtx {
            float4 Color : TEXCOORD0;
        };

        struct vs_output {
            float4 Position : SV_POSITION;
            float4 Color : COLOR0;
        };

        struct draw_data {
            float4x4 Projection;
        };

        ConstantBuffer<draw_data> DrawData : register(b0, space0);

        vs_output VS_Main(mesh_vtx M, inst_vtx I, uint InstId : SV_InstanceID) {
            vs_output R;
            float ox = (InstId % 2u) * 32.0f;
            float oy = (InstId / 2u) * 32.0f;
            float2 world = M.Position + float2(ox, oy);
            R.Position = mul(float4(world, 0.0f, 1.0f), DrawData.Projection);
            R.Color = I.Color;
            return R;
        }

        float4 PS_Main(vs_output Pxl) : SV_TARGET0 {
            return Pxl.Color;
        };

        )";

        IDxcBlob* VtxShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("vs_6_0"), String_Lit("VS_Main"), {String_Lit("-fvk-invert-y")});
        IDxcBlob* PxlShader = Compile_Shader(String_Null_Term(ShaderCode), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
        ASSERT_TRUE(VtxShader && PxlShader);

        gdi_vtx_attribute MeshAttrs[] = {
            { String_Lit("POSITION"), GDI_FORMAT_R32G32_FLOAT },
        };
        gdi_vtx_binding MeshBinding = {
            .Stride = sizeof(vtx_mesh_p2),
            .InputRate = GDI_VERTEX_INPUT_RATE_VERTEX,
            .Attributes = { .Ptr = MeshAttrs, .Count = Array_Count(MeshAttrs) },
        };

        gdi_vtx_attribute InstAttrs[] = {
            { String_Lit("TEXCOORD"), GDI_FORMAT_R32G32B32A32_FLOAT },
        };
        gdi_vtx_binding InstBinding = {
            .Stride = sizeof(vtx_inst_color),
            .InputRate = GDI_VERTEX_INPUT_RATE_INSTANCE,
            .Attributes = { .Ptr = InstAttrs, .Count = Array_Count(InstAttrs) },
        };

        gdi_vtx_binding VtxBindings[] = { MeshBinding, InstBinding };

        gdi_shader_create_info CreateInfo = {
            .VS = { .Ptr = (u8*)VtxShader->GetBufferPointer(), .Size = VtxShader->GetBufferSize() },
            .PS = { .Ptr = (u8*)PxlShader->GetBufferPointer(), .Size = PxlShader->GetBufferSize() },
            .BindGroupLayouts = { .Ptr = &Tests->BufferLayout, .Count = 1 },
            .VtxBindings = { .Ptr = VtxBindings, .Count = Array_Count(VtxBindings) },
            .RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
            .DebugName = String_Lit("Instanced draw shader"),
        };

        Shader = GDI_Create_Shader(&CreateInfo);
        ASSERT_FALSE(GDI_Is_Null(Shader));

        VtxShader->Release();
        PxlShader->Release();
    }

    texture RenderTarget;
    ASSERT_TRUE(Texture_Create(&RenderTarget, {
        .Format = GDI_FORMAT_R8G8B8A8_UNORM,
        .Dim = V2i(64, 64),
        .Usage = GDI_TEXTURE_USAGE_RENDER_TARGET | GDI_TEXTURE_USAGE_READBACK,
    }));
    gdi_texture_info TextureInfo = GDI_Get_Texture_Info(RenderTarget.Handle);

    m4 Orthographic = M4_Orthographic(0, (f32)TextureInfo.Dim.x, (f32)TextureInfo.Dim.y, 0, -10, 10);
    Orthographic = M4_Transpose(&Orthographic);

    gpu_buffer MatrixBuffer;
    ASSERT_TRUE(GPU_Buffer_Create(&MatrixBuffer, {
        .Size = sizeof(m4),
        .Usage = GDI_BUFFER_USAGE_CONSTANT,
        .InitialData = Make_Buffer(&Orthographic, sizeof(m4)),
    }));

    vtx_mesh_p2 MeshVerts[] = {
        { V2(0, 0) },
        { V2(0, 32) },
        { V2(32, 32) },
        { V2(32, 0) },
    };
    u32 Indices[] = { 0, 1, 2, 2, 3, 0 };

    vtx_inst_color Instances[] = {
        { V4(1, 0, 0, 1) },
        { V4(0, 1, 0, 1) },
        { V4(0, 0, 1, 1) },
        { V4(1, 1, 0, 1) },
    };

    gdi_buffer_create_info MeshBufferInfo = {
        .Size = sizeof(MeshVerts),
        .Usage = GDI_BUFFER_USAGE_VTX,
        .InitialData = Make_Buffer(MeshVerts, sizeof(MeshVerts)),
        .DebugName = String_Lit("Instanced test mesh vtx"),
    };
    gdi_buffer MeshBuffer = GDI_Create_Buffer(&MeshBufferInfo);
    ASSERT_FALSE(GDI_Is_Null(MeshBuffer));

    gdi_buffer_create_info InstanceBufferInfo = {
        .Size = sizeof(Instances),
        .Usage = GDI_BUFFER_USAGE_VTX,
        .InitialData = Make_Buffer(Instances, sizeof(Instances)),
        .DebugName = String_Lit("Instanced test instance vtx"),
    };
    gdi_buffer InstanceBuffer = GDI_Create_Buffer(&InstanceBufferInfo);
    ASSERT_FALSE(GDI_Is_Null(InstanceBuffer));

    gdi_buffer_create_info IdxBufferInfo = {
        .Size = sizeof(Indices),
        .Usage = GDI_BUFFER_USAGE_IDX,
        .InitialData = Make_Buffer(Indices, sizeof(Indices)),
        .DebugName = String_Lit("Instanced test idx"),
    };
    gdi_buffer IdxBuffer = GDI_Create_Buffer(&IdxBufferInfo);
    ASSERT_FALSE(GDI_Is_Null(IdxBuffer));

    simple_test_context TestContext = {
        .Texels = (u8*)Arena_Push(Scratch.Arena, TextureInfo.Dim.x * TextureInfo.Dim.y * GDI_Get_Format_Size(TextureInfo.Format)),
    };

    gdi_render_pass_begin_info BeginInfo = {
        .RenderTargetViews = { RenderTarget.View },
        .ClearColors = { { .ShouldClear = true, .F32 = { 0.2f, 0.2f, 0.2f, 1.0f } } },
    };
    gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);

    Render_Set_Shader(RenderPass, Shader);
    Render_Set_Bind_Group(RenderPass, 0, MatrixBuffer.BindGroup);
    Render_Set_Scissor(RenderPass, 0, 0, TextureInfo.Dim.x, TextureInfo.Dim.y);
    Render_Set_Vtx_Buffer(RenderPass, 0, MeshBuffer);
    Render_Set_Vtx_Buffer(RenderPass, 1, InstanceBuffer);
    Render_Set_Idx_Buffer(RenderPass, IdxBuffer, GDI_IDX_FORMAT_32_BIT);
    Render_Draw_Idx_Instanced(RenderPass, 6, 0, 0, 4, 0);

    GDI_End_Render_Pass(RenderPass);
    GDI_Submit_Render_Pass(RenderPass);

    gdi_texture_readback TextureReadback = {
        .Texture = RenderTarget.Handle,
        .UserData = &TestContext,
        .ReadbackFunc = Simple_Texture_Readback,
    };

    gdi_render_params RenderParams = {
        .TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 },
    };

    GDI_Render(&RenderParams);
    GDI_Flush();

    ASSERT_EQ(TestContext.Format, TextureInfo.Format);
    ASSERT_EQ(TestContext.Dim.x, TextureInfo.Dim.x);
    ASSERT_EQ(TestContext.Dim.y, TextureInfo.Dim.y);

    struct inst_expected_tile {
        v2i Min;
        v2i Max;
        v4 Color;
    };
    inst_expected_tile Tiles[] = {
        { V2i(0, 0), V2i(32, 32), V4(1, 0, 0, 1) },
        { V2i(32, 0), V2i(64, 32), V4(0, 1, 0, 1) },
        { V2i(0, 32), V2i(32, 64), V4(0, 0, 1, 1) },
        { V2i(32, 32), V2i(64, 64), V4(1, 1, 0, 1) },
    };

    u32* TexelAt = (u32*)TestContext.Texels;
    f32 Epsilon = 1.0f / 255.0f;
    for (s32 y = 0; y < TestContext.Dim.y; y++) {
        for (s32 x = 0; x < TestContext.Dim.x; x++) {
            b32 HitRect = false;
            v4 HitColor = V4_Zero();
            for (size_t i = 0; i < Array_Count(Tiles); i++) {
                v2i Min = Tiles[i].Min;
                v2i Max = Tiles[i].Max;
                if (x >= Min.x && y >= Min.y && x < Max.x && y < Max.y) {
                    HitRect = true;
                    HitColor = Tiles[i].Color;
                    break;
                }
            }
            ASSERT_TRUE(HitRect);
            v4 TexelColor = V4_Color_From_U32(*TexelAt);
            ASSERT_NEAR(TexelColor.x, HitColor.x, Epsilon);
            ASSERT_NEAR(TexelColor.y, HitColor.y, Epsilon);
            ASSERT_NEAR(TexelColor.z, HitColor.z, Epsilon);
            ASSERT_NEAR(TexelColor.w, HitColor.w, Epsilon);
            TexelAt++;
        }
    }

    GDI_Delete_Buffer(IdxBuffer);
    GDI_Delete_Buffer(InstanceBuffer);
    GDI_Delete_Buffer(MeshBuffer);
    GPU_Buffer_Delete(&MatrixBuffer);
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
	// Overlap uses timestamps on both graphics and async compute; only valid when Vulkan reports one time domain.
	if(!GDI_Is_Async_Compute_Supported() || !GDI_Are_Graphics_And_Compute_Timestamps_Comparable()) return;

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
	GDI_Write_Timestamp_Async(QueryPool, QUERY_COMPUTE_BEGIN);

	{
		gdi_dispatch Dispatch = {
			.Shader = ComputeShader,
			.ThreadGroupCount = V3i(ComputeTextureInfo.Dim.x / 8, ComputeTextureInfo.Dim.y / 8, 1)
		};
		GDI_Submit_Async_Compute_Pass({ .Ptr = &ComputeTexture.View, .Count = 1 }, {}, { .Ptr = &Dispatch, .Count = 1 });
	}

	GDI_Write_Timestamp_Async(QueryPool, QUERY_COMPUTE_END);
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

	GDI_Wait_Signal(ComputeSignal);

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

	GDI_Wait_Signal(Signal);

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

	// --- Shaders (heavy sin/cos loops by default; waste is folded into alpha only so RGB matches CPU) ---

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
			float Waste = 0;
			for (int i = 0; i < 100000; i++) {
				Waste += sin((float)i * 0.001f) + cos((float)i * 0.001f);
			}
			// Burn time only: do not add Waste to RGB or tests drift (float4 sin/cos pollutes G/B).
			Output[ThreadID.xy] = float4(PushData.Intensity, 0, 0, 1 + Waste * 1e-15f);
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
			float Waste = 0;
			for (int i = 0; i < 100000; i++) {
				Waste += sin((float)i * 0.001f) + cos((float)i * 0.001f);
			}
			Output[ThreadID.xy] = float4(0, 0, PushData.Intensity, 1 + Waste * 1e-15f);
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
			float4 Load = InputTexture.Load(int3(ThreadID.xy, 0));
			float Waste = 0;
			for (int i = 0; i < 100000; i++) {
				Waste += sin((float)i * 0.001f) + cos((float)i * 0.001f);
			}
			Output[ThreadID.xy] = float4(Load.rgb, Load.a + Waste * 1e-15f);
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
			float Waste = 0;
			for (int i = 0; i < 100000; i++) {
				Waste += sin((float)i * 0.001f) + cos((float)i * 0.001f);
			}
			return float4(0, PushData.Intensity, 0, 1 + Waste * 1e-15f);
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
			float Waste = 0;
			for (int i = 0; i < 100000; i++) {
				Waste += sin((float)i * 0.001f) + cos((float)i * 0.001f);
			}
			return float4(R, G, B, 1 + Waste * 1e-15f);
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

	static const u32 FrameCount = 10;

	// --- Frame loop: pixel correctness (readback). No overlap queries — see AsyncComputeMultiStepOverlapStressTest. ---

	for(u32 Frame = 0; Frame < FrameCount; Frame++) {
		f32 T = (f32)Frame / (f32)(FrameCount - 1);
		f32 RedIntensity   = T;
		f32 BlueIntensity  = 1.0f - T;
		f32 GreenIntensity = 0.5f + 0.5f * T;

		gdi_dispatch RedDispatch = { .Shader = RedShader, .PushConstantCount = 1, .ThreadGroupCount = TGC };
		Memory_Copy(RedDispatch.PushConstants, &RedIntensity, sizeof(f32));
		gdi_signal RedSignal = GDI_Submit_Async_Compute_Pass(
			{ .Ptr = &TexRed.View, .Count = 1 }, {}, { .Ptr = &RedDispatch, .Count = 1 });

		gdi_dispatch BlueDispatch = { .Shader = BlueShader, .PushConstantCount = 1, .ThreadGroupCount = TGC };
		Memory_Copy(BlueDispatch.PushConstants, &BlueIntensity, sizeof(f32));
		gdi_signal BlueSignal = GDI_Submit_Async_Compute_Pass(
			{ .Ptr = &TexBlue.View, .Count = 1 }, {}, { .Ptr = &BlueDispatch, .Count = 1 });

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

		GDI_Wait_Signal(GreenSignal);

		gdi_dispatch CopyDispatch = {
			.Shader = CopyShader,
			.BindGroups = { CopyBindGroup },
			.ThreadGroupCount = TGC
		};
		gdi_signal CopySignal = GDI_Submit_Async_Compute_Pass(
			{ .Ptr = &TexGreen.View, .Count = 1 }, {}, { .Ptr = &CopyDispatch, .Count = 1 });

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
	}

	// Cleanup
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

// Stress: many back-to-back (async compute -> graphics) steps with no waits in between.
// Each step must be its own GDI_Render: one VK_Render batches *all* async work into one compute submit and
// all graphics into one graphics submit (vk_gdi.c), so per-step timestamps from a single render never
// interleave on the GPU — overlap only matches AsyncComputeOverlapTest when each step is submitted separately.
UTEST(gdi, AsyncComputeMultiStepOverlapStressTest) {
	if(!GDI_Is_Async_Compute_Supported() || !GDI_Are_Graphics_And_Compute_Timestamps_Comparable()) return;

	scratch Scratch;

	static const u32 StepCount = 8;
	enum {
		Q_COMPUTE_BEGIN = 0,
		Q_COMPUTE_END   = 1,
		Q_RENDER_BEGIN  = 2,
		Q_RENDER_END    = 3,
		Q_PER_STEP      = 4
	};

	gdi_query_pool_create_info QueryPoolInfo = {
		.Count = StepCount * Q_PER_STEP,
		.DebugName = String_Lit("Multi-step Async Overlap Query Pool")
	};
	gdi_query_pool QueryPool = GDI_Create_Query_Pool(&QueryPoolInfo);
	ASSERT_FALSE(GDI_Is_Null(QueryPool));

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
			.DebugName = String_Lit("Multi-step Heavy Compute Shader")
		};

		ComputeShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeShader));
		CSBlob->Release();
	}

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
			.DebugName = String_Lit("Multi-step Heavy Render Shader")
		};

		RenderShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(RenderShader));
		VtxBlob->Release();
		PxlBlob->Release();
	}

	v2i Dim = V2i(64, 64);
	v3i TGC = V3i(Dim.x / 8, Dim.y / 8, 1);

	texture ComputeTex[2];
	texture RenderTex[2];
	for(u32 i = 0; i < 2; i++) {
		ASSERT_TRUE(Texture_Create(&ComputeTex[i], {
			.Format = GDI_FORMAT_R8G8B8A8_UNORM,
			.Dim = Dim,
			.Usage = GDI_TEXTURE_USAGE_STORAGE
		}));
		ASSERT_TRUE(Texture_Create(&RenderTex[i], {
			.Format = GDI_FORMAT_R8G8B8A8_UNORM,
			.Dim = Dim,
			.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET
		}));
	}

	f64 Period = GDI_Get_Timestamp_Period();
	ASSERT_GT(Period, 0.0);

	for(u32 Step = 0; Step < StepCount; Step++) {
		u32 Base = Step * Q_PER_STEP;
		u32 Ping = Step & 1u;

		GDI_Write_Timestamp_Async(QueryPool, Base + Q_COMPUTE_BEGIN);

		{
			gdi_dispatch Dispatch = {
				.Shader = ComputeShader,
				.ThreadGroupCount = TGC
			};
			GDI_Submit_Async_Compute_Pass(
				{ .Ptr = &ComputeTex[Ping].View, .Count = 1 }, {}, { .Ptr = &Dispatch, .Count = 1 });
		}

		GDI_Write_Timestamp_Async(QueryPool, Base + Q_COMPUTE_END);
		GDI_Write_Timestamp(QueryPool, Base + Q_RENDER_BEGIN);

		{
			gdi_render_pass_begin_info BeginInfo = {
				.RenderTargetViews = { RenderTex[Ping].View },
				.ClearColors = { { .ShouldClear = true, .F32 = { 0, 0, 0, 1 } } }
			};

			gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);
			Render_Set_Shader(RenderPass, RenderShader);
			Render_Draw(RenderPass, 3, 0);
			GDI_End_Render_Pass(RenderPass);
			GDI_Submit_Render_Pass(RenderPass);
		}

		GDI_Write_Timestamp(QueryPool, Base + Q_RENDER_END);

		gdi_render_params EmptyRender = {};
		GDI_Render(&EmptyRender);
		GDI_Flush();

		gdi_timestamp_result StepResults[Q_PER_STEP];
		b32 Got = GDI_Get_Query_Results(QueryPool, Base, Q_PER_STEP, StepResults);
		ASSERT_TRUE(Got);

		for(u32 i = 0; i < Q_PER_STEP; i++) {
			ASSERT_TRUE(StepResults[i].Available);
		}

		if(Step == 0) {
			f64 ComputeMs = (f64)(StepResults[Q_COMPUTE_END].Ticks - StepResults[Q_COMPUTE_BEGIN].Ticks) * Period / 1000000.0;
			f64 RenderMs  = (f64)(StepResults[Q_RENDER_END].Ticks - StepResults[Q_RENDER_BEGIN].Ticks) * Period / 1000000.0;
			ASSERT_GT(ComputeMs, 0.1);
			ASSERT_GT(RenderMs, 0.1);
		}

		b32 HasOverlap =
			StepResults[Q_COMPUTE_END].Ticks > StepResults[Q_RENDER_BEGIN].Ticks &&
			StepResults[Q_COMPUTE_BEGIN].Ticks < StepResults[Q_RENDER_END].Ticks;

		ASSERT_TRUE(HasOverlap);
	}

	for(u32 i = 0; i < 2; i++) {
		Texture_Delete(&RenderTex[i]);
		Texture_Delete(&ComputeTex[i]);
	}
	GDI_Delete_Shader(RenderShader);
	GDI_Delete_Shader(ComputeShader);
	GDI_Delete_Query_Pool(QueryPool);
}

// CPU upload -> compute copy -> fullscreen draw -> readback. Always run: GDI_Submit_Async_Compute_Pass falls
// back to graphics-queue compute when async is unavailable, and GDI_Wait_Signal is a no-op there; transfer
// uses the graphics queue when there is no dedicated transfer queue. With dedicated queues, this also
// exercises transfer timeline -> compute and async compute vs graphics (vk_gdi.c).
UTEST(gdi, DedicatedTransferQueueAsyncComputeGraphicsTest) {
	scratch Scratch;

	v2i Dim = V2i(64, 64);
	v3i TGC = V3i(Dim.x / 8, Dim.y / 8, 1);

	texture SourceTex;
	ASSERT_TRUE(Texture_Create(&SourceTex, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = Dim,
		.Usage = GDI_TEXTURE_USAGE_SAMPLED
	}));

	texture StorageTex;
	ASSERT_TRUE(Texture_Create(&StorageTex, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = Dim,
		.Usage = GDI_TEXTURE_USAGE_STORAGE | GDI_TEXTURE_USAGE_SAMPLED
	}));

	texture RenderTarget;
	ASSERT_TRUE(Texture_Create(&RenderTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = Dim,
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET | GDI_TEXTURE_USAGE_READBACK
	}));

	gdi_layout ComputeLayout;
	gdi_shader ComputeShader;
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
			.DebugName = String_Lit("Transfer Chain Copy CS")
		};

		ComputeShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeShader));
		CSBlob->Release();
	}

	gdi_layout RenderLayout;
	gdi_shader RenderShader;
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
		Texture2D<float4> Tex : register(t0, space0);
		float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
			float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
			return float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
		}
		float4 PS_Main(float4 Position : SV_POSITION) : SV_TARGET0 {
			int3 Coord = int3(Position.xy, 0);
			return Tex.Load(Coord);
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
			.DebugName = String_Lit("Transfer Chain Present PS")
		};

		RenderShader = GDI_Create_Shader(&CreateInfo);
		ASSERT_FALSE(GDI_Is_Null(RenderShader));
		VtxBlob->Release();
		PxlBlob->Release();
	}

	gdi_bind_group ComputeInputBindGroup;
	{
		gdi_bind_group_write Write = { .TextureViews = { .Ptr = &SourceTex.View, .Count = 1 } };
		gdi_bind_group_create_info BGInfo = {
			.Layout = ComputeLayout,
			.Writes = { .Ptr = &Write, .Count = 1 }
		};
		ComputeInputBindGroup = GDI_Create_Bind_Group(&BGInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeInputBindGroup));
	}

	gdi_bind_group RenderTexBindGroup;
	{
		gdi_bind_group_write Write = { .TextureViews = { .Ptr = &StorageTex.View, .Count = 1 } };
		gdi_bind_group_create_info BGInfo = {
			.Layout = RenderLayout,
			.Writes = { .Ptr = &Write, .Count = 1 }
		};
		RenderTexBindGroup = GDI_Create_Bind_Group(&BGInfo);
		ASSERT_FALSE(GDI_Is_Null(RenderTexBindGroup));
	}

	size_t MipPixelBytes = (size_t)Dim.x * (size_t)Dim.y * GDI_Get_Format_Size(GDI_FORMAT_R8G8B8A8_UNORM);
	u8* UploadPixels = (u8*)Arena_Push(Scratch.Arena, MipPixelBytes);
	// Per-texel u32: byte0=R, byte1=G, byte2=B, byte3=A (same unpacking as MultiFrame stress below).
	const u32 Fill = 0xFF4070E0; /* R=0xE0, G=0x70, B=0x40, A=0xFF */
	for(size_t i = 0; i < (size_t)Dim.x * (size_t)Dim.y; i++) {
		((u32*)UploadPixels)[i] = Fill;
	}

	buffer MipBuffers[1] = { { .Ptr = UploadPixels, .Size = MipPixelBytes } };
	gdi_texture_update TexUpdate = {
		.Texture = SourceTex.Handle,
		.MipOffset = 0,
		.MipCount = 1,
		.Offset = V2i(0, 0),
		.Dim = Dim,
		.UpdateData = MipBuffers
	};
	GDI_Update_Textures({ .Ptr = &TexUpdate, .Count = 1 });

	gdi_dispatch Dispatch = {
		.Shader = ComputeShader,
		.BindGroups = { ComputeInputBindGroup },
		.ThreadGroupCount = TGC
	};
	gdi_signal ComputeSignal = GDI_Submit_Async_Compute_Pass(
		{ .Ptr = &StorageTex.View, .Count = 1 }, {}, { .Ptr = &Dispatch, .Count = 1 });

	GDI_Wait_Signal(ComputeSignal);

	{
		gdi_render_pass_begin_info BeginInfo = {
			.RenderTargetViews = { RenderTarget.View },
			.ClearColors = { { .ShouldClear = true, .F32 = { 0, 0, 0, 1 } } }
		};
		gdi_render_pass* RP = GDI_Begin_Render_Pass(&BeginInfo);
		Render_Set_Shader(RP, RenderShader);
		Render_Set_Bind_Group(RP, 0, RenderTexBindGroup);
		Render_Draw(RP, 3, 0);
		GDI_End_Render_Pass(RP);
		GDI_Submit_Render_Pass(RP);
	}

	gdi_texture_info RTInfo = GDI_Get_Texture_Info(RenderTarget.Handle);
	simple_test_context TestContext = {
		.Texels = (u8*)Arena_Push(Scratch.Arena, MipPixelBytes),
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

	ASSERT_EQ(TestContext.Dim.x, RTInfo.Dim.x);
	ASSERT_EQ(TestContext.Dim.y, RTInfo.Dim.y);

	const u8 ExpR = 0xE0;
	const u8 ExpG = 0x70;
	const u8 ExpB = 0x40;
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

	GDI_Delete_Bind_Group(RenderTexBindGroup);
	GDI_Delete_Bind_Group(ComputeInputBindGroup);
	GDI_Delete_Shader(RenderShader);
	GDI_Delete_Shader(ComputeShader);
	GDI_Delete_Bind_Group_Layout(RenderLayout);
	GDI_Delete_Bind_Group_Layout(ComputeLayout);
	Texture_Delete(&RenderTarget);
	Texture_Delete(&StorageTex);
	Texture_Delete(&SourceTex);
}

/* Vulkan-compatible sizes: VkDrawIndirectCommand = 16 bytes, VkDrawIndexedIndirectCommand = 20 bytes, VkDispatchIndirectCommand = 12 bytes. */
enum {
	INDIRECT_SCRATCH_OFF_DISPATCH = 0,
	INDIRECT_SCRATCH_OFF_DRAW_VTX = 16,
	INDIRECT_SCRATCH_OFF_DRAW_IDX = 32,
	INDIRECT_SCRATCH_OFF_COUNT_U32 = 52,
	INDIRECT_SCRATCH_OFF_DRAW_VTX_COUNT_0 = 64,
	INDIRECT_SCRATCH_OFF_DRAW_VTX_COUNT_1 = 80,
	INDIRECT_SCRATCH_OFF_DRAW_IDX_COUNT_0 = 96,
	INDIRECT_SCRATCH_OFF_DRAW_IDX_COUNT_1 = 116,
	INDIRECT_SCRATCH_SIZE = 256,
	DISPATCH_VERIFY_WORDS = 64,
};

struct indirect_buffer_readback_ctx {
	u8*  Data;
	size_t Size;
};

function GDI_BUFFER_READBACK_DEFINE(Simple_Buffer_Readback) {
	indirect_buffer_readback_ctx* Ctx = (indirect_buffer_readback_ctx*)UserData;
	Assert(Size <= Ctx->Size);
	Memory_Copy(Ctx->Data, Data, Size);
}

UTEST(gdi, IndirectDrawAndDispatchTest) {
	gdi_tests* Tests = GDI_Get_Tests();
	ASSERT_TRUE(Tests);
	scratch Scratch;

	/* --- Layouts: writer (storage scratch), verify (storage out), render (CB + texture) --- */
	gdi_layout FillLayout;
	gdi_layout VerifyLayout;
	{
		gdi_bind_group_binding StorageBufferBinding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_BUFFER, .Count = 1 };
		gdi_bind_group_layout_create_info LayoutInfo = { .Bindings = { .Ptr = &StorageBufferBinding, .Count = 1 } };
		FillLayout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
		ASSERT_FALSE(GDI_Is_Null(FillLayout));
		VerifyLayout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
		ASSERT_FALSE(GDI_Is_Null(VerifyLayout));
	}

	gdi_layout MatrixLayout = Tests->BufferLayout;

	/* --- Shaders --- */
	gdi_shader FillShader;
	gdi_shader VerifyShaderSync;
	gdi_shader VerifyShaderAsync;
	gdi_shader RenderShader;
	{
		const char* FillHlsl = R"(
		RWByteAddressBuffer Scratch : register(u0);
		[numthreads(1, 1, 1)]
		void CS_Main(uint3 tid : SV_DispatchThreadID) {
			// VkDispatchIndirectCommand @ 0: (2,2,1) groups, [numthreads(4,4,1)] -> 64 threads
			Scratch.Store(0, 2u);
			Scratch.Store(4, 2u);
			Scratch.Store(8, 1u);
			// VkDrawIndirectCommand @ 16: Q0 red — 6 verts, firstVertex 0
			Scratch.Store(16, 6u);
			Scratch.Store(20, 1u);
			Scratch.Store(24, 0u);
			Scratch.Store(28, 0u);
			// VkDrawIndexedIndirectCommand @ 32: Q1 green
			Scratch.Store(32, 6u);
			Scratch.Store(36, 1u);
			Scratch.Store(40, 6u);
			Scratch.Store(44, asuint((int)6));
			Scratch.Store(48, 0u);
			// draw count buffer value @ 52
			Scratch.Store(52, 1u);
			// Two VkDrawIndirectCommand for count draw @ 64, 80 — only first used when count==1
			Scratch.Store(64, 6u);
			Scratch.Store(68, 1u);
			Scratch.Store(72, 12u);
			Scratch.Store(76, 0u);
			Scratch.Store(80, 6u);
			Scratch.Store(84, 1u);
			Scratch.Store(88, 0u);
			Scratch.Store(92, 0u);
			// VkDrawIndexedIndirectCommand @ 96 — Q3 yellow (IB 18–23: relative 0,1,2,2,4,0; vertexOffset 18)
			Scratch.Store(96, 6u);
			Scratch.Store(100, 1u);
			Scratch.Store(104, 18u);
			Scratch.Store(108, asuint((int)18));
			Scratch.Store(112, 0u);
		}
		)";
		IDxcBlob* Blob = Compile_Shader(String_Null_Term(FillHlsl), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(Blob);
		gdi_bind_group_binding WritableStorageBinding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_BUFFER, .Count = 1 };
		gdi_shader_create_info ShaderCreateInfo = {
			.CS = { .Ptr = (u8*)Blob->GetBufferPointer(), .Size = Blob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &FillLayout, .Count = 1 },
			.WritableBindings = { .Ptr = &WritableStorageBinding, .Count = 1 },
			.DebugName = String_Lit("Indirect fill CS"),
		};
		FillShader = GDI_Create_Shader(&ShaderCreateInfo);
		ASSERT_FALSE(GDI_Is_Null(FillShader));
		Blob->Release();
	}
	{
		const char* VerifyHlsl = R"(
		RWByteAddressBuffer OutBuf : register(u0);
		[numthreads(4, 4, 1)]
		void CS_Main(uint3 dtid : SV_DispatchThreadID) {
			uint idx = dtid.x + dtid.y * 8u;
			if (idx < 64u)
				OutBuf.Store(idx * 4, 0xABCDEF01u);
		}
		)";
		IDxcBlob* Blob = Compile_Shader(String_Null_Term(VerifyHlsl), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(Blob);
		gdi_bind_group_binding WritableStorageBinding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_BUFFER, .Count = 1 };
		gdi_shader_create_info ShaderCreateInfo = {
			.CS = { .Ptr = (u8*)Blob->GetBufferPointer(), .Size = Blob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &VerifyLayout, .Count = 1 },
			.WritableBindings = { .Ptr = &WritableStorageBinding, .Count = 1 },
			.DebugName = String_Lit("Indirect dispatch verify CS"),
		};
		VerifyShaderSync = GDI_Create_Shader(&ShaderCreateInfo);
		ASSERT_FALSE(GDI_Is_Null(VerifyShaderSync));
		VerifyShaderAsync = GDI_Create_Shader(&ShaderCreateInfo);
		ASSERT_FALSE(GDI_Is_Null(VerifyShaderAsync));
		Blob->Release();
	}
	{
		const char* RenderHlsl = R"(
		struct vs_in { float2 P : POSITION0; float4 C : COLOR0; };
		struct vs_out { float4 Pos : SV_POSITION; float4 C : COLOR0; };
		struct cb { float4x4 M; };
		ConstantBuffer<cb> PC : register(b0, space0);
		vs_out VS_Main(vs_in I) {
			vs_out O;
			O.Pos = mul(float4(I.P, 0, 1), PC.M);
			O.C = I.C;
			return O;
		}
		float4 PS_Main(vs_out I) : SV_TARGET0 { return I.C; }
		)";
		IDxcBlob* VS = Compile_Shader(String_Null_Term(RenderHlsl), String_Lit("vs_6_0"), String_Lit("VS_Main"), { String_Lit("-fvk-invert-y") });
		IDxcBlob* PS = Compile_Shader(String_Null_Term(RenderHlsl), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VS && PS);
		gdi_vtx_attribute IndirectDrawAttributes[] = {
			{ String_Lit("POSITION"), GDI_FORMAT_R32G32_FLOAT },
			{ String_Lit("COLOR"), GDI_FORMAT_R32G32B32A32_FLOAT },
		};
		gdi_vtx_binding IndirectDrawVtxBinding = {
			.Stride = sizeof(vtx_p2_c4),
			.Attributes = { .Ptr = IndirectDrawAttributes, .Count = Array_Count(IndirectDrawAttributes) },
		};
		gdi_shader_create_info ShaderCreateInfo = {
			.VS = { .Ptr = (u8*)VS->GetBufferPointer(), .Size = VS->GetBufferSize() },
			.PS = { .Ptr = (u8*)PS->GetBufferPointer(), .Size = PS->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &MatrixLayout, .Count = 1 },
			.VtxBindings = { .Ptr = &IndirectDrawVtxBinding, .Count = 1 },
			.RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
			.DebugName = String_Lit("Indirect draw RT"),
		};
		RenderShader = GDI_Create_Shader(&ShaderCreateInfo);
		ASSERT_FALSE(GDI_Is_Null(RenderShader));
		VS->Release();
		PS->Release();
	}

	gdi_buffer_create_info IndirectScratchInfo = {
		.Size = INDIRECT_SCRATCH_SIZE,
		.Usage = GDI_BUFFER_USAGE_STORAGE | GDI_BUFFER_USAGE_INDIRECT,
		.DebugName = String_Lit("indirect scratch"),
	};
	gdi_buffer IndirectScratch = GDI_Create_Buffer(&IndirectScratchInfo);
	ASSERT_FALSE(GDI_Is_Null(IndirectScratch));

	size_t VerifyBytes = DISPATCH_VERIFY_WORDS * sizeof(u32);
	gdi_buffer_create_info DispatchVerifyBufferInfo = {
		.Size = VerifyBytes,
		.Usage = GDI_BUFFER_USAGE_STORAGE | GDI_BUFFER_USAGE_READBACK,
		.DebugName = String_Lit("dispatch verify"),
	};
	gdi_buffer DispatchVerifyBuffer = GDI_Create_Buffer(&DispatchVerifyBufferInfo);
	ASSERT_FALSE(GDI_Is_Null(DispatchVerifyBuffer));

	gdi_buffer_create_info DispatchVerifyBufferAsyncInfo = {
		.Size = VerifyBytes,
		.Usage = GDI_BUFFER_USAGE_STORAGE | GDI_BUFFER_USAGE_READBACK,
		.DebugName = String_Lit("dispatch verify async"),
	};
	gdi_buffer DispatchVerifyBufferAsync = GDI_Create_Buffer(&DispatchVerifyBufferAsyncInfo);
	ASSERT_FALSE(GDI_Is_Null(DispatchVerifyBufferAsync));

	gdi_bind_group ScratchFillBindGroup;
	{
		gdi_bind_group_buffer IndirectScratchBindGroupBuffer = { .Buffer = IndirectScratch };
		gdi_bind_group_write ScratchFillWrite = { .Buffers = { .Ptr = &IndirectScratchBindGroupBuffer, .Count = 1 } };
		gdi_bind_group_create_info BindGroupInfo = { .Layout = FillLayout, .Writes = { .Ptr = &ScratchFillWrite, .Count = 1 } };
		ScratchFillBindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
		ASSERT_FALSE(GDI_Is_Null(ScratchFillBindGroup));
	}
	gdi_bind_group DispatchVerifyBindGroup;
	{
		gdi_bind_group_buffer DispatchVerifyBindGroupBuffer = { .Buffer = DispatchVerifyBuffer };
		gdi_bind_group_write DispatchVerifyWrite = { .Buffers = { .Ptr = &DispatchVerifyBindGroupBuffer, .Count = 1 } };
		gdi_bind_group_create_info BindGroupInfo = { .Layout = VerifyLayout, .Writes = { .Ptr = &DispatchVerifyWrite, .Count = 1 } };
		DispatchVerifyBindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
		ASSERT_FALSE(GDI_Is_Null(DispatchVerifyBindGroup));
	}
	gdi_bind_group DispatchVerifyAsyncBindGroup;
	{
		gdi_bind_group_buffer DispatchVerifyAsyncBindGroupBuffer = { .Buffer = DispatchVerifyBufferAsync };
		gdi_bind_group_write DispatchVerifyAsyncWrite = { .Buffers = { .Ptr = &DispatchVerifyAsyncBindGroupBuffer, .Count = 1 } };
		gdi_bind_group_create_info BindGroupInfo = { .Layout = VerifyLayout, .Writes = { .Ptr = &DispatchVerifyAsyncWrite, .Count = 1 } };
		DispatchVerifyAsyncBindGroup = GDI_Create_Bind_Group(&BindGroupInfo);
		ASSERT_FALSE(GDI_Is_Null(DispatchVerifyAsyncBindGroup));
	}

	/*
	   Per quad: 6 vtx triangle list v0,v1,v2,v2,v3,v0 with corners TL,TR,BR,BL — tris (0,1,2) and (3,4,5)=(v2,v3,v0).
	   Indexed draws use 0,1,2,2,4,0 so the four unique corners are slots 0,1,2,4 (slot 3 repeats v2 for the non-indexed list).
	*/
	v4 CR = V4(1, 0, 0, 1), CG = V4(0, 1, 0, 1), CB = V4(0, 0, 1, 1), CY = V4(1, 1, 0, 1);
	vtx_p2_c4 Verts[24] = {
		{ V2(0, 0), CR }, { V2(32, 0), CR }, { V2(32, 32), CR }, { V2(32, 32), CR }, { V2(0, 32), CR }, { V2(0, 0), CR },
		{ V2(64, 0), CG }, { V2(96, 0), CG }, { V2(96, 32), CG }, { V2(96, 32), CG }, { V2(64, 32), CG }, { V2(64, 0), CG },
		{ V2(0, 64), CB }, { V2(32, 64), CB }, { V2(32, 96), CB }, { V2(32, 96), CB }, { V2(0, 96), CB }, { V2(0, 64), CB },
		{ V2(64, 64), CY }, { V2(96, 64), CY }, { V2(96, 96), CY }, { V2(96, 96), CY }, { V2(64, 96), CY }, { V2(64, 64), CY },
	};

	u32 Indices[24];
	for (u32 q = 0; q < 4; q++) {
		u32 IndexOffset = q * 6;
		Indices[IndexOffset + 0] = 0; Indices[IndexOffset + 1] = 1; Indices[IndexOffset + 2] = 2;
		Indices[IndexOffset + 3] = 2; Indices[IndexOffset + 4] = 4; Indices[IndexOffset + 5] = 0;
	}

	gdi_buffer_create_info VertexBufferInfo = {
		.Size = sizeof(Verts),
		.Usage = GDI_BUFFER_USAGE_VTX,
		.InitialData = Make_Buffer(Verts, sizeof(Verts)),
	};
	gdi_buffer VertexBuffer = GDI_Create_Buffer(&VertexBufferInfo);
	gdi_buffer_create_info IndexBufferInfo = {
		.Size = sizeof(Indices),
		.Usage = GDI_BUFFER_USAGE_IDX,
		.InitialData = Make_Buffer(Indices, sizeof(Indices)),
	};
	gdi_buffer IndexBuffer = GDI_Create_Buffer(&IndexBufferInfo);
	ASSERT_FALSE(GDI_Is_Null(VertexBuffer));
	ASSERT_FALSE(GDI_Is_Null(IndexBuffer));

	texture RenderTarget;
	ASSERT_TRUE(Texture_Create(&RenderTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = V2i(128, 128),
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET | GDI_TEXTURE_USAGE_READBACK,
	}));

	m4 Ortho = M4_Orthographic(0, 128, 128, 0, -1, 1);
	Ortho = M4_Transpose(&Ortho);
	gpu_buffer MatrixBuffer;
	ASSERT_TRUE(GPU_Buffer_Create(&MatrixBuffer, {
		.Size = sizeof(m4),
		.Usage = GDI_BUFFER_USAGE_CONSTANT,
		.InitialData = Make_Buffer(&Ortho, sizeof(m4)),
	}));

	/* 1) Fill indirect buffers (direct compute dispatch) */
	gdi_dispatch FillDispatch = {
		.Shader = FillShader,
		.BindGroups = { ScratchFillBindGroup },
		.ThreadGroupCount = V3i(1, 1, 1),
	};
	gdi_signal FillSignal = GDI_Submit_Compute_Pass({}, { .Ptr = &IndirectScratch, .Count = 1 }, { .Ptr = &FillDispatch, .Count = 1 });

	/* 2) Indirect dispatch — sync queue */
	gdi_dispatch VerifyDispatchSync = {
		.Shader = VerifyShaderSync,
		.BindGroups = { DispatchVerifyBindGroup },
		.IndirectBuffer = IndirectScratch,
		.IndirectOffset = INDIRECT_SCRATCH_OFF_DISPATCH,
	};
	GDI_Submit_Compute_Pass({}, { .Ptr = &DispatchVerifyBuffer, .Count = 1 }, { .Ptr = &VerifyDispatchSync, .Count = 1 });

	GDI_Wait_Signal(FillSignal);
	/* 3) Same indirect dispatch on async compute (when supported), different output buffer */
	gdi_dispatch VerifyDispatchAsync = {
		.Shader = VerifyShaderAsync,
		.BindGroups = { DispatchVerifyAsyncBindGroup },
		.IndirectBuffer = IndirectScratch,
		.IndirectOffset = INDIRECT_SCRATCH_OFF_DISPATCH,
	};
	gdi_signal AsyncComputeSignal = GDI_Submit_Async_Compute_Pass({}, { .Ptr = &DispatchVerifyBufferAsync, .Count = 1 }, { .Ptr = &VerifyDispatchAsync, .Count = 1 });
	GDI_Wait_Signal(AsyncComputeSignal);

	/* 4) Render: four indirect draw variants into quadrants */
	gdi_render_pass_begin_info BeginInfo = {
		.RenderTargetViews = { RenderTarget.View },
		.ClearColors = { { .ShouldClear = true, .F32 = { 0.1f, 0.1f, 0.1f, 1 } } },
	};
	gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);
	Render_Set_Shader(RenderPass, RenderShader);
	Render_Set_Bind_Group(RenderPass, 0, MatrixBuffer.BindGroup);
	Render_Set_Vtx_Buffer(RenderPass, 0, VertexBuffer);
	Render_Set_Idx_Buffer(RenderPass, IndexBuffer, GDI_IDX_FORMAT_32_BIT);
	Render_Set_Scissor(RenderPass, 0, 0, 128, 128);

	Render_Set_Scissor(RenderPass, 0, 0, 64, 64);
	Render_Draw_Indirect(RenderPass, IndirectScratch, INDIRECT_SCRATCH_OFF_DRAW_VTX, 1, (u32)sizeof(gdi_indirect_draw_info));

	Render_Set_Scissor(RenderPass, 64, 0, 128, 64);
	Render_Draw_Idx_Indirect(RenderPass, IndirectScratch, INDIRECT_SCRATCH_OFF_DRAW_IDX, 1, (u32)sizeof(gdi_indexed_indirect_draw_info));

	Render_Set_Scissor(RenderPass, 0, 64, 64, 128);
	Render_Draw_Indirect_Count(RenderPass, IndirectScratch, INDIRECT_SCRATCH_OFF_DRAW_VTX_COUNT_0, IndirectScratch, INDIRECT_SCRATCH_OFF_COUNT_U32, 2, (u32)sizeof(gdi_indirect_draw_info));

	Render_Set_Scissor(RenderPass, 64, 64, 128, 128);
	Render_Draw_Idx_Indirect_Count(RenderPass, IndirectScratch, INDIRECT_SCRATCH_OFF_DRAW_IDX_COUNT_0, IndirectScratch, INDIRECT_SCRATCH_OFF_COUNT_U32, 2, (u32)sizeof(gdi_indexed_indirect_draw_info));

	GDI_End_Render_Pass(RenderPass);
	GDI_Submit_Render_Pass(RenderPass);

	u8* SyncDispatchVerifyBytes = (u8*)Arena_Push(Scratch.Arena, VerifyBytes);
	u8* AsyncDispatchVerifyBytes = (u8*)Arena_Push(Scratch.Arena, VerifyBytes);
	Memory_Clear(SyncDispatchVerifyBytes, VerifyBytes);
	Memory_Clear(AsyncDispatchVerifyBytes, VerifyBytes);

	indirect_buffer_readback_ctx SyncDispatchVerifyReadbackContext = { .Data = SyncDispatchVerifyBytes, .Size = VerifyBytes };
	indirect_buffer_readback_ctx AsyncDispatchVerifyReadbackContext = { .Data = AsyncDispatchVerifyBytes, .Size = VerifyBytes };

	gdi_texture_info TextureInfo = GDI_Get_Texture_Info(RenderTarget.Handle);
	simple_test_context TestContext = {
		.Texels = (u8*)Arena_Push(Scratch.Arena, TextureInfo.Dim.x * TextureInfo.Dim.y * GDI_Get_Format_Size(TextureInfo.Format)),
	};
	gdi_buffer_readback BufferReadbacks[2];
	BufferReadbacks[0] = { .Buffer = DispatchVerifyBuffer, .UserData = &SyncDispatchVerifyReadbackContext, .ReadbackFunc = Simple_Buffer_Readback };
	BufferReadbacks[1] = { .Buffer = DispatchVerifyBufferAsync, .UserData = &AsyncDispatchVerifyReadbackContext, .ReadbackFunc = Simple_Buffer_Readback };
	gdi_texture_readback TextureReadback = {
		.Texture = RenderTarget.Handle,
		.UserData = &TestContext,
		.ReadbackFunc = Simple_Texture_Readback,
	};
	gdi_render_params RenderParams = {
		.TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 },
		.BufferReadbacks = { .Ptr = BufferReadbacks, .Count = 2 },
	};
	GDI_Render(&RenderParams);
	GDI_Flush();

	for (size_t i = 0; i < DISPATCH_VERIFY_WORDS; i++) {
		ASSERT_EQ(((u32*)SyncDispatchVerifyBytes)[i], 0xABCDEF01u);
		ASSERT_EQ(((u32*)AsyncDispatchVerifyBytes)[i], 0xABCDEF01u);
	}

	f32 Eps = 1.0f / 255.0f;
	auto ExpectColor = [&](s32 x, s32 y, v4 ExpectedColor) {
		u32* Texels = (u32*)TestContext.Texels;
		u32 Sample = Texels[x + y * TextureInfo.Dim.x];
		v4 GotColor = V4_Color_From_U32(Sample);
		ASSERT_NEAR(GotColor.x, ExpectedColor.x, Eps);
		ASSERT_NEAR(GotColor.y, ExpectedColor.y, Eps);
		ASSERT_NEAR(GotColor.z, ExpectedColor.z, Eps);
		ASSERT_NEAR(GotColor.w, ExpectedColor.w, Eps);
	};
	ExpectColor(16, 16, CR);
	ExpectColor(80, 16, CG);
	ExpectColor(16, 80, CB);
	ExpectColor(80, 80, CY);

	GDI_Delete_Bind_Group(DispatchVerifyAsyncBindGroup);
	GDI_Delete_Bind_Group(DispatchVerifyBindGroup);
	GDI_Delete_Bind_Group(ScratchFillBindGroup);
	GDI_Delete_Buffer(IndexBuffer);
	GDI_Delete_Buffer(VertexBuffer);
	GPU_Buffer_Delete(&MatrixBuffer);
	Texture_Delete(&RenderTarget);
	GDI_Delete_Buffer(DispatchVerifyBufferAsync);
	GDI_Delete_Buffer(DispatchVerifyBuffer);
	GDI_Delete_Buffer(IndirectScratch);
	GDI_Delete_Shader(RenderShader);
	GDI_Delete_Shader(VerifyShaderAsync);
	GDI_Delete_Shader(VerifyShaderSync);
	GDI_Delete_Shader(FillShader);
	GDI_Delete_Bind_Group_Layout(VerifyLayout);
	GDI_Delete_Bind_Group_Layout(FillLayout);
}

/* Per-frame logical size for dynamic indirect ring (one gdi_indirect_draw_info per slot). */
enum { INDIRECT_DYNAMIC_FRAME_STRIDE = 64 };

UTEST(gdi, DynamicIndirectBufferMultiFrameTest) {
	gdi_tests* Tests = GDI_Get_Tests();
	ASSERT_TRUE(Tests);
	scratch Scratch;

	gdi_layout FillLayout;
	{
		gdi_bind_group_binding StorageBufferBinding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_BUFFER, .Count = 1 };
		gdi_bind_group_layout_create_info LayoutInfo = { .Bindings = { .Ptr = &StorageBufferBinding, .Count = 1 } };
		FillLayout = GDI_Create_Bind_Group_Layout(&LayoutInfo);
		ASSERT_FALSE(GDI_Is_Null(FillLayout));
	}

	gdi_shader FillShader;
	{
		const char* FillHlsl = R"(
		struct push_data {
			uint Slot;
			uint SlotStride;
			uint Quad;
		};
		[[vk::push_constant]]
		ConstantBuffer<push_data> Push : register(b0);
		RWByteAddressBuffer Scratch : register(u0);
		[numthreads(1, 1, 1)]
		void CS_Main(uint3 tid : SV_DispatchThreadID) {
			uint b = Push.Slot * Push.SlotStride;
			// gdi_indexed_indirect_draw_info @ b — full quad per color (IB: 6 indices / quad)
			Scratch.Store(b + 0, 6u);
			Scratch.Store(b + 4, 1u);
			Scratch.Store(b + 8, Push.Quad * 6u);
			Scratch.Store(b + 12, asuint((int)(Push.Quad * 4)));
			Scratch.Store(b + 16, 0u);
		}
		)";
		IDxcBlob* Blob = Compile_Shader(String_Null_Term(FillHlsl), String_Lit("cs_6_0"), String_Lit("CS_Main"), {});
		ASSERT_TRUE(Blob);
		gdi_bind_group_binding WritableStorageBinding = { .Type = GDI_BIND_GROUP_TYPE_STORAGE_BUFFER, .Count = 1 };
		gdi_shader_create_info ShaderCreateInfo = {
			.CS = { .Ptr = (u8*)Blob->GetBufferPointer(), .Size = Blob->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &FillLayout, .Count = 1 },
			.WritableBindings = { .Ptr = &WritableStorageBinding, .Count = 1 },
			.PushConstantCount = 3,
			.DebugName = String_Lit("Dynamic indirect fill CS"),
		};
		FillShader = GDI_Create_Shader(&ShaderCreateInfo);
		ASSERT_FALSE(GDI_Is_Null(FillShader));
		Blob->Release();
	}

	gdi_layout MatrixLayout = Tests->BufferLayout;
	gdi_shader RenderShader;
	{
		const char* RenderHlsl = R"(
		struct vs_in { float2 P : POSITION0; float4 C : COLOR0; };
		struct vs_out { float4 Pos : SV_POSITION; float4 C : COLOR0; };
		struct cb { float4x4 M; };
		ConstantBuffer<cb> PC : register(b0, space0);
		vs_out VS_Main(vs_in I) {
			vs_out O;
			O.Pos = mul(float4(I.P, 0, 1), PC.M);
			O.C = I.C;
			return O;
		}
		float4 PS_Main(vs_out I) : SV_TARGET0 { return I.C; }
		)";
		IDxcBlob* VS = Compile_Shader(String_Null_Term(RenderHlsl), String_Lit("vs_6_0"), String_Lit("VS_Main"), { String_Lit("-fvk-invert-y") });
		IDxcBlob* PS = Compile_Shader(String_Null_Term(RenderHlsl), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VS && PS);
		gdi_vtx_attribute DynamicIndirectAttributes[] = {
			{ String_Lit("POSITION"), GDI_FORMAT_R32G32_FLOAT },
			{ String_Lit("COLOR"), GDI_FORMAT_R32G32B32A32_FLOAT },
		};
		gdi_vtx_binding DynamicIndirectVtxBinding = {
			.Stride = sizeof(vtx_p2_c4),
			.Attributes = { .Ptr = DynamicIndirectAttributes, .Count = Array_Count(DynamicIndirectAttributes) },
		};
		gdi_shader_create_info ShaderCreateInfo = {
			.VS = { .Ptr = (u8*)VS->GetBufferPointer(), .Size = VS->GetBufferSize() },
			.PS = { .Ptr = (u8*)PS->GetBufferPointer(), .Size = PS->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &MatrixLayout, .Count = 1 },
			.VtxBindings = { .Ptr = &DynamicIndirectVtxBinding, .Count = 1 },
			.RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
			.DebugName = String_Lit("Dynamic indirect MF RT"),
		};
		RenderShader = GDI_Create_Shader(&ShaderCreateInfo);
		ASSERT_FALSE(GDI_Is_Null(RenderShader));
		VS->Release();
		PS->Release();
	}

	v4 QuadColors[4] = {
		V4(1, 0, 0, 1), V4(0, 1, 0, 1), V4(0, 0, 1, 1), V4(1, 1, 0, 1),
	};
	/* 4 quads × 4 corners (TL, TR, BR, BL); indexed draws use 0,1,2,2,3,0 per quad. */
	vtx_p2_c4 Verts[16];
	for (u32 q = 0; q < 4; q++) {
		v4 C = QuadColors[q];
		Verts[q * 4 + 0] = { V2(0, 0), C };
		Verts[q * 4 + 1] = { V2(64, 0), C };
		Verts[q * 4 + 2] = { V2(64, 64), C };
		Verts[q * 4 + 3] = { V2(0, 64), C };
	}
	u32 Indices[24];
	for (u32 q = 0; q < 4; q++) {
		u32 O = q * 6;
		Indices[O + 0] = 0; Indices[O + 1] = 1; Indices[O + 2] = 2;
		Indices[O + 3] = 2; Indices[O + 4] = 3; Indices[O + 5] = 0;
	}

	gdi_buffer_create_info VertexBufferInfo = {
		.Size = sizeof(Verts),
		.Usage = GDI_BUFFER_USAGE_VTX,
		.InitialData = Make_Buffer(Verts, sizeof(Verts)),
	};
	gdi_buffer VertexBuffer = GDI_Create_Buffer(&VertexBufferInfo);
	ASSERT_FALSE(GDI_Is_Null(VertexBuffer));
	gdi_buffer_create_info IndexBufferInfo = {
		.Size = sizeof(Indices),
		.Usage = GDI_BUFFER_USAGE_IDX,
		.InitialData = Make_Buffer(Indices, sizeof(Indices)),
	};
	gdi_buffer IndexBuffer = GDI_Create_Buffer(&IndexBufferInfo);
	ASSERT_FALSE(GDI_Is_Null(IndexBuffer));

	texture RenderTarget;
	ASSERT_TRUE(Texture_Create(&RenderTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = V2i(64, 64),
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET | GDI_TEXTURE_USAGE_READBACK,
	}));

	m4 Ortho = M4_Orthographic(0, 64, 64, 0, -1, 1);
	Ortho = M4_Transpose(&Ortho);
	gpu_buffer MatrixBuffer;
	ASSERT_TRUE(GPU_Buffer_Create(&MatrixBuffer, {
		.Size = sizeof(m4),
		.Usage = GDI_BUFFER_USAGE_CONSTANT,
		.InitialData = Make_Buffer(&Ortho, sizeof(m4)),
	}));

	struct indirect_fill_push {
		u32 Slot;
		u32 SlotStride;
		u32 Quad;
	};

	u32 FrameSlotCount = GDI_Get()->FramesInFlight + 1;
	size_t DynamicIndirectRingBytes = FrameSlotCount * INDIRECT_DYNAMIC_FRAME_STRIDE;

	/* --- Compute fills the dynamic indirect buffer each frame --- */
	{
		gdi_buffer_create_info DynamicIndirectBufferInfo = {
			.Size = DynamicIndirectRingBytes,
			.Usage = GDI_BUFFER_USAGE_STORAGE | GDI_BUFFER_USAGE_INDIRECT | GDI_BUFFER_USAGE_DYNAMIC,
			.DebugName = String_Lit("dyn indirect cs"),
		};
		gdi_buffer ComputeDynamicIndirectBuffer = GDI_Create_Buffer(&DynamicIndirectBufferInfo);
		ASSERT_FALSE(GDI_Is_Null(ComputeDynamicIndirectBuffer));

		gdi_bind_group_buffer IndirectBufferForBindGroup = { .Buffer = ComputeDynamicIndirectBuffer };
		gdi_bind_group_write FillBindGroupWrite = { .Buffers = { .Ptr = &IndirectBufferForBindGroup, .Count = 1 } };
		gdi_bind_group_create_info FillBindGroupInfo = { .Layout = FillLayout, .Writes = { .Ptr = &FillBindGroupWrite, .Count = 1 } };
		gdi_bind_group FillBindGroup = GDI_Create_Bind_Group(&FillBindGroupInfo);
		ASSERT_FALSE(GDI_Is_Null(FillBindGroup));

		for (u32 i = 0; i < 10; i++) {
			u32 Quad = i % 4;
			u32 Slot = i % FrameSlotCount;
			indirect_fill_push FillPush = { .Slot = Slot, .SlotStride = INDIRECT_DYNAMIC_FRAME_STRIDE, .Quad = Quad };
			gdi_dispatch FillDispatch = {};
			FillDispatch.Shader = FillShader;
			FillDispatch.BindGroups[0] = FillBindGroup;
			FillDispatch.PushConstantCount = sizeof(indirect_fill_push) / sizeof(u32);
			FillDispatch.ThreadGroupCount = V3i(1, 1, 1);
			Memory_Copy(FillDispatch.PushConstants, &FillPush, sizeof(FillPush));
			GDI_Submit_Compute_Pass({}, { .Ptr = &ComputeDynamicIndirectBuffer, .Count = 1 }, { .Ptr = &FillDispatch, .Count = 1 });

			gdi_render_pass_begin_info BeginInfo = {
				.RenderTargetViews = { RenderTarget.View },
				.ClearColors = { { .ShouldClear = true, .F32 = { 0.15f, 0.15f, 0.15f, 1 } } },
			};
			gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);
			Render_Set_Shader(RenderPass, RenderShader);
			Render_Set_Bind_Group(RenderPass, 0, MatrixBuffer.BindGroup);
			Render_Set_Vtx_Buffer(RenderPass, 0, VertexBuffer);
			Render_Set_Idx_Buffer(RenderPass, IndexBuffer, GDI_IDX_FORMAT_32_BIT);
			Render_Set_Scissor(RenderPass, 0, 0, 64, 64);
			Render_Draw_Idx_Indirect(RenderPass, ComputeDynamicIndirectBuffer, (u64)(Slot * INDIRECT_DYNAMIC_FRAME_STRIDE), 1, (u32)sizeof(gdi_indexed_indirect_draw_info));
			GDI_End_Render_Pass(RenderPass);
			GDI_Submit_Render_Pass(RenderPass);

			dynamic_indirect_frame_readback_ctx* FrameReadbackCtx = Arena_Push_Struct(Scratch.Arena, dynamic_indirect_frame_readback_ctx);
			FrameReadbackCtx->ExpectedColor = QuadColors[Quad];
			gdi_texture_readback FrameReadback = {
				.Texture = RenderTarget.Handle,
				.UserData = FrameReadbackCtx,
				.ReadbackFunc = Dynamic_Indirect_Frame_Texture_Readback,
			};
			gdi_render_params FrameReadbackParams = { .TextureReadbacks = { .Ptr = &FrameReadback, .Count = 1 } };
			GDI_Render(&FrameReadbackParams);
		}
		GDI_Flush();

		GDI_Delete_Bind_Group(FillBindGroup);
		GDI_Delete_Buffer(ComputeDynamicIndirectBuffer);
	}

	/* --- CPU maps the current frame slice and writes gdi_indirect_draw_info --- */
	{
		gdi_buffer_create_info DynamicIndirectBufferInfo = {
			.Size = DynamicIndirectRingBytes,
			.Usage = GDI_BUFFER_USAGE_STORAGE | GDI_BUFFER_USAGE_INDIRECT | GDI_BUFFER_USAGE_DYNAMIC,
			.DebugName = String_Lit("dyn indirect cpu"),
		};
		gdi_buffer CpuDynamicIndirectBuffer = GDI_Create_Buffer(&DynamicIndirectBufferInfo);
		ASSERT_FALSE(GDI_Is_Null(CpuDynamicIndirectBuffer));

		for (u32 i = 0; i < 10; i++) {
			u32 Quad = i % 4;
			u32 Slot = i % FrameSlotCount;
			gdi_indexed_indirect_draw_info Draw = {
				.IdxCount = 6,
				.InstanceCount = 1,
				.IdxOffset = Quad * 6,
				.VtxOffset = Quad * 4,
				.InstanceOffset = 0,
			};
			size_t SlotByteOffset = (size_t)Slot * INDIRECT_DYNAMIC_FRAME_STRIDE;
			u8* MappedSlice = (u8*)GDI_Map_Buffer(CpuDynamicIndirectBuffer, SlotByteOffset, INDIRECT_DYNAMIC_FRAME_STRIDE);
			ASSERT_TRUE(MappedSlice);
			Memory_Copy(MappedSlice, &Draw, sizeof(Draw));
			GDI_Unmap_Buffer(CpuDynamicIndirectBuffer);

			gdi_render_pass_begin_info BeginInfo = {
				.RenderTargetViews = { RenderTarget.View },
				.ClearColors = { { .ShouldClear = true, .F32 = { 0.15f, 0.15f, 0.15f, 1 } } },
			};
			gdi_render_pass* RenderPass = GDI_Begin_Render_Pass(&BeginInfo);
			Render_Set_Shader(RenderPass, RenderShader);
			Render_Set_Bind_Group(RenderPass, 0, MatrixBuffer.BindGroup);
			Render_Set_Vtx_Buffer(RenderPass, 0, VertexBuffer);
			Render_Set_Idx_Buffer(RenderPass, IndexBuffer, GDI_IDX_FORMAT_32_BIT);
			Render_Set_Scissor(RenderPass, 0, 0, 64, 64);
			Render_Draw_Idx_Indirect(RenderPass, CpuDynamicIndirectBuffer, (u64)SlotByteOffset, 1, (u32)sizeof(gdi_indexed_indirect_draw_info));
			GDI_End_Render_Pass(RenderPass);
			GDI_Submit_Render_Pass(RenderPass);

			dynamic_indirect_frame_readback_ctx* FrameReadbackCtx = Arena_Push_Struct(Scratch.Arena, dynamic_indirect_frame_readback_ctx);
			FrameReadbackCtx->ExpectedColor = QuadColors[Quad];
			gdi_texture_readback FrameReadback = {
				.Texture = RenderTarget.Handle,
				.UserData = FrameReadbackCtx,
				.ReadbackFunc = Dynamic_Indirect_Frame_Texture_Readback,
			};
			gdi_render_params FrameReadbackParams = { .TextureReadbacks = { .Ptr = &FrameReadback, .Count = 1 } };
			GDI_Render(&FrameReadbackParams);
		}
		GDI_Flush();

		GDI_Delete_Buffer(CpuDynamicIndirectBuffer);
	}

	GDI_Delete_Shader(RenderShader);
	GDI_Delete_Shader(FillShader);
	GPU_Buffer_Delete(&MatrixBuffer);
	Texture_Delete(&RenderTarget);
	GDI_Delete_Buffer(IndexBuffer);
	GDI_Delete_Buffer(VertexBuffer);
	GDI_Delete_Bind_Group_Layout(FillLayout);
}

/*
 * Uploads a non-dynamic constant buffer via Map/Unmap (staging copy on the transfer queue),
 * draws a fullscreen triangle with the graphics pipeline reading that CB, then readbacks the
 * render target to verify the GPU saw the uploaded constants.
 */
UTEST(gdi, ConstantBufferUpload_GraphicsPipeline_RenderTargetReadback) {
	gdi_tests* Tests = GDI_Get_Tests();
	ASSERT_TRUE(Tests);
	scratch Scratch;

	struct ps_cb {
		v4 Color;
	};

	v4 ExpectedColor = V4(0.125f, 0.375f, 0.625f, 1.0f);

	gdi_buffer_create_info CbInfo = {
		.Size = sizeof(ps_cb),
		.Usage = GDI_BUFFER_USAGE_CONSTANT,
		.DebugName = String_Lit("CB upload graphics readback test"),
	};
	gdi_buffer Cb = GDI_Create_Buffer(&CbInfo);
	ASSERT_FALSE(GDI_Is_Null(Cb));

	void* Mapped = GDI_Map_Buffer(Cb, 0, sizeof(ps_cb));
	ASSERT_TRUE(Mapped);
	*(ps_cb*)Mapped = { ExpectedColor };
	GDI_Unmap_Buffer(Cb);

	gdi_bind_group_buffer BgBuf = { .Buffer = Cb };
	gdi_bind_group_write BgWrite = { .Buffers = { .Ptr = &BgBuf, .Count = 1 } };
	gdi_bind_group_create_info BgCreate = {
		.Layout = Tests->BufferLayout,
		.Writes = { .Ptr = &BgWrite, .Count = 1 },
	};
	gdi_bind_group BindGroup = GDI_Create_Bind_Group(&BgCreate);
	ASSERT_FALSE(GDI_Is_Null(BindGroup));

	gdi_shader Shader;
	{
		const char* Hlsl = R"(
		cbuffer PsCb : register(b0, space0) {
			float4 Color;
		};
		float4 VS_Main(uint VertexID : SV_VertexID) : SV_POSITION {
			float2 Texcoord = float2((VertexID << 1) & 2, VertexID & 2);
			return float4(Texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
		}
		float4 PS_Main(float4 Pos : SV_POSITION) : SV_TARGET0 {
			return Color;
		}
		)";
		IDxcBlob* VS = Compile_Shader(String_Null_Term(Hlsl), String_Lit("vs_6_0"), String_Lit("VS_Main"), { String_Lit("-fvk-invert-y") });
		IDxcBlob* PS = Compile_Shader(String_Null_Term(Hlsl), String_Lit("ps_6_0"), String_Lit("PS_Main"), {});
		ASSERT_TRUE(VS && PS);
		gdi_shader_create_info ShaderCreateInfo = {
			.VS = { .Ptr = (u8*)VS->GetBufferPointer(), .Size = VS->GetBufferSize() },
			.PS = { .Ptr = (u8*)PS->GetBufferPointer(), .Size = PS->GetBufferSize() },
			.BindGroupLayouts = { .Ptr = &Tests->BufferLayout, .Count = 1 },
			.RenderTargetFormats = { GDI_FORMAT_R8G8B8A8_UNORM },
			.DebugName = String_Lit("Constant buffer readback pass"),
		};
		Shader = GDI_Create_Shader(&ShaderCreateInfo);
		ASSERT_FALSE(GDI_Is_Null(Shader));
		VS->Release();
		PS->Release();
	}

	texture RenderTarget;
	ASSERT_TRUE(Texture_Create(&RenderTarget, {
		.Format = GDI_FORMAT_R8G8B8A8_UNORM,
		.Dim = V2i(16, 16),
		.Usage = GDI_TEXTURE_USAGE_RENDER_TARGET | GDI_TEXTURE_USAGE_READBACK,
	}));

	gdi_render_pass_begin_info BeginInfo = {
		.RenderTargetViews = { RenderTarget.View },
		.ClearColors = { { .ShouldClear = true, .F32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
	};
	gdi_render_pass* RP = GDI_Begin_Render_Pass(&BeginInfo);
	Render_Set_Shader(RP, Shader);
	Render_Set_Bind_Group(RP, 0, BindGroup);
	Render_Draw(RP, 3, 0);
	GDI_End_Render_Pass(RP);
	GDI_Submit_Render_Pass(RP);

	gdi_texture_info TexInfo = GDI_Get_Texture_Info(RenderTarget.Handle);
	simple_test_context TestContext = {
		.Texels = (u8*)Arena_Push(Scratch.Arena, TexInfo.Dim.x * TexInfo.Dim.y * GDI_Get_Format_Size(TexInfo.Format)),
	};
	gdi_texture_readback TextureReadback = {
		.Texture = RenderTarget.Handle,
		.UserData = &TestContext,
		.ReadbackFunc = Simple_Texture_Readback,
	};
	gdi_render_params RenderParams = { .TextureReadbacks = { .Ptr = &TextureReadback, .Count = 1 } };
	GDI_Render(&RenderParams);
	GDI_Flush();

	f32 Eps = 2.0f / 255.0f;
	u32* Pixels = (u32*)TestContext.Texels;
	for (s32 y = 0; y < TestContext.Dim.y; y++) {
		for (s32 x = 0; x < TestContext.Dim.x; x++) {
			v4 Got = V4_Color_From_U32(Pixels[x + y * TestContext.Dim.x]);
			ASSERT_NEAR(Got.x, ExpectedColor.x, Eps);
			ASSERT_NEAR(Got.y, ExpectedColor.y, Eps);
			ASSERT_NEAR(Got.z, ExpectedColor.z, Eps);
			ASSERT_NEAR(Got.w, ExpectedColor.w, Eps);
		}
	}

	GDI_Delete_Bind_Group(BindGroup);
	GDI_Delete_Buffer(Cb);
	GDI_Delete_Shader(Shader);
	Texture_Delete(&RenderTarget);
}
