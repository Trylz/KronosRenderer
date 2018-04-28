//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#include "stdafx.h"

#include "Graphics/Light/DirectionnalLight.h"
#include "Graphics/Light/OmniLight.h"
#include "Graphics/Material/DefaultDielectric.h"
#include "Graphics/Material/DefaultMetal.h"
#include "Graphics/Renderer/Realtime/Dx12/Dx12Renderer.h"
#include "ForwardLighning.h"
#include <dxgi1_4.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
using namespace DirectX;

ForwardLighning::ForwardLighning(const DXGI_SAMPLE_DESC& sampleDesc)
: BaseEffect(sampleDesc)
{
	initRootSignature();
	initPipelineStateObjects();
	initStaticConstantBuffers();

	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		m_pixelShaderMaterialCBUploadHeaps[i] = nullptr;
		m_pixelShaderMaterialCBDefaultHeaps[i] =  nullptr;
	}
}

ForwardLighning::~ForwardLighning()
{
	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		if (m_pixelShaderMaterialCBUploadHeaps[i])
			m_pixelShaderMaterialCBUploadHeaps[i]->Release();

		if (m_pixelShaderMaterialCBDefaultHeaps[i])
			m_pixelShaderMaterialCBDefaultHeaps[i]->Release();
	}
}

void ForwardLighning::onNewScene(const Graphics::Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList)
{
	initDynamicMaterialConstantBuffer(scene, commandList);
}

void ForwardLighning::onUpdateGroupMaterial(const Graphics::Scene::BaseScene& scene, const Graphics::Model::MeshGroupId& groupId, ID3D12GraphicsCommandList* commandList)
{
	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pixelShaderMaterialCBDefaultHeaps[i], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	}

	updateGroupMaterial(scene, groupId, commandList);

	fromMaterialUploadHeapToDefaulHeap(commandList);
}

void ForwardLighning::initRootSignature()
{
	D3D12_ROOT_PARAMETER rootParameters[5];

	// A root descriptor, which explains where to find the data for the parameter
	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	// 0 : Root parameter for the vertex shader constant buffer
	int paramIdx = 0;
	rootParameters[paramIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
	rootParameters[paramIdx].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
	rootParameters[paramIdx].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our vertex shader will be the only shader accessing this parameter for now
	++paramIdx;

	// 1 & 2 : Root parameter for the light and material pixel shader constant buffer
	for (int i = 0; i < 2; ++i, ++paramIdx)
	{
		rootCBVDescriptor.ShaderRegister = i;
		rootParameters[paramIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[paramIdx].Descriptor = rootCBVDescriptor;
		rootParameters[paramIdx].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}

	// 3 & 4 Root parameter for the pixel shader textures.
	// create a descriptor range and fill it out
	// this is a range of descriptors inside a descriptor heap
	D3D12_DESCRIPTOR_RANGE descriptorTableRanges[2]; // only one range right now

	for (int i = 0; i < 2; ++i, ++paramIdx)
	{
		descriptorTableRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // this is a range of shader resource views (descriptors)
		descriptorTableRanges[i].NumDescriptors = 1;
		descriptorTableRanges[i].RegisterSpace = 0; // space 0. can usually be zero
		descriptorTableRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables
		descriptorTableRanges[i].BaseShaderRegister = i; // start index of the shader registers in the range
		
		// create a descriptor table
		D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
		descriptorTable.NumDescriptorRanges = 1; // we only have one range
		descriptorTable.pDescriptorRanges = &descriptorTableRanges[i]; // the pointer to the beginning of our ranges array

		// fill out the parameter for our descriptor table. Remember it's a good idea to sort parameters by frequency of change. Our constant
		// buffer will be changed multiple times per frame, while our descriptor table will not be changed at all (in this tutorial)
		rootParameters[paramIdx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
		rootParameters[paramIdx].DescriptorTable = descriptorTable; // this is our descriptor table for this root parameter
		rootParameters[paramIdx].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // our pixel shader will be the only shader accessing this parameter for now
	}

	// create root signature
	createRootSignature(rootParameters, _countof(rootParameters));
}

void ForwardLighning::initPipelineStateObjects()
{
	// create input layout
	D3D12_INPUT_ELEMENT_DESC inputLayoutElement[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// The blending description
	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Effect macros
	auto maxLightDef = std::to_string(Graphics::Light::maxLightsPerScene);
	D3D_SHADER_MACRO macros[] = { "MAX_LIGHTS", maxLightDef.c_str(), NULL, NULL };

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
	dsDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER;

	// Back-facing pixels.
	dsDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER;

	compile(m_mainPSO,
		std::wstring(L"ForwardLightning_VS.hlsl"),
		std::wstring(L"ForwardLightning_PS.hlsl"),
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC),
		macros,
		blendDesc,
		dsDesc
	);
	
	m_mainPSO->SetName(L"Forward lightning main PSO");
}

void ForwardLighning::initStaticConstantBuffers()
{
	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		// 0 : Vertex shader constant Buffer Upload Resource Heap
		HRESULT hr = D3d12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&m_vertexShaderCBUploadHeaps[i]));

		KRONOS_ASSERT(SUCCEEDED(hr));

		m_vertexShaderCBUploadHeaps[i]->SetName(L"Vertex shader Constant Buffer Upload heap");
		KRONOS_ASSERT(SUCCEEDED(hr));

		// map the resource heap to get a gpu virtual address to the beginning of the heap
		hr = m_vertexShaderCBUploadHeaps[i]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_vertexShaderCBGPUAddress[i]));
		KRONOS_ASSERT(SUCCEEDED(hr));
		
		// 1 : Pixel shader lights constant Buffer Upload Resource Heap"
		//TODO: Put lights in a default heap instead since they does not change often(Not at all actually 2017/06/25)
		hr = D3d12Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(PixelShaderLightCBAlignedSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pixelShaderLightsCBUploadHeaps[i]));

		KRONOS_ASSERT(SUCCEEDED(hr));
		m_pixelShaderLightsCBUploadHeaps[i]->SetName(L"Pixel shader lights Constant Buffer Upload heap");

		hr = m_pixelShaderLightsCBUploadHeaps[i]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_pixelShaderLightsCBGPUAddress[i]));
		KRONOS_ASSERT(SUCCEEDED(hr));
	}
}

void ForwardLighning::initDynamicMaterialConstantBuffer(const Graphics::Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList)
{
	HRESULT hr;

	auto dx12Model = static_cast<const DX12Model*>(scene.getModel().get());
	const auto& groups = dx12Model->getMeshHandlesByGroup();

	// Will reserve GPU material memory for every group.
	const uint64_t nbGroups = groups.size();

	// 0 : Init heaps
	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		if (m_pixelShaderMaterialCBUploadHeaps[i])
			m_pixelShaderMaterialCBUploadHeaps[i]->Release();

		if (m_pixelShaderMaterialCBDefaultHeaps[i])
			m_pixelShaderMaterialCBDefaultHeaps[i]->Release();

		// Create upload heap
		hr = D3d12Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(PixelShaderMaterialCBAlignedSize * nbGroups),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pixelShaderMaterialCBUploadHeaps[i]));

		KRONOS_ASSERT(SUCCEEDED(hr));
		m_pixelShaderMaterialCBUploadHeaps[i]->SetName(L"Pixel shader material Constant Buffer Upload heap");

		hr = m_pixelShaderMaterialCBUploadHeaps[i]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_pixelShaderMaterialCBGPUAddress[i]));
		KRONOS_ASSERT(SUCCEEDED(hr));

		// Create default heap
		hr = D3d12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(PixelShaderMaterialCBAlignedSize * nbGroups),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_pixelShaderMaterialCBDefaultHeaps[i]));

		KRONOS_ASSERT(SUCCEEDED(hr));
		m_pixelShaderMaterialCBDefaultHeaps[i]->SetName(L"Pixel shader material Constant Buffer Default Resource heap");
	}

	// 1 : Update maerials in upload heap
	for (auto& group : groups)
	{
		updateGroupMaterial(scene, group.first, commandList);
	}

	// 2: Copy to default heap
	fromMaterialUploadHeapToDefaulHeap(commandList);
}

void ForwardLighning::fromMaterialUploadHeapToDefaulHeap(ID3D12GraphicsCommandList* commandList)
{
	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		// Copy upload heap to default
		commandList->CopyResource(m_pixelShaderMaterialCBDefaultHeaps[i], m_pixelShaderMaterialCBUploadHeaps[i]);

		// Transition to pixel shader resource.
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pixelShaderMaterialCBDefaultHeaps[i], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}
}

void ForwardLighning::updateGroupMaterial(const Graphics::Scene::BaseScene& scene, const Graphics::Model::MeshGroupId& groupId, ID3D12GraphicsCommandList* commandList)
{
	using namespace Graphics::Material;

	const DX12Model* dx12Model = static_cast<const DX12Model*>(scene.getModel().get());

	const auto& groups = dx12Model->getMeshHandlesByGroup();
	auto groupIter = groups.find(groupId);
	KRONOS_ASSERT(groupIter != groups.end());

	auto* mesh = groupIter->second[0];

	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		PixelShaderMaterialCB pixelShaderMaterialCB;
		auto& materialCB = pixelShaderMaterialCB.material;
		ZeroMemory(&pixelShaderMaterialCB, sizeof(pixelShaderMaterialCB));

		if (auto materialHandle = mesh->material.lock())
		{
			const BaseMaterial* material = dx12Model->getMaterial(materialHandle->matId);
			if (material->isFresnelMaterial())
			{
				const FresnelMaterial* fresnelMat = static_cast<const FresnelMaterial*>(material);

				materialCB.ambient = { fresnelMat->getAmbient().r, fresnelMat->getAmbient().g, fresnelMat->getAmbient().b, 1.0f };
				materialCB.shininess = fresnelMat->getShininess();
				materialCB.fresnel0 = fresnelMat->getFresnel0();
			}
			else
			{
				materialCB.diffuse = { defaultAmbient, defaultAmbient, defaultAmbient, 1.0f };
				materialCB.ambient = { defaultAmbient, defaultAmbient, defaultAmbient, 1.0f };
			}

			switch (material->getType())
			{
			case BaseMaterial::Type::DefaultDieletric:
			{
				const DefaultDielectric* dielectricMat = static_cast<const DefaultDielectric*>(material);
				materialCB.diffuse = { dielectricMat->getDiffuse().r, dielectricMat->getDiffuse().g, dielectricMat->getDiffuse().b, 1.0f };
				materialCB.specular = { dielectricMat->getSpecular().r, dielectricMat->getSpecular().g, dielectricMat->getSpecular().b, 1.0f };
				materialCB.emissive = { dielectricMat->getEmissive().r, dielectricMat->getEmissive().g, dielectricMat->getEmissive().b, 1.0f };
			}break;

			case BaseMaterial::Type::DefaultMetal:
			{
				const DefaultMetal* metalMat = static_cast<const DefaultMetal*>(material);
				materialCB.diffuse = { metalMat->getReflectance().r, metalMat->getReflectance().g, metalMat->getReflectance().b, 1.0f };
			}break;
			}

			materialCB.opacity = material->getOpacity();
			materialCB.type = static_cast<INT>(material->getType());

			materialCB.hasDiffuseTex = materialHandle->diffuseTexture.isValid();
			materialCB.hasSpecularTex = materialHandle->specularTexture.isValid();
		}

		uint64_t posInCB = std::distance(groups.begin(), groupIter) * PixelShaderMaterialCBAlignedSize;
		memcpy(m_pixelShaderMaterialCBGPUAddress[i] + posInCB, &pixelShaderMaterialCB, sizeof(PixelShaderMaterialCB));
	}
}

void ForwardLighning::updateVertexShaderCB(ForwardLightningPushArgs& data, int frameIndex)
{
	VertexShaderCB vertexShaderCB;

	XMStoreFloat4x4(&vertexShaderCB.wvpMat,
		data.scene.getCamera()->getDirectXTransposedMVP());

	memcpy(m_vertexShaderCBGPUAddress[frameIndex], &vertexShaderCB, sizeof(VertexShaderCB));
}

void ForwardLighning::updatePixelShaderLightsCB(ForwardLightningPushArgs& data, int frameIndex)
{
	const auto& lights = data.scene.getLights();

	const uint32_t lightSize = static_cast<uint32_t>(lights.size());
	m_pixelShaderLightsCB.nbLights = lightSize;

	const glm::vec3 cameraPos = data.scene.getCamera().get()->getPosition();
	m_pixelShaderLightsCB.eyePosition = { cameraPos.x, cameraPos.y, cameraPos.z, 0.0f};

	const auto& ambientColor = data.scene.getAmbientColor();
	m_pixelShaderLightsCB.sceneAmbient = { ambientColor.x, ambientColor.y, ambientColor.z, 0.0f};

	int pos = 0;

	for (auto& lightIter : lights)
	{
		auto& dx12Light = m_pixelShaderLightsCB.lights[pos];
		auto lightType = lightIter.second->getType();

		dx12Light.type = static_cast<int>(lightType);

		if (lightIter.second->getType() == Graphics::Light::LightType::Point)
		{
			auto* omniLight = static_cast<Graphics::Light::OmniLight*>(lightIter.second.get());

			const auto& pos = omniLight->getPosition();
			dx12Light.position = { pos.x, pos.y, pos.z, 0.0f };
			dx12Light.range = omniLight->getRange();
		}
		else if (lightIter.second->getType() == Graphics::Light::LightType::Directionnal)
		{
			auto* directionnalLight = static_cast<Graphics::Light::DirectionnalLight*>(lightIter.second.get());
			const auto& direction = directionnalLight->getNormalizedDirection();

			dx12Light.direction = { direction.x, direction.y, direction.z, 0.0f };
		}

		const auto& color = lightIter.second->getFinalColor();
		dx12Light.color = { color.r, color.g, color.r, 0.0f };

		++pos;
	}

	memcpy(m_pixelShaderLightsCBGPUAddress[frameIndex], &m_pixelShaderLightsCB, sizeof(PixelShaderLightsCB));
}

void ForwardLighning::pushDrawCommands(ForwardLightningPushArgs& data, ID3D12GraphicsCommandList* commandList, int frameIndex)
{
	//Set Pso
	commandList->SetPipelineState(m_mainPSO);

	// set root signature
	commandList->SetGraphicsRootSignature(m_rootSignature);

	// set the primitive topology
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// update sared constant buffers
	updateVertexShaderCB(data, frameIndex);
	updatePixelShaderLightsCB(data, frameIndex);

	// set shared constant buffers views
	commandList->SetGraphicsRootConstantBufferView(0, m_vertexShaderCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, m_pixelShaderLightsCBUploadHeaps[frameIndex]->GetGPUVirtualAddress());

	auto dx12Model = static_cast<const DX12Model*>(data.scene.getModel().get());

	uint32_t materialIncrement = 0u;

	const auto& groups = dx12Model->getMeshHandlesByGroup();
	for (auto& group : groups)
	{
		commandList->SetGraphicsRootConstantBufferView(2, m_pixelShaderMaterialCBDefaultHeaps[frameIndex]->GetGPUVirtualAddress() + materialIncrement);
		materialIncrement += PixelShaderMaterialCBAlignedSize;

		for (auto* mesh : group.second)
		{
			if (auto materialHandle = mesh->material.lock())
			{
				//Set textures states.
				auto diffuseTex = materialHandle->diffuseTexture.isValid() ? dx12Model->getTexture(materialHandle->diffuseTexture) : nullptr;
				if (diffuseTex)
				{
					commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(diffuseTex->getHandle().buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
					commandList->SetGraphicsRootDescriptorTable(3, diffuseTex->getHandle().descriptorHandle.getGpuHandle());
				}

				auto specularTex = materialHandle->specularTexture.isValid() ? dx12Model->getTexture(materialHandle->specularTexture) : nullptr;
				if (specularTex)
				{
					commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(specularTex->getHandle().buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
					commandList->SetGraphicsRootDescriptorTable(4, specularTex->getHandle().descriptorHandle.getGpuHandle());
				}

				commandList->IASetVertexBuffers(0, 1, &mesh->vertexBuffer.bufferView);
				commandList->IASetIndexBuffer(&mesh->indexBuffer.bufferView);

				// Draw mesh.
				commandList->DrawIndexedInstanced(mesh->nbIndices, 1, 0, 0, 0);

				// Reset texture states
				if (diffuseTex)
				{
					commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(diffuseTex->getHandle().buffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON));
				}
				if (specularTex)
				{
					commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(specularTex->getHandle().buffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON));
				}
			}
		}
	}
}
}}}}}