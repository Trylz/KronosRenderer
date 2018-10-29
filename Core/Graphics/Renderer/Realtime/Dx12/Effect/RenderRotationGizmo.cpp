//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"
#include "RenderRotationGizmo.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

void RenderRotationGizmo::updateConstantBuffers(const Camera::SharedCameraPtr& camera, const DX12RotationGizmo* gizmo, nbInt32 frameIndex)
{
	// Vertex shader
	updateVertexShaderConstantBuffer(camera, gizmo, frameIndex);

	// Pixel shader
	const auto updatePixelShaderCb = [&](nbInt32 heapIdx, XMFLOAT4 color)
	{
		PixelShaderCB pixelShaderCB;
		pixelShaderCB.color = gizmo->isAxisType(heapIdx + 1) ? s_hoverColor : color;
		memcpy(m_pixelShaderCBGPUAddress[frameIndex] + PixelShaderCBAlignedSize * heapIdx, &pixelShaderCB, sizeof(PixelShaderCB));
	};

	updatePixelShaderCb(0, s_xAxisColor);
	updatePixelShaderCb(1, s_yAxisColor);
	updatePixelShaderCb(2, s_zAxisColor);
}

void RenderRotationGizmo::pushDrawCommands(RenderGizmoPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex)
{
	const DX12RotationGizmo* dx12Gizmo = dynamic_cast<const DX12RotationGizmo*>(data.scene.getCurrentGizmo());
	NEBULA_ASSERT(dx12Gizmo);

	if (!dx12Gizmo->getEnabled())
		return;

	updateConstantBuffers(data.scene.getCamera(), dx12Gizmo, frameIndex);

	commandList->SetPipelineState(m_PSO);
	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetIndexBuffer(&dx12Gizmo->getIndexBuffer().bufferView);
	commandList->SetGraphicsRootConstantBufferView(0, m_vertexShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

	nbInt32 pixelShaderIdx = 0;
	const UINT instanceCount = dx12Gizmo->getIndexBuffer().count;

	for (auto& vtxBuffer : dx12Gizmo->getVertexBuffers())
	{
		commandList->SetGraphicsRootConstantBufferView(1, pixelShaderIdx * PixelShaderCBAlignedSize + m_pixelShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());
		commandList->IASetVertexBuffers(0, 1, &vtxBuffer.second.bufferView);
		commandList->DrawIndexedInstanced(instanceCount, 1, 0, 0, 0);

		++pixelShaderIdx;
	}
}
}}}}}