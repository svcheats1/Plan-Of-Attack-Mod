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

	#define CWeaponPP90M1 C_WeaponPP90M1
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponPP90M1 : public CBaseMachineGun
{
public:
	DECLARE_CLASS( CWeaponPP90M1, CBaseMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponPP90M1();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_PP90M1; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_SMG; }
private:

	CWeaponPP90M1( const CWeaponPP90M1 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPP90M1, DT_WeaponPP90M1 )

BEGIN_NETWORK_TABLE( CWeaponPP90M1, DT_WeaponPP90M1 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponPP90M1 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_pp90m1, CWeaponPP90M1 );
PRECACHE_WEAPON_REGISTER( weapon_pp90m1 );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponPP90M1);
#endif

CWeaponPP90M1::CWeaponPP90M1() { }
