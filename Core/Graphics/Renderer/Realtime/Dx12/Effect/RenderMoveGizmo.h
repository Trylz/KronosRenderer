//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "TRenderGizmo.h"
#include "Graphics/Gizmo/TMoveGizmo.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using DX12MoveGizmo = Graphics::Gizmo::TMoveGizmo<DX12_GRAPHIC_ALLOC_PARAMETERS>;

class RenderMoveGizmo : public TRenderGizmo<6>
{
public:
	RenderMoveGizmo::RenderMoveGizmo(const DXGI_SAMPLE_DESC& sampleDesc)
	: TRenderGizmo<6>(sampleDesc)
	{
	}

	void pushDrawCommands(RenderGizmoPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex) override;

private:
	void updateConstantBuffers(const Camera::SharedCameraPtr& camera, const DX12MoveGizmo* gizmo, nbInt32 frameIndex);

};
}}}}}