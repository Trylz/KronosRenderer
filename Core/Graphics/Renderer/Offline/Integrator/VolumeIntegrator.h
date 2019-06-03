//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "BaseIntegrator.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
struct VolumeIntegrator : BaseIntegrator
{
	static Spectrum sample(const Intersector::BaseIntersectorPtr& intersector,
		const Scene::BaseScene* scene,
		const Spectrum& inRadiance,
		const glm::vec3& startPt,
		const glm::vec3& endPt,
		const MediaPtr& media);
};
}}}}