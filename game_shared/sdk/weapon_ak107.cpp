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

	#define CWeaponAK107 C_WeaponAK107
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponAK107 : public CBaseMachineGun
{
public:
	DECLARE_CLASS( CWeaponAK107, CBaseMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponAK107();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_AK107; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_RIFLE; }

private:

	CWeaponAK107( const CWeaponAK107 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAK107, DT_WeaponAK107 )

BEGIN_NETWORK_TABLE( CWeaponAK107, DT_WeaponAK107 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAK107 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_ak107, CWeaponAK107 );
PRECACHE_WEAPON_REGISTER( weapon_ak107 );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponAK107);
#endif

CWeaponAK107::CWeaponAK107()
{
	m_bDoSecondaryZoom = true;
}