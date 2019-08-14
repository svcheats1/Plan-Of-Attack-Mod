//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MOUSEOVERPANELBUTTON_H
#define MOUSEOVERPANELBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/EditablePanel.h>
#include <filesystem.h>

//extern vgui::Panel *g_lastPanel;

//-----------------------------------------------------------------------------
// Purpose: Triggers a new panel when the mouse goes over the button
//-----------------------------------------------------------------------------
class MouseOverPanelButton : public vgui::Button
{
private:
	DECLARE_CLASS_SIMPLE( MouseOverPanelButton, vgui::Button );
	
public:
	MouseOverPanelButton(vgui::Panel *parent, const char *panelName, const char *szText);
	MouseOverPanelButton(vgui::Panel *parent, const char *panelName, vgui::Panel *templatePanel = NULL);

	void SetResName(const char *szName);

	void SetPanelWide(int iWidth) {m_pPanel->SetWide(iWidth);}
	void SetPanelTall(int iTall) {m_pPanel->SetTall(iTall);}
	void SetPanelPos(int iXPos, int iYPos) {m_pPanel->SetPos(iXPos, iYPos);}

	void ShowPage();
	void HidePage();
	
	const char *GetPanelRes(void);
	virtual void ApplySettings( KeyValues *resourceData );

	vgui::EditablePanel *GetPanel(void) { return m_pPanel; }

private:

	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	vgui::EditablePanel *m_pPanel;
	char m_szResName[256];
};


#endif // MOUSEOVERPANELBUTTON_H
