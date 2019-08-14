#include "cbase.h"
#include "hud_selftimer.h"
#include "c_sdk_player.h"
#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Default constructor
**/
CHudSelfTimer::CHudSelfTimer(const char *szName)
	: CHudBaseTimer(NULL, szName) , CHudElement(szName)
{
	// start at negative 1
	m_fEndTime = -1;
}

/**
* Sets the duration of the timer
*
* @param float fDuration Length of the timer
* @return void
**/
void CHudSelfTimer::SetTimerDuration(float fDuration)
{
	// set the end time
	m_fDuration = fDuration;
}

/**
* Sets the end time
*
* @param float m_fTime Time to end at
* @return void
**/
void CHudSelfTimer::SetEndTime(float fTime)
{
	// set it
	m_fEndTime = fTime;
}

/**
* Determines if the hud element needs to be drawn
* Only draw if we have a time that is non-negative
*
* @return bool
**/
bool CHudSelfTimer::ShouldDraw(void)
{
	return (m_fEndTime >= 0 && CHudElement::ShouldDraw());
}

/**
* Determines the number of minutes in the given number of seconds
*
* @return int
**/
int CHudSelfTimer::DetermineMinutes()
{
	float flSeconds = m_fEndTime - gpGlobals->curtime;
	return flSeconds / 60;
}

/**
* Determines the number of seconds left after converting iSeconds to minutes
*
* @return int
**/
int CHudSelfTimer::DetermineSeconds()
{
	float flSeconds = m_fEndTime - gpGlobals->curtime;
	return Round(flSeconds) % 60;
}

/**
 * Determines which rounding function we use for float -> int display
 */
int CHudSelfTimer::Round(float fl)
{
	return floor(fl);
}

/**
* Called prior to painting by VGui::Panel.  Updates the time.
*
* @return void
**/
void CHudSelfTimer::OnThink(void)
{
	// are we past time?
	if(gpGlobals->curtime > m_fEndTime)
		m_fEndTime = -1;

	// do we need to bother?
	if(m_fEndTime != -1)
	{
		// do we need to update the time?
		if(DetermineSeconds() != GetSeconds())
		{
			// set the minutes and seconds
			SetMinutes(DetermineMinutes());
			SetSeconds(DetermineSeconds());
		}
	}

	// go down some
	BaseClass::OnThink();
}

/*********************************************************/
/* HudRoundTimer
/*********************************************************/

DECLARE_HUDELEMENT(HudRoundTimer);
DECLARE_HUD_MESSAGE(HudRoundTimer, SyncRoundTimer);

/**
* Default constructor
**/
HudRoundTimer::HudRoundTimer(const char *szName)
	: CHudSelfTimer(szName)
{
	// hide when this stuff happens
	SetHiddenBits( 0 );

	// load our textures
	AddToSet("vgui/hud_timer_icon_attack");
	AddToSet("vgui/hud_timer_icon_defend");

	// position the icon and size it
	SetIconPos(ROUND_TIMER_ICON_XOFFSET, ROUND_TIMER_ICON_YOFFSET);
	SetIconSize(ROUND_TIMER_ICON_WIDTH, ROUND_TIMER_ICON_HEIGHT);
}

/**
* Initializes the element
*
* @return void
**/
void HudRoundTimer::Init(void)
{
	// hook the message
	HOOK_HUD_MESSAGE(HudRoundTimer, SyncRoundTimer);
	CHudElement::Reset();
}

/**
* Handles messages for syncing the round timer
*
* @param bf_read &msg The message to handle
* @return void
**/
void HudRoundTimer::MsgFunc_SyncRoundTimer(bf_read &msg)
{
	// set the timer
	SetEndTime(msg.ReadFloat() + gpGlobals->curtime);
}

/**
* Determines if we should draw this timer
*
* @return bool True if we should draw
**/
bool HudRoundTimer::ShouldDraw(void)
{
	return CHudSelfTimer::ShouldDraw();
}

/**
* Applies the new scheme settings
*
* @param IScheme *pScheme The new scheme
* @return void
**/
void HudRoundTimer::ApplySchemeSettings(IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// move down
	BaseClass::ApplySchemeSettings(pScheme);

	// set the background
	SetBgColor(m_pScheme->GetColor("Menu.BgColor", Color(255, 255, 255, 0)));
	this->SetPaintBackgroundEnabled(false);
}

/**
* Futzes with the texture name to add a prefix based on the clients team
*
* @param const char *szName Name of the texture
* @param char *szStr The string to fill in
* @param int iSize The amount of buffer space we have
* @return void
**/
void HudRoundTimer::GetBackgroundTextureName(const char *szName, char *szStr, int iSize)
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
	else
		Q_snprintf(szStr, iSize, "vgui/red/%s", szName);
}

/**
* Paints the timer
*
* @return void
**/
void HudRoundTimer::Paint(void)
{	
	// don't draw if the hud is hidden
	// our base classes aren't hud elements...
	if(gHUD.IsHidden(m_iHiddenBits))
		return;

	// jump down
	BaseClass::Paint();

	// do we have a player?
	if(CBasePlayer::GetLocalPlayer() && CBasePlayer::GetLocalPlayer()->GetTeam())
	{
		// not on real team = -1
		int iTeamNumber = clamp(CBasePlayer::GetLocalPlayer()->GetTeamNumber(), TEAM_A, TEAM_B);
	
		C_Team *pTeam = GetGlobalTeam(iTeamNumber);
		
		// set the active icon
		m_iActiveIcon = (pTeam->IsOffensive()
								? ROUND_TIMER_ATTACK
								: ROUND_TIMER_DEFEND);
	}

	// draw the icon
	DrawSelf();
}

/**
* Handles case where the screen size changes
*
* @param int iOldWidth
* @param int iOldHeight
* @return void
**/
void HudRoundTimer::OnScreenSizeChanged(int iOldWidth, int iOldHeight)
{
	// jump down
	BaseClass::OnScreenSizeChanged(iOldWidth, iOldHeight);

	// reset all of our size info
	SetIconPos(ROUND_TIMER_ICON_XOFFSET, ROUND_TIMER_ICON_YOFFSET);
	SetIconSize(ROUND_TIMER_ICON_WIDTH, ROUND_TIMER_ICON_HEIGHT);
}