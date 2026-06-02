// Geometry-pass shader for textured LitMeshComponent objects (Katamari ball).
// Uses the same POSITION+NORMAL vertex format as GBufferShader.
// UV is derived from local-space object position (spherical mapping matching
// the existing MegaSimpleTextureShader).

cbuffer GBufferCB : register(b0) {
    matrix worldViewProj;
    matrix world;
    float4 matDiffuse;
    float4 matSpecular;
};

Texture2D  diffuseTex : register(t0);
SamplerState samp     : register(s0);

struct VS_IN {
    float4 pos    : POSITION0;
    float4 normal : NORMAL0;
};

struct PS_IN {
    float4 pos      : SV_POSITION;
    float3 worldNorm: TEXCOORD0;
    float3 objPos   : TEXCOORD1;   // local-space position for spherical UV
};

struct GBuf {
    float4 diffSpec : SV_Target0;
    float4 normal   : SV_Target1;
};

PS_IN VSMain(VS_IN input) {
    PS_IN o;
    o.pos       = mul(input.pos, worldViewProj);
    o.worldNorm = mul(float4(input.normal.xyz, 0.f), world).xyz;
    o.objPos    = input.pos.xyz;
    return o;
}

[earlydepthstencil]
GBuf PSMain(PS_IN input) {
    GBuf o;
    float3 n  = normalize(input.worldNorm);

    // Spherical UV from local-space normal (matches MegaSimpleTextureShader.hlsl).
    float3 ln = normalize(input.objPos);
    float2 uv = float2(ln.x * 0.5 + 0.5, ln.y * -0.5 + 0.5);
    float3 texCol = diffuseTex.Sample(samp, uv).rgb;

    o.diffSpec = float4(texCol * matDiffuse.rgb, matSpecular.w / 128.0);
    o.normal   = float4(n, 0.f);
    return o;
}
