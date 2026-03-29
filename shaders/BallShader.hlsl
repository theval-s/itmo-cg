cbuffer ConstBuffer : register(b0)
{
	float4 offset;
};

struct VS_IN
{
	float4 pos : POSITION0;
	float4 col : COLOR0;
};

struct PS_IN
{
	float4 pos : SV_POSITION;
 	float4 col : COLOR;
	float2 uv  : TEXCOORD0;
};

PS_IN VSMain( VS_IN input )
{
	PS_IN output = (PS_IN)0;

	output.pos = input.pos + offset;
	output.col = input.col;
	output.uv = input.pos.xy;
	
	return output;
}

float4 PSMain( PS_IN input ) : SV_Target
{
	float4 col = float4(0.231f, 0.953f, 0.988f, 1.0f);
	float2 uv = input.uv / float2(0.05f, 0.05f);

	float dist = length(uv);
	if (dist > 1.0f) discard;
	return col;
}