cbuffer ConstBuffer : register(b0)
{
	matrix m;
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct VS_IN
{
	float4 pos : POSITION0;
	float4 col  : COLOR0;
};

struct PS_IN
{
	float4 pos : SV_POSITION;
	float3 objPos  : TEXCOORD0;
};

PS_IN VSMain(VS_IN input)
{
	PS_IN output = (PS_IN)0;
	output.pos = mul(float4(input.pos.xyz, 1.0f), m);
	output.objPos  = input.pos;
	return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
	float3 n  = normalize(input.objPos);
	float2 uv = float2(n.x * 0.5 + 0.5, n.y * -0.5 + 0.5);
	return tex.Sample(samp, uv);
}
