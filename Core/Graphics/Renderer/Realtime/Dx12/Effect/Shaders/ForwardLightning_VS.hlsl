//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

struct VS_INPUT
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float2 texCoord: TEXCOORD;
};

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float4 worldPosition : POSITION;
	float3 normal : NORMAL;
	float2 texCoord: TEXCOORD;
};

cbuffer VertexShaderCB : register(b0)
{
	float4x4 wvpMat;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = mul(input.position, wvpMat);
	output.worldPosition = input.position;
	output.texCoord = input.texCoord;
	output.normal = input.normal;

	return output;
}