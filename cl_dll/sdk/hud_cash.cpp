#include "cbase.h"
#include "hud_cash.h"
#include "hud_macros.h"
#include "c_sdk_player.h"

#include <Color.h>
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Default constructor
**/
HudCash::HudCash(const char *szName)
	: CHudElement(szName), CHudNumericDisplay(NULL, szName)	
{

	// no cash to start
	m_iCash = 0;
	Q_snprintf(m_szLabel, sizeof(m_szLabel), "$");

	// set the initial value and the label
	SetDisplayValue(m_iCash);
	SetLabelText((wchar_t *)(&(m_szLabel[0])));

	// we're proportional
	SetProportional(true);
}

/**
 * No cash when you restart.
 */
void HudCash::LevelInit(void)
{
	m_iCash = 0;
	SetDisplayValue(m_iCash);
}

/**
* Determines whether or not we should draw the hud cash element
*
* @return bool
**/
bool HudCash::ShouldDraw(void)
{
	return true;
}

/**
* Called when we need to change the value displayed on the hud
*
* @param int iCash The new cash value
* @return void
**/
void HudCash::UpdateCash(int iCash)
{
	// determine the difference
	m_iDifference = m_iCash - iCash;

	// set the value
	m_iCash = iCash;

	// change the number
	SetDisplayValue(m_iCash);
}

/**
* Initializes the element
*
* @return void
**/
void HudCash::Init(void)
{
	// hook the message
	CHudElement::Reset();
}

/**
* Paints the cash
*
* @return void
**/
void HudCash::Paint(void)
{
	CSDKPlayer *pPlayer;

	// do we have a player yet?
	if(!CBasePlayer::GetLocalPlayer())
		return;

	// did we get them?
	pPlayer = ToSDKPlayer(CBasePlayer::GetLocalPlayer());
	if(!pPlayer)
		return;

	// update our cash
	UpdateCash(pPlayer->GetCash());

	// go down
	CHudNumericDisplay::Paint();
}