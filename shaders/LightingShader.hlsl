// Lighting pass shader for deferred rendering.
// Draws one fullscreen quad per light with additive blending.
// Reads the G-buffer (t0=DiffuseSpec, t1=Normal, t2=Depth) and reconstructs
// world position from depth to compute Phong shading.

#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1
#define LIGHT_SPOT        2
#define LIGHT_AMBIENT     3

cbuffer LightCB : register(b0) {
    matrix invViewProj;       // (view*proj).Invert(), transposed for row-vector mul
    matrix viewMatrix;        // for cascade-split view-space Z
    float4 cameraPos;
    float4 screenSize;        // xy = viewport width, height
    float4 ambient;           // global ambient colour
    float4 lightDirOrPos;     // direction (dir) or world-space position (point/spot)
    float4 lightColor;
    int    lightType;
    float  attenConst;
    float  attenLinear;
    float  attenQuad;
    float4 spotDirection;     // xyz = cone axis, w = cos(inner half-angle)
    float  spotOuterCos;
    float3 _lpad;
    // CSM shadow data (directional only)
    matrix lightViewProj0;
    matrix lightViewProj1;
    matrix lightViewProj2;
    float4 cascadeSplits;     // x,y = view-Z end planes of cascades 0 and 1
    int    shadowsEnabled;
    float3 _spad;
};

Texture2D DiffuseTex : register(t0);
Texture2D NormalTex  : register(t1);
Texture2D DepthTex   : register(t2);

Texture2D shadowMap0 : register(t3);
Texture2D shadowMap1 : register(t4);
Texture2D shadowMap2 : register(t5);
SamplerComparisonState shadowSampler : register(s0);

// Fullscreen quad from SV_VertexID (teacher's slide approach).
struct PS_IN {
    float4 position : SV_POSITION;
    float2 tex      : TEXCOORD;
};

PS_IN VSMain(uint id : SV_VertexID) {
    PS_IN o;
    o.tex      = float2(id & 1, (id & 2) >> 1);
    o.position = float4(o.tex * float2(2, -2) + float2(-1, 1), 0, 1);
    return o;
}

// Shadow factor matching the cascade selection logic in PhongShader.hlsl.
float GetShadowFactor(float3 worldPos, float viewZ) {
    [branch]
    if (!shadowsEnabled) return 1.0;

    static const float bias = 0.002;
    float4 ls; float2 uv; float d;

    if (viewZ < cascadeSplits.x) {
        ls = mul(float4(worldPos, 1), lightViewProj0);
        uv = ls.xy / ls.w * float2(0.5, -0.5) + 0.5;
        d  = saturate(ls.z / ls.w - bias);
        if (all(uv >= 0) && all(uv <= 1))
            return shadowMap0.SampleCmpLevelZero(shadowSampler, uv, d);
        return 1.0;
    }
    if (viewZ < cascadeSplits.y) {
        ls = mul(float4(worldPos, 1), lightViewProj1);
        uv = ls.xy / ls.w * float2(0.5, -0.5) + 0.5;
        d  = saturate(ls.z / ls.w - bias);
        if (all(uv >= 0) && all(uv <= 1))
            return shadowMap1.SampleCmpLevelZero(shadowSampler, uv, d);
        return 1.0;
    }
    ls = mul(float4(worldPos, 1), lightViewProj2);
    uv = ls.xy / ls.w * float2(0.5, -0.5) + 0.5;
    d  = saturate(ls.z / ls.w - bias);
    if (all(uv >= 0) && all(uv <= 1))
        return shadowMap2.SampleCmpLevelZero(shadowSampler, uv, d);
    return 1.0;
}

float4 PSMain(PS_IN input) : SV_Target {
    int2 px = int2(input.position.xy);

    // Discard background pixels (no geometry written).
    float depth = DepthTex.Load(int3(px, 0)).r;
    if (depth >= 1.0) discard;

    // Reconstruct world position from depth + NDC (row-vector convention).
    float4 ndc     = float4(input.tex * float2(2, -2) + float2(-1, 1), depth, 1);
    float4 worldH  = mul(ndc, invViewProj);
    float3 worldPos = worldH.xyz / worldH.w;

    // Read G-buffer.
    float4 diffSpec = DiffuseTex.Load(int3(px, 0));
    float3 diffuse  = diffSpec.rgb;
    float  shininess = max(diffSpec.a * 128.0, 1.0);
    float3 N = normalize(NormalTex.Load(int3(px, 0)).xyz);

    float3 V = normalize(cameraPos.xyz - worldPos);
    float3 result = 0;

    if (lightType == LIGHT_AMBIENT) {
        result = ambient.rgb * diffuse;

    } else if (lightType == LIGHT_DIRECTIONAL) {
        float3 L = normalize(lightDirOrPos.xyz);
        float3 R = reflect(-L, N);
        float  diff = max(dot(N, L), 0.0);
        float  spec = pow(max(dot(V, R), 0.0), shininess);
        // View-space Z for cascade selection.
        float viewZ = mul(float4(worldPos, 1), viewMatrix).z;
        float shadow = GetShadowFactor(worldPos, viewZ);
        result = (diff * diffuse + spec * 0.4) * lightColor.rgb * shadow;

    } else if (lightType == LIGHT_POINT) {
        float3 delta = lightDirOrPos.xyz - worldPos;
        float  dist  = length(delta);
        float3 L     = delta / dist;
        float  atten = 1.0 / (attenConst + attenLinear * dist + attenQuad * dist * dist);
        float3 R     = reflect(-L, N);
        float  diff  = max(dot(N, L), 0.0);
        float  spec  = pow(max(dot(V, R), 0.0), shininess);
        result = (diff * diffuse + spec * 0.4) * lightColor.rgb * atten;

    } else if (lightType == LIGHT_SPOT) {
        float3 delta = lightDirOrPos.xyz - worldPos;
        float  dist  = length(delta);
        float3 L     = delta / dist;
        float  atten = 1.0 / (attenConst + attenLinear * dist + attenQuad * dist * dist);
        float3 R     = reflect(-L, N);
        float  diff  = max(dot(N, L), 0.0);
        float  spec  = pow(max(dot(V, R), 0.0), shininess);
        float  cosA  = dot(-L, normalize(spotDirection.xyz));
        float  spotF = smoothstep(spotOuterCos, spotDirection.w, cosA);  // w = innerCos
        result = (diff * diffuse + spec * 0.4) * lightColor.rgb * atten * spotF;
    }

    return float4(result, 0.0);
}
