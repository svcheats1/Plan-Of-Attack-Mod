#include "cbase.h"
#include "hud_stamina.h"

/**
* Constructor
**/
HudStamina::HudStamina(const char *szName)
	: BaseClass(NULL, "HudStamina"), CHudElement(szName)
{
	// set the parent
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// hide when this stuff happens
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD);

	// we should have stamina
	m_bOutOfStamina = false;
}

/**
* Accessor allowing others to know that we are out of stamina
*
* @param bool bOutOfStamina True if we are out of stamina
* @return void
**/
void HudStamina::SetOutOfStamina(bool bOutOfStamina)
{
	m_bOutOfStamina = bOutOfStamina;
}

/**
* Determines if we should draw the element
*
* @return bool
**/
bool HudStamina::ShouldDraw(void)
{
	CSDKPlayer *pPlayer;

	// do we have a player?
	if(!CBasePlayer::GetLocalPlayer())
		return false;

	// does the player have stamina?
	pPlayer = ToSDKPlayer(CBasePlayer::GetLocalPlayer());
	if(!pPlayer)
		return false;
	
	// check to see if we have regained our stamina
	if(pPlayer->GetStamina() >= REQ_STAMINA_JUMP)
		m_bOutOfStamina = false;
	
	// should we display?  only if we are currently regaining stamina or we are out of stamina
	if(m_bOutOfStamina || pPlayer->GetStamina() <= 0)
	{
		// set the var for next time and don't draw
		m_bOutOfStamina = true;
		return CHudElement::ShouldDraw();
	}

	return false;
}