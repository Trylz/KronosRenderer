//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"
#include "AOIntegrator.h"
#include "Math/Generator/RandomPrimitiveSampleGenerator.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
	nbFloat32 AOIntegrator::sample(const Intersector::BaseIntersectorPtr& intersector,
		const IntersectionProperties& isectProps,
		nbBool useDistanceMode)
	{
		// Perform one ao sample
		const nbFloat32 r1 = s_nbGenerator.generateSignedNormalized();
		const nbFloat32 r2 = s_nbGenerator.generateUnsignedNormalized();
		glm::vec3 aoSample = Math::uniformSphericalSample(r1, r2);

		nbFloat32 NoL = glm::dot(isectProps.N, aoSample);
		if (NoL < 0.0f)
		{
			NoL *= -1.0f;
			aoSample *= -1.0f;
		}

		Math::Ray aoRay(isectProps.deltaP, aoSample);
		nbFloat32 occlusionStrength;
		
		if (useDistanceMode)
		{
			Intersector::IntersectionInfo info;
			if (intersector->intersect(aoRay, info))
				occlusionStrength = std::exp(-info.meshIntersectData.packetIntersectionResult.t);
			else
				occlusionStrength = 0.0f;
		}
		else
		{
			occlusionStrength = intersector->occlusion(aoRay);
		}

		return (1.0f - occlusionStrength) * NoL;
	}

}}}}