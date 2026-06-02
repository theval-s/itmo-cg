struct VSOut
{
    float4 Position : SV_POSITION;
    float2 UV       : TEXCOORD0;
};

VSOut VSMain(uint vertexID : SV_VertexID)
{
    float2 pos[6] =
    {
        float2(-1,-1),
        float2(-1, 1),
        float2( 1, 1),

        float2(-1,-1),
        float2( 1, 1),
        float2( 1,-1)
    };

    VSOut o;
    o.Position = float4(pos[vertexID], 0, 1);
    o.UV = pos[vertexID] * float2(0.5f, -0.5f) + 0.5f;
    return o;
}

Texture2D DepthTex : register(t0);
SamplerState Samp  : register(s0);

float4 PSMain(VSOut input) : SV_Target
{
    float d = DepthTex.Sample(Samp, input.UV).r;

    // visibility enhancement
    d = pow(d, 20.0f);

    return float4(d, d, d, 1);
}