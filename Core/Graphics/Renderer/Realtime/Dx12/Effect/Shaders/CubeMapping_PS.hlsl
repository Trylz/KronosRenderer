//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float3 texCoord: TEXCOORD;
};

TextureCube cubeMapTex : register(t0);
SamplerState tSampler : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
	return cubeMapTex.Sample(tSampler, input.texCoord);
}