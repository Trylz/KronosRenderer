//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"

#include "MeshGroupConstantBuffer.h"


namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

MeshGroupConstantBuffer::MeshGroupConstantBuffer()
: m_vertexShaderCBUploadHeap(nullptr)
, m_vertexShaderCBDefaultHeap(nullptr)
{
}

MeshGroupConstantBuffer::~MeshGroupConstantBuffer()
{
	NEBULA_DX12_SAFE_RELEASE(m_vertexShaderCBUploadHeap);
	NEBULA_DX12_SAFE_RELEASE(m_vertexShaderCBDefaultHeap);
}

void MeshGroupConstantBuffer::initVertexShaderConstantBuffer(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList)
{
	const auto& meshesByGroup = scene.getModel()->getMeshesByGroup();
	const nbUint32 bufferSize = (nbUint32)meshesByGroup.size();

	// 0 : Init heaps
	NEBULA_DX12_SAFE_RELEASE(m_vertexShaderCBUploadHeap);
	NEBULA_DX12_SAFE_RELEASE(m_vertexShaderCBDefaultHeap);

	// Create upload heap
	HRESULT hr = D3d12Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(VertexShaderCBAlignedSize * bufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexShaderCBUploadHeap));

	NEBULA_ASSERT(SUCCEEDED(hr));
	m_vertexShaderCBUploadHeap->SetName(L"Vertex shader group constant Buffer Upload heap");

	hr = m_vertexShaderCBUploadHeap->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_vertexShaderCBGPUAddress));
	NEBULA_ASSERT(SUCCEEDED(hr));

	// Create default heap
	hr = D3d12Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(VertexShaderCBAlignedSize * bufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_vertexShaderCBDefaultHeap));

	NEBULA_ASSERT(SUCCEEDED(hr));
	m_vertexShaderCBDefaultHeap->SetName(L"Vertex shader group constant Buffer Default heap");

	// 1 : Update upload heap
	for (auto& group : meshesByGroup)
	{
		updateMeshGroup(scene, group.first, commandList);
	}

	// 2: Copy to default heap
	fromVertexShaderUploadToDefaulHeap(commandList);
}

void MeshGroupConstantBuffer::onUpdateMeshGroup(const Scene::BaseScene& scene, const Model::MeshGroupId& groupId, ID3D12GraphicsCommandList* commandList)
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexShaderCBDefaultHeap, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));

	updateMeshGroup(scene, groupId, commandList);

	fromVertexShaderUploadToDefaulHeap(commandList);
}

void MeshGroupConstantBuffer::fromVertexShaderUploadToDefaulHeap(ID3D12GraphicsCommandList* commandList)
{
	// Copy upload heap to default
	commandList->CopyResource(m_vertexShaderCBDefaultHeap, m_vertexShaderCBUploadHeap);

	// Transition to pixel shader resource.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexShaderCBDefaultHeap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
}

void MeshGroupConstantBuffer::updateMeshGroup(const Scene::BaseScene& scene, const Model::MeshGroupId& groupId, ID3D12GraphicsCommandList* commandList)
{
	// Read first mesh tranform
	const auto& meshesByGroup = scene.getModel()->getMeshesByGroup();
	const auto groupIter = meshesByGroup.find(groupId);
	const glm::mat4& matrix = groupIter->second[0]->getTransform()->getMatrix();

	XMMATRIX modelMatrix = XMMATRIX(&matrix[0][0]);
	modelMatrix = XMMatrixTranspose(modelMatrix);

	VertexShaderGroupCB vertexShaderCB;
	XMStoreFloat4x4(&vertexShaderCB.modelMat, modelMatrix);
	// Set flat id of first mesh since group ids are strings
	vertexShaderCB.groupId = groupIter->second[0]->getFlatId();

	const nbUint64 posInCB = std::distance(meshesByGroup.begin(), groupIter) * VertexShaderCBAlignedSize;
	memcpy(m_vertexShaderCBGPUAddress + posInCB, &vertexShaderCB, sizeof(VertexShaderGroupCB));
}
}}}}}
