//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sniper.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "iclientmode.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/******************************************************************************************************/
/** CBaseSniperRifle definition ***********************************************************************/
/******************************************************************************************************/

IMPLEMENT_NETWORKCLASS_ALIASED(BaseSniperRifle, DT_BaseSniperRifle)

BEGIN_NETWORK_TABLE(CBaseSniperRifle, DT_BaseSniperRifle)
	#ifndef CLIENT_DLL
		SendPropBool(SENDINFO(m_bDrawScope)),
	#else
		RecvPropBool(RECVINFO(m_bDrawScope)),
	#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CBaseSniperRifle)
END_PREDICTION_DATA()
 
LINK_ENTITY_TO_CLASS(weapon_base_sniperrifle, CBaseSniperRifle);

/**
* Constructor
**/
CBaseSniperRifle::CBaseSniperRifle()
{	
	// we want to zoom out after we fire
	m_bZoomOutAfterFire = true;
	m_bForcedZoomOut = false;
	m_bHasBoltAction = true;
	m_bDoBoltAction = false;

	// no scope
	m_bDrawScope = false;
	#ifdef CLIENT_DLL
		m_pScope = NULL;
	#endif
}

/**
* We only want one shot at a time
*
* @return void
**/
void CBaseSniperRifle::PrimaryAttack(void)
{
	// make sure we pressed the button
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if (!pPlayer || !(pPlayer->m_afButtonPressed & IN_ATTACK)) {
		return;
	}

	// other quit conditions: out of ammo, already reloading
	if ((GetClip1() == 0 && GetAmmo1() == 0) || m_bInReload)
		return;

	// no scope, bad aim
	if (!m_bDrawScope)
		m_flCurrentAM = 1.0;
	// scope, we aim perfect
	else
		m_flCurrentAM = 0.0;

	// go down
	BaseClass::PrimaryAttack();

	// if we're out of ammo, make sure we're reloading
	if (GetClip1() == 0 && GetAmmo1() > 0 && !m_bInReload) {
		Reload();
		return;
	}

	// if we need to unzoom and if we are currently drawing the scope
	if(m_bZoomOutAfterFire && m_bDrawScope && HasPrimaryAmmo())
	{
		// hide the scope
		ShowScope(false);

		// we'll need to zoom back in a second
		m_bForcedZoomOut = true;
	}
}

/**
 * Reload: turn off the scope.
 */
bool CBaseSniperRifle::Reload(void)
{
	if(BaseClass::Reload())
	{
		// only hide the scope if we actually reloaded
		ShowScope(false);
		return true;
	}
	return false;
}

/**
 * Called when the player respawns. Turn off the scope!
 */
void CBaseSniperRifle::PlayerSpawn(void)
{
	ShowScope(false);
	m_bDoBoltAction = false;
	m_bForcedZoomOut = false;
}

/**
* Secondary attack displays the scope view
*
* @return void
**/
void CBaseSniperRifle::SecondaryAttack(void)
{
	// make sure we pressed the button
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if (!(pPlayer->m_afButtonPressed & IN_ATTACK2) || !HasAnyAmmo())
		return;

	// change the scope state
	ShowScope(!m_bDrawScope);
}

/**
* Hides or displays the scope
* Will return false on the client if it fails to tell the scope to displays itself
*
* @param bool bShouldDraw
* @return void
**/
void CBaseSniperRifle::ShowScope(bool bShouldDraw)
{
#ifndef CLIENT_DLL
	// change whether we're hidden or not
	m_bDrawScope = bShouldDraw;

	CBaseViewModel *vm = NULL;
	CBasePlayer *pOwner;

	// pull the owner
	pOwner = ToBasePlayer(GetOwner());
	if(pOwner && pOwner->IsAlive())
	{
		// pull the view model
		vm = pOwner->GetViewModel(m_nViewModelIndex);
		if(vm)
		{
			// draw it or don't
			if(m_bDrawScope)
				vm->AddEffects(EF_NODRAW);
			else
				vm->RemoveEffects(EF_NODRAW);
		}

		// zoom in or out
		if(m_bDrawScope)
			pOwner->SetFOV(this, pOwner->GetDefaultFOV() * SCOPE_FOV_RATIO, SCOPE_ZOOMIN_RATE);
		else
			pOwner->SetFOV(this, pOwner->GetDefaultFOV(), SCOPE_ZOOMOUT_RATE);
	}
#endif
}

/**
 * Holster: Turn off the scope.
 */
bool CBaseSniperRifle::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	// hide the scope
	ShowScope(false);
	m_bForcedZoomOut = false;
	if (gpGlobals->curtime < m_flNextPrimaryAttack)
		m_bDoBoltAction = true;

	return BaseClass::Holster(pSwitchingTo);
}

/**
 * Drop: Turn off the scope.
 */
void CBaseSniperRifle::Drop(const Vector &velocity)
{
	ShowScope(false);
	m_bForcedZoomOut = false;
	if (gpGlobals->curtime < m_flNextPrimaryAttack)
		m_bDoBoltAction = true;

	BaseClass::Drop(velocity);
}

/**
 * Deploy: Turn off the scope!
 */
bool CBaseSniperRifle::Deploy()
{
	if (BaseClass::Deploy()) {
		ShowScope(false);

		// if we're out of ammo, we should auto-reload
		if (GetClip1() == 0 && GetAmmo1() > 0 && !m_bInReload) {
			Reload();
		}
		else if (m_bDoBoltAction) {
			BoltAction();
			m_bDoBoltAction = false;
		}
		
		return true;
	}

	return false;
}

/**
 * play the bolt action anim and incr the primary attack time by the seqence length
 */
void CBaseSniperRifle::BoltAction(void)
{
	if (!m_bHasBoltAction)
		return;

	SendWeaponAnim( ACT_VM_PULLPIN );

	m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

/**
* Gets called when the weapon is bored
*
* @return void
**/
void CBaseSniperRifle::WeaponIdle(void)
{
	// were we forced out?
	if(m_bForcedZoomOut)
	{
		// are we still zoomed out?
		ShowScope(true);

		// not anymore
		m_bForcedZoomOut = false;
	}

	// go down
	BaseClass::WeaponIdle();
}

#ifdef CLIENT_DLL
	/**
	* Finds the scope and sticks it in our member var
	*
	* @return HudSniperScope *
	**/
	HudSniperScope *CBaseSniperRifle::GetScope(void)
	{
		// do we have it yet?
		if(!m_pScope)
		{
			vgui::Panel *pPanel;

			// grab the view port
			pPanel = g_pClientMode->GetViewport();

			// find the panel
			m_pScope = dynamic_cast<HudSniperScope *>(pPanel->FindChildByName("HudSniperScope"));
		}

		return m_pScope;
	}
#endif

/**
* Handles the case that we are out of ammo
*
* @return void
**/
void CBaseSniperRifle::OutOfAmmo(void)
{
	// turn off the scope
	ShowScope(false);

	// go down
	BaseClass::OutOfAmmo();
}