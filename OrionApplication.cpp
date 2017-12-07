//==========================================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017 - All Rights Reserved
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
// 
// Highest level class of Orion(2017/12/06). Make git recognize project as a cpp project. 
//==========================================================================================

#include "stdafx.h"

#if defined (_WINDOWS)
#include "Graphics/Renderer/Realtime/Dx12/Dx12Renderer.h"
#endif

#include "Input/Event/Keyboard.h"
#include "OrionApplication.h"

using Controller::CoreControllerSingleton;
using namespace Graphics::Renderer::Realtime;
using namespace MainEditor;

wxIMPLEMENT_APP(OrionApplication);

#define SPLASH_SCREEN_TIMEOUT_MS 1500

bool OrionApplication::OnInit()
{
	// Init systems
	Controller::CoreControllerSingleton::create();

	wxXmlResource::Get()->InitAllHandlers();

	m_mainEditor = new MainFrame();

#if defined (_WINDOWS)
	Dx12::InitArgs dx12InitArgs;
	dx12InitArgs.hwnd = m_mainEditor->getRealtimeRenderControl()->GetHWND();

	int w, h;
	m_mainEditor->GetSize(&w, &h);
	dx12InitArgs.screenAspectRatio = (float)w / (float)h;

	Dx12::DX12Renderer* dx12Renderer = new Dx12::DX12Renderer();
	ORION_ASSERT(dx12Renderer->init(dx12InitArgs));

	Controller::CoreControllerSingleton::instance()->onNewRenderer(dx12Renderer);
#endif

	m_isRunning = true;
	m_mainEditor->onInit();

	// Play splash screen
	wxImage::AddHandler(new wxPNGHandler());

	m_splashScreen = new wxSplashScreen(wxBITMAP_PNG(OrionSplashScreen),
		wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT,
		SPLASH_SCREEN_TIMEOUT_MS, NULL, -1, wxDefaultPosition, wxDefaultSize,
		wxBORDER_SIMPLE | wxSTAY_ON_TOP);

	m_mainEditor->AddChild(m_splashScreen);

	m_splashScreen->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& WXUNUSED(ev)) {
		m_splashScreen->Show(false);
		m_mainEditor->Show(true);
	});

	return true;
}

int OrionApplication::OnExit()
{
	// wxwidgets already destroy windows on exit. So no need to delete
	m_splashScreen = nullptr;
	m_mainEditor = nullptr;

	m_isRunning = false;

	Controller::CoreControllerSingleton::destroy();

	return 0;
}
