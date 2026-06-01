#define MAX_LIGHTS 8
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1

struct LightData {
    float4 dirOrPos;   // directional: toward-light; point: world position
    float4 color;      // rgb, w unused
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
    float4    matSpecular; // w = shininess (Ns)
};

struct VS_IN {
    float4 pos    : POSITION0;
    float4 normal : NORMAL0;
};

struct PS_IN {
    float4 pos      : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal   : TEXCOORD1;
};

PS_IN VSMain(VS_IN input) {
    PS_IN o;
    o.pos      = mul(input.pos, worldViewProj);
    o.worldPos = mul(input.pos, world).xyz;
    o.normal   = mul(float4(input.normal.xyz, 0.0f), world).xyz;
    return o;
}

float4 PSMain(PS_IN input) : SV_Target {
    float3 N = normalize(input.normal);
    float3 V = normalize(cameraPos.xyz - input.worldPos);

    float3 result = matAmbient.xyz; // global ambient, added once

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
                          + lights[i].attenQuad  * dist * dist);
        }

        float3 R = reflect(-L, N);
        float3 diffuse  = max(dot(N, L), 0.0f) * matDiffuse.xyz  * lights[i].color.xyz;
        float3 specular = pow(max(dot(V, R), 0.0f), max(matSpecular.w, 1.0f))
                          * matSpecular.xyz * lights[i].color.xyz;
        result += (diffuse + specular) * atten;
    }

    return float4(result, 1.0f);
}
