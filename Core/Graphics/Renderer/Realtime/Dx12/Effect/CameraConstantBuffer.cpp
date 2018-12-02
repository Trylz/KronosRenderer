//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"
#include "CameraConstantBuffer.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
CameraConstantBuffer::CameraConstantBuffer()
{
	MAKE_SWAP_CHAIN_ITERATOR_I
	{
		HRESULT hr = D3D12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexShaderCBUploadHeaps[i]));

		NEBULA_ASSERT(SUCCEEDED(hr));

		m_vertexShaderCBUploadHeaps[i]->SetName(L"Vertex shader camera constant buffer");
		NEBULA_ASSERT(SUCCEEDED(hr));

		hr = m_vertexShaderCBUploadHeaps[i]->Map(0, &ReadRangeGPUOnly, reinterpret_cast<void**>(&m_vertexShaderCBGPUAddress[i]));
		NEBULA_ASSERT(SUCCEEDED(hr));
	}
}

CameraConstantBuffer::~CameraConstantBuffer()
{
	MAKE_SWAP_CHAIN_ITERATOR_I
		NEBULA_DX12_SAFE_RELEASE(m_vertexShaderCBUploadHeaps[i]);
}

void CameraConstantBuffer::update(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex)
{
	VertexShaderCB vertexShaderCB;

	XMStoreFloat4x4(&vertexShaderCB.vpMat,
		scene.getCamera()->getDirectXTransposedVP());

	memcpy(m_vertexShaderCBGPUAddress[frameIndex], &vertexShaderCB, sizeof(VertexShaderCB));
}
}}}}}
