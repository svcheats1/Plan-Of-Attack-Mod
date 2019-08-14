//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include "sdk_weapon_parse.h"
#include "ammodef.h"

#define ACCURACY_RATIO 0.8

static const char *s_ItemTypeAliasInfo[] = 
{
	"pistol",				// BI_TYPE_PISTOL		
	"shotgun",				// BI_TYPE_SHOTGUN
	"smg",					// BI_TYPE_SMG
	"rifle",				// BI_TYPE_RIFLE
	"explosives",			// BI_TYPE_EXPLOSIVES
	NULL,					// BI_TYPE_COUNT
};

static const char *s_ItemBuyTeamAliasInfo[] =
{
	"TEAM_BOTH",
	"TEAM_A",
	"TEAM_B",
	NULL,
};

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CSDKWeaponInfo;
}


CSDKWeaponInfo::CSDKWeaponInfo()
{
	m_aRecoilValues = NULL;
}

CSDKWeaponInfo::~CSDKWeaponInfo()
{
	delete [] m_aRecoilValues;
}

void CSDKWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_iDamage		= pKeyValuesData->GetInt( "Damage", 42 ); // Douglas Adams 1952 - 2001
	m_iBullets		= pKeyValuesData->GetInt( "Bullets", 1 );
	m_flCycleTime	= pKeyValuesData->GetFloat( "CycleTime", 0.15 );

	strcpy(m_szAnimExtension, pKeyValuesData->GetString( "anim_suffix", "mp5" ));

	m_iPrice		= pKeyValuesData->GetInt( "Price", 999999 );
	m_iItemPosition = pKeyValuesData->GetInt( "ItemPosition", 0 );

	char szItemType[20];
	m_iItemType = BI_TYPE_NONE;
	Q_strncpy(szItemType, pKeyValuesData->GetString( "ItemType", "" ), 20);
	for(int i = 0; i < BI_TYPE_COUNT; ++i) {
		if (Q_stricmp(szItemType, s_ItemTypeAliasInfo[i]) == 0) {
			m_iItemType = (BI_Type) i;
			break;
		}
	}

	char szBuyTeam[20];
	m_iBuyableTeam = BI_TEAM_BOTH;
	Q_strncpy(szBuyTeam, pKeyValuesData->GetString("ItemTeam", ""), 20);
	for(int i = 0; i < BI_TEAM_COUNT; ++i) {
		if (Q_stricmp(szBuyTeam, s_ItemBuyTeamAliasInfo[i]) == 0) {
			m_iBuyableTeam = (BI_Team) i;
			break;
		}
	}

	m_iMaxAmmo1	= pKeyValuesData->GetInt( "MaxAmmo1", -1 );
	m_iMaxAmmo2	= pKeyValuesData->GetInt( "MaxAmmo2", -1 );

	m_flBaseSpread = pKeyValuesData->GetFloat("BaseSpread", 0.0225) * ACCURACY_RATIO;
	m_flMinAM = pKeyValuesData->GetFloat("MinAM", 0.5);
	m_flMaxAM = pKeyValuesData->GetFloat("MaxAM", 5.0);
	m_flGrowRateAM = pKeyValuesData->GetFloat("GrowRateAM", 0.3);		// per shot
	m_flShrinkRateAM = pKeyValuesData->GetFloat("ShrinkRateAM", 1.0);	// per second
	
	// get the recoil type
	char szRecoilType[20];
	Q_strncpy(szRecoilType, pKeyValuesData->GetString("recoil_type", "none"), 20);

	// move it to our enum
	if (Q_stricmp(szRecoilType, "simple") == 0)
		m_iRecoilType = RT_SIMPLE;
	else if (Q_stricmp(szRecoilType, "random") == 0)
		m_iRecoilType = RT_RANDOM;
	else if(Q_stricmp(szRecoilType, "random_gaussian") == 0)
		m_iRecoilType = RT_RANDOM_GAUSSIAN;
	else
		m_iRecoilType = RT_NONE;

	// do we need recoil values?
	if (m_iRecoilType != RT_NONE) {
		// get the recoil values
		char szRecoilValues[30];
		Q_strncpy(szRecoilValues, pKeyValuesData->GetString("recoil_values", ""), 30);

		// if its simple, parse 3 numbers
		if (m_iRecoilType == RT_SIMPLE) {
			m_aRecoilValues = new float[3];
			memset(m_aRecoilValues, 0, sizeof(m_aRecoilValues));
			sscanf(szRecoilValues, "%f %f %f", &m_aRecoilValues[0], &m_aRecoilValues[1], &m_aRecoilValues[2]);
		}
		// if its random, parse 6 numbers
		else {
			m_aRecoilValues = new float[6];
			memset(m_aRecoilValues, 0, sizeof(m_aRecoilValues));
			sscanf(szRecoilValues, "%f %f %f %f %f %f", 
				&m_aRecoilValues[0], &m_aRecoilValues[1], &m_aRecoilValues[2],
				&m_aRecoilValues[3], &m_aRecoilValues[4], &m_aRecoilValues[5]);
		}
	}
}


