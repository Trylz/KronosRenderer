//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "TSimpleColor.h"
#include "Graphics/Gizmo/TMoveGizmo.h"
#include "Scene/BaseScene.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
struct LightVisualLinesPushArgs
{
	const Camera::SharedCameraPtr& camera;
	const Light::DatabaseLightPtr& light;
	const nbFloat32 sceneBoundsSize;
};

class LightVisualLines : public TSimpleColor<LightVisualLinesPushArgs&, 1>
{
public:
	LightVisualLines(const DXGI_SAMPLE_DESC& sampleDesc)
	: TSimpleColor<LightVisualLinesPushArgs&, 1>(sampleDesc)
	{
		initVertexAndIndexBuffer();
	}

	~LightVisualLines();

	void pushDrawCommands(LightVisualLinesPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex) override;

private:
	void initVertexAndIndexBuffer();
	void updateConstantBuffers(const LightVisualLinesPushArgs& data, nbInt32 frameIndex);

	Dx12VertexBufferHandle m_boxVtxBuffer;
	Dx12IndexBufferHandle m_boxIndexBuffer;
};
}}}}}