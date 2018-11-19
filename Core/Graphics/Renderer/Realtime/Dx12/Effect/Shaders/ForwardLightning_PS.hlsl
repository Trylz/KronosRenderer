//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#define InvPI 0.31830988618f
#define InvPI8 0.03978873577f

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float4 worldPosition : POSITION;
	float3 tangent : Tangent;
	float3 bitangent : Bitangent;
	float3 normal : NORMAL;
	float2 texCoord: TEXCOORD;
};

Texture2D diffuseTex : register(t0);
Texture2D specularTex : register(t1);
Texture2D normalTex : register(t2);

SamplerState tSampler : register(s0);

struct MaterialStruct
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float4 emissive;

	float opacity;
	float shininess;
	float roughness;

	float fresnel0;
	int type;

    bool hasDiffuseTex;
	bool hasSpecularTex;
	bool hasNormalTex;
}; 

struct LightStruct
{
	float4 position;
	float4 direction;
	float4 color;

	float range;
	int type;
};

cbuffer EnvironmentCB : register(b0)
{
	float4 MediaInfo;

    float4 EyePosition;
	float4 SceneAmbient;

	int NbLights;
	LightStruct Lights[MAX_LIGHTS];
}; 

cbuffer MaterialCB : register(b1)
{
	MaterialStruct Material;
};

float4 ambientLighning(float4 matDiffuse)
{
	return SceneAmbient * Material.ambient * matDiffuse;
}

float4 diffuseLighning(float4 matDiffuse, float roughness, float fresnel, float NoL, float NoV, float VoL)
{
	if (Material.type == DEFAULT_METAL_IDX)
	{
		// No diffusion.
		return float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	if (Material.type == PERFECT_MIRROR_IDX)
	{
		// Mirror we give it a low grey.
		return float4(0.32f, 0.32f, 0.32f, 0.0f);
	}

	// Hair or other dielectrics
	float4 diffuse = matDiffuse * InvPI;
	if (Material.type == PLASTIC_IDX)
	{
		// Plastic. Oren Nayar reflectance model from the Unreal Engine
		float a = roughness * roughness;
		float s = a;
		float s2 = s * s;
		float Cosri = VoL - NoV * NoL;
		float C1 = 1.0f - 0.5f * s2 / (s2 + 0.33f);
		float C2 = 0.45f * s2 / (s2 + 0.09f) * Cosri * (Cosri >= 0.0f ? (1.0f / max(NoL, NoV)) : 1.0f);

		diffuse *= (C1 + C2) * (1.0f + roughness * 0.5f);
	}

	if (Material.type == HAIR_IDX)
	{
		// Realtime hair shading is not supported. Returns flat diffuse.
		return diffuse;
	}

	return (1.0f - fresnel) * diffuse;
}

float4 specularLightning(float4 matDiffuse, float2 texCoord, float HoN, float fresnel)
{
	if (Material.type == PERFECT_MIRROR_IDX || Material.type == HAIR_IDX)
		return float4(0.0f, 0.0f, 0.0f, 0.0f);

	float4 matSpecular;
	if (Material.type == DEFAULT_DIELECTRIC_IDX || Material.type == PLASTIC_IDX || Material.type == SSS_IDX)
	{
		matSpecular = Material.specular;
		if (Material.hasSpecularTex)
		{
			matSpecular *= specularTex.Sample(tSampler, texCoord);
		}
	}
	else
	{
		// Metal, use albedo as specular color.
		matSpecular = matDiffuse;
	}

	float normalizationFactor = saturate((Material.shininess + 8) * InvPI8);

	return saturate(
		matSpecular * fresnel * pow(HoN, Material.shininess) * normalizationFactor
	);
}

float schlick(float HoN)
{
	return saturate(Material.fresnel0 + (1.0f - Material.fresnel0) * pow(1.0f - HoN, 5.0f));
}

float3 computeNormal(VS_OUTPUT input)
{
	float3 N = input.normal;
	if (Material.hasNormalTex)
	{
		float3 bumpMapNormal = normalTex.Sample(tSampler, input.texCoord) * 2.0f - 1.0f;
		float3x3 tbn = float3x3(input.tangent, input.bitangent, N);

		N = normalize(mul(bumpMapNormal, tbn));
	}

	return N;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	const float3 P = input.worldPosition.xyz;
	const float3 N = computeNormal(input);

	float3 V = EyePosition.xyz - P;
	const float vLength = length(V);
	V /= vLength;

	const float NoV = dot(N, V);

	float4 matDiffuse = Material.diffuse;
	if (Material.hasDiffuseTex)
	{
		matDiffuse *= diffuseTex.Sample(tSampler, input.texCoord);
	}

	float4 pixelColor = ambientLighning(matDiffuse) + Material.emissive;

	for (int i = 0; i < NbLights; ++i)
	{
		float3 L;
		// Determinisic branch on a constant buffer so its ok.
		if (Lights[i].type == 0)
		{
			L = -Lights[i].direction.xyz;
		}
		else
		{
			L = Lights[i].position.xyz - P;
			float len = length(L);
			if (len >= Lights[i].range)
				continue;

			L /= len;
		}

		float NoL = dot(N, L);
		if (NoL <= 0.0f)
			continue;

		const float3 H = normalize(L + V);
		const float HoN = max(dot(N, H), 0.0f);

		const float VoL = dot(V, L);

		// Compute fresnel
		const float fresnel = schlick(HoN);

		// Diffuse contribution
		float4 color = diffuseLighning(matDiffuse, Material.roughness, fresnel, NoL, NoV, VoL);

		// Specular contribution
		color += specularLightning(matDiffuse, input.texCoord, HoN, fresnel);

		// Lambert cosine law
		color *= NoL;

		// Multiply by light intensity
		color *= Lights[i].color;

		// Finally add light contribution to pixel color
		pixelColor += color;
	}

	if (MediaInfo.x > 0.001f)
	{
		pixelColor = lerp(float4(MediaInfo.y, MediaInfo.y, MediaInfo.y, MediaInfo.y),
			pixelColor,
			exp2(-MediaInfo.z * vLength));
	}

	pixelColor.a = Material.opacity;

	return pixelColor;
}