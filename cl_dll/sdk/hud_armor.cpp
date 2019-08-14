//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
//
// implementation of CHudArmor class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

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

#define INIT_ARMOR -1

//-----------------------------------------------------------------------------
// Purpose: Armor panel
//-----------------------------------------------------------------------------
class CHudArmor : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE( CHudArmor, CHudNumericDisplay );

public:
	CHudArmor( const char *pElementName );
	~CHudArmor();
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Reset( void );
	virtual void OnThink();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void ApplySettings(KeyValues *pData);
	virtual void Paint(void);
	virtual void OnScreenSizeChanged(int iOldWidth, int iOldHeight);

private:
	int m_iArmor;

	CPanelAnimationVarAliasType(int, m_iIcon, "icon", "", "textureid");
	CHudTexture *m_pIcon;

	IScheme *m_pScheme;
};	

DECLARE_HUDELEMENT( CHudArmor );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudArmor::CHudArmor( const char *pElementName ) : CHudElement( pElementName ), CHudNumericDisplay(NULL, "HudArmor")
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	m_pIcon = NULL;
}

/**
* Destructor
**/
CHudArmor::~CHudArmor()
{
	// kill the texture
	if(m_pIcon)
		delete(m_pIcon);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArmor::Init()
{
	Reset();
}

/**
* Applies the data to the hud element
*
* @param KeyValues *pData The data we need to apply
* @return void
**/
void CHudArmor::ApplySettings(KeyValues *pData)
{
	// let the base class do it
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArmor::Reset()
{
	m_iArmor		= INIT_ARMOR;

	/*wchar_t *tempString = vgui::localize()->Find("#Hud_ARMOR");
	if (tempString)
	{
		SetLabelText(tempString);
	}
	else
	{
		SetLabelText(L"ARMOR");
	}
	SetIndent(true);*/
	SetDisplayValue(m_iArmor);
}

/**
* Applies the new scheme settings
*
* @param IScheme *pScheme The new scheme
* @return void
**/
void CHudArmor::ApplySchemeSettings(IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// move down
	BaseClass::ApplySchemeSettings(pScheme);

	// set the background
	SetBgColor(m_pScheme->GetColor("Menu.BgColor", Color(255, 255, 255, 0)));
	this->SetPaintBackgroundEnabled(false);
	//this->SetPaintBackgroundType(2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArmor::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArmor::OnThink()
{
	int newArmor = 0;

	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( local )
	{
		// Never below zero
		newArmor = max( local->GetArmor(), 0 );
	}

	// Only update the fade if we've changed health
	if (newArmor == m_iArmor)
	{
		return;
	}

	m_iArmor = newArmor;

	SetDisplayValue(m_iArmor);
}

/**
* Paints the armor value
*
* @return void
**/
void CHudArmor::Paint(void)
{
	// jump down
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
void CHudArmor::OnScreenSizeChanged(int iOldWidth, int iOldHeight)
{
	// jump down
	BaseClass::OnScreenSizeChanged(iOldWidth, iOldHeight);

	// reset all of our size info
	if(m_pIcon)
		m_pIcon->SetBounds(0, XRES(51), 0, YRES(51));
}