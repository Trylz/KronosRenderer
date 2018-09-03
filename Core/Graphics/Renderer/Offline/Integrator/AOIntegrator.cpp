//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#include "stdafx.h"
#include "AOIntegrator.h"
#include "Math/Generator/RandomPrimitiveSampleGenerator.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
	kFloat32 AOIntegrator::sample(const Intersector::BaseIntersectorPtr& intersector,
		const IntersectionProperties& isectProps)
	{
		// Perform one ao sample
		const kFloat32 r1 = s_nbGenerator.generateSignedNormalized();
		const kFloat32 r2 = s_nbGenerator.generateUnsignedNormalized();
		glm::vec3 aoSample = Math::uniformSphericalSample(r1, r2);

		kFloat32 NoL = glm::dot(isectProps.N, aoSample);
		if (NoL < 0.0f)
		{
			NoL *= -1.0f;
			aoSample *= -1.0f;
		}

		Math::Ray aoRay(isectProps.deltaP, aoSample);
		kFloat32 occlusionStrength = intersector->occlusion(aoRay);

		return (1.0f - occlusionStrength) * NoL;
	}

}}}}