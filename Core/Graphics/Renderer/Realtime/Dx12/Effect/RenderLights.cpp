//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
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

	for (int i = 0; i < swapChainBufferCount; ++i)
		m_vShaderCenterCBUploadHeaps[i] = nullptr;
}

RenderLights::~RenderLights()
{
	// Heaps
	for (int i = 0; i < swapChainBufferCount; ++i)
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
	int paramIdx;
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

	// create root signature
	createRootSignature(rootParameters, _countof(rootParameters));
}

void RenderLights::initPipelineStateObjects()
{
	D3D12_INPUT_ELEMENT_DESC inputLayoutElement[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	compile(m_PSO,
		std::wstring(L"Billboard_VS.hlsl"),
		std::wstring(L"Billboard_PS.hlsl"),
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC),
		nullptr
	);

	m_PSO->SetName(L"Render lights PSO");
}

void RenderLights::initVertexShaderSharedCB()
{
	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		HRESULT hr = D3d12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vShaderSharedCBUploadHeaps[i]));

		ORION_ASSERT(SUCCEEDED(hr));

		m_vShaderSharedCBUploadHeaps[i]->SetName(L"RenderLightsEffect : Vertex shader static constant Buffer Upload heap");

		hr = m_vShaderSharedCBUploadHeaps[i]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_vShaderSharedCBGPUAddress[i]));
		ORION_ASSERT(SUCCEEDED(hr));
	}
}

void RenderLights::initVertexShaderCenterCB(const Graphics::Scene::BaseScene& scene)
{
	if (!scene.hasLights())
		return;

	const auto& lights = scene.getLights();
	const uint32_t nbLights = (uint32_t)lights.size();

	// 0 : Init heaps.
	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		if (m_vShaderCenterCBUploadHeaps[i])
			m_vShaderCenterCBUploadHeaps[i]->Release();

		// Create upload heap
		HRESULT hr = D3d12Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(VtxShaderCenterCBAlignedSize * nbLights),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vShaderCenterCBUploadHeaps[i]));

		ORION_ASSERT(SUCCEEDED(hr));
		m_vShaderCenterCBUploadHeaps[i]->SetName(L"RenderLightsEffect : Center Constant Buffer Upload heap");

		hr = m_vShaderCenterCBUploadHeaps[i]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_vShaderCenterCBGPUAddress[i]));
		ORION_ASSERT(SUCCEEDED(hr));
	}
}

void RenderLights::initVertexAndIndexBuffer()
{
	ORION_ASSERT(GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->createVertexBuffer(
		Billboard::s_billboardVertices, m_vtxBuffer)
	);

	ORION_ASSERT(GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->createIndexBuffer(
		Billboard::s_billboardIndices, m_idxBuffer)
	);
}

void RenderLights::initTextures()
{
	using Graphics::Light::LightType;

	auto initTexture = [this](LightType lType, const std::string& path){
		try
		{
			m_images.emplace(lType, new Texture::RGBAImage(path)) ;
			GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS>->createRGBATexture2D(m_images[lType], m_textures[lType]);
		}
		catch(Texture::CreateImageException)
		{
			if (m_images[lType])
				delete m_images[lType];

			m_images[lType] = nullptr;
		}
		catch(CreateTextureException)
		{
			ORION_ASSERT(false);
		}
	};

	initTexture(Directionnal, ORION_CORE_FOLDER + "Resource/dirLight.jpg");
	initTexture(Point, ORION_CORE_FOLDER + "Resource/omniLight.jpg");
}

void RenderLights::updateVertexShaderCenterCB(struct RenderLightsPushArgs& data, int frameIndex)
{
	int i = 0;
	for (auto& light : data.scene.getLights())
	{
		const glm::vec3& lightPos = light.second->getPosition();
		auto centerCameraSpace = data.scene.getCamera()->getViewMatrix() * glm::vec4(lightPos.x, lightPos.y, lightPos.z, 1.0f);

		VertexShaderCenterCB buffer;
		buffer.centerCameraSpace = { centerCameraSpace.x, centerCameraSpace.y, centerCameraSpace.z, 1.0f};

		uint64_t posInCB = i * VtxShaderCenterCBAlignedSize;
		memcpy(m_vShaderCenterCBGPUAddress[frameIndex] + posInCB, &buffer, sizeof(VertexShaderCenterCB));

		++i;
	}
}

void RenderLights::updateVertexShaderSharedCB(RenderLightsPushArgs& data, int frameIndex)
{
	auto projMatrix = data.scene.getCamera()->getDirectXPerspectiveMatrix(DX12Renderer::s_screenAspectRatio);
	projMatrix = XMMatrixTranspose(projMatrix);

	VertexShaderSharedCB vertexShaderCB;
	XMStoreFloat4x4(&vertexShaderCB.projMatrix, projMatrix);
	vertexShaderCB.billboardScale = BaseLight::s_billboardSize;

	memcpy(m_vShaderSharedCBGPUAddress[frameIndex], &vertexShaderCB, sizeof(VertexShaderSharedCB));
}

void RenderLights::pushDrawCommands(RenderLightsPushArgs& data, ID3D12GraphicsCommandList* commandList, int frameIndex)
{
	using Graphics::Light::LightType;

	commandList->SetPipelineState(m_PSO);
	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	updateVertexShaderSharedCB(data, frameIndex);
	updateVertexShaderCenterCB(data, frameIndex);

	commandList->SetGraphicsRootConstantBufferView(0, m_vShaderSharedCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

	auto& lights = data.scene.getLights();

	int i = 0;
	for (auto& lightIter : lights)
	{
		commandList->SetGraphicsRootConstantBufferView(1, i * VtxShaderCenterCBAlignedSize + m_vShaderCenterCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

		auto tex = m_textures.find(lightIter.second->getType());
		ORION_ASSERT(tex != m_textures.end());

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
