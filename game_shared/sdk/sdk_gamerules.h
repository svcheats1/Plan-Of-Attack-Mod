//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef SDK_GAMERULES_H
#define SDK_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

#include "teamplay_gamerules.h"
#include "convar.h"
#include "gamevars_shared.h"
#include "igameevents.h"

#ifdef CLIENT_DLL
	#include "c_baseplayer.h"
	#include "c_sdk_player.h"
#else
	#include "player.h"
	#include "round.h"
	#include "suppressionobjectivemanager.h"
#endif

class CRound;

#ifdef CLIENT_DLL
	#define CSDKGameRules C_SDKGameRules
	#define CSDKGameRulesProxy C_SDKGameRulesProxy
#endif

#define PLAYER_COLD_BLOODED_MURDER_XP 4

extern ConVar mp_gamestoplay;

class CSDKGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CSDKGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CSDKGameRules : public CTeamplayRules, public IGameEventListener2
{
public:
	DECLARE_CLASS( CSDKGameRules, CTeamplayRules );

	virtual bool	ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual int		PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual bool	IsTeamplay( void ) { return true;	}
	virtual const char *GetGameDescription( void ) { return "PoA Beta 4"; }
	virtual bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return false; }
	virtual bool CanShowEnemiesOnMap(CSDKPlayer *pPlayer) { return true; }
	virtual bool ShouldRemoveLeftovers(void) { return false; }
	virtual bool CanJoinClass(CSDKPlayer *pPlayer, const char *szSkill) { return true; }
	virtual bool CanCommitSuicide(CSDKPlayer *pPlayer) { return true; }

	// events
	virtual void FireGameEvent(IGameEvent * pEvent);

	CSDKGameRules();
	virtual ~CSDKGameRules();

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

	virtual int CanLocalPlayerBuy();
	virtual float GetRemainingBuyTime();
	virtual void SetBuyTime(float fTime);

protected:

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.

	void RecoverTeams(void);
	virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem );
	virtual void PlayerSpawn( CBasePlayer *pBasePlayer );
	virtual void FindSpawns(void);
	virtual void ClearSpawns(void);
	virtual void ResetSpawns(void) { }
	virtual CBaseEntity *GetValidSpawnPoint(CSDKPlayer *pPlayer);
	virtual bool GetSpawnPointName(char szName[], int iLen, int iTeam);
	virtual bool IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer );
	virtual bool CanSpawn(CBasePlayer *pPlayer);
	virtual bool ClientCommand( const char *pcmd, CBaseEntity *pEnt );
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void ClientSettingsChanged( CBasePlayer *pBasePlayer );
	virtual void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore );
	virtual void Think();
	virtual bool IsFirstRound(void);
	virtual CRound *GetNewRound(void);
	virtual bool CanDelayTeamSwitch(void) { return true; }
	virtual bool AtMaxForSkill(CSDKPlayer *pPlayer, const char *szSkill);

	virtual const char *GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer );

	virtual void RestartGame(void);
	void RestartMap(void);
	void ResetTeams(void);
	void SwapTeams(void);
	virtual bool IsGameOver(void);
	virtual void GoToIntermission(void);
	virtual void ChangeLevel(void);
	virtual void CleanMap(void);
	CRound *GetCurrentRound(void);
	virtual bool PlayerCanHearChat(CBasePlayer *pListener, CBasePlayer *pSpeaker);
	virtual bool MoreGamesToPlay(void) { return mp_gamestoplay.GetInt() > m_iGamesPlayed; }
	virtual void SendWinLoseTip(void);
	void IncrementGamesPlayed(void) { ++m_iGamesPlayed; }
	
	// freeze time
	virtual bool IsPlayerFrozen(CSDKPlayer *pPlayer);
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer );

	// round hooks
	virtual void RoundEnded(void);
	virtual void RoundRestart(void);

	// buy time
	virtual bool InBuyTime(void);

	// teams
	virtual void ProcessAutoTeamBalance(void);
	virtual void PrepareAutoTeamBalance(void);
	virtual void AnnounceAutoTeamBalance(void);
	virtual bool CanJoinTeam(CSDKPlayer *pPlayer, int iTeamNum);
	virtual void ProcessTeamChangeRequests(void);
	virtual void GetTeamCounts(int iCounts[]);
	virtual int TeamWithFewestPlayers( void );
	virtual void UpdateTeamScores(void);
	virtual void PlayerChangedTeam(CSDKPlayer *pPlayer) { /* just a hook */ }

	// idle players
	virtual void RemoveIdlePlayers();

	// xp and morale
	void UpdateMorale(void);
	bool IsGameState(int iTeam, bool bWon1, bool bWon2, bool bWon3);
	void UpdateGameState(CSDKTeam *pWinner);
	int GetMoraleXPAward(CSDKTeam *pTeam);
	void InitGameState(void);
	virtual void PlayerChangedSkill(CSDKPlayer *pPlayer) { /* just a hook */ }
	virtual int GetPlayerKillXPReward(CSDKPlayer *pPlayer) { return PLAYER_COLD_BLOODED_MURDER_XP; }
	virtual int MinXPForGame(void);

	// objectives
	virtual bool CanTeamCaptureObjective(CSDKTeam *pTeam, CObjective *pObj) { return true; }
	virtual bool IsObjectiveRelevantToTeam(CSDKTeam *pTeam, CObjective *pObj) { return true; }
	virtual CObjectiveManager *GetNewObjectiveManager(void) { return new CObjectiveManager(); }

	// @HACK - i apologize
	static bool s_bFirstPlayer;

protected:
	// spawn points
	CUtlVector<CBaseEntity *> m_aSpawns;

private:
	void ResetGame(void);

	void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, bool bIgnoreWorld );

	// the current round
	CRound *m_pRound;

	// which team we're switching to
	int m_iAutoBalanceTeam;
	// the players we're switching
	CUtlVector<int> m_AutoBalancePlayers;

	// xp and experience
	int m_iMorale[MAX_TEAMS];
	bool m_bGameState[MAX_TEAMS][3];
	int m_iRoundCount;

	// game looping
	int m_iGamesPlayed;

#endif

#ifdef CLIENT_DLL
	float m_fBuyEndTime;
#endif
};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CSDKGameRules* SDKGameRules()
{
	return static_cast<CSDKGameRules*>(g_pGameRules);
}


#endif // SDK_GAMERULES_H
