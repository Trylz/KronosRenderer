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
	uint passIdx;
};

cbuffer VertexShaderGroupCB : register(b2)
{
	float4x4 modelMat;
	uint groupId;
};

VS_OUTPUT main(VS_INPUT input)
{
	float4 worldPosition;
	switch (passIdx)
	{
		case UNIFORM_SCALE_PASS_IDX:
		{
			const float4x4 scaleMatrix = float4x4(
				scale, 0.0f, 0.0f, 0.0f,
				0.0f, scale, 0.0f, 0.0f,
				0.0f, 0.0f, scale, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);

			worldPosition = mul(input.position, mul(scaleMatrix, modelMat));
		}
		break;

		case ORIENTED_SCALE_PASS_IDX:
		{
			worldPosition = mul(input.position, modelMat);
			const float3 eyeVector = worldPosition.xyz - eyePosition;

			float4 scaleDir = float4(input.normal.x, input.normal.y, input.normal.z, 0.0f);
			scaleDir *= length(eyeVector) * scale;

			worldPosition += scaleDir;
		}
		break;

		default:
		{
			worldPosition = mul(input.position, modelMat);
		}
		break;
	}

	VS_OUTPUT output;
	output.position = mul(worldPosition, vpMat);

	return output;
}