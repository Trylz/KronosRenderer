//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#pragma once

#include "Scene/BaseScene.h"

#define KRONOS_REALTIME_VSYNC_ENABLED 1

namespace Graphics { namespace Renderer { namespace Realtime
{
class RealtimeRenderer
{
public:
	virtual ~RealtimeRenderer() {};
	virtual void release() = 0;

	virtual Graphics::Model::ModelPtr createModelFromRaw(const std::string& path) const = 0;
	virtual Graphics::Model::ModelPtr createModelFromNode(const Utilities::Xml::XmlNode& node) const = 0;

	virtual Graphics::Texture::CubeMapPtr createCubeMapFromRaw(Graphics::Texture::CubeMapConstructRawArgs& args) const = 0;
	virtual Graphics::Texture::CubeMapPtr createCubeMapFromNode(const Graphics::Texture::CubeMapConstructNodeArgs& args) const = 0;

	virtual void resizeBuffers(const glm::uvec2& newSize) = 0;
	virtual void onMeshSelectionMaterialChanged(const Graphics::Scene::BaseScene& scene, const Model::MeshSelectable* currentSelection) = 0;
	virtual void onNewlightAdded(const Graphics::Scene::BaseScene& scene) = 0;
	virtual void drawScene(const Graphics::Scene::BaseScene& scene) = 0;

	virtual void present() = 0;
	virtual void finishTasks() {}
	virtual void startCommandRecording() {}
	virtual void endSceneLoadCommandRecording(const Graphics::Scene::BaseScene* scene) {}
	virtual void endCommandRecording() {}
};

using RealTimeRendererPtr = std::unique_ptr<RealtimeRenderer>;
}}}