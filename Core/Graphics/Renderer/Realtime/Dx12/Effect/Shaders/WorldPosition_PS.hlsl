//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float4 worldPosition : POSITION;
	uint groupId: BLENDINDICES;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 outpout = input.worldPosition;
	outpout.a = input.groupId + 1.0f;

	return outpout;
}