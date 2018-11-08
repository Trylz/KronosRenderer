//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"

#include "CameraConstantBuffer.h"
#include "CubeMapping.h"
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
	D3D12_ROOT_PARAMETER rootParameters[3];

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;

	nbUint32 paramIdx = 0;
	// 0 & 1: vertex shader constant buffers
	for (; paramIdx < 2; ++paramIdx)
	{
		rootCBVDescriptor.ShaderRegister = paramIdx;
		rootParameters[paramIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[paramIdx].Descriptor = rootCBVDescriptor;
		rootParameters[paramIdx].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	}

	// 2 : Root parameter for the cube map texture.
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

	rootParameters[paramIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[paramIdx].DescriptorTable = descriptorTable;
	rootParameters[paramIdx].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	createRootSignature(rootParameters, _countof(rootParameters));
}

void CubeMapping::initPipelineStateObjects()
{
	D3D12_INPUT_ELEMENT_DESC inputLayoutElement[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	ID3DBlob* vertexShader = compileShader(std::wstring(L"CubeMapping_VS.hlsl"), true);
	ID3DBlob* pixelShader = compileShader(std::wstring(L"CubeMapping_PS.hlsl"), false);

	compilePipeline(m_PSO,
		vertexShader,
		pixelShader,
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC)
	);

	m_PSO->SetName(L"Cube mapping PSO");
}

void CubeMapping::initVertexShaderCB()
{
	for (nbInt32 i = 0; i < SwapChainBufferCount; ++i)
	{
		HRESULT hr = D3D12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, 
			IID_PPV_ARGS(&m_vertexShaderCBUploadHeaps[i]));

		NEBULA_ASSERT(SUCCEEDED(hr));
		m_vertexShaderCBUploadHeaps[i]->SetName(L"CubeMappingEffect : Vertex shader Constant Buffer Upload heap");

		hr = m_vertexShaderCBUploadHeaps[i]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_vertexShaderCBGPUAddress[i]));
		NEBULA_ASSERT(SUCCEEDED(hr));
	}
}

void CubeMapping::updateVertexShaderCB(CubeMappingPushArgs& data, nbInt32 frameIndex)
{
	VertexShaderCB vertexShaderCB;

	auto center = data.scene.getCubeMap()->getBox().getCenter();
	vertexShaderCB.cubeMapCenter = { center.x, center.y, center.z};

	memcpy(m_vertexShaderCBGPUAddress[frameIndex], &vertexShaderCB, sizeof(VertexShaderCB));
}

void CubeMapping::pushDrawCommands(CubeMappingPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex)
{
	auto& cubeMap = data.scene.getCubeMap();
	NEBULA_ASSERT(cubeMap);

	commandList->SetPipelineState(m_PSO);
	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	updateVertexShaderCB(data, frameIndex);
	commandList->SetGraphicsRootConstantBufferView(0, CameraConstantBufferSingleton::instance()->getUploadtHeaps()[frameIndex]->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, m_vertexShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

	auto dx12CubeMap = static_cast<const DX12CubeMap*>(cubeMap.get());

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx12CubeMap->m_textureHandle.buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	
	commandList->SetGraphicsRootDescriptorTable(2, dx12CubeMap->m_textureHandle.descriptorHandle.getGpuHandle());

	commandList->IASetVertexBuffers(0, 1, &dx12CubeMap->m_vtxHandle.bufferView);
	commandList->IASetIndexBuffer(&dx12CubeMap->m_idxHandle.bufferView);

	// Draw mesh.
	commandList->DrawIndexedInstanced(dx12CubeMap->getIndexCount(), 1, 0, 0, 0);

	// Reset texture state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx12CubeMap->m_textureHandle.buffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON));
}
}}}}}