//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "Spectrum.h"
#include "Scene/BaseScene.h"
#include "../Intersector/IntersectionInfo.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
inline kBool isNegligeable(const RGBSpectrum& s)
{
	static const kFloat32 oeps = 1e-5f;
	return (s.x < oeps) && (s.y < oeps) && (s.z < oeps);
}

inline glm::vec3 getOffsetedPositionInDirection(const glm::vec3& P, const glm::vec3& D, const kFloat32 eps)
{
	return P + (eps * D);
}

struct IntersectionProperties
{
	glm::vec3 P;
	glm::vec3 deltaP;
	glm::vec3 V;
	glm::vec3 N;
	glm::vec2 texCoord;
};

IntersectionProperties buildIntersectionProperties(const Math::Ray& ray, const Intersector::IntersectionInfo& info, const Scene::BaseScene* scene);
}}}}