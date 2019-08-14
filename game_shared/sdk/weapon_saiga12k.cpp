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

	#define CWeaponSaiga12k C_WeaponSaiga12k
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponSaiga12k : public CBaseMachineGun
{
public:
	DECLARE_CLASS( CWeaponSaiga12k, CBaseMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponSaiga12k();

	virtual void PrimaryAttack(void);
	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_SAIGA12K; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_SHOTGUN; }
private:

	CWeaponSaiga12k( const CWeaponSaiga12k & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSaiga12k, DT_WeaponSaiga12k )

BEGIN_NETWORK_TABLE( CWeaponSaiga12k, DT_WeaponSaiga12k )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSaiga12k )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_saiga12k, CWeaponSaiga12k );
PRECACHE_WEAPON_REGISTER( weapon_saiga12k );

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponSaiga12k);
#endif

CWeaponSaiga12k::CWeaponSaiga12k() { }

void CWeaponSaiga12k::PrimaryAttack()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if (!(pPlayer->m_afButtonPressed & IN_ATTACK)) {
		return;
	}

	BaseClass::PrimaryAttack();
}