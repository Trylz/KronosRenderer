//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#include "stdafx.h"
#include "DirectLightningIntegrator.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
	RGBSpectrum DirectLightningIntegrator::sample(const Scene::BaseScene::LightContainer& lights,
		const Intersector::BaseIntersectorPtr& intersector,
		const Material::BaseMaterial& material,
		const Material::BaseMaterial::BaseColorCachePtr& colorCache,
		const IntersectionProperties& isectProps)
	{
		RGBSpectrum outDirect = BlackRGBSpectrum;

		for (auto& lightIter : lights)
		{
			auto sampleToLight = lightIter.second->generateSampleToLight(s_nbGenerator, isectProps.P);
			if (!sampleToLight.canProcess)
				continue;

			kFloat32 NoL = glm::dot(isectProps.N, sampleToLight.L);
			if (NoL <= 0.0f)
				continue;

			Math::Ray sRay(isectProps.deltaP, sampleToLight.L, sampleToLight.length);
			kFloat32 occlusionStrength = intersector->occlusion(sRay);

			if (!Math::isApprox(occlusionStrength, 1.0f, 1e-6f))
			{
				// Sample the brdf to get the scale
				auto sampledBrdf = material.sampleBsdf(colorCache, sampleToLight.L, isectProps.V, isectProps.N, NoL, false);

				// Multiply by light radiance
				sampledBrdf *= lightIter.second->getFinalColor();

				// Multiply by visibility
				sampledBrdf *= (1.0f - occlusionStrength);

				// Finally add light contribution
				outDirect += sampledBrdf;
			}
		}

		return outDirect;
	}

}}}}