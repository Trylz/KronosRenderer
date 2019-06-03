//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"
#include "DirectLightningIntegrator.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
	Spectrum DirectLightningIntegrator::sample(const EntityIdentifierArray& lights,
		const Intersector::BaseIntersectorPtr& intersector,
		const Material::BaseMaterial& material,
		const Material::BaseMaterial::BaseColorCachePtr& colorCache,
		const IntersectionProperties& isectProps)
	{
		Spectrum outDirect = BlackRGBSpectrum;

		for (const auto& lightId : lights)
		{
			const auto light = Entity::EntityDatabaseSingleton::instance()->getEntity<Light::BaseLight>(lightId);

			const auto sampleToLight = light->generateSampleToLight(s_nbGenerator, isectProps.P);
			if (!sampleToLight.canProcess)
				continue;

			const nbFloat32 NoL = glm::dot(isectProps.N, sampleToLight.L);
			if (NoL <= 0.0f)
				continue;

			Math::Ray sRay(isectProps.deltaP, sampleToLight.L, sampleToLight.length);
			const nbFloat32 occlusionStrength = intersector->occlusion(sRay);

			if (occlusionStrength != 1.0f)
			{
				// Sample the brdf to get the scale
				auto sampledBrdf = material.sampleBsdf(colorCache, sampleToLight.L, isectProps.V, isectProps.N, NoL, false);

				// Multiply by light radiance
				sampledBrdf *= light->getFinalColor();

				// Multiply by visibility
				sampledBrdf *= (1.0f - occlusionStrength);

				// Finally add light contribution
				outDirect += sampledBrdf;
			}
		}

		return outDirect;
	}

}}}}
