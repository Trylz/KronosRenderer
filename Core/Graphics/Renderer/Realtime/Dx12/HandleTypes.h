//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#pragma once

#include "d3dx12.h"

#include <atlbase.h>
#include <vector>

#define DX12_GRAPHIC_ALLOC_PARAMETERS Dx12TextureHandle,\
Dx12VertexBufferHandle,\
Dx12IndexBufferHandle

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12
{
	struct ResourceBuffer
	{
		// A default buffer in GPU memory that we will load array data into
		ID3D12Resource* buffer = nullptr;
	};

	struct Block
	{
		UINT start;
		UINT count;
	};

	struct DescriptorHandle
	{
		inline DescriptorHandle()
		: m_block{0,0}
		{
		}

		inline DescriptorHandle(const CD3DX12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
			const CD3DX12_GPU_DESCRIPTOR_HANDLE& gpuHandle,
			const D3D12_DESCRIPTOR_HEAP_TYPE& descType,
			const Block& block)
			: m_cpuHandle(cpuHandle)
			, m_gpuHandle(gpuHandle)
			, m_descType(descType)
			, m_block(block)
		{
		}

		inline const CD3DX12_CPU_DESCRIPTOR_HANDLE& getCpuHandle() const { return m_cpuHandle ;}
		inline const CD3DX12_GPU_DESCRIPTOR_HANDLE& getGpuHandle() const { return m_gpuHandle;}
		inline const D3D12_DESCRIPTOR_HEAP_TYPE& getDescriptorType() const { return m_descType;}
		inline const Block& getBlock() const { return m_block;}

		inline bool isEmpty() const { return m_block.count == 0u; }

	private:
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;
		D3D12_DESCRIPTOR_HEAP_TYPE m_descType;
		Block m_block;
	};

	struct Dx12TextureHandle : ResourceBuffer
	{
		DescriptorHandle descriptorHandle;
		DXGI_FORMAT format;
	};

	template <typename BufferViewType>
	struct ArrayBuffer : ResourceBuffer
	{
		BufferViewType bufferView;
	};

	using Dx12VertexBufferHandle = ArrayBuffer<D3D12_VERTEX_BUFFER_VIEW>;

	using Dx12IndexBufferHandle = ArrayBuffer<D3D12_INDEX_BUFFER_VIEW>;

	using SharedDevicePtr = CComPtr<ID3D12Device>;

	using ResourceArray = std::vector<ID3D12Resource*>;
}}}}

