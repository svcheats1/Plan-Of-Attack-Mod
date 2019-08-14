#ifndef _ROUND_H_
#define _ROUND_H_

#include "convar.h"
#include "sdk_player.h"
#include "sdk_team.h"
#include "objectivemanager.h"
#include "sdk_gamerules.h"
#include "recipientfilter.h"

#define BASE_CASH_AWARD 1600
#define TEAM_WIN_CASH_AWARD 1000
#define END_OF_FIRST_ROUND_CASH 2000

/**
* Class definition for functor that respawns the player given to it
**/
class RespawnFunctor
{
public:
	bool operator() ( CBasePlayer *pPlayer )
	{
		// if they're dead, steal their shit
		if(pPlayer->GetTeam() && pPlayer->GetTeam()->IsPlayingTeam())
		{
			if(pPlayer->IsDead())
				pPlayer->RemoveAllItems(true);
		
			// respawn
			pPlayer->Spawn();
		}
		return true;
	}
};

/**
* Class definition for functor that un/freezes the player if they are on the correct team
**/
class FreezeFunctor
{
public:
	// constructor
	FreezeFunctor(int iTeam, bool bShouldFreeze = true)
		: m_iTeam(iTeam), m_bShouldFreeze(bShouldFreeze)
	{
		// wah!?!?
	}

	// paren operator
	bool operator() ( CBasePlayer *pPlayer )
	{
		// are we on the correct team?
		if(pPlayer->GetTeamNumber() == m_iTeam)
			// change the state
			((CSDKPlayer *)pPlayer)->SetFreezeState(m_bShouldFreeze);

		return true;
	}
private:
	int m_iTeam;
	bool m_bShouldFreeze;
};

/**
* CRound class definition
*
* CRound represents a single round of play
**/
class CRound
{
public:
	DECLARE_CLASS_NOBASE(CRound);

	// constructor
	CRound();
	~CRound();

	friend class CStrategyManager;

	// inherited
	virtual void Think(void);
	
	// accessors
	virtual void Restart(void /*bool bRespawnPlayers = true*/);
	bool IsFirstRound(void) { return m_bIsFirstRound; }
	void SetFirstRound(bool bFirstRound) { m_bIsFirstRound = bFirstRound; }
	bool InBuyTime(void);
	bool InIntermission(void);
	virtual bool IsRoundOver(void);
	virtual void AwardTeams(CSDKTeam *pWinner);
	float GetEndTime(void) { return m_fRoundEndTime; }
	
	// freeze time
	bool IsPlayerFrozen(CSDKPlayer *pPlayer);
	bool InOffensiveFreezeTime(void);
	bool InDefensiveFreezeTime(void);
	bool InDefensiveChooseTime(void);
	float GetRemainingOffensiveFreezeTime(void);
	float GetRemainingDefensiveFreezeTime(void);
	float GetRemainingDefensiveChooseTime(void);

	// spawns
	virtual bool CanSpawn(CBasePlayer *pPlayer);
	void StartNewClientTimer(CSDKPlayer *pPlayer);
	void SendPlayerBuyTime(CBasePlayer *pPlayer);
	void SendAllPlayersBuyTime();

	// orders
	void SendAttackOrders(CObjective *pObj);
	void SendDefendOrders(void);

protected:
	// helpers
	void StartClientTimers(float fTime);
	void SendBuyTime(CRecipientFilter &filter);
	virtual void StartRound(void);
	bool RoundTimeExpired(void);
	void NotifyTeams(CSDKTeam *pWinner);
	void WriteObjectiveInfo(CObjective *pObj);
	void SwitchTeams(void);
	virtual bool ShouldSwitchTeams(void) { return true; }

	// intermission
	void GoToIntermission(void);
	bool IntermissionOver(void);

	// end of round
	CSDKTeam* DetermineWinner(void);
	CSDKTeam* DetermineWinnerByObjective(void);
	CSDKTeam* DetermineWinnerByDeathmatch(void);
	bool IsRoundADraw(void);

	// freeze
	bool OffensiveFreezeTimeOver(void);
	bool DefensiveFreezeTimeOver(void);
	bool DefensiveChooseTimeOver(void);
	void StartFreezeTime(void);
	void EndOffensiveFreezeTime(void);
	void EndDefensiveFreezeTime(void);
	void EndDefensiveChooseTime(void);

protected:
	// round status
	bool m_bInIntermission;
	float m_fRoundEndTime;
	bool m_bIsFirstRound;
	bool m_bRoundStarted;

	// intermissions
	float m_fIntermissionEndTime;

	// freeze time
	int m_fOffensiveFreezeEndTime;
	bool m_bInOffensiveFreezeTime;
	int m_fDefensiveFreezeEndTime;
	bool m_bInDefensiveFreezeTime;

	int m_fDefensiveChooseEndTime;
	bool m_bInDefensiveChooseTime;


	// buy time
	float m_fBuyStartTime;

	// teams
	int m_iOffensiveTeam;
	int m_iDefensiveTeam;
	CSDKTeam* m_pWinningTeam;
};

#endif