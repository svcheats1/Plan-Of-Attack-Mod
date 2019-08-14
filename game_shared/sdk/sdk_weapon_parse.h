//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SDK_WEAPON_PARSE_H
#define SDK_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_parse.h"
#include "networkvar.h"
#include "buyableitem.h"

typedef enum 
{
	RT_NONE,
	RT_SIMPLE,
	RT_RANDOM,
	RT_RANDOM_GAUSSIAN,
	RT_MAX
} Recoil_Type;

//--------------------------------------------------------------------------------------------------------

class CSDKWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CSDKWeaponInfo, FileWeaponInfo_t );
	
	CSDKWeaponInfo();
	~CSDKWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

	char m_szAnimExtension[16];		// string used to generate player animations with this weapon

	// Parameters for FX_FireBullets:
	int		m_iDamage;
	int		m_iBullets;
	float	m_flCycleTime;

	int		m_iPrice;
	int		m_iItemPosition;
	BI_Type m_iItemType;
	BI_Team m_iBuyableTeam;

	float	*m_aRecoilValues;
	Recoil_Type	m_iRecoilType;

	int		m_iMaxAmmo1;
	int		m_iMaxAmmo2;

	float m_flBaseSpread;
	float m_flMinAM;
	float m_flMaxAM;
	float m_flGrowRateAM;
	float m_flShrinkRateAM;
};


#endif // SDK_WEAPON_PARSE_H
