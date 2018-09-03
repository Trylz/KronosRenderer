//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#pragma once

#include "Scene/BaseScene.h"

namespace Graphics { namespace Renderer { namespace Realtime
{
class RealtimeRenderer
{
public:
	virtual ~RealtimeRenderer() {};
	virtual void release() = 0;

	virtual Model::ModelPtr createModelFromRaw(const std::string& path) const = 0;
	virtual Model::ModelPtr createModelFromNode(const Utilities::Xml::XmlNode& node) const = 0;

	virtual Texture::CubeMapPtr createCubeMapFromRaw(Graphics::Texture::CubeMapConstructRawArgs& args) const = 0;
	virtual Texture::CubeMapPtr createCubeMapFromNode(const Graphics::Texture::CubeMapConstructNodeArgs& args) const = 0;

	virtual void resizeBuffers(const glm::uvec2& newSize) = 0;
	virtual void onMaterialChanged(const Scene::BaseScene& scene, const MaterialIdentifier& matId) = 0;

	virtual void updateLightBuffers(const Scene::BaseScene& scene) = 0;
	virtual void updateMaterialBuffers(const Scene::BaseScene& scene) = 0;

	virtual void drawScene(const Scene::BaseScene& scene) = 0;

	virtual void present() = 0;
	virtual void finishTasks() {}
	virtual void startCommandRecording() {}
	virtual void endSceneLoadCommandRecording(const Scene::BaseScene* scene) {}
	virtual void endCommandRecording() {}
};

using RealTimeRendererPtr = std::unique_ptr<RealtimeRenderer>;
}}}