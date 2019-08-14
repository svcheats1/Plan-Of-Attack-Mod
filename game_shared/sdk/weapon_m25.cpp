//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sniper.h"
#include "sdk_fx_shared.h"

#if defined( CLIENT_DLL )

	#define CWeaponM25 C_WeaponM25
	#include "c_sdk_player.h"
	#include "buymenu.h"
#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Class definition for the M25
**/
class CWeaponM25 : public CBaseSniperRifle
{
public:
	DECLARE_CLASS( CWeaponM25, CBaseSniperRifle );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponM25();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_M25; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_SNIPER; }
protected:

private:

	CWeaponM25( const CWeaponM25 & );
};

/******************************************************************************************************/
/** CWeaponM25 definition *****************************************************************************/
/******************************************************************************************************/

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponM25, DT_WeaponM25 )

BEGIN_NETWORK_TABLE( CWeaponM25, DT_WeaponM25 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponM25 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_m25, CWeaponM25 );
PRECACHE_WEAPON_REGISTER( weapon_m25 );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponM25);
#endif

/**
* Constructor
**/
CWeaponM25::CWeaponM25()
{
	m_bZoomOutAfterFire = false;
	m_bHasBoltAction = false;
}
