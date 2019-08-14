//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sniper.h"
#include "sdk_fx_shared.h"

#if defined( CLIENT_DLL )

	#define CWeaponSVU C_WeaponSVU
	#include "c_sdk_player.h"
	#include "buymenu.h"
#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Class definition for the SVU
**/
class CWeaponSVU : public CBaseSniperRifle
{
public:
	DECLARE_CLASS( CWeaponSVU, CBaseSniperRifle );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponSVU();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_SVU; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_SNIPER; }
protected:

private:

	CWeaponSVU( const CWeaponSVU & );
};

/******************************************************************************************************/
/** CWeaponSVU definition *****************************************************************************/
/******************************************************************************************************/

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSVU, DT_WeaponSVU )

BEGIN_NETWORK_TABLE( CWeaponSVU, DT_WeaponSVU )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSVU )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_svu, CWeaponSVU );
PRECACHE_WEAPON_REGISTER( weapon_svu );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponSVU);
#endif

/**
* Constructor
**/
CWeaponSVU::CWeaponSVU()
{
	m_bZoomOutAfterFire = false;
	m_bHasBoltAction = false;
}
