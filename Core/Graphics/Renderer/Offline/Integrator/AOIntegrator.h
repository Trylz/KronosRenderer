//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#pragma once

#include "BaseIntegrator.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
struct AOIntegrator : public BaseIntegrator
{
	static kFloat32 sample(const Intersector::BaseIntersectorPtr& intersector,
		const IntersectionProperties& isectProps);
};
}}}}