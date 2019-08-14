#include "cbase.h"
#include "hud_sniperscope.h"
#include "c_sdk_player.h"
#include "weapon_sniper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Default constructor
**/
HudSniperScope::HudSniperScope(const char *szName)
	: CHudElement(szName), Panel(NULL, "HudSniperScope")
{
	// set the parent
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// set all the parameters
	Q_strncpy(szShortName, "SniperScopeTex", sizeof(szShortName));
	Q_strncpy(szTextureFile, SNIPERSCOPE_TEX, sizeof(szTextureFile));
	bRenderUsingFont = false;

	// figure out a texture id
	textureId = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile(textureId, szTextureFile, true, false);

	// set the color
	m_clr = Color(255, 255, 255, 255);

	// we're visible
	SetVisible(true);
}

/**
* Applies the new scheme settings
*
* @param IScheme *pScheme The new scheme
* @return void
**/
void HudSniperScope::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// move down
	BaseClass::ApplySchemeSettings(pScheme);
}

/**
* Determines whether or not we should draw the overlay
* @HACK - ask the weapon
*
* @return bool
**/
bool HudSniperScope::ShouldDraw(void)
{
	CBasePlayer *pPlayer;
	CBaseSniperRifle *pWpn;

	// get the current player
	pPlayer = CSDKPlayer::GetLocalPlayer();
	if(!pPlayer)
		return false;

	// see if we have a sniper rifle
	pWpn = dynamic_cast<CBaseSniperRifle *>(pPlayer->GetActiveWeapon());

	// must be using a sniper rifle and be zoomed in
	return pWpn && !pWpn->IsScopeHidden();
}

/**
* Initializes the element
*
* @return void
**/
void HudSniperScope::Init(void)
{
	// reset
	CHudElement::Reset();
}

/**
* Paints the overlay
*
* @return void
**/
void HudSniperScope::Paint(void)
{
	// go down
	BaseClass::Paint();

	// are we drawing?
	if(ShouldDraw())
	{
		int iWide, iTall, iX;
		float fScale;

		// get the size of the screen
		vgui::surface()->GetScreenSize(iWide, iTall);

		// make us the full size of the screen
		SetSize(iWide, iTall);

		// figure out the scale
		fScale = (float)iTall / (float)SNIPERSCOPE_TEX_HEIGHT;

		// where are we drawing it?
		iX = ((float)iWide - ((float)SNIPERSCOPE_TEX_WIDTH * fScale)) / 2;

		// draw the left gap
		vgui::surface()->DrawSetColor(0, 0, 0, 255);
		vgui::surface()->DrawFilledRect(0, 0, iX, iTall);

		// draw the hole
		DrawSelf(iX, 0, (float)SNIPERSCOPE_TEX_WIDTH * fScale, (float)SNIPERSCOPE_TEX_HEIGHT * fScale, m_clr);

		// draw the right gap
		vgui::surface()->DrawSetColor(0, 0, 0, 255);
		vgui::surface()->DrawFilledRect(iWide - iX, 0, iWide, iTall);

		// draw the cross hairs
		vgui::surface()->DrawFilledRect((iWide / 2) - 1, (iTall / 2) - SNIPERSCOPE_RADIUS - 2, 
									(iWide / 2) + 1, (iTall / 2) - CROSSHAIR_RADIUS);
		vgui::surface()->DrawFilledRect((iWide / 2) - 1, (iTall / 2) + CROSSHAIR_RADIUS, 
									(iWide / 2) + 1, (iTall / 2) + SNIPERSCOPE_RADIUS + 2);
		vgui::surface()->DrawFilledRect((iWide / 2) - SNIPERSCOPE_RADIUS - 2, (iTall / 2) - 1,
									(iWide / 2) - CROSSHAIR_RADIUS, (iTall / 2) + 1);
		vgui::surface()->DrawFilledRect((iWide / 2) + CROSSHAIR_RADIUS, (iTall / 2) - 1,
									(iWide / 2) + SNIPERSCOPE_RADIUS + 2, (iTall / 2) + 1);

		// draw the dot
		vgui::surface()->DrawSetColor(192, 28, 0, 140);
		vgui::surface()->DrawOutlinedCircle(iWide / 2, iTall / 2, 2, 32);
	}
}

/**
* Draws the hud texture
*
* @param int iX X-coord to draw at
* @param int iY Y-coord to draw at
* @param int iW Width to draw
* @param int iH Height to draw
* @param Color &clr The color to use for the texture
* @return void
**/
void HudSniperScope::DrawSelf(int iX, int iY, int iW, int iH, Color& clr) const
{
	// do we have a texture?
	if ( textureId == -1 )
			return;

	// set the texture and color
	vgui::surface()->DrawSetTexture(textureId);
	vgui::surface()->DrawSetColor(clr);
	vgui::surface()->DrawTexturedSubRect(iX, iY, iX + iW, iY + iH, 0, 0, 1, 1);
}