//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

#pragma once

#include "Exception/BaseException.h"

namespace Graphics { namespace Renderer { namespace Realtime
{
class CreateTextureException : public Exception::BaseException
{
public:
	CreateTextureException(const char* msg) : BaseException(msg) {}
	CreateTextureException(const std::string& msg) : BaseException(msg) {}
};
}}}