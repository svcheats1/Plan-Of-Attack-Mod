//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_BASESDKGRENADE_H
#define WEAPON_BASESDKGRENADE_H
#ifdef _WIN32
#pragma once
#endif

#include "iclientnetworkable.h"
#include "weapon_sdkbase.h"


#ifdef CLIENT_DLL
	
	#define CBaseSDKGrenade C_BaseSDKGrenade

#endif


class CBaseSDKGrenade : public CWeaponSDKBase
{
public:
	DECLARE_CLASS( CBaseSDKGrenade, CWeaponSDKBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CBaseSDKGrenade();

	virtual void	Spawn();
	virtual void	Precache();

	virtual bool	Deploy();
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	FillAmmo();

	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();

	virtual bool	Reload();

	virtual void	PlayerSpawn();
	virtual void	ItemPostFrame();
	
	virtual void	DecrementAmmo( CBaseCombatCharacter *pOwner );
	virtual bool	HasWeaponExclusion() const { return false; }
	virtual void	StartGrenadeThrow();
	virtual void	ThrowGrenade();
	virtual void	DropGrenade();
	virtual void	PlayerKilled();

#ifndef CLIENT_DLL
	virtual bool	DropSpecial();
#endif
	// congratulations, you found a grenade!
	virtual bool	EquipAmmoFromWeapon(CBaseCombatWeapon *pWeapon);
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	bool IsPinPulled() const;

#ifndef CLIENT_DLL
	DECLARE_DATADESC();

	virtual bool AllowsAutoSwitchFrom( void ) const;

	int		CapabilitiesGet();
	
	// Each derived grenade class implements this.
	virtual void EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer );
#endif

protected:
	CNetworkVar( bool, m_bRedraw );	// Draw the weapon again after throwing a grenade
	CNetworkVar( bool, m_bThrowing );
	CNetworkVar( bool, m_bPinPulled );	// Set to true when the pin has been pulled but the grenade hasn't been thrown yet.
	CNetworkVar( float, m_fThrowTime ); // the time at which the grenade will be thrown.  If this value is 0 then the time hasn't been set yet.

private:
	int m_iOldAmmo1;

	CBaseSDKGrenade( const CBaseSDKGrenade & ) {}
};


inline bool CBaseSDKGrenade::IsPinPulled() const
{
	return m_bPinPulled;
}


#endif // WEAPON_BASESDKGRENADE_H
