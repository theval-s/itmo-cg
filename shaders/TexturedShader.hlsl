cbuffer ConstBuffer : register(b0)
{
	matrix m;
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct VS_IN
{
	float4 pos : POSITION0;
	float2 uv  : TEXCOORD0;
};

struct PS_IN
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD;
};

PS_IN VSMain(VS_IN input)
{
	PS_IN output = (PS_IN)0;
	output.pos = mul(float4(input.pos.xyz, 1.0f), m);
	output.uv  = input.uv;
	return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
	return tex.Sample(samp, input.uv);
}
