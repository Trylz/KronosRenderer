//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "BaseEffect.h"
#include <DirectXMath.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
// TODO : Make this class an effect accessible by other effects.
// Currently the same shaders are compiled for each derived class.
template <typename PushDrawArgs, nbUint32 PixelCBElementCount>
class TSimpleColor : public BaseEffect<PushDrawArgs>
{
public:
	TSimpleColor(const DXGI_SAMPLE_DESC& sampleDesc);

protected:
	NEBULA_DX12_ATTRIBUTE_ALIGN struct VertexShaderCB
	{
		DirectX::XMFLOAT4X4 wvpMat;
	};

	NEBULA_DX12_ATTRIBUTE_ALIGN struct PixelShaderCB
	{
		DirectX::XMFLOAT4 color;
	};

	nbUint32 PixelShaderCBAlignedSize = (sizeof(PixelShaderCB) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);

	void initRootSignature() override;
	void initPipelineStateObjects() override;
	void initConstantBuffers();

	PipelineStatePtr m_PSO;

	CComPtr<ID3D12Resource> m_vertexShaderCBUploadHeaps[SwapChainBufferCount];
	UINT8* m_vertexShaderCBGPUAddress[SwapChainBufferCount];

	CComPtr<ID3D12Resource> m_pixelShaderCBUploadHeaps[SwapChainBufferCount];
	UINT8* m_pixelShaderCBGPUAddress[SwapChainBufferCount];
};

template <typename PushDrawArgs, nbUint32 PixelCBElementCount>
TSimpleColor<PushDrawArgs, PixelCBElementCount>::TSimpleColor(const DXGI_SAMPLE_DESC& sampleDesc)
	: BaseEffect(sampleDesc)
{
	initRootSignature();
	initPipelineStateObjects();
	initConstantBuffers();
}

template <typename PushDrawArgs, nbUint32 PixelCBElementCount>
void TSimpleColor<PushDrawArgs, PixelCBElementCount>::initRootSignature()
{
	D3D12_ROOT_PARAMETER rootParameters[2];

	for (nbInt32 i = 0; i< 2; ++i)
	{
		D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
		rootCBVDescriptor.RegisterSpace = 0;
		rootCBVDescriptor.ShaderRegister = 0;
		rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[i].Descriptor = rootCBVDescriptor;
	}

	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	createRootSignature(rootParameters, _countof(rootParameters));
}

template <typename PushDrawArgs, nbUint32 PixelCBElementCount>
void TSimpleColor<PushDrawArgs, PixelCBElementCount>::initPipelineStateObjects()
{
	// Compile shaders
	ID3DBlob* vertexShader = compileShader(std::wstring(L"SimpleColor_VS.hlsl"), true);
	ID3DBlob* pixelShader = compileShader(std::wstring(L"SimpleColor_PS.hlsl"), false);

	// Compile pipeline
	D3D12_INPUT_ELEMENT_DESC inputLayoutElement[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_DEPTH_STENCIL_DESC dsDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	dsDesc.DepthEnable = false;

	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizerDesc.MultisampleEnable = TRUE;

	compilePipeline(m_PSO,
		vertexShader,
		pixelShader,
		inputLayoutElement,
		sizeof(inputLayoutElement) / sizeof(D3D12_INPUT_ELEMENT_DESC),
		blendDesc,
		dsDesc,
		rasterizerDesc
	);

	m_PSO->SetName(L"Simple color PSO");
}

template <typename PushDrawArgs, nbUint32 PixelCBElementCount>
void TSimpleColor<PushDrawArgs, PixelCBElementCount>::initConstantBuffers()
{
	for (nbInt32 i = 0; i < SwapChainBufferCount; ++i)
	{
		// Vertex shader
		HRESULT hr = D3D12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexShaderCBUploadHeaps[i]));

		NEBULA_ASSERT(SUCCEEDED(hr));

		m_vertexShaderCBUploadHeaps[i]->SetName(L"RenderMoveGizmo : Vertex shader upload heap");

		hr = m_vertexShaderCBUploadHeaps[i]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_vertexShaderCBGPUAddress[i]));
		NEBULA_ASSERT(SUCCEEDED(hr));

		// Pixel shader
		hr = D3D12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(PixelShaderCBAlignedSize * PixelCBElementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pixelShaderCBUploadHeaps[i]));

		NEBULA_ASSERT(SUCCEEDED(hr));

		m_pixelShaderCBUploadHeaps[i]->SetName(L"RenderMoveGizmo : Pixel shader upload heap");

		hr = m_pixelShaderCBUploadHeaps[i]->Map(0, &readRangeGPUOnly, reinterpret_cast<void**>(&m_pixelShaderCBGPUAddress[i]));
		NEBULA_ASSERT(SUCCEEDED(hr));
	}
}
}}}}}