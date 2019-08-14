//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEAMMENU_H
#define TEAMMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <cl_dll/iviewport.h>
#include <vgui/KeyCode.h>
#include "hilitebutton.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CTeamMenu : public vgui::Frame, public IViewPortPanel
{
public:
	DECLARE_CLASS_SIMPLE( CTeamMenu, vgui::Frame );

	// constructopr
	CTeamMenu(IViewPort *pViewPort);

	// inherited methods
	virtual const char *GetName( void ) { return PANEL_TEAM; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update(void) {};
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
	virtual void ApplySettings(KeyValues *pData);
	
	// accessors
	virtual void SetForceChooseState(bool bState) { m_bForceChooseState = bState; }

	friend void __MsgFunc_TeamDifference(bf_read &msg);

protected:
	
	// inherited methods
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnKeyCodeReleased(vgui::KeyCode code);
	virtual void OnCommand(const char * szCommand);
	virtual Panel *CreateControlByName(const char *szControlName);

	IViewPort	*m_pViewPort;
	IScheme *m_pScheme;

	int m_iJumpKey;
	int m_iScoreBoardKey;
	bool m_bForceChooseState;

	static vgui::Label *s_pTDLabel;
};


#endif // TEAMMENU_H
