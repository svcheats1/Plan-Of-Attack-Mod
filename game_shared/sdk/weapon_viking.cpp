//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "sdk_fx_shared.h"
#include "in_buttons.h"

#if defined( CLIENT_DLL )

	#define CWeaponViking C_WeaponViking
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponViking : public CBaseMachineGun
{
public:
	DECLARE_CLASS( CWeaponViking, CBaseMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponViking();

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_VIKING; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_PISTOL; }
	virtual bool IsAutomaticWeapon(void) const { return false; }
private:

	CWeaponViking( const CWeaponViking & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponViking, DT_WeaponViking )

BEGIN_NETWORK_TABLE( CWeaponViking, DT_WeaponViking )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponViking )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_viking, CWeaponViking );
PRECACHE_WEAPON_REGISTER( weapon_viking );

CWeaponViking::CWeaponViking() { }