// GPU-driven particle system — emit + simulate (compute) and billboard draw.
//
// Structure follows the lecture slides: each particle stores pos/prevPos/
// velocity/acceleration/energy/size/sizeDelta/weight/weightDelta/color/colorDelta.
// The alive count lives only on the GPU; the draw is issued with
// DrawIndexedInstancedIndirect and each particle is expanded to a camera-facing
// quad in the vertex shader (VertexId = corner index — no geometry shader, so it
// stays portable to backends without GS support).

// Per-particle record. Laid out so each float3 is followed by a scalar (fills a
// 16-byte row) — keeps the HLSL structured-buffer layout identical to the C++
// struct without surprise padding. 112 bytes.
struct Particle
{
    float3 pos;          float energy;
    float3 prevPos;      float size;
    float3 velocity;     float sizeDelta;
    float3 acceleration; float weight;
    float4 color;
    float4 colorDelta;
    float  weightDelta;  float3 _pad;
};

cbuffer ParticleCB : register(b0)
{
    matrix viewProj;       // world -> clip (transposed for HLSL)
    float4 camRight;       // xyz: camera right (billboard basis)
    float4 camUp;          // xyz: camera up
    float4 origin;         // xyz: emitter position
    float4 force;          // xyz: gravity / wind acting on the system
    float  dt;             // frame delta time
    uint   emitCount;      // particles to spawn this frame
    uint   emitHead;       // ring-buffer write cursor
    uint   maxParticles;   // pool capacity
    float  time;           // running time, used to seed RNG
    float3 _cbpad;
};

// ---- Compute views ----
RWStructuredBuffer<Particle> particlesRW     : register(u0);
AppendStructuredBuffer<uint> aliveListAppend : register(u1);

// ---- Draw views ----
StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<uint>     aliveList  : register(t1);

// --- Hash-based RNG (PCG-ish): cheap per-thread randomness ---
uint Hash(uint x)
{
    x ^= x >> 16; x *= 0x7feb352du;
    x ^= x >> 15; x *= 0x846ca68bu;
    x ^= x >> 16; return x;
}
float Rand(inout uint state)
{
    state = Hash(state);
    return state * (1.0 / 4294967296.0);
}

// =============================== Emit ====================================
[numthreads(64, 1, 1)]
void CSEmit(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= emitCount) return;

    uint slot = (emitHead + id.x) % maxParticles;
    uint seed = Hash(slot * 747796405u + asuint(time) + id.x * 2891336453u);

    Particle p = (Particle)0;

    float  azimuth = Rand(seed) * 6.2831853;
    float  spread  = Rand(seed) * 1.6;             // radial scatter of the fountain
    float  speed   = lerp(5.0, 8.0, Rand(seed));   // upward launch speed
    p.velocity     = float3(cos(azimuth) * spread, speed, sin(azimuth) * spread);

    p.pos          = origin.xyz;
    p.prevPos      = p.pos;
    p.acceleration = force.xyz;
    p.energy       = lerp(1.2, 2.0, Rand(seed));
    p.size         = lerp(0.08, 0.18, Rand(seed));
    p.sizeDelta    = 0.05;                          // particles grow slightly
    p.weight       = 1.0;
    p.weightDelta  = 0.0;

    float3 rgb     = float3(lerp(0.3, 0.6, Rand(seed)), lerp(0.7, 0.9, Rand(seed)), 1.0);
    p.color        = float4(rgb, 1.0);
    p.colorDelta   = float4(0, 0, 0, -1.0 / p.energy);  // fade alpha out over life

    particlesRW[slot] = p;
}

// ============================= Simulate ==================================
[numthreads(64, 1, 1)]
void CSSimulate(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= maxParticles) return;

    Particle p = particlesRW[id.x];
    if (p.energy <= 0.0) return;        // dead slot

    p.energy -= dt;
    if (p.energy <= 0.0) {              // died this frame
        p.energy = 0.0;
        particlesRW[id.x] = p;
        return;
    }

    p.prevPos       = p.pos;
    p.acceleration  = force.xyz * p.weight;
    p.velocity     += p.acceleration * dt;
    p.pos          += p.velocity * dt;
    p.size          = max(p.size + p.sizeDelta * dt, 0.0);
    p.weight       += p.weightDelta * dt;
    p.color         = saturate(p.color + p.colorDelta * dt);

    particlesRW[id.x] = p;
    aliveListAppend.Append(id.x);       // GPU-only alive count -> indirect draw
}

// =============================== Draw ====================================
struct PS_IN
{
    float4 pos   : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float4 color : COLOR0;
};

PS_IN VSMain(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    PS_IN o = (PS_IN)0;

    Particle p = particles[aliveList[instanceId]];

    // Index buffer is {0,1,2, 0,2,3}; the fetched value is the quad corner.
    const float2 corners[4] = {
        float2(-1,  1), float2( 1,  1), float2( 1, -1), float2(-1, -1)
    };
    float2 c = corners[vertexId];
    float  h = p.size * 0.5;

    float3 worldPos = p.pos + (camRight.xyz * c.x + camUp.xyz * c.y) * h;
    o.pos   = mul(float4(worldPos, 1.0), viewProj);
    o.uv    = c * 0.5 + 0.5;
    o.color = p.color;
    return o;
}

float4 PSMain(PS_IN i) : SV_Target
{
    float2 d = i.uv * 2.0 - 1.0;
    float  r = dot(d, d);
    float  falloff = saturate(1.0 - r);
    falloff *= falloff;                 // brighter core, soft edge
    return float4(i.color.rgb, i.color.a * falloff);
}
