//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"
#include "RenderScaleGizmo.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

void RenderScaleGizmo::updateConstantBuffers(const Camera::SharedCameraPtr& camera, const DX12ScaleGizmo* gizmo, nbInt32 frameIndex)
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
	updatePixelShaderCb(3, s_whiteColor);
}

void RenderScaleGizmo::pushDrawCommands(RenderGizmoPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex)
{
	const DX12ScaleGizmo* dx12Gizmo = dynamic_cast<const DX12ScaleGizmo*>(data.scene.getCurrentGizmo());
	NEBULA_ASSERT(dx12Gizmo);

	updateConstantBuffers(data.scene.getCamera(), dx12Gizmo, frameIndex);

	commandList->SetPipelineState(m_PSO);
	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetIndexBuffer(&dx12Gizmo->getBoxesIndexBuffer().bufferView);
	commandList->SetGraphicsRootConstantBufferView(0, m_vertexShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

	nbInt32 pixelShaderIdx = 0;

	auto pushCommands = [&](size_t instanceCount, const D3D12_VERTEX_BUFFER_VIEW& view)
	{
		commandList->SetGraphicsRootConstantBufferView(1, pixelShaderIdx * PixelShaderCBAlignedSize + m_pixelShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());
		++pixelShaderIdx;

		commandList->IASetVertexBuffers(0, 1, &view);
		commandList->DrawIndexedInstanced((UINT)instanceCount, 1, 0, 0, 0);
	};

	// 0: Big boxes
	for (auto& iter : dx12Gizmo->getBigBoxVertexBuffers())
	{
		pushCommands(Math::Shape::Box::s_indices.size(), iter.second.bufferView);
	}

	// 1: Thin boxes
	pixelShaderIdx = 0;
	for (auto& iter : dx12Gizmo->getThinBoxVertexBuffers())
	{
		pushCommands(Math::Shape::Box::s_indices.size(), iter.second.bufferView);
	}
}
}}}}}