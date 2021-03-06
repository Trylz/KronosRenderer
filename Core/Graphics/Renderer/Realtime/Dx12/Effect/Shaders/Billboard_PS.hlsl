//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================


struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float2 texCoord: TEXCOORD;
};

Texture2D billboardTex : register(t0);
SamplerState tSampler : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
	return billboardTex.Sample(tSampler, input.texCoord);
}