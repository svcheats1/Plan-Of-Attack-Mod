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

	#define CWeaponDeagle C_WeaponDeagle
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponDeagle : public CBaseMachineGun
{
public:
	DECLARE_CLASS( CWeaponDeagle, CBaseMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponDeagle();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_DEAGLE; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_PISTOL; }
	virtual bool IsAutomaticWeapon() const { return false; }

private:

	CWeaponDeagle( const CWeaponDeagle & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponDeagle, DT_WeaponDeagle )

BEGIN_NETWORK_TABLE( CWeaponDeagle, DT_WeaponDeagle )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponDeagle )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_deagle, CWeaponDeagle );
PRECACHE_WEAPON_REGISTER( weapon_deagle );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponDeagle);
#endif

CWeaponDeagle::CWeaponDeagle() { }