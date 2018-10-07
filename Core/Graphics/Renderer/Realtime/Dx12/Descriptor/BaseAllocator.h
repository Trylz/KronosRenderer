//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "../HandleTypes.h"
#include <functional>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Descriptor
{
// TODO: Improvements.
// Free blocks system is quite simple.
// There is no defragmentation.
template <D3D12_DESCRIPTOR_HEAP_TYPE descType, UINT pageSize>
class BaseAllocator
{
public:
	BaseAllocator(const SharedDevicePtr& device);
	~BaseAllocator() { m_heap->Release(); }

	UINT getDescriptorSize() const;
	ID3D12DescriptorHeap* getHeap();

	void release(const DescriptorHandle& handle);

protected:
	using createViewInHeapFunc = std::function<void(ID3D12Resource*, CD3DX12_CPU_DESCRIPTOR_HANDLE&)>;

	DescriptorHandle createResourceViews(const ResourceArray& resources, const createViewInHeapFunc& createFunc);
	void createResourceViewsInHeap(const ResourceArray& resources,
		const createViewInHeapFunc& createFunc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& heapCpuHandle,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& dstCpuHandle,
		CD3DX12_GPU_DESCRIPTOR_HANDLE& dstGpuHandle,
		UINT startIdx);

	SharedDevicePtr m_device;

private:
	void createHeap(ID3D12DescriptorHeap** buffer, UINT capacity);
	void increaseCapacity(UINT additionalCount);
	nbInt32 retrieveFreeBlock(UINT count);

	ID3D12DescriptorHeap* m_heap = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_heapCpuHandle;
	std::vector<Block> m_freeBlocks;

	UINT m_descriptorSize;

	// Total capacity of the heap.
	UINT m_capacity;

	// Total count of used descriptors, including free blocks.
	UINT m_count;
};

template <D3D12_DESCRIPTOR_HEAP_TYPE descType, UINT pageSize>
inline BaseAllocator<descType, pageSize>::BaseAllocator(const SharedDevicePtr& device)
{
	m_device = device;
	m_count = 0u;
	m_descriptorSize = m_device->GetDescriptorHandleIncrementSize(descType);

	createHeap(&m_heap, pageSize);
	m_capacity = pageSize;
	m_heapCpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
}

template <D3D12_DESCRIPTOR_HEAP_TYPE descType, UINT pageSize>
inline UINT BaseAllocator<descType, pageSize>::getDescriptorSize() const
{
	return m_descriptorSize;
}

template <D3D12_DESCRIPTOR_HEAP_TYPE descType, UINT pageSize>
inline ID3D12DescriptorHeap* BaseAllocator<descType, pageSize>::getHeap()
{
	return m_heap;
}

template <D3D12_DESCRIPTOR_HEAP_TYPE descType, UINT pageSize>
inline void BaseAllocator<descType, pageSize>::release(const DescriptorHandle& handle)
{
	KRONOS_ASSERT(handle.getDescriptorType() == descType);

	const Block& block = handle.getBlock();
	KRONOS_ASSERT(block.count > 0u);
	KRONOS_ASSERT(block.start + block.count <= m_count);

	m_freeBlocks.push_back(block);
}

template <D3D12_DESCRIPTOR_HEAP_TYPE descType, UINT pageSize>
inline nbInt32 BaseAllocator<descType, pageSize>::retrieveFreeBlock(UINT count)
{
	nbInt32 blockStart = -1;

	for (UINT i = 0u; i < m_freeBlocks.size(); ++i)
	{
		if (m_freeBlocks[i].count >= count)
		{
			UINT residuum = m_freeBlocks[i].count - count;
			if (residuum)
			{
				Block newBlock;
				newBlock.start = m_freeBlocks[i].start + count;
				newBlock.count = residuum;

				m_freeBlocks.push_back(newBlock);
			}

			blockStart = m_freeBlocks[i].start;
			m_freeBlocks.erase(m_freeBlocks.begin() + i);

			break;
		}
	}

	return blockStart;
}

template <D3D12_DESCRIPTOR_HEAP_TYPE descType, UINT pageSize>
inline DescriptorHandle BaseAllocator<descType, pageSize>::createResourceViews(const ResourceArray& resources, const createViewInHeapFunc& createFunc)
{
	const UINT resCount = (UINT)resources.size();
	KRONOS_ASSERT(resCount > 0u);

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	Block block;

	const nbInt32 freeBlockStart = retrieveFreeBlock(resCount);
	if (freeBlockStart >= 0)
	{
		block.start = freeBlockStart;

		CD3DX12_CPU_DESCRIPTOR_HANDLE startHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_heap->GetCPUDescriptorHandleForHeapStart());
		startHandle.Offset(freeBlockStart, m_descriptorSize);
			
		createResourceViewsInHeap(resources, createFunc, startHandle, cpuHandle, gpuHandle, (UINT)freeBlockStart);
	}
	else
	{
		block.start = m_count;

		const UINT available = m_capacity - m_count;
		if (resCount > available)
		{
			increaseCapacity(resCount - available);
		}

		createResourceViewsInHeap(resources, createFunc, m_heapCpuHandle, cpuHandle, gpuHandle, m_count);

		m_count += resCount;
	}

	block.count = resCount;

	return DescriptorHandle(cpuHandle, gpuHandle, descType, block);
}

template <D3D12_DESCRIPTOR_HEAP_TYPE descType, UINT pageSize>
inline void BaseAllocator<descType, pageSize>::createResourceViewsInHeap(const ResourceArray& resources,
	const createViewInHeapFunc& createFunc,
	CD3DX12_CPU_DESCRIPTOR_HANDLE& heapCpuHandle,
	CD3DX12_CPU_DESCRIPTOR_HANDLE& dstCpuHandle,
	CD3DX12_GPU_DESCRIPTOR_HANDLE& dstGpuHandle,
	UINT startIdx)
{
	dstCpuHandle = heapCpuHandle;

	dstGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_heap->GetGPUDescriptorHandleForHeapStart());
	dstGpuHandle.Offset(startIdx, m_descriptorSize);

	for (nbUint32 i = 0u; i < resources.size(); ++i)
	{
		createFunc(resources[i], heapCpuHandle);
		heapCpuHandle.Offset(1, m_descriptorSize);
	}
}

template <D3D12_DESCRIPTOR_HEAP_TYPE descType, UINT pageSize>
inline void BaseAllocator<descType, pageSize>::createHeap(ID3D12DescriptorHeap** buffer, UINT capacity)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = capacity;
	heapDesc.Type = descType;

	if (descType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	else
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(buffer));
	KRONOS_ASSERT(SUCCEEDED(hr));
}

template <D3D12_DESCRIPTOR_HEAP_TYPE descType, UINT pageSize>
inline void BaseAllocator<descType, pageSize>::increaseCapacity(UINT additionalCount)
{
	// Copying descriptor is currently not possible with a shader visible source.
	// These heaps must only have one page that is large enough.
	KRONOS_ASSERT(descType != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	nbUint32 nbPages = additionalCount / pageSize;
	if (additionalCount % pageSize)
		++nbPages;

	const UINT newCapacity = m_capacity + (pageSize * nbPages);

	ID3D12DescriptorHeap* tempHeap = nullptr;
	createHeap(&tempHeap, newCapacity);

	m_device->CopyDescriptorsSimple(m_capacity,
		tempHeap->GetCPUDescriptorHandleForHeapStart(),
		m_heap->GetCPUDescriptorHandleForHeapStart(),
		descType);

	m_heap->Release();
	m_heap = tempHeap;
	m_capacity = newCapacity;

	m_heapCpuHandle = tempHeap->GetCPUDescriptorHandleForHeapStart();
	m_heapCpuHandle.Offset(m_count, m_descriptorSize);
}
}}}}}