//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"
#include "RenderMoveGizmo.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

void RenderMoveGizmo::updateConstantBuffers(const Camera::SharedCameraPtr& camera, const DX12MoveGizmo* gizmo, nbInt32 frameIndex)
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

	// Pixel shader
	//Axes
	updatePixelShaderCb(0, s_xAxisColor);
	updatePixelShaderCb(1, s_yAxisColor);
	updatePixelShaderCb(2, s_zAxisColor);

	//Planes
	updatePixelShaderCb(3, {1.0f, 0.0f, 1.0f, 0.5f});
	updatePixelShaderCb(4, {0.3f, 0.3f, 0.3f, 0.5f});
	updatePixelShaderCb(5, {0.0f, 1.0f, 1.0f, 0.5f});
}

void RenderMoveGizmo::pushDrawCommands(RenderGizmoPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex)
{
	const DX12MoveGizmo* dx12Gizmo = dynamic_cast<const DX12MoveGizmo*>(data.scene.getCurrentGizmo());
	NEBULA_ASSERT(dx12Gizmo);

	if (!dx12Gizmo->getEnabled())
		return;

	updateConstantBuffers(data.scene.getCamera(), dx12Gizmo, frameIndex);

	commandList->SetPipelineState(m_PSO);
	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetGraphicsRootConstantBufferView(0, m_vertexShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

	nbInt32 pixelShaderIdx = 0;

	auto pushCommands = [&](size_t instanceCount, const D3D12_VERTEX_BUFFER_VIEW& view)
	{
		commandList->SetGraphicsRootConstantBufferView(1, pixelShaderIdx * PixelShaderCBAlignedSize + m_pixelShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());
		++pixelShaderIdx;

		commandList->IASetVertexBuffers(0, 1, &view);
		commandList->DrawIndexedInstanced((UINT)instanceCount, 1, 0, 0, 0);
	};

	// 0: Boxes
	commandList->IASetIndexBuffer(&dx12Gizmo->getBoxesIndexBuffer().bufferView);

	for (auto& iter : dx12Gizmo->getBoxVertexBuffers())
	{
		pushCommands(Math::Shape::Box::s_indices.size(), iter.second.bufferView);
	}

	// 1: Planes
	commandList->IASetIndexBuffer(&dx12Gizmo->getQuadsIndexBuffer().bufferView);

	for (auto& iter : dx12Gizmo->getQuadVertexBuffers())
	{
		pushCommands(6, iter.second.bufferView);
	}

	// 2: Pyramids
	pixelShaderIdx = 0;
	commandList->IASetIndexBuffer(&dx12Gizmo->getPyramidsIndexBuffer().bufferView);

	for (auto& iter : dx12Gizmo->getPyramidVertexBuffers())
	{
		pushCommands(Gizmo::MoveGizmo::s_pyramidIndices.size(), iter.second.bufferView);
	}
}
}}}}}