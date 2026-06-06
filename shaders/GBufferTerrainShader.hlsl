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

    // Smooth per-vertex normal: horizontal lanes (x,z) are stored in COLOR.zw;
    // reconstruct the +Y/up lane (heightmap terrain never overhangs, so ny >= 0).
    float nx = input.normal_xy.x;
    float nz = input.normal_xy.y;
    float ny = sqrt(max(0.0, 1.0 - nx*nx - nz*nz));
    float3 N = normalize(float3(nx, ny, nz));
    o.normal = float4(N, 0.f);
    return o;
}
