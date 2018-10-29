//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"

#include "LightVisualLines.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

LightVisualLines::~LightVisualLines()
{
	GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->releaseVertexBuffer(m_boxVtxBuffer);
	GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->releaseIndexBuffer(m_boxIndexBuffer);
}

void LightVisualLines::initVertexAndIndexBuffer()
{
	Math::Shape::Box box;
	box.m_min = glm::vec3(-0.5f, -0.5f, 0.0f);
	box.m_max = glm::vec3(0.5f, 0.5f, 1.0f);

	NEBULA_ASSERT(GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->createVertexBuffer(
		box.createVertices(), m_boxVtxBuffer)
	);

	NEBULA_ASSERT(GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->createIndexBuffer(
		Math::Shape::Box::s_indices, m_boxIndexBuffer)
	);
}

void LightVisualLines::updateConstantBuffers(const LightVisualLinesPushArgs& data, nbInt32 frameIndex)
{
	// Compute dynamic scale
	const auto& camera = data.camera;

	const nbFloat32 distance = glm::length(camera->getPosition() - data.light->getTransform()->getPosition());
	const nbFloat32 distanceRatio = distance / (data.sceneBoundsSize + 1e-6f) ;

	const nbFloat32 scale = Light::BaseLight::s_billboardSize * distanceRatio * camera->getFovy().getValue();

	const nbFloat32 scaleXY = scale * 0.12f;
	const nbFloat32 scaleZ = scale * 2.8f;

	// Compute MVP
	const glm::mat4 glmModelMatrix = glm::scale(
		data.light->getTransform()->getMatrix(),
		glm::vec3(scaleXY, scaleXY, scaleZ)
	);

	const XMMATRIX modelMatrix = XMMATRIX(&glmModelMatrix[0][0]);
	const XMMATRIX projectionMatrix = camera->getDirectXPerspectiveMatrix();
	const XMMATRIX viewMatrix = camera->getDirectXViewMatrix();

	const XMMATRIX mvp = XMMatrixTranspose(modelMatrix * viewMatrix * projectionMatrix);

	// Vertex shader CB
	VertexShaderCB vertexShaderCB;
	XMStoreFloat4x4(&vertexShaderCB.wvpMat, mvp);
	memcpy(m_vertexShaderCBGPUAddress[frameIndex], &vertexShaderCB, sizeof(VertexShaderCB));

	// Vertex shader CB
	PixelShaderCB pixelShaderCB;
	pixelShaderCB.color = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	memcpy(m_pixelShaderCBGPUAddress[frameIndex], &pixelShaderCB, sizeof(PixelShaderCB));
}

void LightVisualLines::pushDrawCommands(LightVisualLinesPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex)
{
	if (data.light->getType() != Light::Directionnal)
		return;

	updateConstantBuffers(data, frameIndex);

	commandList->SetPipelineState(m_PSO);
	commandList->SetGraphicsRootSignature(m_rootSignature);

	commandList->SetGraphicsRootConstantBufferView(0, m_vertexShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, m_pixelShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetIndexBuffer(&m_boxIndexBuffer.bufferView);
	commandList->IASetVertexBuffers(0, 1, &m_boxVtxBuffer.bufferView);

	commandList->DrawIndexedInstanced((UINT)Math::Shape::Box::s_indices.size(), 1, 0, 0, 0);
}
}}}}}