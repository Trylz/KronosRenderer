//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"
#include "Helpers.h"
#include "Graphics/Material/FresnelMaterial.h"

namespace Graphics { namespace Renderer { namespace Offline { namespace Integrator
{
	IntersectionProperties buildIntersectionProperties(const Math::Ray& ray, const Intersector::IntersectionInfo& info, const Scene::BaseScene* scene)
	{
		const auto mesh = info.object;
		const auto P = ray.getPoint(info.meshIntersectData.packetIntersectionResult.t);

		const nbInt32 trianglePacketIdx = info.meshIntersectData.trianglePacketIdx;
		const nbInt32 packedInternalIdx = info.meshIntersectData.packetIntersectionResult.triIdx;

		const nbUint32 triStartIdx = (NEBULA_INTRINSICS_NB_FLOAT * trianglePacketIdx + packedInternalIdx) * NEBULA_PRIMITIVE_NB_VTX;

		NEBULA_ASSERT(NEBULA_PRIMITIVE_NB_VTX == 3u);
		const auto& v1 = mesh->buildTransformedVertexFromIndices(triStartIdx);
		const auto& v2 = mesh->buildTransformedVertexFromIndices(triStartIdx + 1);
		const auto& v3 = mesh->buildTransformedVertexFromIndices(triStartIdx + 2);

		// Compute barycentric interpolation weights
		//see <-- https://en.wikibooks.org/wiki/GLSL_Programming/Rasterization ->
		const auto& packedTriangles = mesh->getTrianglePackets()[trianglePacketIdx];
		const nbFloat32 alpha1 = 0.5f * glm::length(glm::cross(v2.position - P, v3.position - P)) * packedTriangles.invArea[packedInternalIdx];
		const nbFloat32 alpha2 = 0.5f * glm::length(glm::cross(v1.position - P, v3.position - P)) * packedTriangles.invArea[packedInternalIdx];
		const nbFloat32 alpha3 = 0.5f * glm::length(glm::cross(v1.position - P, v2.position - P)) * packedTriangles.invArea[packedInternalIdx];

		// Texture coordinates
		auto texCoord = (v1.texCoord * alpha1) + (v2.texCoord * alpha2) + (v3.texCoord * alpha3);
		texCoord.s = std::abs(texCoord.s);
		texCoord.t = std::abs(texCoord.t);

		nbFloat64 dummy;
		if (texCoord.s > 1.0f)
			texCoord.s = (nbFloat32)std::modf(texCoord.s, &dummy);

		if (texCoord.t > 1.0f)
			texCoord.t = (nbFloat32)std::modf(texCoord.t, &dummy);

		// Eye vector
		auto V = -ray.m_direction;

		// Compute normal
		auto N = (v1.normal * alpha1) + (v2.normal * alpha2) + (v3.normal * alpha3);

		// Read model
		const Model::ModelPtr& model = scene->getModel();

		// Apply normal mapping
		const Material::BaseMaterial* material = model->getMaterial(mesh->getMaterialId());
		if (material->isFresnelMaterial())
		{
			const auto* fresnelMat = static_cast<const Material::FresnelMaterial*>(material);
			const ImageIdentifier normalMapId = fresnelMat->getNormalImageId();
			
			if (normalMapId.isValid())
			{
				// Read bump map
				const Texture::RGBAImage* image = model->getImage(normalMapId);
				const RGBAColor bumpMapNormal = image->getNormalizedPixelFromRatio(texCoord) * 2.0f - 1.0f;

				// Tangent space matrix
				const glm::vec3 tangent = (v1.tangent * alpha1) + (v2.tangent * alpha2) + (v3.tangent * alpha3);
				const glm::vec3 bitangent = (v1.bitangent * alpha1) + (v2.bitangent * alpha2) + (v3.bitangent * alpha3);
				const glm::mat3 tbn = glm::mat3(tangent, bitangent, N);

				// Bump mapped normal
				N = tbn * glm::swizzle<glm::X, glm::Y, glm::Z>(bumpMapNormal);
			}
		}

		// Finalize normal
		N = glm::normalize(N);

		if (glm::dot(N, V) < 0.0f)
			N *= -1;

		IntersectionProperties props;
		props.P = P;
		props.deltaP = getOffsetedPositionInDirection(P, N, scene->getRenderSettings()->m_rayEpsilon);
		props.inDeltaP = getOffsetedPositionInDirection(P, -N, scene->getRenderSettings()->m_rayEpsilon);
		props.V = V;
		props.N = N;
		props.texCoord = texCoord;

		return props;
	}

	RGBSpectrum getSkyColor(const Scene::BaseScene* scene, const Math::Ray& ray, nbBool useSceneAmbient)
	{
		if (auto& cubeMap = scene->getCubeMap())
		{
			auto sky = cubeMap->sample(ray);
			return RGBSpectrum(sky.x, sky.y, sky.z);
		}
		
		if (useSceneAmbient)
			return scene->getAmbientColor();

		return BlackRGBSpectrum;
	}

}}}}
