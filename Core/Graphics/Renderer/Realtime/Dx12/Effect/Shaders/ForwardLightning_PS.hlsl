//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

#define PI 3.14159265359f
#define PI8 25.132741228f

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float4 worldPosition : POSITION;
	float3 normal : NORMAL;
	float2 texCoord: TEXCOORD;
};

Texture2D diffuseTex : register(t0);
Texture2D specularTex : register(t1);
SamplerState tSampler : register(s0);

struct MaterialStruct
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float4 emissive;

	float opacity;
	float shininess;

	float fresnel0;
	int type;

    bool hasDiffuseTex;
	bool hasSpecularTex;
}; 

struct LightStruct
{
	float4 position;
	float4 direction;
	float4 color;

	float range;
	int type;
};

cbuffer LightsCB : register(b0)
{
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

float4 diffuseLighning(float4 matDiffuse, float fresnel)
{
	if (Material.type == 1)
	{
		// Metal no diffusion.
		return float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	return (1.0f - fresnel) * matDiffuse / PI;
}

float4 specularLightning(float4 matDiffuse, float2 texCoord, float HoN, float fresnel)
{
	float4 matSpecular;
	if (Material.type == 0)
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

	float normalizationFactor = (Material.shininess + 8) / PI8;

	return matSpecular * fresnel * pow(HoN, Material.shininess) * normalizationFactor;
}

float schlick(float HoN)
{
	return saturate(Material.fresnel0 + (1.0f - Material.fresnel0) * pow(1.0f - HoN, 5.0f));
}

float getLightAttenuation(int lightIdx, float3 vertexPos)
{
	if (Lights[lightIdx].type == 0)
		return 1.0f;

	return length(Lights[lightIdx].position.xyz - vertexPos) * 0.018f;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	const float3 P = input.worldPosition.xyz;
	const float3 V = normalize(EyePosition.xyz - P);
	const float3 N = normalize(input.normal);

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

		// Compute fresnel
		const float fresnel = schlick(HoN);

		// Diffuse contribution
		float4 color = diffuseLighning(matDiffuse, fresnel);

		// Specular contribution
		color += specularLightning(matDiffuse, input.texCoord, HoN, fresnel);

		// Lambert cosine law
		color *= NoL;

		// Multiply by light intensity
		color *= Lights[i].color;

		// Irradiance falloff hack
		color /= getLightAttenuation(i, P);

		// Finally add light contribution to pixel color
		pixelColor += color;
	}

	pixelColor.a = Material.opacity;

	return pixelColor;
}