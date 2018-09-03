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
struct DirectLightningIntegrator : BaseIntegrator
{
	static RGBSpectrum sample(const Scene::BaseScene::LightContainer& lights,
		const Intersector::BaseIntersectorPtr& intersector,
		const Material::BaseMaterial& material,
		const Material::BaseMaterial::BaseColorCachePtr& colorCache,
		const IntersectionProperties& isectProps);
};
}}}}