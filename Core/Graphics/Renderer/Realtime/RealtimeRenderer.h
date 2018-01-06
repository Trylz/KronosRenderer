//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

#pragma once

#include "Graphics/Scene.h"
#include "Graphics/Texture/CubeMap.h"

namespace Graphics { namespace Renderer { namespace Realtime
{
constexpr uint32_t swapChainBufferCount = 3u;

using MeshSelection = Graphics::Model::MeshGroupId;

class RealtimeRenderer
{
public:
	virtual ~RealtimeRenderer() {};

	virtual Graphics::Model::ModelPtr createModelFromRaw(const std::string& path) const = 0;
	virtual Graphics::Model::ModelPtr createModelFromNode(const Utilities::Xml::XmlNode& node) const = 0;

	virtual Graphics::Texture::CubeMapPtr createCubeMapFromRaw(Graphics::Texture::CubeMapConstructRawArgs& args) const = 0;
	virtual Graphics::Texture::CubeMapPtr createCubeMapFromNode(const Graphics::Texture::CubeMapConstructNodeArgs& args) const = 0;

	virtual void release() = 0;
	virtual void onRenderBufferSizeChanged(const glm::uvec2& newSize) = 0;
	virtual void onMeshSelectionMaterialChanged(const Graphics::Scene& scene, const MeshSelection& currentSelection) = 0;
	virtual void drawScene(const Graphics::Scene& scene, const MeshSelection& currentSelection) = 0;
	virtual void present() = 0;

	virtual void finishTasks() {}
	virtual void startCommandRecording() {}
	virtual void endSceneLoadCommandRecording(const Graphics::Scene* scene) {}
	virtual void endCommandRecording() {}
};

using RealTimeRendererPtr = std::unique_ptr<RealtimeRenderer>;
}}}