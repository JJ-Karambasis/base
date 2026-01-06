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
    float2 InvResolution;
};

Texture2D Texture : register(t0, space0);
SamplerState Sampler : register(s1, space0);

[[vk::push_constant]]
ConstantBuffer<shader_global> Globals : register(b0, space1);

vs_output VS_Main(vs_input Vtx) {
    vs_output Result = (vs_output)0;

	Result.P = float4((2.0f*Vtx.P.x*Globals.InvResolution.x)-1.0f,
                      (2.0f*Vtx.P.y*Globals.InvResolution.y)-1.0f, 
                      0.0f, 1.0f);    
    
    Result.UV = Vtx.UV;
    Result.C = Vtx.C;

    return Result;
}

float4 PS_Main(vs_output Pxl) : SV_TARGET0 {
    float4 Result = Pxl.C*Texture.Sample(Sampler, Pxl.UV);
    return Result;
}