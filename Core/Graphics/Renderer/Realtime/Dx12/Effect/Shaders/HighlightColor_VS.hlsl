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
};


cbuffer CameraVertexShaderCB : register(b0)
{
	float4x4 vpMat;
};

cbuffer VertexShaderSharedCB : register(b1)
{
	float3 eyePosition;
	float scale;
};

cbuffer VertexShaderGroupCB : register(b2)
{
	float4x4 modelMat;
	uint groupId;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float4 worldPosition = mul(input.position, modelMat);
	float3 eyeVector = worldPosition.xyz - eyePosition;
	float4 scaleDir = float4(input.normal.x, input.normal.y, input.normal.z, 0.0f);
	scaleDir *= length(eyeVector) * scale;

	output.position = mul(worldPosition + scaleDir, vpMat);

	return output;
}