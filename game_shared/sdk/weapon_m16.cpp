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

	#define CWeaponM16 C_WeaponM16
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponM16 : public CBaseMachineGun
{
public:
	DECLARE_CLASS( CWeaponM16, CBaseMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponM16();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_M16; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_RIFLE; }
private:

	CWeaponM16( const CWeaponM16 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponM16, DT_WeaponM16 )

BEGIN_NETWORK_TABLE( CWeaponM16, DT_WeaponM16 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponM16 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_m16, CWeaponM16 );
PRECACHE_WEAPON_REGISTER( weapon_m16 );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponM16);
#endif

CWeaponM16::CWeaponM16()
{
	m_bDoSecondaryZoom = true;
}
