cbuffer constants : register(b0)
{
	float3 Offset;
	float Scale;
	float Color;
}

struct V_INPUT
{
	float4 position : POSITION;
	float4 color : COLOR;
};

struct P_INPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

P_INPUT mainVS(V_INPUT input)
{
	P_INPUT output;

	float3 scaledVertex = input.position * Scale;
	output.position = float4(Offset + scaleVertex, 1.0f);

	out.color = float4(color, 1.0f);

	return output;
}

float4 mainPS(P_INPUT input) : SV_TARGET
{
	return input.color;
}