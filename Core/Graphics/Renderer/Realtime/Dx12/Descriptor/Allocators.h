//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "BaseAllocator.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Descriptor
{
struct CBV_SRV_UAV_DescriptorAllocator : BaseAllocator<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024>
{
	inline DescriptorHandle allocate(const ResourceArray& resources, const D3D12_SHADER_RESOURCE_VIEW_DESC& viewDesc)
	{
		return createResourceViews(resources, [&viewDesc](ID3D12Resource* resource, CD3DX12_CPU_DESCRIPTOR_HANDLE& handle)
		{
			D3D12Device->CreateShaderResourceView(resource, &viewDesc, handle);
		});
	}
};

struct RTV_DescriptorAllocator : BaseAllocator<D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 16>
{
	inline DescriptorHandle allocate(const ResourceArray& resources)
	{
		return createResourceViews(resources, [](ID3D12Resource* resource, CD3DX12_CPU_DESCRIPTOR_HANDLE& handle)
		{
			D3D12Device->CreateRenderTargetView(resource, nullptr, handle);
		});
	}
};

struct DSV_DescriptorAllocator : BaseAllocator<D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 16>
{
	inline DescriptorHandle allocate(const ResourceArray& resources, const D3D12_DEPTH_STENCIL_VIEW_DESC& viewDesc)
	{
		return createResourceViews(resources, [&viewDesc](ID3D12Resource* resource, CD3DX12_CPU_DESCRIPTOR_HANDLE& handle)
		{
			D3D12Device->CreateDepthStencilView(resource, &viewDesc, handle);
		});
	}
};
}}}}}