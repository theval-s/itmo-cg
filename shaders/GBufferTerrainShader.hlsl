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
    float2 normal_xy : TEXCOORD2;
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
    o.normal_xy = float2(input.uv.z, input.uv.w);
    return o;
}

[earlydepthstencil]
GBuf PSMain(PS_IN input) {
    GBuf o;
    float3 texCol = diffuseTex.Sample(samp, input.uv).rgb;
    o.diffSpec = float4(texCol * matDiffuse.rgb, matSpecular.w / 128.0);

    // Use pre-computed vertex normal (x,y packed, reconstruct z assuming unit length).
    float nx = input.normal_xy.x;
    float ny = input.normal_xy.y;
    float nz = sqrt(max(0.f, 1.f - nx*nx - ny*ny));
    float3 N = normalize(float3(nx, ny, nz));
    o.normal = float4(N, 0.f);
    return o;
}
