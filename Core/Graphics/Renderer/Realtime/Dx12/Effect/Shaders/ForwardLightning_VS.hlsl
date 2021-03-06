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
	float3 tangent : Tangent;
	float3 bitangent : Bitangent;
	float3 normal : NORMAL;
	float2 texCoord: TEXCOORD;
};

cbuffer VertexShaderSharedCB : register(b0)
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
	output.texCoord = input.texCoord;
	output.normal = normalize(mul(float4(input.normal, 0.0f), modelMat));
	output.tangent = normalize(mul(float4(input.tangent, 0.0f), modelMat));
	output.bitangent = normalize(mul(float4(input.bitangent, 0.0f), modelMat));

	return output;
}