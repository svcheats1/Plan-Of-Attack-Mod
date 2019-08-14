//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_playerresource.h"
#include "c_team.h"
#include "skillclass.h"

#ifdef HL2MP
#include "hl2mp_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_PlayerResource, DT_PlayerResource, CPlayerResource)
	RecvPropArray3( RECVINFO_ARRAY(m_iPing), RecvPropInt( RECVINFO(m_iPing[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iScore), RecvPropInt( RECVINFO(m_iScore[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iKills), RecvPropInt( RECVINFO(m_iKills[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iCaps), RecvPropInt( RECVINFO(m_iCaps[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iDeaths), RecvPropInt( RECVINFO(m_iDeaths[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_bConnected), RecvPropInt( RECVINFO(m_bConnected[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iTeam), RecvPropInt( RECVINFO(m_iTeam[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_bAlive), RecvPropInt( RECVINFO(m_bAlive[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iHealth), RecvPropInt( RECVINFO(m_iHealth[0]))),
	RecvPropArray3(RECVINFO_ARRAY(m_iXP), RecvPropInt(RECVINFO(m_iXP[0]))),
	RecvPropArray3(RECVINFO_ARRAY(m_iSkill), RecvPropInt(RECVINFO(m_iSkill[0]))),
	RecvPropArray3(RECVINFO_ARRAY(m_iUserID), RecvPropInt(RECVINFO(m_iUserID[0]))),
	RecvPropBool(RECVINFO(m_bIsFirstRound)),
END_RECV_TABLE()

C_PlayerResource *g_PR;

IGameResources * GameResources( void ) { return g_PR; }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::C_PlayerResource()
{
	memset( m_szName, 0, sizeof( m_szName ) );
	memset( m_iPing, 0, sizeof( m_iPing ) );
//	memset( m_iPacketloss, 0, sizeof( m_iPacketloss ) );
	memset( m_iScore, 0, sizeof( m_iScore ) );
	memset( m_iCaps, 0, sizeof( m_iCaps ) );
	memset( m_iDeaths, 0, sizeof( m_iDeaths ) );
	memset( m_bConnected, 0, sizeof( m_bConnected ) );
	memset( m_iTeam, 0, sizeof( m_iTeam ) );
	memset( m_bAlive, 0, sizeof( m_bAlive ) );
	memset( m_iHealth, 0, sizeof( m_iHealth ) );
	memset( m_iKills, 0, sizeof( m_iKills ) );

	// @TRJ
	memset(m_iSkill, 0, sizeof(m_iSkill));
	memset(m_iXP, 0, sizeof(m_iXP));
	memset(m_iUserID, 0, sizeof(m_iUserID));

	m_bIsFirstRound = false;
	
	m_Colors[TEAM_A] = COLOR_BLUE;
	m_Colors[TEAM_B] = COLOR_RED;
	m_Colors[TEAM_SPECTATOR] = COLOR_GREY;

	g_PR = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::~C_PlayerResource()
{
		g_PR = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_PlayerResource::GetPlayerName( int iIndex )
{
	if ( iIndex < 0 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return "ERRORNAME";
	}
	
	if ( !IsConnected( iIndex ) )
		return "unconnected";

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( iIndex );
	if (!pPlayer)
		return "unconnected";

	const char *name = pPlayer->GetPlayerName();
	if (!name)
		return "unconnected";

	return name;
}

bool C_PlayerResource::IsAlive(int iIndex )
{
	return m_bAlive[iIndex];
}

int C_PlayerResource::GetTeam(int iIndex )
{
	if ( iIndex < 0 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return 0;
	}
	else
	{
		return m_iTeam[iIndex];
	}
}

const char * C_PlayerResource::GetTeamName(int index)
{
	C_Team *team = GetGlobalTeam( index );

	if ( !team )
		return "Unknown";

	return team->Get_Name();
}

int C_PlayerResource::GetTeamScore(int index)
{
	C_Team *team = GetGlobalTeam( index );

	if ( !team )
		return 0;

	return team->Get_Score();
}

int C_PlayerResource::GetFrags(int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iKills[iIndex];
}

bool C_PlayerResource::IsLocalPlayer(int index)
{
	C_BasePlayer *pPlayer =	C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return false;

	return ( index == pPlayer->entindex() );
}


bool C_PlayerResource::IsHLTV(int index)
{
	if ( !IsConnected( index ) )
		return false;

	player_info_t sPlayerInfo;
	
	if ( engine->GetPlayerInfo( index, &sPlayerInfo ) )
	{
		return sPlayerInfo.ishltv;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::IsFakePlayer( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return false;

	// Yuck, make sure it's up to date
	player_info_t sPlayerInfo;
	if ( engine->GetPlayerInfo( iIndex, &sPlayerInfo ) )
	{
		return sPlayerInfo.fakeplayer;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetPing( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iPing[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
/*-----------------------------------------------------------------------------
int	C_PlayerResource::GetPacketloss( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iPacketloss[iIndex];
}*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetPlayerScore( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iScore[iIndex];
}

int C_PlayerResource::GetCaps(int iIndex)
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iCaps[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetDeaths( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iDeaths[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetHealth( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iHealth[iIndex];
}

const Color &C_PlayerResource::GetTeamColor(int index )
{
	if ( index < 0 || index > MAX_TEAMS )
	{
		// @JD - this happens somewhat frequently and isnt a problem so just ignore it.
		//Assert( false );
		static Color blah;
		return blah;
	}
	else
	{
		return m_Colors[index];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::IsConnected( int iIndex )
{
	if ( iIndex < 0 || iIndex > MAX_PLAYERS )
		return false;
	else
		return m_bConnected[iIndex];
}

/**
* Gets the players xp value
*
* @param int iIndex Index of the player
* @return int
**/
int C_PlayerResource::GetPlayerXP(int iIndex)
{
	// make sure they are connected
	if(!IsConnected(iIndex))
		return 0;

	return m_iXP[iIndex];
}

/**
* Gets the skill class of the player
*
* @param int iIndex Index of the player
* @return const char *
**/
const char *C_PlayerResource::GetPlayerSkill(int iIndex)
{
	const CSkillClass *pSkill = GetPlayerSkillModel(iIndex);
	if (!pSkill)
		return "";

	// pull the skill classes
	return pSkill->GetClassNameShort();
}

int C_PlayerResource::GetPlayerSkillIndex(int iIndex)
{
	if (!IsConnected(iIndex))
		return -1;

	return m_iSkill[iIndex];
}

const CSkillClass* C_PlayerResource::GetPlayerSkillModel(int iIndex)
{
	// make sure they're connected
	if(!IsConnected(iIndex))
		return NULL;

	// pull the skill classes
	const CSkillClass *pSkill = CSkillClass::GetSkillClassModel((SKILL_CLASS_INDEX)m_iSkill[iIndex]);
	return pSkill;
}

int C_PlayerResource::GetUserID(int iIndex)
{
	if(!IsConnected(iIndex))
		return 0;

	return m_iUserID[iIndex];
}

int C_PlayerResource::FindIndexByUserID(int iUserID)
{
	// JD: Oh, this is how we were supossed to do this.
	return engine->GetPlayerForUserID(iUserID);
    
	/*
	// we think that the user ID is the index, but we're not so sure, so try O(1)
	if (iUserID > 0 && iUserID <= gpGlobals->maxClients) {
		if (iUserID == GetUserID(iUserID))
			return iUserID;
	}

	// O(n)
	for(int i = 1; i <= gpGlobals->maxClients; ++i) {
		if (iUserID == GetUserID(i))
			return i;
	}

	return 0;
	*/
}

/**
* Returns true if we are in the first round
*
* @return bool
**/
bool C_PlayerResource::IsFirstRound(void)
{
	return m_bIsFirstRound;
}
