//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CLASSMENU_H
#define CLASSMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/ILocalize.h>
#include <vgui/KeyCode.h>
#include <cl_dll/iviewport.h>
#include "skillclass.h"
#include "c_sdk_player.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/Label.h>
#include "bgpanel.h"
#include "hilitebutton.h"

using namespace vgui;

#define CANCEL_BUTTON_NAME "CancelButton"

//-----------------------------------------------------------------------------
// Purpose: Draws the class menu
//-----------------------------------------------------------------------------
class CClassMenu : public Frame, public IViewPortPanel
{
public:
	// constructors
	CClassMenu(IViewPort *pViewPort);
	virtual ~CClassMenu();

	// inherited methods
	virtual const char *GetName(void) { return PANEL_CLASS; }
	virtual void Reset(void) {};
	virtual void Update(void) {};
	virtual bool NeedsUpdate(void) { return false; }
	virtual bool HasInputElements(void) { return true; }
	virtual void ShowPanel(bool bShow);
	virtual void SetData(KeyValues *pData) { }
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	// accessors
	void SetForceChooseState(bool bState) { m_bForceChooseState = bState; }

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible(void) { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

protected:

	// helpers
	virtual vgui::Panel *CreateControlByName(const char *controlName);
	void ShowCancelButton(bool bShow);
	void StringToUni(const char *szStr, wchar_t *wszUni, int iSize);
	void SetBgState(CHiliteTextButton *pButton, bool bState);

	// inherited methods
	virtual void OnKeyCodePressed(KeyCode code);
	virtual void OnCommand( const char *szCommand);

private:
	DECLARE_CLASS_SIMPLE(CClassMenu, Frame);

	IViewPort *m_pViewPort;
	vgui::IScheme *m_pScheme;
	int m_iScoreBoardKey;
	bool m_bForceChooseState;
	CHiliteTextButton *m_pSoldierButton;
	CHiliteTextButton *m_pScoutButton;
	CHiliteTextButton *m_pHWButton;
	CHiliteTextButton *m_pSniperButton;
};

/**
* Class declaration for a bg panel for the skill menu that changes its
* background based on the player's current team
**/
class SkillClassBGPanel : public BGPanel
{
public:
	DECLARE_CLASS_SIMPLE(SkillClassBGPanel, BGPanel);

	/**
	* Constructor
	**/
	SkillClassBGPanel(Panel *pParent, const char *szName)
		: BaseClass(pParent, szName)
	{
		// ?
	}

	/**
	* Determines the name of the texture to use
	*
	* @param const char *szName The current name of the texture
	* @param char *szStr The buffer to put the new name in
	* @param int iSize The size of the buffer
	* @return void
	**/
	virtual void GetBackgroundTextureName(const char *szName, char *szStr, int iSize)
	{
		CSDKPlayer *pPlayer;

		// make sure we have a player
		if(!CBasePlayer::GetLocalPlayer())
		{
			BaseClass::GetBackgroundTextureName(szName, szStr, iSize);
			return;
		}

		// put the new name in based on team
		pPlayer = ToSDKPlayer(CBasePlayer::GetLocalPlayer());
		if(pPlayer && pPlayer->GetTeamNumber() == TEAM_A)
		{
			// set the string
			Q_snprintf(szStr, iSize, "%samerican", szName);
			return;
		}
		else if(pPlayer && pPlayer->GetTeamNumber() == TEAM_B)
		{
			Q_snprintf(szStr, iSize, "%scoalition", szName);
			return;
		}

		return BaseClass::GetBackgroundTextureName(szName, szStr, iSize);
	}
};

/**
* Class declaration for a label that cheats when asked for a color
**/
class SkillClassLabel : public vgui::Label
{
public:
	DECLARE_CLASS_SIMPLE(SkillClassLabel, vgui::Label);

	/**
	* Constructor
	**/
	SkillClassLabel(Panel *pParent, const char *szName, const char *szText)
		: Label(pParent, szName, szText)
	{
		// ?
	}

	/**
	* Handles requests for colors from the scheme
	*
	* @param const char *szKeyName Name of the key to search for in the scheme
	* @param IScheme *pScheme The scheme to search in
	* @return Color
	**/
	virtual Color GetSchemeColor(const char *szKeyName, IScheme *pScheme)
	{
		// if they're asking for a regular text color send them back white
		if(!Q_stricmp("Label.TextColor", szKeyName))
			return BaseClass::GetSchemeColor("White", pScheme);

		return BaseClass::GetSchemeColor(szKeyName, pScheme);
	}
};

#endif // CLASSMENU_H
