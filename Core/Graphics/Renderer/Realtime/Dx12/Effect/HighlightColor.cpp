//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"

#include "CameraConstantBuffer.h"
#include "HighlightColor.h"
#include "Graphics/Renderer/Realtime/Dx12/Dx12Renderer.h"
#include <dxgi1_4.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

HighlightColor::HighlightColor(const DXGI_SAMPLE_DESC& sampleDesc)
: BaseEffect(sampleDesc)
{
	initRootSignature();
	initPipelineStateObjects();
	initVertexShaderCB();
}

void HighlightColor::initRootSignature()
{
	D3D12_ROOT_PARAMETER rootParameters[3];

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;

	for (nbUint32 i = 0u; i < 3; ++i)
	{
		rootCBVDescriptor.ShaderRegister = i;
		rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[i].Descriptor = rootCBVDescriptor;
		rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	}

	createRootSignature(rootParameters, _countof(rootParameters));
}

void HighlightColor::initPipelineStateObjects()
{
	// Compile shaders
	ID3DBlob* vertexShader = compileShader(std::wstring(L"HighlightColor_VS.hlsl"), true);
	ID3DBlob* pixelShader = compileShader(std::wstring(L"HighlightColor_PS.hlsl"), false);

	// Compile PSOs
	D3D12_INPUT_ELEMENT_DESC inputLayoutElement[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// The rasterizer desc
	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizerDesc.MultisampleEnable = TRUE;

	// Create depth stencil desc
	D3D12_DEPTH_STENCIL_DESC dsDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	dsDesc.DepthEnable = FALSE;

	// Stencil test parameters
	dsDesc.StencilEnable = TRUE;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Compile first PSO 
	dsDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR_SAT;
	dsDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// Back-facing pixels.
	dsDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR_SAT;
	dsDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	compilePipeline(m_PSOs[0],
		vertexShader,
		pixelShader,
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC),
		CD3DX12_BLEND_DESC(D3D12_DEFAULT),
		dsDesc,
		rasterizerDesc
	);

	// Compile second PSO
	dsDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_DECR_SAT;
	dsDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR_SAT;
	dsDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_DECR_SAT;
	dsDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	dsDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_DECR_SAT;
	dsDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR_SAT;
	dsDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_DECR_SAT;
	dsDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	compilePipeline(m_PSOs[1],
		vertexShader,
		pixelShader,
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC),
		CD3DX12_BLEND_DESC(D3D12_DEFAULT),
		dsDesc,
		rasterizerDesc
	);
}

void HighlightColor::initVertexShaderCB()
{
	for (nbInt32 i = 0; i < s_nbPasses; ++i)
	for (nbInt32 j = 0; j < SwapChainBufferCount; ++j)
	{
		HRESULT hr = D3D12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, 
			IID_PPV_ARGS(&m_vertexShaderSharedCBUploadHeaps[i][j]));

		NEBULA_ASSERT(SUCCEEDED(hr));
		m_vertexShaderSharedCBUploadHeaps[i][j]->SetName(L"SimpleColorEffect : Vertex shader Constant Buffer Upload heap");

		hr = m_vertexShaderSharedCBUploadHeaps[i][j]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_vertexShaderSharedCBGPUAddress[i][j]));
		NEBULA_ASSERT(SUCCEEDED(hr));
	}
}

void HighlightColor::updateVertexShaderSharedCB(HighlightColorPushArgs& data, nbInt32 frameIndex, nbInt32 passIndex)
{
	auto& camera = data.scene.getCamera();
	VertexShaderSharedCB vertexShaderCB;

	// Eye vector
	glm::vec3 eyePosition = camera->getPosition();
	vertexShaderCB.eyePosition = { eyePosition.x, eyePosition.y, eyePosition.z };

	vertexShaderCB.scale = 0.002f * camera->getFovy().getValue();
	if (passIndex > 0)
		vertexShaderCB.scale *= -1.0f;

	memcpy(m_vertexShaderSharedCBGPUAddress[passIndex][frameIndex], &vertexShaderCB, sizeof(VertexShaderSharedCB));
}

void HighlightColor::pushDrawCommands(HighlightColorPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex)
{
	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetGraphicsRootConstantBufferView(0, CameraConstantBufferSingleton::instance()->getUploadtHeaps()[frameIndex]->GetGPUVirtualAddress());

	const auto& meshByGroup = data.scene.getModel()->getMeshesByGroup();

	for (nbInt32 i = 0; i < s_nbPasses; ++i)
	{
		updateVertexShaderSharedCB(data, frameIndex, i);

		commandList->SetPipelineState(m_PSOs[i]);
		commandList->SetGraphicsRootConstantBufferView(1, m_vertexShaderSharedCBUploadHeaps[i][frameIndex]->GetGPUVirtualAddress());

		{
			const auto meshByGroupIter = meshByGroup.find(data.selection->getGroupId());
			const nbUint64 groupPosInCB = std::distance(meshByGroup.begin(), meshByGroupIter) * MeshGroupConstantBuffer::VertexShaderCBAlignedSize;
			commandList->SetGraphicsRootConstantBufferView(2, MeshGroupConstantBufferSingleton::instance()->getDefaultHeap()->GetGPUVirtualAddress() + groupPosInCB);
		}

		const auto dx12Model = static_cast<const DX12Model*>(data.scene.getModel().get());
		const auto& groupHandles = dx12Model->getMeshHandlesByGroup();

		auto groupHandleIter = groupHandles.find(data.selection->getGroupId());
		NEBULA_ASSERT(groupHandleIter != groupHandles.end());

		for (auto* mesh : groupHandleIter->second)
		{
			commandList->IASetVertexBuffers(0, 1, &mesh->vertexBuffer.bufferView);
			commandList->IASetIndexBuffer(&mesh->indexBuffer.bufferView);
			commandList->DrawIndexedInstanced(mesh->nbIndices, 1, 0, 0, 0);
		}
	}
}
}}}}}