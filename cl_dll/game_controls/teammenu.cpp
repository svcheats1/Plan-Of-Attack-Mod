//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "teammenu.h"
#include <vgui/IScheme.h>
#include <vgui_controls/Button.h>
#include "IGameUIFuncs.h" // for key bindings
extern IGameUIFuncs *gameuifuncs; // for key binding details
#include "usermessages.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

vgui::Label *CTeamMenu::s_pTDLabel = NULL;

/**
* Defines a handler for the TeamDifference
*
* @param bf_read &msg The message to handle
* @return void
**/
void __MsgFunc_TeamDifference(bf_read &msg)
{
	// turn on our label
	if(CTeamMenu::s_pTDLabel)
		CTeamMenu::s_pTDLabel->SetVisible(true);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTeamMenu::CTeamMenu(IViewPort *pViewPort) 
	: Frame(NULL, PANEL_TEAM)
{
	// hook the team difference
	usermessages->HookMessage("TeamDifference", __MsgFunc_TeamDifference);

	m_pViewPort = pViewPort;
	m_iJumpKey = KEY_NONE; // this is looked up in Activate()
	m_iScoreBoardKey = KEY_NONE; // this is looked up in Activate()

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible(false);
	SetProportional(true);
	
	// load the layout
	LoadControlSettings("Resource/UI/TeamMenu.res");
	InvalidateLayout(true, true);

	// no scheme
	m_pScheme = NULL;

	// find the label
	s_pTDLabel = (vgui::Label *)FindChildByName("TeamDifferenceLabel");
}

//-----------------------------------------------------------------------------
// Purpose: sets the text color of the map description field
//-----------------------------------------------------------------------------
void CTeamMenu::ApplySchemeSettings(IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// set the background color
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(2);
	SetBgColor(m_pScheme->GetColor("TransparentBlack", Color(0, 0, 0, 0)));
}

/**
* Displays the panel 
*
* @param bool bShow True if we should show the panel
* @return void
**/
void CTeamMenu::ShowPanel(bool bShow)
{
	// are we already visible?
	if(BaseClass::IsVisible() == bShow)
		return;

	// are we hiding or displaying?
	if(bShow)
	{
		// activate the window and accept input
		Activate();
		SetMouseInputEnabled(true);

		// get key bindings if shown
		if(m_iJumpKey == KEY_NONE)
			m_iJumpKey = gameuifuncs->GetVGUI2KeyCodeForBind("jump");
		if(m_iScoreBoardKey == KEY_NONE)
			m_iScoreBoardKey = gameuifuncs->GetVGUI2KeyCodeForBind("showscores");

		// pull the cancel button
		vgui::Button *pButton = dynamic_cast<vgui::Button *>(FindChildByName("CancelButton"));
		if (pButton) 
		{
			// are we forcing them to pick?
			//pButton->SetVisible(!m_bForceChooseState);
			pButton->SetEnabled(!m_bForceChooseState);
		}
	}
	else
	{
		// turn it off
		SetVisible(false);
		SetMouseInputEnabled(false);

		// turn off the label
		if(s_pTDLabel)
			s_pTDLabel->SetVisible(false);
	}

	// show/hide the background
	m_pViewPort->ShowBackGround(bShow);
}

void CTeamMenu::OnCommand( const char *szCommand )
{
	// anything other than a cancel then we want to send it to the client
	if (Q_stricmp("vguicancel", szCommand) != 0)
		engine->ClientCmd(const_cast<char *>(szCommand));

	// turn off the label
	if(s_pTDLabel)
		s_pTDLabel->SetVisible(false);

	// on any command, close and hide the background
	Close();
	gViewPortInterface->ShowBackGround(false);
	BaseClass::OnCommand( szCommand );
}

void CTeamMenu::OnKeyCodeReleased(KeyCode code)
{
	if ( m_iScoreBoardKey != KEY_NONE && m_iScoreBoardKey == code )
	{
		gViewPortInterface->ShowPanel(PANEL_SCOREBOARD, false);
		ShowPanel(true);
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

void CTeamMenu::OnKeyCodePressed(KeyCode code)
{
	if( m_iJumpKey != KEY_NONE && m_iJumpKey == code )
	{
		OnCommand("changeteam 5");
	}
	else if ( m_iScoreBoardKey != KEY_NONE && m_iScoreBoardKey == code )
	{
		ShowPanel(false);
		gViewPortInterface->ShowPanel(PANEL_SCOREBOARD, true);
	}
	// team 1
	else if ( code == gameuifuncs->GetVGUI2KeyCodeForBind( "slot1" ) )
	{
		OnCommand("changeteam 1");
	}
	// team 2
	else if ( code == gameuifuncs->GetVGUI2KeyCodeForBind( "slot2" ) )
	{
		OnCommand("changeteam 2");
	}
	// auto-assign
	else if ( code == gameuifuncs->GetVGUI2KeyCodeForBind( "slot5" ) )
	{
		OnCommand("changeteam 5");
	}
	// spectate
	else if ( code == gameuifuncs->GetVGUI2KeyCodeForBind( "slot6" ) )
	{
		OnCommand("changeteam 6");
	}
	// cancel
	else if ( code == gameuifuncs->GetVGUI2KeyCodeForBind( "slot10" ) )
	{
		OnCommand("vguicancel");
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

/**
* Creates a control of the specified type
*
* @param const char *szControlName Name of the control to create
* @return Panel * The new control or NULL
**/
Panel *CTeamMenu::CreateControlByName(const char *szControlName)
{
	// hilite text button?
	if(!Q_strcmp(szControlName, "HiliteTextButton"))
		return new CHiliteTextButton(this, "", "");
	else if(!Q_strcmp(szControlName, "BGPanel"))
		return new BGPanel(this, "");
	
	// let someelse do it
	return BaseClass::CreateControlByName(szControlName);
}

/**
* Applies the settings to the menu
*
* @param KeyValues *pData The data to apply
* @return void
**/
void CTeamMenu::ApplySettings(KeyValues *pData)
{
	// jump down
	BaseClass::ApplySettings(pData);

	// get the scheme
	m_pScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetScheme("ClientScheme"));

	// set the background color
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(2);
	SetBgColor(m_pScheme->GetColor("TransparentBlack", Color(0, 0, 0, 0)));
}