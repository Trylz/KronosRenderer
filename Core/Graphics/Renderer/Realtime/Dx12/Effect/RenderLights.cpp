//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"

#include "Graphics/Billboard.h"
#include "Graphics/Light/DirectionnalLight.h"
#include "Graphics/Renderer/Realtime/CreateTextureException.h"
#include "Graphics/Renderer/Realtime/Dx12/DX12Renderer.h"
#include "RenderLights.h"

#include <dxgi1_4.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;
using namespace Graphics::Light;

RenderLights::RenderLights(const DXGI_SAMPLE_DESC& sampleDesc)
: BaseEffect(sampleDesc)
{
	initRootSignature();
	initPipelineStateObjects();
	initVertexShaderSharedCB();

	initVertexAndIndexBuffer();
	initTextures();

	MAKE_SWAP_CHAIN_ITERATOR_I
		m_vShaderCenterCBUploadHeaps[i] = nullptr;
}

RenderLights::~RenderLights()
{
	// Heaps
	MAKE_SWAP_CHAIN_ITERATOR_I
	{
		if (m_vShaderCenterCBUploadHeaps[i])
			m_vShaderCenterCBUploadHeaps[i]->Release();
	}

	// Buffers
	GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->releaseVertexBuffer(m_vtxBuffer);
	GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->releaseIndexBuffer(m_idxBuffer);

	// Textures
	for (auto& tex : m_textures)
	{
		if (!tex.second.descriptorHandle.isEmpty())
			GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->releaseTexture(tex.second);
	}

	// Images
	for (auto& im : m_images)
	{
		if (im.second)
			delete im.second;
	}
}

void RenderLights::initRootSignature()
{
	D3D12_ROOT_PARAMETER rootParameters[3];

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;

	// 0 & 1 : vertex shader constant buffers
	nbInt32 paramIdx;
	for (paramIdx = 0; paramIdx < 2; ++paramIdx)
	{
		rootCBVDescriptor.ShaderRegister = paramIdx;
		rootParameters[paramIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[paramIdx].Descriptor = rootCBVDescriptor;
		rootParameters[paramIdx].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	}

	// 2 : Root parameter for the texture.
	D3D12_DESCRIPTOR_RANGE descriptorTableRange;
	descriptorTableRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorTableRange.NumDescriptors = 1;
	descriptorTableRange.RegisterSpace = 0;
	descriptorTableRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	descriptorTableRange.BaseShaderRegister = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = 1;
	descriptorTable.pDescriptorRanges = &descriptorTableRange;

	rootParameters[paramIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; 
	rootParameters[paramIdx].DescriptorTable = descriptorTable;
	rootParameters[paramIdx].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Create root signature
	createRootSignature(rootParameters, _countof(rootParameters));
}

void RenderLights::initPipelineStateObjects()
{
	D3D12_INPUT_ELEMENT_DESC inputLayoutElement[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	ID3DBlob* vertexShader = compileShader(std::wstring(L"Billboard_VS.hlsl"), true);
	ID3DBlob* pixelShader = compileShader(std::wstring(L"Billboard_PS.hlsl"), false);

	// The rasterizer desc
	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizerDesc.MultisampleEnable = TRUE;

	// Depth stencil description
	D3D12_DEPTH_STENCIL_DESC dsDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	dsDesc.StencilEnable = TRUE;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Front-facing pixels.
	dsDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	// Back-facing pixels.
	dsDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	compilePipeline(m_PSO,
		vertexShader,
		pixelShader,
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC),
		CD3DX12_BLEND_DESC(D3D12_DEFAULT),
		dsDesc,
		rasterizerDesc
	);

	m_PSO->SetName(L"Render lights PSO");
}

void RenderLights::initVertexShaderSharedCB()
{
	MAKE_SWAP_CHAIN_ITERATOR_I
	{
		HRESULT hr = D3D12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vShaderSharedCBUploadHeaps[i]));

		NEBULA_ASSERT(SUCCEEDED(hr));

		m_vShaderSharedCBUploadHeaps[i]->SetName(L"RenderLights : Vertex shader constant buffer");

		hr = m_vShaderSharedCBUploadHeaps[i]->Map(0, &ReadRangeGPUOnly, reinterpret_cast<void**>(&m_vShaderSharedCBGPUAddress[i]));
		NEBULA_ASSERT(SUCCEEDED(hr));
	}
}

void RenderLights::initVertexShaderCenterCB(const Scene::BaseScene& scene)
{
	if (!scene.hasLights())
		return;

	const auto& lights = scene.getLights();
	const nbUint32 nbLights = (nbUint32)lights.size();

	MAKE_SWAP_CHAIN_ITERATOR_I
	{
		if (m_vShaderCenterCBUploadHeaps[i])
			m_vShaderCenterCBUploadHeaps[i]->Release();

		// Create upload heap
		HRESULT hr = D3D12Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(VtxShaderCenterCBAlignedSize * nbLights),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vShaderCenterCBUploadHeaps[i]));

		NEBULA_ASSERT(SUCCEEDED(hr));
		m_vShaderCenterCBUploadHeaps[i]->SetName(L"RenderLightsEffect : Center Constant Buffer Upload heap");

		hr = m_vShaderCenterCBUploadHeaps[i]->Map(0, &ReadRangeGPUOnly, reinterpret_cast<void**>(&m_vShaderCenterCBGPUAddress[i]));
		NEBULA_ASSERT(SUCCEEDED(hr));
	}
}

void RenderLights::initVertexAndIndexBuffer()
{
	NEBULA_ASSERT(GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->createVertexBuffer(
		Billboard::s_billboardVertices, m_vtxBuffer)
	);

	NEBULA_ASSERT(GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->createIndexBuffer(
		Billboard::s_billboardIndices, m_idxBuffer)
	);
}

void RenderLights::initTextures()
{
	using Light::LightType;

	auto initTexture = [this](LightType lType, const std::string& path){
		try
		{
			m_images.emplace(lType, Texture::Helper::createRGBAImage(path)) ;
			GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->createTexture2D(m_images[lType], m_textures[lType]);
		}
		catch(Texture::CreateImageException&)
		{
			if (m_images[lType])
				delete m_images[lType];

			m_images[lType] = nullptr;
		}
		catch(CreateTextureException&)
		{
			NEBULA_ASSERT(false);
		}
	};

	initTexture(Directionnal, NEBULA_CORE_FOLDER + "Resource/dirLight.jpg");
	initTexture(Point, NEBULA_CORE_FOLDER + "Resource/omniLight.jpg");
}

void RenderLights::updateVertexShaderCenterCB(struct RenderLightsPushArgs& data, nbInt32 frameIndex)
{
	nbInt32 i = 0;
	for (auto& light : data.scene.getLights())
	{
		const glm::vec3& lightPos = light.second->getPosition();
		auto centerCameraSpace = data.scene.getCamera()->getViewMatrix() * glm::vec4(lightPos.x, lightPos.y, lightPos.z, 1.0f);

		VertexShaderCenterCB buffer;
		buffer.centerCameraSpace = { centerCameraSpace.x, centerCameraSpace.y, centerCameraSpace.z, 1.0f};

		nbUint64 posInCB = i * VtxShaderCenterCBAlignedSize;
		memcpy(m_vShaderCenterCBGPUAddress[frameIndex] + posInCB, &buffer, sizeof(VertexShaderCenterCB));

		++i;
	}
}

void RenderLights::updateVertexShaderSharedCB(RenderLightsPushArgs& data, nbInt32 frameIndex)
{
	auto projMatrix = data.scene.getCamera()->getDirectXPerspectiveMatrix();
	projMatrix = XMMatrixTranspose(projMatrix);

	VertexShaderSharedCB vertexShaderCB;
	XMStoreFloat4x4(&vertexShaderCB.projMatrix, projMatrix);
	vertexShaderCB.billboardScale = BaseLight::s_billboardSize;

	memcpy(m_vShaderSharedCBGPUAddress[frameIndex], &vertexShaderCB, sizeof(VertexShaderSharedCB));
}

void RenderLights::pushDrawCommands(RenderLightsPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex)
{
	using Light::LightType;

	commandList->SetPipelineState(m_PSO);
	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	updateVertexShaderSharedCB(data, frameIndex);
	updateVertexShaderCenterCB(data, frameIndex);

	commandList->SetGraphicsRootConstantBufferView(0, m_vShaderSharedCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

	nbInt32 i = 0;
	for (auto& lightIter : data.scene.getLights())
	{
		commandList->SetGraphicsRootConstantBufferView(1, i * VtxShaderCenterCBAlignedSize + m_vShaderCenterCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

		const auto tex = m_textures.find(lightIter.second->getType());
		NEBULA_ASSERT(tex != m_textures.end());

		if (!tex->second.descriptorHandle.isEmpty())
		{
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(tex->second.buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			commandList->SetGraphicsRootDescriptorTable(2, tex->second.descriptorHandle.getGpuHandle());
			commandList->IASetVertexBuffers(0, 1, &m_vtxBuffer.bufferView);
			commandList->IASetIndexBuffer(&m_idxBuffer.bufferView);
			commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(tex->second.buffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON));
		}

		++i;
	}
}
}}}}}
