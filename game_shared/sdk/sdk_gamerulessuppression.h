#ifdef SUPPRESSION_ENABLED

#ifndef _SDKGAMERULESSUPPRESSION_H_
#define _SDKGAMERULESSUPPRESSION_H_

#include "sdk_gamerules.h"
#include "gamevars_shared.h"

#ifndef CLIENT_DLL
	#include "spawnpoint.h"
	#include "suppressionround.h"
#endif

#ifdef CLIENT_DLL
	#define CSDKGameRulesSuppression C_SDKGameRulesSuppression
	#define CSDKGameRulesSuppressionProxy C_SDKGameRulesSuppressionProxy
#endif

#define PLAYER_COLD_BLOODED_MURDER_XP_SUPPRESSION_AMERICAN 8
#define PLAYER_COLD_BLOODED_MURDER_XP_SUPPRESSION_COALITION 3

extern ConVar mp_suppressionhalftimelimit;
extern ConVar mp_suppressioncancommitsuicide;

class CSDKGameRulesSuppressionProxy : public CSDKGameRulesProxy
{
public:
	DECLARE_CLASS( CSDKGameRulesSuppressionProxy, CSDKGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

#define GET_SUPPRESSION_ROUND() ((CSuppressionRound *)GetCurrentRound())

/**
* Class declaration for suppression game rules
**/
class CSDKGameRulesSuppression : public CSDKGameRules
{
public:
	DECLARE_CLASS(CSDKGameRulesSuppression, CSDKGameRules);

	// misc
	virtual bool CanShowEnemiesOnMap(CSDKPlayer *pPlayer) { return false; }
	virtual bool ShouldRemoveLeftovers(void) { return true; }
	virtual bool CanJoinClass(CSDKPlayer *pPlayer, const char *szSkill);
	virtual bool CanCommitSuicide(CSDKPlayer *pPlayer) { return mp_suppressioncancommitsuicide.GetInt() == 1; }
	virtual int GetForceCam(void) { return mp_suppressionforcecamera.GetInt(); }

#ifndef CLIENT_DLL

	// constructor
	CSDKGameRulesSuppression();

	// spawning
	virtual bool IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer );
	virtual bool GetSpawnPointName(char szName[], int iLen, int iTeam);
	virtual void PlayerSpawn(CBasePlayer *pBasePlayer);
	virtual CBaseEntity *GetPlayerSpawnSpot(CBasePlayer *pPlayer);
	virtual CBaseEntity *GetValidSpawnPoint(CSDKPlayer *pPlayer);
	virtual CSpawnPoint *GetValidCoalitionSpawnPoint(CSDKPlayer *pPlayer);
	virtual CSpawnPoint *GetValidAmericanSpawnPoint(CSDKPlayer *pPlayer);
	virtual void IncrementSpawnGroup(int iTeam);
	virtual void FindSpawns(void);
	virtual void ClearSpawns(void);
	virtual void ResetSpawns(void);
	void EquipPlayer(CSDKPlayer *pPlayer);

	// teams
	virtual bool CanDelayTeamSwitch(void) { return false; }
	virtual void PlayerChangedTeam(CSDKPlayer *pPlayer);

	// game state
	virtual bool IsGameOver(void);
	virtual float GetTimeLimit(void) { return 0; }
	virtual bool IsFirstRoundComplete(void) { return m_bFirstRoundComplete; }
	virtual void SendWinLoseTip(void);

	// kills
	virtual void PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info);

	// skills
	virtual void PlayerChangedSkill(CSDKPlayer *pPlayer); 

	// rewards
	virtual int GetPlayerKillXPReward(CSDKPlayer *pPlayer) { return pPlayer->GetTeamNumber() == TEAM_A ? PLAYER_COLD_BLOODED_MURDER_XP_SUPPRESSION_AMERICAN : PLAYER_COLD_BLOODED_MURDER_XP_SUPPRESSION_COALITION; }

	// objectives
	virtual bool CanTeamCaptureObjective(CSDKTeam *pTeam, CObjective *pObj) { return pTeam->GetTeamNumber() == TEAM_A; }
	virtual bool IsObjectiveRelevantToTeam(CSDKTeam *pTeam, CObjective *pObj) { return false; }
	virtual CObjectiveManager *GetNewObjectiveManager(void) { return new CSuppressionObjectiveManager(); }

	// game state
	void RestartGame(void);

#endif

protected:

#ifndef CLIENT_DLL
	virtual CRound *GetNewRound(void);
#endif

private:

#ifndef CLIENT_DLL
	int m_iSpawnGroups;
	int m_iCurrentAmericanSpawnGroup;
	int m_iCurrentCoalitionSpawnObjective;
	bool m_bFirstRoundComplete;
	int m_iFirstRoundAACaptures;
#endif
};

inline CSDKGameRulesSuppression* SDKGameRulesSuppression()
{
	return static_cast<CSDKGameRulesSuppression*>(g_pGameRules);
}

#endif

#endif