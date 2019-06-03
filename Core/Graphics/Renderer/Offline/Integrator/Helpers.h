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
#include "../Intersector/BaseIntersector.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
inline nbBool isNegligeable(const Spectrum& s)
{
	static const nbFloat32 oeps = 1e-5f;
	return (s.x < oeps) && (s.y < oeps) && (s.z < oeps);
}

inline glm::vec3 getOffsetedPositionInDirection(const glm::vec3& P, const glm::vec3& D, const nbFloat32 eps)
{
	return P + (eps * D);
}

struct IntersectionProperties
{
	glm::vec3 P;
	glm::vec3 deltaP;
	glm::vec3 inDeltaP;
	glm::vec3 V;
	glm::vec3 N;
	glm::vec2 texCoord;
};

IntersectionProperties buildIntersectionProperties(const Math::Ray& ray, const Intersector::IntersectionInfo& info, const Scene::BaseScene* scene);

Spectrum getSkyColor(const Scene::BaseScene* scene, const Math::Ray& ray, nbBool useSceneBackground = false);

}}}}