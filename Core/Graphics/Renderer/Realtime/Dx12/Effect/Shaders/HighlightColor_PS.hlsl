//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	return float4(0.0f, 0.74f, 1.0f, 0.0f);
}