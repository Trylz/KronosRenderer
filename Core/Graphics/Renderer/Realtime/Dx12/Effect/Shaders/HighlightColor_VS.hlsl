//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
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
};

cbuffer VertexShaderCB : register(b0)
{
	float4x4 wvpMat;
	float3 eyePosition;
	float scale;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float3 eyeVector = input.position.xyz - eyePosition;
	float4 scaleDir = float4(input.normal.x, input.normal.y, input.normal.z, 0.0f);
	scaleDir *= length(eyeVector) * scale;

	output.position = mul(input.position + scaleDir, wvpMat);

	return output;
}