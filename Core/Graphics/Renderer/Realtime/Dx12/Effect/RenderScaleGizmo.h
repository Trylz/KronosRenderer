//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "TRenderGizmo.h"
#include "Graphics/Gizmo/TScaleGizmo.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using DX12ScaleGizmo = Gizmo::TScaleGizmo<DX12_GRAPHIC_ALLOC_PARAMETERS>;

class RenderScaleGizmo : public TRenderGizmo<4>
{
public:
	RenderScaleGizmo::RenderScaleGizmo(const DXGI_SAMPLE_DESC& sampleDesc)
	: TRenderGizmo<4>(sampleDesc)
	{
	}

	void pushDrawCommands(RenderGizmoPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex) override;

private:
	void updateConstantBuffers(const Camera::SharedCameraPtr& camera, const DX12ScaleGizmo* gizmo, nbInt32 frameIndex);

};
}}}}}