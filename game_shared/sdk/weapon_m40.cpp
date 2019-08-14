//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sniper.h"
#include "sdk_fx_shared.h"

#if defined( CLIENT_DLL )

	#define CWeaponM40 C_WeaponM40
	#include "c_sdk_player.h"
	#include "buymenu.h"
#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Class definition for the M40
**/
class CWeaponM40 : public CBaseSniperRifle
{
public:
	DECLARE_CLASS( CWeaponM40, CBaseSniperRifle );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponM40();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_M40; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_SNIPER; }
protected:

private:

	CWeaponM40( const CWeaponM40 & );
};

/******************************************************************************************************/
/** CWeaponM40 definition *****************************************************************************/
/******************************************************************************************************/

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponM40, DT_WeaponM40 )

BEGIN_NETWORK_TABLE( CWeaponM40, DT_WeaponM40 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponM40 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_m40, CWeaponM40 );
PRECACHE_WEAPON_REGISTER( weapon_m40 );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponM40);
#endif

/**
* Constructor
**/
CWeaponM40::CWeaponM40()
{
}
