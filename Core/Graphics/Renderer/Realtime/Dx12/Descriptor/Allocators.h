//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//=========================================================================

#pragma once

#include "BaseAllocator.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Descriptor
{
struct CBV_SRV_UAV_DescriptorAllocator : BaseAllocator<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000>
{
	inline CBV_SRV_UAV_DescriptorAllocator(const SharedDevicePtr& device) : BaseAllocator(device) {}

	inline DescriptorHandle allocate(const ResourceArray& resources, const D3D12_SHADER_RESOURCE_VIEW_DESC& viewDesc)
	{
		return createResourceViews(resources, [this, &viewDesc](ID3D12Resource* resource, CD3DX12_CPU_DESCRIPTOR_HANDLE& handle)
		{
			m_device->CreateShaderResourceView(resource, &viewDesc, handle);
		});
	}
};

struct RTV_DescriptorAllocator : BaseAllocator<D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 10>
{
	inline RTV_DescriptorAllocator(const SharedDevicePtr& device) : BaseAllocator(device) {}

	inline DescriptorHandle allocate(const ResourceArray& resources)
	{
		return createResourceViews(resources, [this](ID3D12Resource* resource, CD3DX12_CPU_DESCRIPTOR_HANDLE& handle)
		{
			m_device->CreateRenderTargetView(resource, nullptr, handle);
		});
	}
};

struct DSV_DescriptorAllocator : BaseAllocator<D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 10>
{
	inline DSV_DescriptorAllocator(const SharedDevicePtr& device) : BaseAllocator(device) {}

	inline DescriptorHandle allocate(const ResourceArray& resources, const D3D12_DEPTH_STENCIL_VIEW_DESC& viewDesc)
	{
		return createResourceViews(resources, [this, &viewDesc](ID3D12Resource* resource, CD3DX12_CPU_DESCRIPTOR_HANDLE& handle)
		{
			m_device->CreateDepthStencilView(resource, nullptr, handle);
		});
	}
};
}}}}}