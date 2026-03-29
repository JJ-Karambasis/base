struct vs_input {
    float2 P : POSITION0;
    float2 UV : TEXCOORD0;
    float4 C : COLOR0;
};

struct vs_output {
    float4 P : SV_POSITION;
    float4 C : COLOR0;
    float2 UV : TEXCOORD0;
};

struct shader_global {
    float2 Scale;
    float2 Translate;
};

Texture2D Texture : register(t0, space0);
SamplerState Sampler : register(s1, space0);

[[vk::push_constant]]
ConstantBuffer<shader_global> Globals : register(b0, space1);

#ifdef IMGUI_USE_SRGB
float3 Linear_From_SRGB(float3 DisplayRgb) {
    DisplayRgb = saturate(DisplayRgb);
    float3 Low = DisplayRgb / 12.92f;
    float3 High = pow((DisplayRgb + 0.055f) / 1.055f, 2.4f);
    return float3(
        DisplayRgb.r <= 0.04045f ? Low.r : High.r,
        DisplayRgb.g <= 0.04045f ? Low.g : High.g,
        DisplayRgb.b <= 0.04045f ? Low.b : High.b);
}
#endif

vs_output VS_Main(vs_input Vtx) {
    vs_output Result = (vs_output)0;
    Result.P = float4(Vtx.P * Globals.Scale + Globals.Translate, 0.0f, 1.0f);
    Result.UV = Vtx.UV;
    Result.C = Vtx.C;
#ifdef IMGUI_USE_SRGB
    Result.C.rgb = Linear_From_SRGB(Result.C.rgb);
#endif
    return Result;
}

float4 PS_Main(vs_output Pxl) : SV_TARGET0 {
    float4 Result = Pxl.C*Texture.Sample(Sampler, Pxl.UV);
    return Result;
}
