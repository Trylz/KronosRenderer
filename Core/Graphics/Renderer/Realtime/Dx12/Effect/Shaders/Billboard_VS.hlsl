//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

struct VS_INPUT
{
	float4 position : POSITION;
	float2 texCoord: TEXCOORD;
};

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float2 texCoord: TEXCOORD;
};

cbuffer VertexShaderSharedCB : register(b0)
{
	float4x4 projMatrix;
	float billboardScale;
};

cbuffer VertexShaderCenterCB: register(b1)
{
	float4 centerCameraSpace;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.texCoord = input.texCoord;

	float4 vCameraSpace = centerCameraSpace;
	vCameraSpace.x += input.position.x * billboardScale;
	vCameraSpace.y += input.position.y * billboardScale;
	
	output.position = mul(vCameraSpace, projMatrix);

	return output;
}