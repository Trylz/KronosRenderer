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
struct DirectLightningIntegrator : BaseIntegrator
{
	static Spectrum sample(const EntityIdentifierArray& lights,
		const Intersector::BaseIntersectorPtr& intersector,
		const Material::BaseMaterial& material,
		const Material::BaseMaterial::BaseColorCachePtr& colorCache,
		const IntersectionProperties& isectProps);
};
}}}}