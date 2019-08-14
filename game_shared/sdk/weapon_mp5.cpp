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

	#define CWeaponMP5 C_WeaponMP5
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponMP5 : public CBaseMachineGun
{
public:
	DECLARE_CLASS( CWeaponMP5, CBaseMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponMP5();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_MP5; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_SMG; }
private:

	CWeaponMP5( const CWeaponMP5 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMP5, DT_WeaponMP5 )

BEGIN_NETWORK_TABLE( CWeaponMP5, DT_WeaponMP5 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponMP5 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_mp5, CWeaponMP5 );
PRECACHE_WEAPON_REGISTER( weapon_mp5 );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponMP5);
#endif

CWeaponMP5::CWeaponMP5() { }
