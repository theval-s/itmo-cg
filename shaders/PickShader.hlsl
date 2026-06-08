// GPU picking compute shader.
// Reads the G-buffer at the clicked pixel,
// writes {id, worldPos, normal} into a single-element RWStructuredBuffer


cbuffer PickCB : register(b0) {
    matrix invViewProj;
    uint   clickX;        // click pixel, client-space
    uint   clickY;
    float  screenW;
    float  screenH;
};

Texture2D<float> IdTex     : register(t0);  // R32F per-object id
Texture2D        DepthTex  : register(t1);  // scene depth (R32F)
Texture2D        NormalTex : register(t2);  // world-space normal

struct PickResult {
    uint  id;
    float wx, wy, wz;     // world position
    float nx, ny, nz;     // world normal
};
RWStructuredBuffer<PickResult> Result : register(u0);

[numthreads(1, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID) {
    int3 px = int3(clickX, clickY, 0);

    float depth = DepthTex.Load(px).r;
    uint  id    = (uint)(IdTex.Load(px) + 0.5);
    float3 N    = NormalTex.Load(px).xyz;

    float2 uv     = (float2(px.xy) + 0.5) / float2(screenW, screenH);
    float4 ndc    = float4(uv * float2(2, -2) + float2(-1, 1), depth, 1);
    float4 worldH = mul(ndc, invViewProj);
    float3 wp     = worldH.xyz / worldH.w;

    PickResult r;
    r.id = (depth >= 1.0) ? 0u : id;
    r.wx = wp.x; r.wy = wp.y; r.wz = wp.z;
    r.nx = N.x;  r.ny = N.y;  r.nz = N.z;
    Result[0] = r;
}
