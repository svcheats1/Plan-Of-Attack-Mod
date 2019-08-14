//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
//  hud_msg.cpp
//
#include "cbase.h"
#include "clientmode.h"
#include "hudelement.h"
#include "keyvalues.h"
#include "vgui_controls/AnimationController.h"
#include "engine/IEngineSound.h"
#include <bitbuf.h>
#include <cl_dll/iviewport.h>
#include "teammenu.h"
#include "classmenu.h"
#include "objectivemenu.h"
#include "punishmenu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/// USER-DEFINED SERVER MESSAGE HANDLERS

void CHud::MsgFunc_ResetHUD( bf_read &msg )
{
	ResetHUD();
}

void CHud::ResetHUD()
{
	// clear all hud data
	g_pClientMode->GetViewportAnimationController()->CancelAllAnimations();

	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->Reset();
	}

	g_pClientMode->GetViewportAnimationController()->RunAllAnimationsToCompletion();

	// reset sensitivity
	m_flMouseSensitivity = 0;
	m_flMouseSensitivityFactor = 0;
}

void CHud::MsgFunc_ChangeTeam( bf_read &msg )
{
	if ( gViewPortInterface )
	{
		CTeamMenu *pPanel = (CTeamMenu *)gViewPortInterface->FindPanelByName( PANEL_TEAM );
		pPanel->SetForceChooseState(true);

		gViewPortInterface->ShowPanel(pPanel, true);
	}
}

void CHud::MsgFunc_ChangeSkill(bf_read &msg)
{
	if(gViewPortInterface)
	{
		// grab the panel and show it
		CClassMenu *pPanel = (CClassMenu *)gViewPortInterface->FindPanelByName(PANEL_CLASS);
		pPanel->SetForceChooseState(true);

		gViewPortInterface->ShowPanel(pPanel, true);
	}
}

/**
* Handles messages requesting to display the world map
*
* @return void
**/
void CHud::MsgFunc_WorldMap(bf_read &msg)
{
	// make sure we have the viewport
	if(gViewPortInterface)
		gViewPortInterface->ShowPanel(PANEL_WORLDMAP, true);
}

/**
* Handles messages requesting to display the objectives menu
*
* @return void
**/
void CHud::MsgFunc_ChooseObjective(bf_read &msg)
{
	// viewport?
	if(gViewPortInterface)
	{
		// grab the panel
		CObjectiveMenu *pPanel = (CObjectiveMenu *)gViewPortInterface->FindPanelByName(PANEL_OBJECTIVES);
		// set the timer
		pPanel->SetTimer(msg.ReadFloat());
		
		// show the panel
		gViewPortInterface->ShowPanel(PANEL_OBJECTIVES, true);
	}
}

/**
* Handles messages requesting to display the buy menu
*
* @return void
**/
void CHud::MsgFunc_BuyMenu(bf_read &msg)
{
	// viewport?
	if(gViewPortInterface)
	{
		// show the panel
		gViewPortInterface->ShowPanel(PANEL_BUY, true);

		/*
		CBuyMenu *pPanel = (CBuyMenu *)gViewPortInterface->FindPanelByName( PANEL_BUY );
		pPanel->ShowPanel(true);
		*/
	}
}

/**
* Handles the messages about who we need to punish
*
* @param bf_read &msg The message to handle
* @return void
**/
void CHud::MsgFunc_AllowPunish(bf_read &msg)
{
	char szName[256];
	int iPlayer;

	// pull the player id
	iPlayer = msg.ReadShort();
	msg.ReadString(szName, sizeof(szName));

	// make sure we have the viewport
	if(gViewPortInterface)
	{
		// pull the panel
		CPunishMenu *pPanel = (CPunishMenu *)gViewPortInterface->FindPanelByName(PANEL_PUNISH);

		// set the player and name
		pPanel->SetPlayer(iPlayer);
		pPanel->SetPlayerName(szName);

		// display it
		pPanel->ShowPanel(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CHud::MsgFunc_SendAudio( bf_read &msg )
{
	char szString[2048];
	msg.ReadString( szString, sizeof(szString) );
	
	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, szString );
}
