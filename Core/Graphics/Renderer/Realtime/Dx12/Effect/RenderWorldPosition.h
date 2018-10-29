//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "BaseEffect.h"
#include "Graphics/Renderer/Realtime/RealtimeRenderer.h"

#define NEBULA_WORLD_POSITION_RT_FORMAT DXGI_FORMAT_R32G32B32A32_FLOAT

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
struct RenderWorldPositionPushArgs
{
	const Scene::BaseScene& scene;
};

class RenderWorldPosition : public BaseEffect<RenderWorldPositionPushArgs&>
{
public:
	RenderWorldPosition(const DXGI_SAMPLE_DESC& sampleDesc);
	void pushDrawCommands(RenderWorldPositionPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex) override;

private:
	void initRootSignature() override;
	void initPipelineStateObjects() override;

	PipelineStatePtr m_PSO;
};
}}}}}