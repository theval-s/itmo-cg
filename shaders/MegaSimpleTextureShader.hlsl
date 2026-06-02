#define MAX_LIGHTS 8
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1

struct LightData {
    float4 dirOrPos;
    float4 color;
    int    type;
    float  attenConst;
    float  attenLinear;
    float  attenQuad;
};

cbuffer PhongBuffer : register(b0) {
    matrix    worldViewProj;
    matrix    world;
    float4    cameraPos;
    int       lightCount;
    float3    _pad;
    LightData lights[MAX_LIGHTS];
    float4    matAmbient;
    float4    matDiffuse;
    float4    matSpecular; // w = shininess
};

cbuffer ShadowBuffer : register(b1) {
    matrix lightViewProj0;
    matrix lightViewProj1;
    matrix lightViewProj2;
    float4 cascadeSplits;
    int    shadowsEnabled;
    float3 _shadowPad;
};

Texture2D              shadowMap0   : register(t0);
Texture2D              shadowMap1   : register(t1);
Texture2D              shadowMap2   : register(t2);
Texture2D              diffuseTex   : register(t3);
SamplerComparisonState shadowSampler: register(s0);
SamplerState           diffSampler  : register(s1);

struct VS_IN {
    float4 pos : POSITION0;
    float4 col : COLOR0;
};

struct PS_IN {
    float4 pos      : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 worldNorm: TEXCOORD1;
    float  viewDepth: TEXCOORD2;
    float3 objPos   : TEXCOORD3;
};

PS_IN VSMain(VS_IN input) {
    PS_IN o;
    o.pos       = mul(float4(input.pos.xyz, 1.0f), worldViewProj);
    o.worldPos  = mul(float4(input.pos.xyz, 1.0f), world).xyz;
    o.worldNorm = mul(float4(normalize(input.pos.xyz), 0.0f), world).xyz;
    o.viewDepth = o.pos.w;
    o.objPos    = input.pos.xyz;
    return o;
}

float GetShadowFactor(float3 worldPos, float viewDepth) {
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

float4 PSMain(PS_IN input) : SV_Target {
    float3 N = normalize(input.worldNorm);
    float3 V = normalize(cameraPos.xyz - input.worldPos);

    // Spherical UV from local-space position so texture rolls with the ball
    float3 n  = normalize(input.objPos);
    float2 uv = float2(n.x * 0.5 + 0.5, n.y * -0.5 + 0.5);
    float3 texColor = diffuseTex.Sample(diffSampler, uv).rgb;

    float shadow = GetShadowFactor(input.worldPos, input.viewDepth);

    float3 result = matAmbient.xyz * texColor;

    for (int i = 0; i < lightCount; ++i) {
        float3 L;
        float  atten = 1.0f;

        if (lights[i].type == LIGHT_DIRECTIONAL) {
            L = normalize(lights[i].dirOrPos.xyz);
        } else {
            float3 delta = lights[i].dirOrPos.xyz - input.worldPos;
            float  dist  = length(delta);
            L     = delta / dist;
            atten = 1.0f / (lights[i].attenConst
                          + lights[i].attenLinear * dist
                          + lights[i].attenQuad   * dist * dist);
        }

        float3 R        = reflect(-L, N);
        float3 diffuse  = max(dot(N, L), 0.0f) * matDiffuse.xyz * texColor * lights[i].color.xyz;
        float3 specular = pow(max(dot(V, R), 0.0f), max(matSpecular.w, 1.0f))
                          * matSpecular.xyz * lights[i].color.xyz;
        result += (diffuse + specular) * atten * shadow;
    }

    return float4(result, 1.0f);
}
