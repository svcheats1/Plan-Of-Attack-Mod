//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_PLAYERRESOURCE_H
#define C_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "const.h"
#include "c_baseentity.h"
#include "skillclass.h"
#include <igameresources.h>
#include "vector.h"

class C_PlayerResource : public C_BaseEntity, public IGameResources
{
	DECLARE_CLASS( C_PlayerResource, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

					C_PlayerResource();
	virtual			~C_PlayerResource();

public : // IGameResources intreface

	// Team data access 
	virtual int		GetTeamScore( int index );
	virtual const char *GetTeamName( int index );
	virtual const Color&GetTeamColor( int index );

	// Player data access
	virtual bool	IsConnected( int index );
	virtual bool	IsAlive( int index );
	virtual bool	IsFakePlayer( int index );
	virtual bool	IsLocalPlayer( int index  );
	virtual bool	IsHLTV(int index);

	virtual const char *GetPlayerName( int index );
	virtual int		GetPing( int index );
//	virtual int		GetPacketloss( int index );
	virtual int		GetPlayerScore( int index );
	virtual int		GetDeaths( int index );
	virtual int		GetTeam( int index );
	virtual int		GetFrags( int index );
	virtual int		GetHealth( int index );
	virtual int		GetCaps( int index );

	// @TRJ
	virtual int GetPlayerXP(int iIndex);
	virtual const char* GetPlayerSkill(int iIndex);
	virtual int GetPlayerSkillIndex(int iIndex);
	virtual const CSkillClass* GetPlayerSkillModel(int iIndex);
	virtual int		GetUserID(int index);
	virtual int		FindIndexByUserID(int iUserID);
	virtual bool IsFirstRound(void);
	
protected:
	// Data for each player that's propagated to all clients
	// Stored in individual arrays so they can be sent down via datatables
	char	m_szName[MAX_PLAYERS+1][ MAX_PLAYER_NAME_LENGTH ];
	int		m_iPing[MAX_PLAYERS+1];
	int		m_iScore[MAX_PLAYERS+1];
	int		m_iKills[MAX_PLAYERS+1];
	int		m_iCaps[MAX_PLAYERS+1];
	int		m_iDeaths[MAX_PLAYERS+1];
	bool	m_bConnected[MAX_PLAYERS+1];
	int		m_iTeam[MAX_PLAYERS+1];
	bool	m_bAlive[MAX_PLAYERS+1];
	int		m_iHealth[MAX_PLAYERS+1];
	Color	m_Colors[MAX_TEAMS+1];

	// @TRJ
	int	m_iXP[MAX_PLAYERS + 1];
	int m_iSkill[MAX_PLAYERS + 1];
	int	m_iUserID[MAX_PLAYERS + 1];
	Vector m_aPosition[MAX_PLAYERS + 1];
	bool m_bIsFirstRound;
};

extern C_PlayerResource *g_PR;

#endif // C_PLAYERRESOURCE_H
