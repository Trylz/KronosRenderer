//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

struct VS_INPUT
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
	float2 texCoord: TEXCOORD;
};

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float4 worldPosition : POSITION;
	uint groupId: BLENDINDICES;
};

cbuffer CameraVertexShaderCB : register(b0)
{
	float4x4 vpMat;
};

cbuffer VertexShaderGroupCB : register(b1)
{
	float4x4 modelMat;
	uint groupId;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.worldPosition = mul(input.position, modelMat);
	output.position = mul(output.worldPosition, vpMat);
	output.groupId = groupId;

	return output;
}