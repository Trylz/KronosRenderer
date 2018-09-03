//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#include "stdafx.h"

#include "Math/Generator/RandomPrimitiveSampleGenerator.h"
#include "DirectLightningIntegrator.h"
#include "VolumeIntegrator.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
	// @See: https://cs.dartmouth.edu/~wjarosz/publications/dissertation/chapter4.pdf
	RGBSpectrum VolumeIntegrator::sample(const Intersector::BaseIntersectorPtr& intersector,
		const Scene::BaseScene* scene,
		const RGBSpectrum& inRadiance,
		const glm::vec3& startPt,
		const glm::vec3& endPt)
	{
		const MediaPtr& media = scene->getMedia();
		const MediaSettings& mediaSettings = media->getSettings();

		const glm::vec3 direction = endPt - startPt;
		const kFloat32 dirLength = glm::length(direction);

		kUint32 nbSamples = mediaSettings.m_nbSamples;
		if (mediaSettings.m_dynamicNbSamples)
		{
			// Compute dynamic nb samples based on line size.
			const kFloat32 sceneSize = glm::length(scene->getModel()->getBounds().getSize());
			const kFloat32 lengthOverSceneSize = dirLength / sceneSize;

			nbSamples = (kUint32)(nbSamples * lengthOverSceneSize);
		}

		RGBSpectrum accInRadiance;
		{
			// Accumulated in-scattering radiance
			const glm::vec3 step = direction / (kFloat32)nbSamples;

			//const kFloat32 stepSize = dirLength / nbSamples;
			//const kFloat32 segmentTransmittance = std::exp2f(-stepSize * media->getExtinctionCoeff());
			//const kFloat32 scatteringTransmitance = segmentTransmittance * mediaSettings.m_scatteringCoeff;

			std::vector<glm::vec3> indirectSamples(2);
			glm::vec3 currentPt = startPt;

			for (kUint32 i = 0u; i < nbSamples; ++i, currentPt += step)
			{
				RGBSpectrum directRadiance;
				{
					// Compute direct contribution from lights
					for (auto& lightIter : scene->getLights())
					{
						const auto sampleToLight = lightIter.second->generateSampleToLight(s_nbGenerator, currentPt);
						if (!sampleToLight.canProcess)
							continue;

						const Math::Ray sRay(currentPt, sampleToLight.L, sampleToLight.length);
						const kFloat32 occlusionStrength = intersector->occlusion(sRay);

						if (!Math::isApprox(occlusionStrength, 1.0f, 1e-6f))
						{
							kFloat32 phase = media->sample(direction, sampleToLight.L);
							directRadiance += phase *
								lightIter.second->getFinalColor() *
								(1.0f - occlusionStrength);
						}
					}
				}

				RGBSpectrum indirectRadiance;
				if (mediaSettings.m_multipleStattering)
				{
					// Compute indirect contributions.
					// Compute two samples. One randomly chosen and its opposite.
					const kFloat32 r1 = s_nbGenerator.generateSignedNormalized();
					const kFloat32 r2 = s_nbGenerator.generateUnsignedNormalized();

					indirectSamples[0] = Math::uniformSphericalSample(r1, r2);
					indirectSamples[1] = -indirectSamples[0];

					for (const glm::vec3& sample : indirectSamples)
					{
						const Math::Ray ray(currentPt, sample);

						Intersector::IntersectionInfo isectResult;
						if (intersector->intersect(ray, isectResult))
						{
							const auto isectProps = buildIntersectionProperties(ray, isectResult, scene);
							const auto material = scene->getModel()->getMaterial(isectResult.object->m_materialId);

							const auto materialColorCache = material->buildColorCache(scene->getAmbientColor(),
								isectProps.texCoord,
								scene->getModel()->getImageContainer());

							indirectRadiance += DirectLightningIntegrator::sample(scene->getLights(),
								intersector,
								*material,
								materialColorCache,
								isectProps);
						}
						else if (auto& cubeMap = scene->getCubeMap())
						{
							kFloat32 phase = media->sample(direction, sample);
							indirectRadiance += phase * glm::swizzle<glm::X, glm::Y, glm::Z>(
									cubeMap->sample(ray)
								);
						}
					}
				}

				accInRadiance += directRadiance + indirectRadiance;
			}

			accInRadiance /= nbSamples;
		}

		// Reduced surface radiance
		const kFloat32 transmittance = std::exp2f(-dirLength * media->getExtinctionCoeff());

		const RGBSpectrum reducedRadiance = inRadiance * transmittance;

		return reducedRadiance + accInRadiance;
	}

}}}}