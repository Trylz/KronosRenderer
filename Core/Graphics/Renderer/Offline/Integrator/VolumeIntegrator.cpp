//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"

#include "Math/Generator/RandomPrimitiveSampleGenerator.h"
#include "DirectLightningIntegrator.h"
#include "VolumeIntegrator.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
	// @See: https://cs.dartmouth.edu/~wjarosz/publications/dissertation/chapter4.pdf
	Spectrum VolumeIntegrator::sample(const Intersector::BaseIntersectorPtr& intersector,
		const Scene::BaseScene* scene,
		const Spectrum& inRadiance,
		const glm::vec3& startPt,
		const glm::vec3& endPt,
		const MediaPtr& media)
	{
		const MediaSettings& mediaSettings = media->getMediaSettings();

		const glm::vec3 direction = endPt - startPt;
		const nbFloat32 dirLength = glm::length(direction);

		nbUint32 nbSamples = mediaSettings.m_nbSamples;
		if (mediaSettings.m_dynamicNbSamples)
		{
			// Compute dynamic nb samples based on line size.
			const nbFloat32 boundsSize = glm::length(scene->getModel()->getBounds().getSize());
			const nbFloat32 lengthOverBoundsSize = dirLength / boundsSize;

			nbSamples = (nbUint32)(nbSamples * lengthOverBoundsSize);
		}


		Spectrum accInRadiance;
		{
			// Accumulated in-scattering radiance
			const glm::vec3 step = direction / (nbFloat32)nbSamples;

			//const nbFloat32 stepSize = dirLength / nbSamples;
			//const nbFloat32 segmentTransmittance = std::exp2f(-stepSize * extinction);
			//const nbFloat32 scatteringTransmitance = segmentTransmittance * mediaSettings.m_scatteringCoeff;

			glm::vec3 indirectSamples[2];
			glm::vec3 currentPt = startPt;

			for (nbUint32 i = 0u; i < nbSamples; ++i, currentPt += step)
			{
				Spectrum directRadiance;
				{
					// Compute direct contribution from lights
					for (const auto& lightId : scene->getLights())
					{
						const auto light = Light::getLightFromEntity(lightId);
						const auto sampleToLight = light->generateSampleToLight(s_nbGenerator, currentPt);
						if (!sampleToLight.canProcess)
							continue;

						const Math::Ray sRay(currentPt, sampleToLight.L, sampleToLight.length);
						const nbFloat32 occlusionStrength = intersector->occlusion(sRay);

						if (occlusionStrength != 1.0f)
						{
							const nbFloat32 phase = media->sample(direction, sampleToLight.L);
							const nbFloat32 distanceFactor = light->getType() != Light::Point ? 1.0f :
								std::exp2f(-sampleToLight.length * media->getExtinctionCoeff());

							directRadiance += phase *
								distanceFactor *
								light->getFinalColor() *
								(1.0f - occlusionStrength);
						}
					}
				}

				Spectrum indirectRadiance;
				if (mediaSettings.m_multipleStattering)
				{
					// Compute indirect contributions.
					// Compute two samples. One randomly chosen and its opposite.
					const nbFloat32 r1 = s_nbGenerator.generateSignedNormalized();
					const nbFloat32 r2 = s_nbGenerator.generateUnsignedNormalized();

					indirectSamples[0] = Math::uniformSphericalSample(r1, r2);
					indirectSamples[1] = -indirectSamples[0];

					for (const glm::vec3& sample : indirectSamples)
					{
						const Math::Ray ray(currentPt, sample);

						Intersector::IntersectionInfo isectResult;
						if (intersector->intersect(ray, isectResult))
						{
							const auto isectProps = buildIntersectionProperties(ray, isectResult, scene);
							const auto material = scene->getModel()->getMaterialFromEntityOrDefault(isectResult.object->getMaterialId());

							const auto materialColorCache = material->buildBsdfCache(scene->getAmbientColor(), isectProps.texCoord);

							const nbFloat32 distanceFactor = std::exp2f(-isectResult.meshIntersectData.packetIntersectionResult.t
							* media->getExtinctionCoeff());

							indirectRadiance += distanceFactor * DirectLightningIntegrator::sample(scene->getLights(),
								intersector,
								*material,
								materialColorCache,
								isectProps);
						}
						else
						{
							nbFloat32 phase = media->sample(direction, sample);
							indirectRadiance += phase * getSkyColor(scene, ray);
						}
					}
				}

				accInRadiance += directRadiance + indirectRadiance;
			}

			accInRadiance /= nbSamples;
		}

		// Reduced surface radiance
		const nbFloat32 transmittance = std::exp2f(-dirLength * media->getExtinctionCoeff());

		const Spectrum reducedRadiance = inRadiance * transmittance;

		return (reducedRadiance + accInRadiance) * (mediaSettings.m_noise ? s_nbGenerator.generateBeetween(0.8f, 1.0f) : 1.0f);
	}

}}}}