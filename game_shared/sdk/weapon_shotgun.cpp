//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_fx_shared.h"

#ifdef CLIENT_DLL
	#include "buymenu.h"
#endif

#if defined( CLIENT_DLL )

	#define CWeaponShotgun C_WeaponShotgun
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"
	#include "te_firebullets.h"

#endif

#define ANIM_RELOAD_START_LEN 0.75
#define ANIM_RELOAD_LEN 0.5
#define ANIM_RELOAD_END_LEN 0.75

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponShotgun : public CBaseMachineGun
{
public:
	DECLARE_CLASS( CWeaponShotgun, CBaseMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponShotgun();

	virtual void PrimaryAttack();
	virtual bool Reload();
	virtual void WeaponIdle();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_SHOTGUN; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_SHOTGUN; }
protected:

private:

	CWeaponShotgun( const CWeaponShotgun & );

	float m_flPumpTime;
	CNetworkVar( int, m_fInSpecialReload );

};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponShotgun, DT_WeaponShotgun )

BEGIN_NETWORK_TABLE( CWeaponShotgun, DT_WeaponShotgun )

	#ifdef CLIENT_DLL
		RecvPropInt( RECVINFO( m_fInSpecialReload ) )
	#else
		SendPropInt( SENDINFO( m_fInSpecialReload ), 2, SPROP_UNSIGNED )
	#endif

END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponShotgun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_shotgun, CWeaponShotgun );
PRECACHE_WEAPON_REGISTER( weapon_shotgun );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponShotgun);
#endif

CWeaponShotgun::CWeaponShotgun()
{
	m_flPumpTime = 0;
}

void CWeaponShotgun::PrimaryAttack()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	// don't fire underwater
	if (pPlayer->GetWaterLevel() == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		return;
	}

	// Out of ammo?
	if ( GetClip1() <= 0 )
	{
		Reload();
		if ( GetClip1() == 0 )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
		}

		return;
	}

	BaseClass::PrimaryAttack();

	if (m_iClip1 != 0) {
		m_flPumpTime = gpGlobals->curtime + 0.5;
		SetWeaponIdleTime( gpGlobals->curtime + 2.5 );
	}
	else
		SetWeaponIdleTime( gpGlobals->curtime + 0.875 );

	m_fInSpecialReload = 0;

	// Update punch angles.
	DoRecoil();
}


bool CWeaponShotgun::Reload()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();

	if (GetAmmo1() <= 0 || GetClip1() == GetMaxClip1())
		return true;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return true;
		
	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		pPlayer->SetAnimation( PLAYER_RELOAD );

		// move the gun to the side
		SendWeaponAnim( ACT_SHOTGUN_RELOAD_START );
		// we want to reload it next
		m_fInSpecialReload = 1;

		// this takes 0.5 seconds
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = pPlayer->m_flNextAttack = m_flTimeWeaponIdle = gpGlobals->curtime + ANIM_RELOAD_START_LEN;
		return true;
	}
	else if (m_fInSpecialReload == 1)
	{
		// waiting for gun to move to side
		if (m_flTimeWeaponIdle > gpGlobals->curtime)
			return true;
		// next we want to finish the reload, or reload again
		m_fInSpecialReload = 2;

		// reload it
		SendWeaponAnim( ACT_VM_RELOAD );
		// this takes 0.45 seconds
		SetWeaponIdleTime( gpGlobals->curtime + ANIM_RELOAD_LEN );
	}
	else
	{
		// we're done reloading, add them to the clip
		m_iClip1++;
		m_iAmmo1--;
		
		// send it to all the clients
#ifdef GAME_DLL
		SendReloadEvents();
#endif
		
		// reload it again
		m_fInSpecialReload = 1;
	}

	return true;
}


void CWeaponShotgun::WeaponIdle()
{
	if (m_flPumpTime && m_flPumpTime < gpGlobals->curtime)
	{
		// play pumping sound
		m_flPumpTime = 0;
	}

	// we can idle
	if (m_flTimeWeaponIdle < gpGlobals->curtime)
	{
		// auto-reload
		if (m_iClip1 == 0 && m_fInSpecialReload == 0 && GetAmmo1())
		{
			Reload( );
		}
		// we're currently reloading
		else if (m_fInSpecialReload != 0)
		{
			// weapon not full yet
			if (GetClip1() != GetMaxClip1() && GetAmmo1())
			{
				// reload again
				Reload();
			}
			else
			{
				// no more reloading
				m_fInSpecialReload = 0;

				// done reloading, move towards idle
				SendWeaponAnim( ACT_SHOTGUN_RELOAD_FINISH );

				// this takes 1.5 seconds
				SetWeaponIdleTime( gpGlobals->curtime + ANIM_RELOAD_END_LEN );
			}
		}
		else
		{
			SendWeaponAnim(ACT_VM_IDLE);
		}
	}
}
