#ifndef _SUPPRESSIONROUND_H_
#define _SUPPRESSIONROUND_H_

#include "round.h"

#ifdef SUPPRESSION_ENABLED

#define SUPPRESSION_CAPTURE_AWARD 30

/**
* Class declaration for a suppression round
*	We're modeling the entire game as a big long round
**/
class CSuppressionRound : public CRound
{
public:
	DECLARE_CLASS(CSuppressionRound, CRound);

	// constructor / destructor
	CSuppressionRound();
	~CSuppressionRound();

	// initialization, states, and thinking
	virtual void Think(void);
	virtual void StartRound(void);
	virtual bool IsRoundOver(void);
	bool InFirstSpawn(void) { return gpGlobals->curtime < mp_initialsuppressionspawntime.GetFloat(); }
	virtual bool IsFirstRound(void) { return false; }
	virtual void Restart(void /*bool bRespawnPlayers = true*/);

	// teams
	virtual bool ShouldSwitchTeams(void) { return false; }

	// spawning
	virtual bool CanSpawn(CBasePlayer *pPlayer);
	void QueuePlayerSpawn(CBasePlayer *pPlayer);
	void SpawnWave(void);
	void InitSpawn(void);
	void SetNextSpawnTime(float fTime) { m_fNextSpawnTime = fTime; }

	// rewards
	virtual void AwardTeams(CSDKTeam *pWinner);

private:
	CUtlVector<CSDKPlayer *> m_aSpawnQueue;
	float m_fNextSpawnTime;
	bool m_bSpawningWave;
	float m_fLastTimeLimit;
};

#endif

#endif