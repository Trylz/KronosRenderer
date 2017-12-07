//==========================================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017 - All Rights Reserved
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
// 
// Highest level class of Orion(2017/12/06). Make git recognize project as a cpp project. 
//==========================================================================================

#pragma once

#include "Controller/CoreController.h"
#include "MainEditor/MainFrame.h"

#include <memory>
#include <wx/splash.h>

class OrionApplication : public wxApp
{
	bool OnInit() override;
	int OnExit() override;

	bool m_isRunning = false;
	wxSplashScreen* m_splashScreen;
	MainEditor::MainFrame* m_mainEditor;
};


