//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// Health.cpp
//
// implementation of CHudHealth class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#include "iclientmode.h"
#include "c_sdk_player.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

#include <vgui/ILocalize.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_numericdisplay.h"

#include "ConVar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_HEALTH -1

//-----------------------------------------------------------------------------
// Purpose: Health panel
//-----------------------------------------------------------------------------
class CHudHealth : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE( CHudHealth, CHudNumericDisplay );

public:
	CHudHealth( const char *pElementName );
	~CHudHealth();
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Reset( void );
	virtual void OnThink();
	void MsgFunc_Damage( bf_read &msg );
	virtual void Paint(void);
	virtual void ApplySettings(KeyValues *pData);
	virtual void OnScreenSizeChanged(int iOldWidth, int iOldHeight);

protected:
	// inherited methods
	virtual void GetBackgroundTextureName(const char *szName, char *szStr, int iSize);

private:
	CPanelAnimationVarAliasType(int, m_iIcon, "icon", "", "textureid");
	CHudTexture *m_pIcon;

	// old variables
	int		m_iHealth;
	
	int		m_bitsDamage;
};	

DECLARE_HUDELEMENT( CHudHealth );
DECLARE_HUD_MESSAGE( CHudHealth, Damage );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudHealth::CHudHealth( const char *pElementName ) 
	: CHudElement( pElementName ), CHudNumericDisplay(NULL, "HudHealth")
{
	// hide when this stuff happens
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	// no background
	SetPaintBackgroundEnabled(false);

	// no icon
	m_pIcon = NULL;
}

/**
* Destructor
**/
CHudHealth::~CHudHealth()
{
	// kill the texture
	if(m_pIcon)
		delete(m_pIcon);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::Init()
{
	HOOK_HUD_MESSAGE( CHudHealth, Damage );
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::Reset()
{
	m_iHealth		= INIT_HEALTH;
	m_bitsDamage	= 0;

	/*wchar_t *tempString = vgui::localize()->Find("#Hud_HEALTH");
	if (tempString)
	{
		SetLabelText(tempString);
	}
	else
	{
		SetLabelText(L"HEALTH");
	}
	SetIndent(true);*/
	SetDisplayValue(m_iHealth);

	// no background
	SetPaintBackgroundEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::OnThink()
{
	int newHealth = 0;

	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( local )
	{
		// Never below zero
		newHealth = max( local->GetHealth(), 0 );
	}

	// Only update the fade if we've changed health
	if ( newHealth == m_iHealth)
	{
		return;
	}

	m_iHealth = newHealth;
	
	//if ( m_iHealth >= 20 )
	//{
	// JD: apparently this draws a transparent box (with rounded corners!) around it. Frankly
	// it's kind of odd.
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthIncreasedAbove20");
	//}
	/*
	else if ( m_iHealth > 0 )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthIncreasedBelow20");
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthLow");
	}
	*/

	SetDisplayValue(m_iHealth);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

// @TRJ - this message is never sent.  all syncing is done through the send tables
void CHudHealth::MsgFunc_Damage( bf_read &msg )
{

	int armor = msg.ReadByte();	// armor
	int damageTaken = msg.ReadByte();	// health
	long bitsDamage = msg.ReadLong(); // damage bits
	armor; damageTaken; bitsDamage; // variable still sent but not used

	Vector vecFrom;

	vecFrom.x = msg.ReadBitCoord();
	vecFrom.y = msg.ReadBitCoord();
	vecFrom.z = msg.ReadBitCoord();

	/*
	// Actually took damage?
	if ( damageTaken > 0 || armor > 0 )
	{
		if ( damageTaken > 0 )
		{
			// start the animation
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthDamageTaken");
		}
	}
	*/
}

/**
* Futzes with the texture name to add a prefix based on the clients team
*
* @param const char *szName Name of the texture
* @param char *szStr The string to fill in
* @param int iSize The amount of buffer space we have
* @return void
**/
void CHudHealth::GetBackgroundTextureName(const char *szName, char *szStr, int iSize)
{
	// do we have a player yet?
	if(CSDKPlayer::GetLocalPlayer())
	{
		// red or blue?
		if(CSDKPlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_B)
			Q_snprintf(szStr, iSize, "vgui/red/%s", szName);
		else
			Q_snprintf(szStr, iSize, "vgui/blue/%s", szName);
	}
}

/**
* Applies the new settings to the health info
*
* @param KeyValues *pData The data we need to set
* @return void
**/
void CHudHealth::ApplySettings(KeyValues *pData)
{
	// jump down
	BaseClass::ApplySettings(pData);

	// create the icon
	if(m_iIcon != -1 && !m_pIcon)
	{
		// set all the icon info
		m_pIcon = new CHudTexture();
		m_pIcon->bRenderUsingFont = false;
		m_pIcon->SetBounds(0, XRES(51), 0, YRES(51));
		m_pIcon->textureId = m_iIcon;
	}
}

/**
* Paints the health info
*
* @return void
**/
void CHudHealth::Paint(void)
{
	// paint the base class
	BaseClass::Paint();

	// draw the icon
	if(m_pIcon)
		m_pIcon->DrawSelf(0, 0, Color(255, 255, 255, 255));
}

/**
* Handles case where the screen size changes
*
* @param int iOldWidth
* @param int iOldHeight
* @return void
**/
void CHudHealth::OnScreenSizeChanged(int iOldWidth, int iOldHeight)
{
	// jump down
	BaseClass::OnScreenSizeChanged(iOldWidth, iOldHeight);

	// reset the icon size
	if(m_pIcon)
		m_pIcon->SetBounds(0, XRES(51), 0, YRES(51));
}