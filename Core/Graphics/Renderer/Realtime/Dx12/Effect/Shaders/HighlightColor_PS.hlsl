//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
};

cbuffer PixelShaderCB : register(b0)
{
	float4 color;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	return color;
}