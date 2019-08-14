#include "cbase.h"
#include "hud_dyncrosshair.h"
#include "c_sdk_player.h"
#include <vgui/isurface.h>
#include "weapon_sdkbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT(HudDynamicCrosshair);

#define MIN_GROW_RATE 0.5
#define SCALE_RATIO (float)ScreenWidth() / 800.0

/**
* Convar stuff
**/
static ConVar cl_crosshair("cl_crosshair", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Sets the type of crosshair to use (0 = circle, 1 = lines)");

/**
* Constructor
**/
HudDynamicCrosshair::HudDynamicCrosshair(const char *szName)
	: CHudElement(szName), m_bShouldDraw(false), Panel(NULL, "HudDynamicCrosshair")
{
	// set the parent
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// not visible
	SetVisible(true);
	m_bShouldDraw = true;
	m_flCurrentRadius = 0;
}

/**
* Applies the new scheme settings
*
* @param IScheme *pScheme The new scheme
* @return void
**/
void HudDynamicCrosshair::ApplySchemeSettings(IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// move down
	BaseClass::ApplySchemeSettings(pScheme);
}

/**
* Determines whether or not we should draw the HudDynamicCrosshair element
*
* @return bool
**/
bool HudDynamicCrosshair::ShouldDraw(void)
{
	C_SDKPlayer *pPlayer = dynamic_cast<C_SDKPlayer*>(C_BasePlayer::GetLocalPlayer());
	if (!pPlayer)
		return false;

	// if i'm not a real team, i don't need to shoot anything
	if(pPlayer->GetTeamNumber() < 0)
		return false;

	// if i'm an observer...
	if(pPlayer->IsObserver()) {
		// am I watching in first person?
		if (pPlayer->GetObserverMode() != OBS_MODE_IN_EYE)
			return false;
	
		// am I watching someone? are they alive?
		C_SDKPlayer *pObsTarget = dynamic_cast<C_SDKPlayer*>(pPlayer->GetObserverTarget());
		if (!pObsTarget || !pObsTarget->IsAlive())
			return false;

		// do they have a gun?
        CBaseMachineGun *pWpn = dynamic_cast<CBaseMachineGun*>(pObsTarget->GetActiveWeapon());
		if (!pWpn)
			return false;
		
		// does that gun say it should draw a crosshair?
		return pWpn->ShouldDrawCrosshair();
	}

	// i'm not an observer, get my gun
    CBaseMachineGun *pWpn = dynamic_cast<CBaseMachineGun*>(pPlayer->GetActiveWeapon());
	if (!pWpn)
		return false;
	
	// does my gun say it needs a crosshair?
	return pWpn->ShouldDrawCrosshair();
}

/**
* Initializes the element
*
* @return void
**/
void HudDynamicCrosshair::Init(void)
{
	// reset
	CHudElement::Reset();
}

/**
* Paints the texture and current skill level
*
* @return void
**/
void HudDynamicCrosshair::Paint(void)
{
	// the ShouldDraw thing seems "laggy" so I'm going to punch it in the nose instead
	if (!ShouldDraw())
		return;

	int iCenter[2];
	iCenter[0] = ScreenWidth() >> 1;
	iCenter[1] = ScreenHeight() >> 1;

	float flRadius = GetAimAsPixels();
	float flScaleRatio = SCALE_RATIO;
	float flGrowRate = MIN_GROW_RATE;

	// JD made up some numbers.
	flGrowRate *= max(1, abs(m_flCurrentRadius - flRadius) / (flGrowRate * 20 * flScaleRatio));

	if (m_flCurrentRadius == 0)
		m_flCurrentRadius = flRadius;
	else if (m_flCurrentRadius > flRadius) {	// shrinking
		m_flCurrentRadius -= min(m_flCurrentRadius - flRadius, flScaleRatio * flGrowRate);
	}
	else if (m_flCurrentRadius < flRadius) {	// growing
		m_flCurrentRadius += min(flRadius - m_flCurrentRadius, flScaleRatio * flGrowRate);
	}
	
	// draw the dot
	surface()->DrawSetColor(CROSSHAIR_COLOR);
	vgui::surface()->DrawOutlinedCircle(iCenter[0], iCenter[1], 1, 4);

	// circle or lines?
	if(cl_crosshair.GetInt() == 0)
		vgui::surface()->DrawOutlinedCircle(iCenter[0], iCenter[1], m_flCurrentRadius, 90);
	else
	{
		// draw the four lines
		vgui::surface()->DrawLine(iCenter[0] + m_flCurrentRadius, iCenter[1], iCenter[0] + m_flCurrentRadius + CROSSHAIR_LINE_LENGTH, iCenter[1]);
		vgui::surface()->DrawLine(iCenter[0] - m_flCurrentRadius, iCenter[1], iCenter[0] - m_flCurrentRadius - CROSSHAIR_LINE_LENGTH, iCenter[1]);
		vgui::surface()->DrawLine(iCenter[0], iCenter[1] + m_flCurrentRadius, iCenter[0], iCenter[1] + m_flCurrentRadius + CROSSHAIR_LINE_LENGTH);
		vgui::surface()->DrawLine(iCenter[0], iCenter[1] - m_flCurrentRadius, iCenter[0], iCenter[1] - m_flCurrentRadius - CROSSHAIR_LINE_LENGTH);
	}

	// check that we actually have a scheme
	/*if(m_pScheme)
	{
		CharRenderInfo pInfo;
		int iWide, iTall;

		// set all the drawing info
		surface()->DrawSetTextColor(CROSSHAIR_COLOR);
		surface()->DrawSetTextFont(m_pScheme->GetFont(CROSSHAIR_FONT));

		// get all the circle info and assign the left overs
		surface()->DrawGetUnicodeCharRenderInfo(CROSSHAIR_CIRCLE, pInfo);
		pInfo.fontTall = 72;
		pInfo.x = iCenter[0] - pInfo.fontTall;
		pInfo.y = iCenter[1] - pInfo.fontTall;

		// draw the circle
		surface()->DrawRenderCharFromInfo(pInfo);

		// draw the dot
		iWide = surface()->GetCharacterWidth(m_pScheme->GetFont(CROSSHAIR_FONT), CROSSHAIR_DOT);
		iTall = surface()->GetFontTall(m_pScheme->GetFont(CROSSHAIR_FONT));
		surface()->DrawSetTextPos(iCenter[0] - (iWide / 2), iCenter[1] - (iTall / 2));
		surface()->DrawUnicodeChar(CROSSHAIR_DOT);
	}*/

	BaseClass::Paint();
}

/**
 * Returns the vector aim as a number of pixels in radius.
 */
//extern float in_fov;
float HudDynamicCrosshair::GetAimAsPixels(void)
{
	float fDegrees = GetAimVector();		// probably between 0.01 and 0.20

	if (C_BasePlayer::GetLocalPlayer()) {
		// make it bigger if they're zoomed in. I'm not really sure if this equation works (seems to)
		int iFOV = C_BasePlayer::GetLocalPlayer()->m_Local.m_iFOV;
		if (iFOV != 0)
			fDegrees = 90.0 / iFOV * fDegrees;
		
		// minimum of 0.01, so they can still see where they're aiming.
		if (fDegrees < 0.01)
			fDegrees = 0.01;
	}

	// normalize to 800 pixels then guess on the size after that
	return SCALE_RATIO * fDegrees * 400.0;
}

/**
 * Gets the vector that the current aim is affected by
 */
float HudDynamicCrosshair::GetAimVector(void)
{
	C_SDKPlayer *pPlayer = dynamic_cast<C_SDKPlayer*>(C_BasePlayer::GetLocalPlayer());
	if (!pPlayer)
		return 0.0;

	// if i'm an observer
	if(pPlayer->IsObserver()) {
		C_SDKPlayer *pObsTarget = dynamic_cast<C_SDKPlayer*>(pPlayer->GetObserverTarget());
		if (!pObsTarget)
            return 0.0;

		CBaseMachineGun *pWpn = dynamic_cast<CBaseMachineGun*>(pObsTarget->GetActiveWeapon());
		if (!pWpn)
			return 0.0;

        return pWpn->GetSpread(pObsTarget);
	}

	CBaseMachineGun *pWpn = dynamic_cast<CBaseMachineGun*>(pPlayer->GetActiveWeapon());
	if (!pWpn)
		return 0.0;

    return pWpn->GetSpread(pPlayer);
}