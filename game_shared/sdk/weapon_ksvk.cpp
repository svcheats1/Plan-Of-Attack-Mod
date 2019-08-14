//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sniper.h"
#include "sdk_fx_shared.h"

#if defined( CLIENT_DLL )

	#define CWeaponKSVK C_WeaponKSVK
	#include "c_sdk_player.h"
	#include "buymenu.h"
#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Class definition for the KSVK
**/
class CWeaponKSVK : public CBaseSniperRifle
{
public:
	DECLARE_CLASS( CWeaponKSVK, CBaseSniperRifle );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponKSVK();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_KSVK; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_SNIPER; }
protected:

private:

	CWeaponKSVK( const CWeaponKSVK & );
};

/******************************************************************************************************/
/** CWeaponKSVK definition *****************************************************************************/
/******************************************************************************************************/

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponKSVK, DT_WeaponKSVK )

BEGIN_NETWORK_TABLE( CWeaponKSVK, DT_WeaponKSVK )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponKSVK )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_ksvk, CWeaponKSVK );
PRECACHE_WEAPON_REGISTER( weapon_ksvk );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponKSVK);
#endif

/**
* Constructor
**/
CWeaponKSVK::CWeaponKSVK()
{
}
