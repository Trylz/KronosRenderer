//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

struct VS_INPUT
{
	float4 position : POSITION;
};

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float3 texCoord: TEXCOORD;
};

cbuffer VertexShaderCB : register(b0)
{
	float4x4 wvpMat;
	float3 cubeMapCenter;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = mul(input.position, wvpMat);
	output.texCoord = normalize(input.position - cubeMapCenter);

	// Because of inverted texture coordinate system.
	output.texCoord.y *= -1;

	return output;
}