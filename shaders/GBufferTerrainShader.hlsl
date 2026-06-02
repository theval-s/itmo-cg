// Geometry-pass shader for TerrainComponent.
// Vertex format: POSITION (float4) + COLOR (float4, xy = UV).
// Normal is reconstructed from screen-space derivatives of world position.

cbuffer GBufferCB : register(b0) {
    matrix worldViewProj;
    matrix world;
    float4 matDiffuse;
    float4 matSpecular;
};

Texture2D  diffuseTex : register(t0);
SamplerState samp     : register(s0);

struct VS_IN {
    float4 pos : POSITION0;
    float4 uv  : COLOR0;
};

struct PS_IN {
    float4 pos      : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 uv       : TEXCOORD1;
};

struct GBuf {
    float4 diffSpec : SV_Target0;
    float4 normal   : SV_Target1;
};

PS_IN VSMain(VS_IN input) {
    PS_IN o;
    o.pos      = mul(float4(input.pos.xyz, 1.f), worldViewProj);
    o.worldPos = mul(float4(input.pos.xyz, 1.f), world).xyz;
    o.uv       = input.uv.xy;
    return o;
}

[earlydepthstencil]
GBuf PSMain(PS_IN input) {
    GBuf o;
    float3 texCol = diffuseTex.Sample(samp, input.uv).rgb;
    o.diffSpec = float4(texCol * matDiffuse.rgb, matSpecular.w / 128.0);

    // Reconstruct normal from geometry derivatives.
    float3 N = normalize(cross(ddy(input.worldPos), ddx(input.worldPos)));
    o.normal = float4(N, 0.f);
    return o;
}
