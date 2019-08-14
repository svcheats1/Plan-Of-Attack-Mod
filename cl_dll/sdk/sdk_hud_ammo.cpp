//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "iclientvehicle.h"
#include "weapon_sdkbase.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include "c_sdk_player.h"
#include "iconset.h"

#define AMMO_ICON_XOFFSET XRES(53)
#define AMMO_ICON_YOFFSET YRES(0)
#define AMMO_ICON_WIDTH XRES(51)
#define AMMO_ICON_HEIGHT YRES(51)

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Displays current ammunition level
//-----------------------------------------------------------------------------
class CHudAmmo : public CHudNumericDisplay, public CHudElement, public IconSet
{
	DECLARE_CLASS_SIMPLE( CHudAmmo, CHudNumericDisplay );

public:
	CHudAmmo( const char *pElementName );
	void Init( void );
	void VidInit( void );
	void Reset();

	void SetAmmo(int ammo, bool playAnimation);
	void SetAmmo2(int ammo2, bool playAnimation);
	virtual void OnScreenSizeChanged(int iOldWidth, int iOldHeight);
	virtual void Paint(void);

protected:
	virtual void OnThink();

	void UpdateAmmoDisplays();
	void UpdatePlayerAmmo( C_BasePlayer *player );
	void GetAmmoInfo(C_WeaponSDKBase *pWpn, int &ammo1, int &ammo2);
	void GetAmmoInfoOld(C_BasePlayer *player, C_BaseCombatWeapon *wpn, int &ammo1, int &ammo2);

	// inherited methods
	virtual void GetBackgroundTextureName(const char *szName, char *szStr, int iSize);

private:
	CHandle< C_BaseCombatWeapon > m_hCurrentActiveWeapon;
	CHandle< C_BaseEntity > m_hCurrentVehicle;
	int		m_iAmmo;
	int		m_iAmmo2;
};

DECLARE_HUDELEMENT( CHudAmmo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudAmmo::CHudAmmo( const char *pElementName ) : BaseClass(NULL, "HudAmmo"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION );

	// load our textures
	AddToSet("vgui/hud_ammo_icon_mgchain");

	// set the active icon
	m_iActiveIcon = 0;

	// position the icon and size it
	SetIconPos(AMMO_ICON_XOFFSET, AMMO_ICON_YOFFSET);
	SetIconSize(AMMO_ICON_WIDTH, AMMO_ICON_HEIGHT);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAmmo::Init( void )
{
	m_iAmmo		= -1;
	m_iAmmo2	= -1;

	/*wchar_t *tempString = vgui::localize()->Find("#Valve_Hud_AMMO");
	if (tempString)
	{
		SetLabelText(tempString);
	}
	else
	{
		SetLabelText(L"AMMO");
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAmmo::VidInit( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Resets hud after save/restore
//-----------------------------------------------------------------------------
void CHudAmmo::Reset()
{
	BaseClass::Reset();

	m_hCurrentActiveWeapon = NULL;
	m_hCurrentVehicle = NULL;
	m_iAmmo = 0;
	m_iAmmo2 = 0;

	UpdateAmmoDisplays();
}

//-----------------------------------------------------------------------------
// Purpose: called every frame to get ammo info from the weapon
//-----------------------------------------------------------------------------
void CHudAmmo::UpdatePlayerAmmo( C_BasePlayer *player )
{
	// Clear out the vehicle entity
	m_hCurrentVehicle = NULL;

	SetPaintBackgroundEnabled(false);

	// Get the weapon
	C_BaseCombatWeapon *wpn = GetActiveWeapon();

	// Check params
	if ( !wpn || !player || !wpn->UsesPrimaryAmmo() )
	{
		SetPaintEnabled(false);
		return;
	}

	// Cast it up
	C_WeaponSDKBase *pWpn = dynamic_cast<C_WeaponSDKBase *>(wpn);

	int ammo1, ammo2;
	// Cast failed, use old ammo system
	if (!pWpn) {
		Assert(pWpn);
		GetAmmoInfoOld(player, wpn, ammo1, ammo2);
	}
	// Use new ammo system
	else {
		GetAmmoInfo(pWpn, ammo1, ammo2);
	}

	SetPaintEnabled(true);

	if (wpn == m_hCurrentActiveWeapon)
	{
		// same weapon, just update counts
		SetAmmo(ammo1, true);
		SetAmmo2(ammo2, true);
	}
	else
	{
		// diferent weapon, change without triggering
		SetAmmo(ammo1, false);
		SetAmmo2(ammo2, false);

		// update whether or not we show the total ammo display
		if (wpn->UsesClipsForAmmo1())
		{
			SetShouldDisplaySecondaryValue(true);
			//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesClips");
		}
		else
		{
			//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseClips");
			SetShouldDisplaySecondaryValue(false);
		}

		//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponChanged");
		m_hCurrentActiveWeapon = wpn;
	}
}

void CHudAmmo::GetAmmoInfo(C_WeaponSDKBase *pWpn, int &ammo1, int &ammo2)
{
	ammo1 = pWpn->Clip1();
	if (ammo1 < 0) {
		ammo1 = pWpn->GetAmmo1();
		ammo2 = 0;
	}
	else {
		ammo2 = pWpn->GetAmmo1();
	}
}

void CHudAmmo::GetAmmoInfoOld(C_BasePlayer *player, C_BaseCombatWeapon *wpn, int &ammo1, int &ammo2)
{
	// get the ammo in our clip
	ammo1 = wpn->Clip1();
	if (ammo1 < 0)
	{
		// we don't use clip ammo, just use the total ammo count
		ammo1 = player->GetAmmoCount(wpn->GetPrimaryAmmoType());
		ammo2 = 0;
	}
	else
	{
		// we use clip ammo, so the second ammo is the total ammo
		ammo2 = player->GetAmmoCount(wpn->GetPrimaryAmmoType());
	}
}

/*
void CHudAmmo::UpdateVehicleAmmo( C_BasePlayer *player, IClientVehicle *pVehicle )
{
	m_hCurrentActiveWeapon = NULL;
	CBaseEntity *pVehicleEnt = pVehicle->GetVehicleEnt();

	if ( !pVehicleEnt || pVehicle->GetPrimaryAmmoType() < 0 )
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		return;
	}

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);

	// get the ammo in our clip
	int ammo1 = pVehicle->GetPrimaryAmmoClip();
	int ammo2;
	if (ammo1 < 0)
	{
		// we don't use clip ammo, just use the total ammo count
		ammo1 = pVehicle->GetPrimaryAmmoCount();
		ammo2 = 0;
	}
	else
	{
		// we use clip ammo, so the second ammo is the total ammo
		ammo2 = pVehicle->GetPrimaryAmmoCount();
	}

	if (pVehicleEnt == m_hCurrentVehicle)
	{
		// same weapon, just update counts
		SetAmmo(ammo1, true);
		SetAmmo2(ammo2, true);
	}
	else
	{
		// diferent weapon, change without triggering
		SetAmmo(ammo1, false);
		SetAmmo2(ammo2, false);

		// update whether or not we show the total ammo display
		if (pVehicle->PrimaryAmmoUsesClips())
		{
			SetShouldDisplaySecondaryValue(true);
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesClips");
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseClips");
			SetShouldDisplaySecondaryValue(false);
		}

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponChanged");
		m_hCurrentVehicle = pVehicleEnt;
	}
}
*/

//-----------------------------------------------------------------------------
// Purpose: called every frame to get ammo info from the weapon
//-----------------------------------------------------------------------------
void CHudAmmo::OnThink()
{
	UpdateAmmoDisplays();
}

//-----------------------------------------------------------------------------
// Purpose: updates the ammo display counts
//-----------------------------------------------------------------------------
void CHudAmmo::UpdateAmmoDisplays()
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	
	UpdatePlayerAmmo( player );
}

//-----------------------------------------------------------------------------
// Purpose: Updates ammo display
//-----------------------------------------------------------------------------
void CHudAmmo::SetAmmo(int ammo, bool playAnimation)
{
	if (ammo != m_iAmmo)
	{
		/*
		if (ammo == 0)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoEmpty");
		}
		else if (ammo < m_iAmmo)
		{
			// ammo has decreased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoDecreased");
		}
		else
		{
			// ammunition has increased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoIncreased");
		}
		*/

		m_iAmmo = ammo;
	}

	SetDisplayValue(ammo);
}

//-----------------------------------------------------------------------------
// Purpose: Updates 2nd ammo display
//-----------------------------------------------------------------------------
void CHudAmmo::SetAmmo2(int ammo2, bool playAnimation)
{
	if (ammo2 != m_iAmmo2)
	{
		/*
		if (ammo2 == 0)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2Empty");
		}
		else if (ammo2 < m_iAmmo2)
		{
			// ammo has decreased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2Decreased");
		}
		else
		{
			// ammunition has increased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2Increased");
		}
		*/

		m_iAmmo2 = ammo2;
	}

	SetSecondaryValue(ammo2);
}

/**
* Futzes with the texture name to add a prefix based on the clients team
*
* @param const char *szName Name of the texture
* @param char *szStr The string to fill in
* @param int iSize The amount of buffer space we have
* @return void
**/
void CHudAmmo::GetBackgroundTextureName(const char *szName, char *szStr, int iSize)
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
* Paints the ammo hud element
*
* @return void
**/
void CHudAmmo::Paint(void)
{
	// go down
	BaseClass::Paint();

	// draw yourself
	DrawSelf();
}

/**
* Handles case where the screen size changes
*
* @param int iOldWidth
* @param int iOldHeight
* @return void
**/
void CHudAmmo::OnScreenSizeChanged(int iOldWidth, int iOldHeight)
{
	// jump down
	BaseClass::OnScreenSizeChanged(iOldWidth, iOldHeight);

	// reset all of our size info
	SetIconPos(AMMO_ICON_XOFFSET, AMMO_ICON_YOFFSET);
	SetIconSize(AMMO_ICON_WIDTH, AMMO_ICON_HEIGHT);
}

//-----------------------------------------------------------------------------
// Purpose: Displays the secondary ammunition level
//-----------------------------------------------------------------------------
class CHudSecondaryAmmo : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudSecondaryAmmo, CHudNumericDisplay );

public:
	CHudSecondaryAmmo( const char *pElementName ) : BaseClass( NULL, "HudAmmoSecondary" ), CHudElement( pElementName )
	{
		m_iAmmo = -1;

		SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_WEAPONSELECTION | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
	}

	void Init( void )
	{
	}

	void VidInit( void )
	{
	}

	void SetAmmo( int ammo )
	{
		if (ammo != m_iAmmo)
		{
			/*
			if (ammo == 0)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoSecondaryEmpty");
			}
			else if (ammo < m_iAmmo)
			{
				// ammo has decreased
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoSecondaryDecreased");
			}
			else
			{
				// ammunition has increased
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoSecondaryIncreased");
			}
			*/

			m_iAmmo = ammo;
		}
		SetDisplayValue( ammo );
	}

	void Reset()
	{
		// hud reset, update ammo state
		BaseClass::Reset();
		m_iAmmo = 0;
		m_hCurrentActiveWeapon = NULL;
		UpdateAmmoState();
		SetAlpha(0);
	}

protected:
	virtual void OnThink()
	{
		// set whether or not the panel draws based on if we have a weapon that supports secondary ammo
		C_BaseCombatWeapon *wpn = GetActiveWeapon();
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
		IClientVehicle *pVehicle = player ? player->GetVehicle() : NULL;
		if (!wpn || !player || pVehicle)
		{
			m_hCurrentActiveWeapon = NULL;
			SetPaintEnabled(false);
			//SetPaintBackgroundEnabled(false);
			return;
		}
		else
		{
			SetPaintEnabled(true);
			//SetPaintBackgroundEnabled(true);
		}

		UpdateAmmoState();
	}

	void UpdateAmmoState()
	{
		C_BaseCombatWeapon *wpn = GetActiveWeapon();
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();

		if (player && wpn && wpn->UsesSecondaryAmmo())
		{
			C_WeaponSDKBase *pWpn = dynamic_cast<C_WeaponSDKBase*>(wpn);
			if (pWpn)
				SetAmmo(pWpn->GetAmmo2());
			else
				SetAmmo(player->GetAmmoCount(wpn->GetSecondaryAmmoType()));
		}

		if ( m_hCurrentActiveWeapon != wpn )
		{
			/*if ( wpn->UsesSecondaryAmmo() )
			{
				// we've changed to a weapon that uses secondary ammo
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesSecondaryAmmo");
			}
			else 
			{
				// we've changed away from a weapon that uses secondary ammo
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseSecondaryAmmo");
			}*/
			m_hCurrentActiveWeapon = wpn;
		}
	}

private:
	CHandle< C_BaseCombatWeapon > m_hCurrentActiveWeapon;
	int		m_iAmmo;
};

DECLARE_HUDELEMENT( CHudSecondaryAmmo );

