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

	#define CWeaponPKM C_WeaponPKM
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif

#define MACHINE_GUN_MAX_SPEED 68

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponPKM : public CBaseMachineGun
{
public:
	DECLARE_CLASS( CWeaponPKM, CBaseMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponPKM();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_PKM; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_MACHINEGUN; }

	virtual float CalcMaxSpeed( void ) const;

private:

	CWeaponPKM( const CWeaponPKM & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPKM, DT_WeaponPKM )

BEGIN_NETWORK_TABLE( CWeaponPKM, DT_WeaponPKM )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponPKM )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_pkm, CWeaponPKM );
PRECACHE_WEAPON_REGISTER( weapon_pkm );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponPKM);
#endif

CWeaponPKM::CWeaponPKM() { }

float CWeaponPKM::CalcMaxSpeed() const
{
	CBasePlayer *pPlayer = GetPlayerOwner();
	if (pPlayer->m_nButtons & IN_ATTACK && GetClip1() && !m_bInReload)
		return MACHINE_GUN_MAX_SPEED;

	return BaseClass::CalcMaxSpeed();
}