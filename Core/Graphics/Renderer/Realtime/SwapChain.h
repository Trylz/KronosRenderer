//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

namespace Graphics { namespace Renderer { namespace Realtime
{
	constexpr nbUint32 SwapChainBufferCount = 3u;

	#define MAKE_SWAP_CHAIN_ITERATOR_I for (nbUint32 i = 0u; i < SwapChainBufferCount; ++i)
}}}