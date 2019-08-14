//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_fx_shared.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "buymenu.h"
#endif

#if defined( CLIENT_DLL )

	#define CWeaponMK48 C_WeaponMK48
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif

#define MACHINE_GUN_MAX_SPEED 68

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponMK48 : public CBaseMachineGun
{
public:
	DECLARE_CLASS( CWeaponMK48, CBaseMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponMK48();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_MK48; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_MACHINEGUN; }

	virtual float CalcMaxSpeed( void ) const;

private:

	CWeaponMK48( const CWeaponMK48 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMK48, DT_WeaponMK48 )

BEGIN_NETWORK_TABLE( CWeaponMK48, DT_WeaponMK48 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponMK48 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_mk48, CWeaponMK48 );
PRECACHE_WEAPON_REGISTER( weapon_mk48 );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponMK48);
#endif

CWeaponMK48::CWeaponMK48() { }

float CWeaponMK48::CalcMaxSpeed() const
{
	CBasePlayer *pPlayer = GetPlayerOwner();
	if (pPlayer->m_nButtons & IN_ATTACK && GetClip1() && !m_bInReload)
		return MACHINE_GUN_MAX_SPEED;

	return BaseClass::CalcMaxSpeed();
}