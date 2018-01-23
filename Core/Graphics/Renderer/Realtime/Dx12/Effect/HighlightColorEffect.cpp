//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

#include "stdafx.h"

#include "HighlightColorEffect.h"
#include "Graphics/Renderer/Realtime/Dx12/Dx12Renderer.h"
#include <dxgi1_4.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

HighlightColorEffect::HighlightColorEffect(const SharedDevicePtr& device, const DXGI_SAMPLE_DESC& sampleDesc)
: BaseEffect(device, sampleDesc)
{
	initRootSignature();
	initPipelineStateObjects();
	initVertexShaderCB();
}

void HighlightColorEffect::initRootSignature()
{
	D3D12_ROOT_PARAMETER rootParameter;

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter.Descriptor = rootCBVDescriptor;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	createRootSignature(&rootParameter, 1);
}

void HighlightColorEffect::initPipelineStateObjects()
{
	D3D12_INPUT_ELEMENT_DESC inputLayoutElement[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

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

	compile(m_PSOs[0],
		std::wstring(L"HighlightColor_VS.hlsl"),
		std::wstring(L"HighlightColor_PS.hlsl"),
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC),
		nullptr,
		CD3DX12_BLEND_DESC(D3D12_DEFAULT),
		dsDesc
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

	// TODO: Share shaders bytecode with previous compile.
	compile(m_PSOs[1],
		std::wstring(L"HighlightColor_VS.hlsl"),
		std::wstring(L"HighlightColor_PS.hlsl"),
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC),
		nullptr,
		CD3DX12_BLEND_DESC(D3D12_DEFAULT),
		dsDesc
	);
}

void HighlightColorEffect::initVertexShaderCB()
{
	HRESULT hr;

	for (int i = 0; i < s_nbPasses; ++i)
	for (int j = 0; j < swapChainBufferCount; ++j)
	{
		hr = m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, 
			IID_PPV_ARGS(&m_vertexShaderCBUploadHeaps[i][j]));

		ORION_ASSERT(SUCCEEDED(hr));

		m_vertexShaderCBUploadHeaps[i][j]->SetName(L"SimpleColorEffect : Vertex shader Constant Buffer Upload heap");
		ZeroMemory(&m_vertexShaderCB, sizeof(VertexShaderCB));

		hr = m_vertexShaderCBUploadHeaps[i][j]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_vertexShaderCBGPUAddress[i][j]));
		ORION_ASSERT(SUCCEEDED(hr));

		memcpy(m_vertexShaderCBGPUAddress[i][j], &m_vertexShaderCB, sizeof(m_vertexShaderCB));
	}
}

void HighlightColorEffect::updateVertexShaderCB(HighlightColorEffectPushArgs& data, int frameIndex, int passIndex)
{
	auto& camera = data.scene.getCamera();

	// MVP
	XMStoreFloat4x4(&m_vertexShaderCB.wvpMat,
		camera->getTransposedMVP(DX12Renderer::s_screenAspectRatio));

	// Eye vector
	glm::vec3 eyePosition = camera->getPosition();
	m_vertexShaderCB.eyePosition = { eyePosition.x, eyePosition.y, eyePosition.z };

	m_vertexShaderCB.scale = 0.002f * camera->getFovy().getValue();
	if (passIndex > 0)
		m_vertexShaderCB.scale *= -1.0f;

	memcpy(m_vertexShaderCBGPUAddress[passIndex][frameIndex], &m_vertexShaderCB, sizeof(VertexShaderCB));
}

void HighlightColorEffect::pushDrawCommands(HighlightColorEffectPushArgs& data, ID3D12GraphicsCommandList* commandList, int frameIndex)
{
	commandList->OMSetStencilRef(0);

	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (int i = 0; i < s_nbPasses; ++i)
	{
		updateVertexShaderCB(data, frameIndex, i);

		commandList->SetPipelineState(m_PSOs[i]);
		commandList->SetGraphicsRootConstantBufferView(0, m_vertexShaderCBUploadHeaps[i][frameIndex]->GetGPUVirtualAddress());

		auto dx12Model = static_cast<const DX12Model*>(data.scene.getModel().get());

		const auto& groups = dx12Model->getMeshHandlesByGroup();

		auto group = groups.find(data.selection);
		ORION_ASSERT(group != groups.end());

		for (auto* mesh : group->second)
		{
			commandList->IASetVertexBuffers(0, 1, &mesh->vertexBuffer.bufferView);
			commandList->IASetIndexBuffer(&mesh->indexBuffer.bufferView);
			commandList->DrawIndexedInstanced(mesh->nbIndices, 1, 0, 0, 0);
		}
	}
}
}}}}}