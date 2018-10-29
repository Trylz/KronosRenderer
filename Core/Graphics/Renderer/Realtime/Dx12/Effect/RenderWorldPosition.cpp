//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"

#include "CameraConstantBuffer.h"
#include "RenderWorldPosition.h"
#include "Graphics/Renderer/Realtime/Dx12/Dx12Renderer.h"
#include <dxgi1_4.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

RenderWorldPosition::RenderWorldPosition(const DXGI_SAMPLE_DESC& sampleDesc)
: BaseEffect(sampleDesc)
{
	initRootSignature();
	initPipelineStateObjects();
}

void RenderWorldPosition::initRootSignature()
{
	D3D12_ROOT_PARAMETER rootParameters[2];

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;

	for (nbUint32 i = 0u; i < 2; ++i)
	{
		rootCBVDescriptor.ShaderRegister = i;
		rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[i].Descriptor = rootCBVDescriptor;
		rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	}

	createRootSignature(rootParameters, _countof(rootParameters));
}

void RenderWorldPosition::initPipelineStateObjects()
{
	// Compile shaders
	ID3DBlob* vertexShader = compileShader(std::wstring(L"WorldPosition_VS.hlsl"), true);
	ID3DBlob* pixelShader = compileShader(std::wstring(L"WorldPosition_PS.hlsl"), false);

	// Compile PSOs
	D3D12_INPUT_ELEMENT_DESC inputLayoutElement[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	compilePipeline(m_PSO,
		vertexShader,
		pixelShader,
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC),
		CD3DX12_BLEND_DESC(D3D12_DEFAULT),
		CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
		CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
		std::vector<DXGI_FORMAT>{ NEBULA_WORLD_POSITION_RT_FORMAT }
	);
}

void RenderWorldPosition::pushDrawCommands(RenderWorldPositionPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex)
{
	commandList->SetPipelineState(m_PSO);
	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetGraphicsRootConstantBufferView(0, CameraConstantBufferSingleton::instance()->getUploadtHeaps()[frameIndex]->GetGPUVirtualAddress());

	const auto* dx12Model = static_cast<const DX12Model*>(data.scene.getModel().get());
	const auto& meshByGroup = dx12Model->getMeshesByGroup();

	for (auto& group : dx12Model->getMeshHandlesByGroup())
	{
		const auto meshByGroupIter = meshByGroup.find(group.first);
		const nbUint64 groupPosInCB = std::distance(meshByGroup.begin(), meshByGroupIter) * MeshGroupConstantBuffer::VertexShaderCBAlignedSize;
		commandList->SetGraphicsRootConstantBufferView(1, MeshGroupConstantBufferSingleton::instance()->getDefaultHeap()->GetGPUVirtualAddress() + groupPosInCB);

		// Draw meshes.
		for (auto* mesh : group.second)
		{
			commandList->IASetVertexBuffers(0, 1, &mesh->vertexBuffer.bufferView);
			commandList->IASetIndexBuffer(&mesh->indexBuffer.bufferView);

			commandList->DrawIndexedInstanced(mesh->nbIndices, 1, 0, 0, 0);
		}
	}
}
}}}}}