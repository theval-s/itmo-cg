// b0: terrain transform matrices
cbuffer TerrainCB : register(b0)
{
    matrix worldViewProj;
    matrix world;
};

// b1: shadow parameters (matches ShadowCBData)
cbuffer ShadowBuffer : register(b1)
{
    matrix lightViewProj0;
    matrix lightViewProj1;
    matrix lightViewProj2;
    float4 cascadeSplits;
    int    shadowsEnabled;
    float3 _shadowPad;
};

// t0-t2: shadow maps bound by ShadowMapComponent::BindForDraw
Texture2D shadowMap0 : register(t0);
Texture2D shadowMap1 : register(t1);
Texture2D shadowMap2 : register(t2);

// t3: terrain diffuse
Texture2D diffuse : register(t3);

// s0: shadow comparison sampler bound by ShadowMapComponent::BindForDraw
SamplerComparisonState shadowSampler : register(s0);

// s1: regular linear sampler for diffuse
SamplerState samp : register(s1);

struct VS_IN
{
    float4 pos : POSITION0;
    float4 uv  : COLOR0;
};

struct PS_IN
{
    float4 pos      : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 uv       : TEXCOORD1;
    float  viewDepth: TEXCOORD2;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN o;
    o.pos      = mul(float4(input.pos.xyz, 1.0f), worldViewProj);
    o.worldPos = mul(float4(input.pos.xyz, 1.0f), world).xyz;
    o.uv       = input.uv.xy;
    o.viewDepth = o.pos.w;
    return o;
}

float GetShadowFactor(float3 worldPos, float viewDepth)
{
    [branch]
    if (!shadowsEnabled) return 1.0;

    static const float bias = 0.002;
    float4 ls;
    float2 uv;
    float  d;

    if (viewDepth < cascadeSplits.x) {
        ls = mul(float4(worldPos, 1), lightViewProj0);
        uv = ls.xy / ls.w * float2(0.5, -0.5) + 0.5;
        d  = saturate(ls.z / ls.w - bias);
        if (uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1)
            return shadowMap0.SampleCmpLevelZero(shadowSampler, uv, d);
        return 1.0;
    }
    if (viewDepth < cascadeSplits.y) {
        ls = mul(float4(worldPos, 1), lightViewProj1);
        uv = ls.xy / ls.w * float2(0.5, -0.5) + 0.5;
        d  = saturate(ls.z / ls.w - bias);
        if (uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1)
            return shadowMap1.SampleCmpLevelZero(shadowSampler, uv, d);
        return 1.0;
    }
    ls = mul(float4(worldPos, 1), lightViewProj2);
    uv = ls.xy / ls.w * float2(0.5, -0.5) + 0.5;
    d  = saturate(ls.z / ls.w - bias);
    if (uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1)
        return shadowMap2.SampleCmpLevelZero(shadowSampler, uv, d);
    return 1.0;
}

float4 PSMain(PS_IN input) : SV_Target
{
    float3 color = diffuse.Sample(samp, input.uv).rgb;
    float shadow = GetShadowFactor(input.worldPos, input.viewDepth);
    // 0.3 ambient keeps shadowed areas from going pitch black
    color *= (0.3 + 0.7 * shadow);
    return float4(color, 1.0f);
}
