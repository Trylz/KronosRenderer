//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "../HandleTypes.h"
#include "../../SwapChain.h"
#include "Graphics/Renderer/Realtime/Dx12/D3D12Device.h"
#include "Graphics/Renderer/Realtime/TGraphicResourceAllocator.h"

// NEBULA_DEPLOYMENT_BUILD
#include "Platform.h"

#include <atlbase.h>
#include <D3Dcompiler.h>

#define NEBULA_DX12_SHADER_BASE_PATH std::wstring(L"Graphics/Renderer/Realtime/Dx12/Effect/Shaders/")

#define NEBULA_DX12_SHADER_PATH(x) (NEBULA_CORE_FOLDERW + NEBULA_DX12_SHADER_BASE_PATH + x).c_str()

#define NEBULA_DX12_ATTRIBUTE_ALIGN __declspec(align(16))

#define NEBULA_DX12_ALIGN_SIZE(x) (sizeof(x) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)

#define NEBULA_DX12_SAFE_RELEASE(x) if (x) x->Release()

#define NEBULA_SWAP_CHAIN_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12{ namespace Effect
{
template <typename DataToProcessType>
class BaseEffect
{
public:
	virtual ~BaseEffect() {}

	virtual void pushDrawCommands(DataToProcessType data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex) = 0;

protected:
	using PipelineStatePtr = CComPtr<ID3D12PipelineState>;

	void createRootSignature(const D3D12_ROOT_PARAMETER* rootParams, UINT nbParams);

	ID3DBlob* compileShader(const std::wstring& shaderPath,
		nbBool isVertexShader,
		const D3D_SHADER_MACRO* macros = nullptr);

	void compilePipeline(PipelineStatePtr& pipelineState,
		ID3DBlob* vertexShader,
		ID3DBlob* pixelShader,
		const D3D12_INPUT_ELEMENT_DESC* inputLayoutElementDesc,
		const UINT inputLayoutNumElements,
		const D3D12_BLEND_DESC& blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
		const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
		const D3D12_RASTERIZER_DESC& rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
		const std::vector<DXGI_FORMAT>& renderTargetFormats = std::vector<DXGI_FORMAT>{ NEBULA_SWAP_CHAIN_FORMAT },
		const D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	DXGI_SAMPLE_DESC m_sampleDesc;
	CComPtr<ID3D12RootSignature> m_rootSignature;

protected:
	inline BaseEffect(){}
	inline BaseEffect(const DXGI_SAMPLE_DESC& desc) { setSampleDesc(desc); }
	inline void setSampleDesc(const DXGI_SAMPLE_DESC& desc) { m_sampleDesc = desc; }

	virtual void initRootSignature() = 0;
	virtual void initPipelineStateObjects() = 0;
};

// Read range for resources that we do not intend to read on the CPU. (so end is less than or equal to begin)
const CD3DX12_RANGE ReadRangeGPUOnly = CD3DX12_RANGE(0, 0);

const DXGI_FORMAT DefaultDSFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

template <typename DataToProcessType>
void BaseEffect<DataToProcessType>::createRootSignature(const D3D12_ROOT_PARAMETER* rootParams, UINT nbParams)
{
	// Create a static sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_ANISOTROPIC;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(nbParams,
		rootParams,
		1,
		&sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ID3DBlob* errorBuff;
	ID3DBlob* signature;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff);
	if (FAILED(hr))
	{
		NEBULA_TRACE((char*)errorBuff->GetBufferPointer());
		NEBULA_ASSERT(false);
		return;
	}

	hr = D3D12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	NEBULA_ASSERT(SUCCEEDED(hr));
}

template <typename DataToProcessType>
ID3DBlob* BaseEffect<DataToProcessType>::compileShader(const std::wstring& shaderPath, nbBool isVertexShader, const D3D_SHADER_MACRO* macros)
{
#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0u;
#endif

	// When debugging, we can compile the shader files at runtime.
	// but for deployment versions, we should compile the hlsl shaders
	// with fxc.exe to create .cso files, which contain the shader
	// bytecode. We can load the .cso files at runtime to get the
	// shader bytecode, which of course is faster than compiling
	// them at runtime

	ID3DBlob* errorBuff;
	ID3DBlob* shader;
	HRESULT hr = D3DCompileFromFile(NEBULA_DX12_SHADER_PATH(shaderPath),
		macros,
		nullptr,
		"main",
		isVertexShader ? "vs_5_0" : "ps_5_0",
		compileFlags,
		0,
		&shader,
		&errorBuff);
	if (FAILED(hr))
	{
		NEBULA_TRACE((char*)errorBuff->GetBufferPointer());
		NEBULA_ASSERT(false);

		return nullptr;
	}

	return shader;
}

template <typename DataToProcessType>
void BaseEffect<DataToProcessType>::compilePipeline(PipelineStatePtr& pipelineState,
	ID3DBlob* vertexShader,
	ID3DBlob* pixelShader,
	const D3D12_INPUT_ELEMENT_DESC* inputLayoutElementDesc,
	const UINT inputLayoutNumElements,
	const D3D12_BLEND_DESC& blendDesc,
	const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc,
	const D3D12_RASTERIZER_DESC& rasterizerDesc,
	const std::vector<DXGI_FORMAT>& renderTargetFormats,
	const D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
	// Fill out a shader bytecode structure, which is basically just a pointer
	// to the shader bytecode and the size of the shader bytecode.
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

	// Fill out a pixel shader bytecode structure.
	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

	// Fill out an input layout description structure
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = inputLayoutNumElements;
	inputLayoutDesc.pInputElementDescs = inputLayoutElementDesc;

	// Fill out a PSO description
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = m_rootSignature;
	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;
	psoDesc.PrimitiveTopologyType = topologyType;

	NEBULA_ASSERT(renderTargetFormats.size() <= 8);
	const UINT numRenderTargets = std::min((UINT)renderTargetFormats.size(), 8u);

	for (nbUint32 i = 0; i < numRenderTargets; ++i)
		psoDesc.RTVFormats[i] = renderTargetFormats[i];

	psoDesc.SampleDesc = m_sampleDesc; // Must be the same sample description as the swapchain and depth/stencil buffer.
	psoDesc.SampleMask = 0xffffffff; // Sample mask has to do with multi-sampling. 0xffffffff means point sampling is done.
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = blendDesc;
	psoDesc.NumRenderTargets = numRenderTargets;
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = DefaultDSFormat;

	// Now create the PSO
	HRESULT hr = D3D12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
	NEBULA_ASSERT(SUCCEEDED(hr));
}
}}}}}