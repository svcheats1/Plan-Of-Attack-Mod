#include "cbase.h"
#include "c_sdk_player.h"
#include "iclientmode.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_levelchanged.h"
#include <vgui_controls/Label.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HUDLVLCHANGE_DISPLAY_TIME 5.0
#define HUDLVLCHANGE_FADE_TIME 3.0

using namespace vgui;

DECLARE_HUDELEMENT(HudLevelChanged);

/**
* Constructor
**/
HudLevelChanged::HudLevelChanged(const char *szName)
	: Label(NULL, "HudLevelChanged", ""), CHudElement(szName)
{
	m_iCurrentLevel = 1;
	m_fEndTime = 0;

	// set the parent
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// hide when this stuff happens
	SetHiddenBits( 0 );
}

/**
 * Sets the label's font and color.
 */
void HudLevelChanged::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFont(pScheme->GetFont("LevelUpIcons"));
	SetFgColor((m_StartColor = pScheme->GetColor("HudLevelChanged.FgColor", Color(255,255,255,255))));
	SetContentAlignment(a_northwest);
}

/**
* Determines if we should draw the element
*
* @return bool
**/
bool HudLevelChanged::ShouldDraw(void)
{
	return (gpGlobals->curtime < m_fEndTime && CHudElement::ShouldDraw());
}

void HudLevelChanged::LevelChanged(int iNewLevel)
{
	char text[32];

	// level went down, not up
	// <= prevents level boost icons
	if (iNewLevel <= m_iCurrentLevel)
	{
		// reset if we're going down
		if(iNewLevel < m_iCurrentLevel)
			m_fEndTime = 0;

		m_iCurrentLevel = max(iNewLevel, 1);
		return;
	}
	CSDKPlayer *pPlayer;

	// set the new level
	m_iCurrentLevel = iNewLevel;

	// do we have a player?
	if(!CBasePlayer::GetLocalPlayer())
		return;

	pPlayer = ToSDKPlayer(CBasePlayer::GetLocalPlayer());
	if (!pPlayer || !pPlayer->GetSkillClass())
		return;

	m_fEndTime = gpGlobals->curtime + HUDLVLCHANGE_DISPLAY_TIME + HUDLVLCHANGE_FADE_TIME;
	m_fStartFadeTime = gpGlobals->curtime + HUDLVLCHANGE_DISPLAY_TIME;

	sprintf(text, "%d %s", m_iCurrentLevel, pPlayer->GetSkillClass()->GetClassLevelIconStr());
	SetText(text);
	SetFgColor(m_StartColor);
}

void HudLevelChanged::Paint(void)
{
	// change the color if we need to fade out
	if (gpGlobals->curtime > m_fStartFadeTime && m_fEndTime)
	{
		SetFgColor(Color(
				m_StartColor.r(),
				m_StartColor.g(), 
				m_StartColor.b(), 
				m_StartColor.a() * (m_fEndTime - gpGlobals->curtime) / HUDLVLCHANGE_FADE_TIME));
	}

	BaseClass::Paint();
}