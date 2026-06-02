// Geometry-pass shader for material-coloured LitMeshComponent objects.
// Writes diffuse+shininess to RT0 and world-space normal to RT1.

cbuffer GBufferCB : register(b0) {
    matrix worldViewProj;
    matrix world;
    float4 matDiffuse;
    float4 matSpecular; // w = shininess
};

struct VS_IN {
    float4 pos    : POSITION0;
    float4 normal : NORMAL0;
};

struct PS_IN {
    float4 pos      : SV_POSITION;
    float3 worldNorm: TEXCOORD0;
};

struct GBuf {
    float4 diffSpec : SV_Target0;  // RT0: diffuse rgb + packed shininess
    float4 normal   : SV_Target1;  // RT1: world-space normal xyz (FP16, stored signed)
};

PS_IN VSMain(VS_IN input) {
    PS_IN o;
    o.pos       = mul(input.pos, worldViewProj);
    o.worldNorm = mul(float4(input.normal.xyz, 0.f), world).xyz;
    return o;
}

[earlydepthstencil]
GBuf PSMain(PS_IN input) {
    GBuf o;
    float3 n = normalize(input.worldNorm);
    o.diffSpec = float4(matDiffuse.rgb, matSpecular.w / 128.0);
    o.normal   = float4(n, 0.f);
    return o;
}
