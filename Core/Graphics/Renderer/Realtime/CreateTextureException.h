//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#pragma once

#include "Utilities/BaseException.h"

namespace Graphics { namespace Renderer { namespace Realtime
{
class CreateTextureException : public Utilities::BaseException
{
public:
	CreateTextureException(const char* msg) : BaseException(msg) {}
	CreateTextureException(const std::string& msg) : BaseException(msg) {}
};
}}}