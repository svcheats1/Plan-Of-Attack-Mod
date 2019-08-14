//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "classmenu.h"
#include "cdll_util.h"
#include "IGameUIFuncs.h"
#include "hilitebutton.h"
#include "sdk_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;


/**
* Constructor
**/
CClassMenu::CClassMenu(IViewPort *pViewport)
	: Frame(NULL, PANEL_CLASS), m_pViewPort(pViewport), m_iScoreBoardKey(KEY_NONE), m_bForceChooseState(false)
{
	// note: these are valve changes
	//m_pFirstButton = NULL;
	m_iScoreBoardKey = -1; // this is looked up in Activate()
	//m_iTeam = 0;

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);

	// load the layout
	LoadControlSettings("Resource/UI/ClassMenu.res");

	// draw backgrounds and borders
	SetPaintBorderEnabled(true);
	SetPaintBackgroundEnabled(true);

	// no scheme
	m_pScheme = NULL;
}

/**
* Destructor
**/
CClassMenu::~CClassMenu()
{
	// blah?
}

/**
* Applies the scheme to the panel
*
* @param vgui::IScheme *pScheme The scheme to apply
* @return void
**/
void CClassMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// set the background info
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(2);

	// make sure we have a player
	if(CBasePlayer::GetLocalPlayer())
	{
		// setup the backgrounds for all my labels
		SetBgState(m_pSoldierButton, !((CSDKGameRules *)g_pGameRules)->CanJoinClass(CSDKPlayer::GetLocalSDKPlayer(), "soldier"));
		SetBgState(m_pHWButton, !((CSDKGameRules *)g_pGameRules)->CanJoinClass(CSDKPlayer::GetLocalSDKPlayer(), "hw"));
		SetBgState(m_pScoutButton, !((CSDKGameRules *)g_pGameRules)->CanJoinClass(CSDKPlayer::GetLocalSDKPlayer(), "scout"));
		SetBgState(m_pSniperButton, !((CSDKGameRules *)g_pGameRules)->CanJoinClass(CSDKPlayer::GetLocalSDKPlayer(), "sniper"));
	}
}

/**
* Hook to create a new type of control
*
* @param const char *szControlName Name of the control to create
* @return Panel *
**/
Panel *CClassMenu::CreateControlByName(const char *szControlName)
{
	if(!Q_stricmp("HiliteTextButton", szControlName))
		return new CHiliteTextButton(this, "", "");
	else if(!Q_stricmp("SkillClassBGPanel", szControlName))
		return new SkillClassBGPanel(this, "");
	else if(!Q_stricmp("SkillClassLabel", szControlName))
		return new SkillClassLabel(this, "", "");
	else
		return BaseClass::CreateControlByName(szControlName);
}

/**
* Turns the cancel button on or off
*
* @param bool bShow Determines if we should show or hide
* @return void
**/
void CClassMenu::ShowCancelButton(bool bShow)
{
	Button *pButton;

	// find the button
	pButton = dynamic_cast<Button *>(FindChildByName(CANCEL_BUTTON_NAME));
	if(pButton)
	{
		// not visible or enabled
		//pButton->SetVisible(bShow);
		pButton->SetEnabled(bShow);
	}
}

/**
* Sets the background state of a given label
*
* @param CHiliteTextButton *pButton The label whose background to setup
* @param bool bState If true we will display the bg
* @return void
**/
void CClassMenu::SetBgState(CHiliteTextButton *pButton, bool bState)
{
	// no scheme? abort
	if(!m_pScheme)
		return;

	// are we displaying it?
	if(bState)
	{
		// set everything up
		pButton->SetBackgroundColor(m_pScheme->GetColor("TransparentBlack", Color(0, 0, 0, 128)));
		pButton->SetBgColor(m_pScheme->GetColor("TransparentBlack", Color(0, 0, 0, 128)));
		pButton->SetPaintBackgroundEnabled(true);
		pButton->SetPaintBackgroundType(2);
		pButton->SetEnabled(false);
	}
	else
	{
		// turn everything off
		pButton->SetBackgroundColor(Color(0, 0, 0, 0));
		pButton->SetBgColor(Color(0, 0, 0, 0));
		pButton->SetPaintBackgroundEnabled(false);
		pButton->SetEnabled(true);
	}
}

/**
* Displays or hides the class menu
*
* @param bool bShow Determines whether we are displaying or hiding
* @return void
**/
void CClassMenu::ShowPanel(bool bShow)
{
	// are we displaying or hiding?
	if ( bShow )
	{
		// sort out the cancel button
		ShowCancelButton(!m_bForceChooseState);

		// grab all our labels
		m_pSoldierButton = (CHiliteTextButton *)FindChildByName("SoldierButton");
		m_pScoutButton = (CHiliteTextButton *)FindChildByName("ScoutButton");
		m_pHWButton = (CHiliteTextButton *)FindChildByName("HWButton");
		m_pSniperButton = (CHiliteTextButton *)FindChildByName("SniperButton");

		// make sure we have a player
		if(CBasePlayer::GetLocalPlayer())
		{
			// setup the backgrounds for all my labels
			SetBgState(m_pSoldierButton, !((CSDKGameRules *)g_pGameRules)->CanJoinClass(CSDKPlayer::GetLocalSDKPlayer(), "soldier"));
			SetBgState(m_pHWButton, !((CSDKGameRules *)g_pGameRules)->CanJoinClass(CSDKPlayer::GetLocalSDKPlayer(), "hw"));
			SetBgState(m_pScoutButton, !((CSDKGameRules *)g_pGameRules)->CanJoinClass(CSDKPlayer::GetLocalSDKPlayer(), "scout"));
			SetBgState(m_pSniperButton, !((CSDKGameRules *)g_pGameRules)->CanJoinClass(CSDKPlayer::GetLocalSDKPlayer(), "sniper"));
		}

		// turn it on and give them mouse input
		Activate();
		SetMouseInputEnabled( true );
		
		// figure out what the scoreboard key is
		if ( m_iScoreBoardKey < 0 ) 
			m_iScoreBoardKey = gameuifuncs->GetEngineKeyCodeForBind( "showscores" );
	}
	else
	{
		// hide it with no mouse input
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
	
	m_pViewPort->ShowBackGround(bShow);
}

/**
* Allows the scoreboard to be displayed while the class menu is up
*
* @param KeyCode code The key that was pressed
* @return void
**/
void CClassMenu::OnKeyCodePressed(KeyCode code)
{
	int lastPressedEngineKey = engine->GetLastPressedEngineKey();

	if ( m_iScoreBoardKey >= 0 && m_iScoreBoardKey == lastPressedEngineKey )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

/**
* Called when the menu recieves a command
*
* @param const char *szCommand The command we need to process
* @return void
**/
void CClassMenu::OnCommand( const char *szCommand)
{
	// if we're not canceling, pass the command on to whoever is listening
	if(Q_stricmp(szCommand, "vguicancel"))
		engine->ClientCmd(const_cast<char *>(szCommand));
	
	// destroy the window
	Close();
	gViewPortInterface->ShowBackGround( false );

	// go down
	BaseClass::OnCommand(szCommand);
}