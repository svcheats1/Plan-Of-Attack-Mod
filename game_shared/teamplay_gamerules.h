//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEAMPLAY_GAMERULES_H
#define TEAMPLAY_GAMERULES_H
#pragma once

#include "gamerules.h"
#include "multiplay_gamerules.h"
#include "gamevars_shared.h"

#ifdef CLIENT_DLL

	#define CTeamplayRules C_TeamplayRules

#else

	#include "takedamageinfo.h"

#endif


//
// teamplay_gamerules.h
//


#define MAX_TEAMNAME_LENGTH	16
#define MAX_TEAMS			2

#define TEAMPLAY_TEAMLISTLENGTH		MAX_TEAMS*MAX_TEAMNAME_LENGTH


class CTeamplayRules : public CMultiplayRules
{
public:
	DECLARE_CLASS( CTeamplayRules, CMultiplayRules );

	virtual int GetForceCam(void) { return mp_forcecamera.GetInt(); }

#ifdef CLIENT_DLL

#else

	CTeamplayRules();
	virtual ~CTeamplayRules() {};

	virtual void Precache( void );

	virtual bool ClientCommand( const char *pcmd, CBaseEntity *pEdict );
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );
	virtual bool IsTeamplay( void );
	virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual bool PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker );
	virtual const char *GetTeamID( CBaseEntity *pEntity );
	virtual bool ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target );
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual int IFragsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void InitHUD( CBasePlayer *pl );
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	//virtual const char *GetGameDescription( void ) { return "Plan of Attack"; }  // this is the game name that gets seen in the server browser
	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual void Think ( void );
	virtual int GetTeamIndex( const char *pTeamName );
	virtual const char *GetIndexedTeamName( int teamIndex );
	virtual bool IsValidTeam( const char *pTeamName );
	virtual const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer );
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib );
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void RestartGame(void);
	bool TeamPlayersExist(void);
	virtual int TeamWithFewestPlayers( void );

protected:
	bool m_DisableDeathMessages;

private:
	void RecountTeams( void );

	bool m_DisableDeathPenalty;
	bool m_teamLimit;				// This means the server set only some teams as valid
	char m_szTeamList[TEAMPLAY_TEAMLISTLENGTH];

#endif
};

#endif // TEAMPLAY_GAMERULES_H
