//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

#include "stdafx.h"

#include "CubeMapping.h"
#include "Graphics/Renderer/Realtime/Dx12/Dx12Renderer.h"
#include <dxgi1_4.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

CubeMapping::CubeMapping(const DXGI_SAMPLE_DESC& sampleDesc)
: BaseEffect(sampleDesc)
{
	initRootSignature();
	initPipelineStateObjects();
	initVertexShaderCB();
}

void CubeMapping::initRootSignature()
{
	D3D12_ROOT_PARAMETER rootParameters[2];

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	// 0 : Root parameter for the vertex shader constant buffer
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor = rootCBVDescriptor;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// 1 : Root parameter for the cube map texture.
	D3D12_DESCRIPTOR_RANGE descriptorTableRanges[1];
	descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorTableRanges[0].NumDescriptors = 1;
	descriptorTableRanges[0].BaseShaderRegister = 0;
	descriptorTableRanges[0].RegisterSpace = 0;
	descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// create a descriptor table
	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
	descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable = descriptorTable;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	createRootSignature(rootParameters, _countof(rootParameters));
}

void CubeMapping::initPipelineStateObjects()
{
	D3D12_INPUT_ELEMENT_DESC inputLayoutElement[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	compile(m_PSO,
		std::wstring(L"CubeMapping_VS.hlsl"),
		std::wstring(L"CubeMapping_PS.hlsl"),
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC),
		nullptr
	);

	m_PSO->SetName(L"Cube mapping PSO");
}

void CubeMapping::initVertexShaderCB()
{
	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		HRESULT hr = D3d12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, 
			IID_PPV_ARGS(&m_vertexShaderCBUploadHeaps[i]));

		ORION_ASSERT(SUCCEEDED(hr));
		m_vertexShaderCBUploadHeaps[i]->SetName(L"CubeMappingEffect : Vertex shader Constant Buffer Upload heap");

		hr = m_vertexShaderCBUploadHeaps[i]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_vertexShaderCBGPUAddress[i]));
		ORION_ASSERT(SUCCEEDED(hr));
	}
}

void CubeMapping::updateVertexShaderCB(CubeMappingPushArgs& data, int frameIndex)
{
	VertexShaderCB vertexShaderCB;

	XMStoreFloat4x4(&vertexShaderCB.wvpMat,
		data.scene.getCamera()->getDirectXTransposedMVP());

	auto& center = data.scene.getCubeMap()->getCenter();
	vertexShaderCB.cubeMapCenter = { center.x, center.y, center.z};

	memcpy(m_vertexShaderCBGPUAddress[frameIndex], &vertexShaderCB, sizeof(VertexShaderCB));
}

void CubeMapping::pushDrawCommands(CubeMappingPushArgs& data, ID3D12GraphicsCommandList* commandList, int frameIndex)
{
	auto& cubeMap = data.scene.getCubeMap();
	ORION_ASSERT(cubeMap);

	commandList->SetPipelineState(m_PSO);
	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	updateVertexShaderCB(data, frameIndex);
	commandList->SetGraphicsRootConstantBufferView(0, m_vertexShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

	auto dx12CubeMap = static_cast<const DX12CubeMap*>(cubeMap.get());

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx12CubeMap->m_textureHandle.buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	
	commandList->SetGraphicsRootDescriptorTable(1, dx12CubeMap->m_textureHandle.descriptorHandle.getGpuHandle());

	commandList->IASetVertexBuffers(0, 1, &dx12CubeMap->m_vtxHandle.bufferView);
	commandList->IASetIndexBuffer(&dx12CubeMap->m_idxHandle.bufferView);

	// Draw mesh.
	commandList->DrawIndexedInstanced(dx12CubeMap->getIndexCount(), 1, 0, 0, 0);

	// Reset texture state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx12CubeMap->m_textureHandle.buffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON));
}
}}}}}