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

#define NEBULA_UNIFORM_SCALE_PASS_IDX 0
#define NEBULA_ORIENTED_SCALE_PASS_IDX 1
#define NEBULA_NO_SCALE_PASS_IDX 2

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

HighlightColor::HighlightColor(const DXGI_SAMPLE_DESC& sampleDesc, ResourceArray& loadingResources, ID3D12GraphicsCommandList* commandList)
: BaseEffect(sampleDesc)
{
	NEBULA_ASSERT(s_nbPasses == 3);
	NEBULA_ASSERT(s_maxColor == 2);

	initRootSignature();
	initPipelineStateObjects();
	initPixelShaderCB(loadingResources, commandList);

	for (nbUint32 i = 0u; i < s_maxColor; ++i)
	for (nbUint32 j = 0u; j < s_nbPasses; ++j)
	for (nbUint32 k = 0u; k < SwapChainBufferCount; ++k)
	{
		m_vertexShaderCBUploadHeaps[i][j][k] = nullptr;
	}
}

HighlightColor::~HighlightColor()
{
	freeVertexShaderCBs();
}

void HighlightColor::freeVertexShaderCBs()
{
	for (nbUint32 i = 0u; i < s_maxColor; ++i)
	for (nbUint32 j = 0u; j < s_nbPasses; ++j)
	for (nbUint32 k = 0u; k < SwapChainBufferCount; ++k)
	{
		NEBULA_DX12_SAFE_RELEASE(m_vertexShaderCBUploadHeaps[i][j][k]);
	}
}

void HighlightColor::initRootSignature()
{
	D3D12_ROOT_PARAMETER rootParameters[4];

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;

	for (nbUint32 i = 0u; i < 4; ++i)
	{
		rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	
		if (i == 3)
		{
			rootCBVDescriptor.ShaderRegister = 0;
			rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		}
		else
		{
			rootCBVDescriptor.ShaderRegister = i;
			rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		}

		rootParameters[i].Descriptor = rootCBVDescriptor;
	}

	createRootSignature(rootParameters, _countof(rootParameters));
}

void HighlightColor::initPipelineStateObjects()
{
	// Compile shaders
	// Vertex shader
	auto uniformScaleDef = std::to_string(NEBULA_UNIFORM_SCALE_PASS_IDX);
	auto orientedScaleDef = std::to_string(NEBULA_ORIENTED_SCALE_PASS_IDX);
	auto noScaleDef = std::to_string(NEBULA_NO_SCALE_PASS_IDX);

	D3D_SHADER_MACRO macros[] = {   "UNIFORM_SCALE_PASS_IDX", uniformScaleDef.c_str(),
									"ORIENTED_SCALE_PASS_IDX", orientedScaleDef.c_str(),
									"NO_SCALE_PASS_IDX", noScaleDef.c_str(), NULL, NULL
	};


	ID3DBlob* vertexShader = compileShader(std::wstring(L"HighlightColor_VS.hlsl"), true, macros);

	// Pixel shader
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
	dsDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_ZERO;
	dsDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// Back-facing pixels.
	dsDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_ZERO;
	dsDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

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

void HighlightColor::resetBuffers(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList)
{
	// Free memory first.
	m_groupVertexShaderCBPositions.clear();
	freeVertexShaderCBs();

	// Now create new buffers.
	const auto meshGroups = scene.getModel()->getMeshGroups();
	const nbUint32 bufferSize = (nbUint32)meshGroups.size();
	if (!bufferSize)
	{
		NEBULA_TRACE("HighlightColor::resetBuffers : no buffer returning");
		return;
	}

	for (nbUint32 i = 0u; i < s_maxColor; ++i)
	for (nbUint32 j = 0u; j < s_nbPasses; ++j)
	for (nbUint32 k = 0u; k < SwapChainBufferCount; ++k)
	{
		// Upload heap
		ID3D12Resource* uploadHeap = nullptr;

		HRESULT hr = D3D12Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(VertexShaderCBAlignedSize * bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadHeap));

		NEBULA_ASSERT(SUCCEEDED(hr));
		uploadHeap->SetName(L"HighlightColor constant buffer upload heap");
		m_vertexShaderCBUploadHeaps[i][j][k] = uploadHeap;

		hr = uploadHeap->Map(0, &ReadRangeGPUOnly, reinterpret_cast<void**>(&m_vertexShaderCBGPUAddress[i][j][k]));
		NEBULA_ASSERT(SUCCEEDED(hr));
	}

	// Init the CB positions.
	for (const EntityIdentifier& groupId : meshGroups)
	{
		m_groupVertexShaderCBPositions.emplace(groupId, (nbUint32)m_groupVertexShaderCBPositions.size() * VertexShaderCBAlignedSize);
	}
}

void HighlightColor::initPixelShaderCB(ResourceArray& loadingResources, ID3D12GraphicsCommandList* commandList)
{
	for (nbUint32 i = 0u; i < s_maxColor; ++i)
	{
		// Create upload heap
		ID3D12Resource* uploadHeap;
		UINT8* uploadHeapCBGPUAddress;

		HRESULT hr = D3D12Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(PixelShaderCBAlignedSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadHeap));

		NEBULA_ASSERT(SUCCEEDED(hr));
		uploadHeap->SetName(L"Hight Color Constant Buffer Upload heap");

		// Add created upload heap to the loading resources
		loadingResources.push_back(uploadHeap);

		// Map upload heap
		hr = uploadHeap->Map(0, &ReadRangeGPUOnly, reinterpret_cast<void**>(&uploadHeapCBGPUAddress));
		NEBULA_ASSERT(SUCCEEDED(hr));

		// Create default heap
		hr = D3D12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(PixelShaderCBAlignedSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_pixelShaderCBDefaultHeaps[i]));

		NEBULA_ASSERT(SUCCEEDED(hr));
		m_pixelShaderCBDefaultHeaps[i]->SetName(L"HighlightColor : Pixel shader Constant Buffer Default heap");

		PixelShaderCB pixelShaderCB;
		pixelShaderCB.color = i == 0u ? XMFLOAT4(AquaRGBColor.x, AquaRGBColor.y, AquaRGBColor.z, 0.0f) 
		: XMFLOAT4(LimeRGBColor.x, LimeRGBColor.y, LimeRGBColor.z, 0.0f);

		memcpy(uploadHeapCBGPUAddress, &pixelShaderCB, sizeof(PixelShaderCB));

		// Copy upload heap to default
		commandList->CopyResource(m_pixelShaderCBDefaultHeaps[i], uploadHeap);

		// Transition to pixel shader resource.
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pixelShaderCBDefaultHeaps[i], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}
}

void HighlightColor::updateVertexShaderSharedCB(HighlightColorPushArgs& data,
	ID3D12GraphicsCommandList* commandList,
	const Model::DatabaseMeshGroupPtr& meshGroup,
	nbUint32 objectIdx,
	nbUint32 frameIndex,
	nbUint32 passIndex)
{
	const auto& camera = data.scene.getCamera();
	const glm::vec3 eyePosition = camera->getPosition();

	VertexShaderCB vertexShaderCB;
	vertexShaderCB.passIdx = passIndex;
	vertexShaderCB.eyePosition = { eyePosition.x, eyePosition.y, eyePosition.z };

	const nbFloat32 distanceFactor = glm::length(eyePosition - meshGroup->getPosition());
	const nbFloat32 baseFactor = camera->getFovy().getValue();

	if (passIndex == NEBULA_UNIFORM_SCALE_PASS_IDX)
	{
		const auto bounds = data.scene.getModel()->getTransformedGroupBounds(meshGroup->getIdentifier());
		vertexShaderCB.scale = 1.0f + (baseFactor * distanceFactor) / (glm::length(bounds.getSize()) * 110.0f);
	}
	else if (passIndex == NEBULA_ORIENTED_SCALE_PASS_IDX)
	{
		vertexShaderCB.scale = 0.0028f * baseFactor;
	}

	// Update GPU data.
	memcpy(m_vertexShaderCBGPUAddress[objectIdx][passIndex][frameIndex] + m_groupVertexShaderCBPositions[meshGroup->getIdentifier()],
		&vertexShaderCB, sizeof(VertexShaderCB));
}

void HighlightColor::pushDrawCommands(HighlightColorPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex)
{
	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetGraphicsRootConstantBufferView(0, CameraConstantBufferSingleton::instance()->getUploadtHeaps()[frameIndex]->GetGPUVirtualAddress());
	
	for (nbUint32 i = 0u; i < s_maxColor; ++i)
	{
		if (i == 0)
		{
			for (const auto& entityId : data.scene.getCurrentSelection().m_entities)
			{
				pushDrawCommandsInternal(data, entityId, commandList, i, frameIndex);
			}
		}
		else if (const auto& entityId = data.scene.getSecondaryHightlightEntity())
		{
			pushDrawCommandsInternal(data, entityId, commandList, i, frameIndex);
		}
	}
}

void HighlightColor::pushDrawCommandsInternal(HighlightColorPushArgs& data, const EntityIdentifier& entityId, ID3D12GraphicsCommandList* commandList, nbInt32 colorIndex, nbInt32 frameIndex)
{
	const Model::DatabaseMeshGroupPtr meshGroup = Model::getMeshGroupFromEntity(entityId);
	NEBULA_ASSERT(meshGroup);

	commandList->SetGraphicsRootConstantBufferView(3, m_pixelShaderCBDefaultHeaps[colorIndex]->GetGPUVirtualAddress());

	for (nbUint32 j = 0u; j < s_nbPasses; ++j)
	{
		updateVertexShaderSharedCB(data, commandList, meshGroup, colorIndex, frameIndex, j);

		if (j == NEBULA_NO_SCALE_PASS_IDX)
			commandList->SetPipelineState(m_PSOs[1]);
		else
			commandList->SetPipelineState(m_PSOs[0]);

		commandList->SetGraphicsRootConstantBufferView(1, m_vertexShaderCBUploadHeaps[colorIndex][j][frameIndex]->GetGPUVirtualAddress() + m_groupVertexShaderCBPositions[entityId]);
		commandList->SetGraphicsRootConstantBufferView(2, MeshGroupConstantBufferSingleton::instance()->getMeshGroupGPUVirtualAddress(entityId));

		const auto dx12Model = static_cast<const DX12Model*>(data.scene.getModel().get());
		const auto& groupHandles = dx12Model->getMeshHandlesByGroup();

		auto groupHandleIter = groupHandles.find(entityId);
		NEBULA_ASSERT(groupHandleIter != groupHandles.end());

		for (auto* meshHandle : groupHandleIter->second)
		{
			commandList->IASetVertexBuffers(0, 1, &meshHandle->vertexBuffer.bufferView);
			commandList->IASetIndexBuffer(&meshHandle->indexBuffer.bufferView);
			commandList->DrawIndexedInstanced(meshHandle->nbIndices, 1, 0, 0, 0);
		}
	}
}

}}}}}