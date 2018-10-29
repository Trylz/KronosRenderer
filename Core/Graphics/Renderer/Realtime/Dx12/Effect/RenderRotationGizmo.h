//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "TRenderGizmo.h"
#include "Graphics/Gizmo/TRotationGizmo.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using DX12RotationGizmo = Gizmo::TRotationGizmo<DX12_GRAPHIC_ALLOC_PARAMETERS>;

class RenderRotationGizmo : public TRenderGizmo<3>
{
public:
	RenderRotationGizmo::RenderRotationGizmo(const DXGI_SAMPLE_DESC& sampleDesc)
	: TRenderGizmo<3>(sampleDesc)
	{
	}

	void pushDrawCommands(RenderGizmoPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex) override;

private:
	void updateConstantBuffers(const Camera::SharedCameraPtr& camera, const DX12RotationGizmo* gizmo, nbInt32 frameIndex);

};
}}}}}