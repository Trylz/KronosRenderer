//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

#pragma once

#include "../HandleTypes.h"
#include "../../SwapChain.h"
#include "Graphics/Renderer/Realtime/TGraphicResourceAllocator.h"

// ORION_DEPLOYMENT_BUILD
#include "Platform.h"

#include <atlbase.h>
#include <D3Dcompiler.h>

#define ORION_DX12_SHADER_BASE_PATH std::wstring(L"Graphics/Renderer/Realtime/Dx12/Effect/Shaders/")

#define ORION_DX12_SHADER_PATH(x) (ORION_CORE_FOLDERW + ORION_DX12_SHADER_BASE_PATH + x).c_str()

#define ORION_DX12_ATTRIBUTE_ALIGN __declspec(align(16))

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12{ namespace Effect
{
extern SharedDevicePtr D3d12Device;

template <typename DataToProcessType>
class BaseEffect
{
public:
	inline BaseEffect(const DXGI_SAMPLE_DESC& sampleDesc)
	: m_sampleDesc(sampleDesc) {}

	virtual ~BaseEffect() {}

	virtual void pushDrawCommands(DataToProcessType data, ID3D12GraphicsCommandList* commandList, int frameIndex) = 0;

protected:
	using PipelineStatePtr = CComPtr<ID3D12PipelineState>;

	void createRootSignature(const D3D12_ROOT_PARAMETER* rootParams, UINT nbParams);

	void compile(PipelineStatePtr& pipelineState,
		const std::wstring& vShader,
		const std::wstring& pShader,
		const D3D12_INPUT_ELEMENT_DESC* inputLayoutElementDesc,
		const UINT inputLayoutNumElements,
		const D3D_SHADER_MACRO* macros = nullptr,
		const D3D12_BLEND_DESC& blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
		const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));

	DXGI_SAMPLE_DESC m_sampleDesc;
	CComPtr<ID3D12RootSignature> m_rootSignature;

protected:
	virtual void initRootSignature() = 0;
	virtual void initPipelineStateObjects() = 0;
};

// Read range for resources that we do not intend to read on the CPU. (so end is less than or equal to begin)
const CD3DX12_RANGE readRangeGPUOnly = CD3DX12_RANGE(0, 0);

const DXGI_FORMAT defaultDSFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

template <typename DataToProcessType>
void BaseEffect<DataToProcessType>::createRootSignature(const D3D12_ROOT_PARAMETER* rootParams, UINT nbParams)
{
	// create a static sampler
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
		ORION_TRACE((char*)errorBuff->GetBufferPointer());
		ORION_ASSERT(false);
		return;
	}

	hr = D3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	ORION_ASSERT(SUCCEEDED(hr));
}

template <typename DataToProcessType>
void BaseEffect<DataToProcessType>::compile(PipelineStatePtr& pipelineState,
	const std::wstring& vShader,
	const std::wstring& pShader,
	const D3D12_INPUT_ELEMENT_DESC* inputLayoutElementDesc,
	const UINT inputLayoutNumElements,
	const D3D_SHADER_MACRO* macros,
	const D3D12_BLEND_DESC& blendDesc,
	const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
{
#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	// when debugging, we can compile the shader files at runtime.
	// but for release versions, we can compile the hlsl shaders
	// with fxc.exe to create .cso files, which contain the shader
	// bytecode. We can load the .cso files at runtime to get the
	// shader bytecode, which of course is faster than compiling
	// them at runtime

	// compile vertex shader
	ID3DBlob* errorBuff;
	ID3DBlob* vertexShader; // d3d blob for holding vertex shader bytecode
	HRESULT hr = D3DCompileFromFile(ORION_DX12_SHADER_PATH(vShader),
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		compileFlags,
		0,
		&vertexShader,
		&errorBuff);
	if (FAILED(hr))
	{
		ORION_TRACE((char*)errorBuff->GetBufferPointer());
		ORION_ASSERT(false);
		return;
	}

	// fill out a shader bytecode structure, which is basically just a pointer
	// to the shader bytecode and the size of the shader bytecode
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

	// compile pixel shader
	ID3DBlob* pixelShader;
	hr = D3DCompileFromFile(ORION_DX12_SHADER_PATH(pShader),
		macros,
		nullptr,
		"main",
		"ps_5_0",
		compileFlags,
		0,
		&pixelShader,
		&errorBuff);
	if (FAILED(hr))
	{
		ORION_TRACE((char*)errorBuff->GetBufferPointer());
		ORION_ASSERT(false);
		return;
	}

	// fill out shader bytecode structure for pixel shader
	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

	// fill out an input layout description structure
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	// we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
	inputLayoutDesc.NumElements = inputLayoutNumElements;
	inputLayoutDesc.pInputElementDescs = inputLayoutElementDesc;

	// create a pipeline state object (PSO)
	// In a real application, you will have many pso's. for each different shader
	// or different combinations of shaders, different blend states or different rasterizer states,
	// different topology types (point, line, triangle, patch), or a different number
	// of render targets you will need a pso

	// VS is the only required shader for a pso. You might be wondering when a case would be where
	// you only set the VS. It's possible that you have a pso that only outputs data with the stream
	// output, and not on a render target, which means you would not need anything after the stream
	// output.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // a structure to define a pso
	psoDesc.InputLayout = inputLayoutDesc; // the structure describing our input layout
	psoDesc.pRootSignature = m_rootSignature; // the root signature that describes the input data this pso needs
	psoDesc.VS = vertexShaderBytecode; // structure describing where to find the vertex shader bytecode and how large it is
	psoDesc.PS = pixelShaderBytecode; // same as VS but for pixel shader
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the render target
	psoDesc.SampleDesc = m_sampleDesc; // must be the same sample description as the swapchain and depth/stencil buffer
	psoDesc.SampleMask = 0xffffffff; // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = blendDesc;
	psoDesc.NumRenderTargets = 1; // we are only binding one render target
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = defaultDSFormat;

	ORION_TRACE("BaseEffect::compile - Disabled culling because seems wrong");

	// create the pso
	hr = D3d12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
	ORION_ASSERT(SUCCEEDED(hr));
}
}}}}}