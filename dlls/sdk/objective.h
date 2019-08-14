#ifndef _OBJECTIVE_H_
#define _OBJECTIVE_H_

#include "triggers.h"
#include "sdk_player.h"
#include "sdk_team.h"

#define HOLDAREA_BOOST_AMOUNT 0
#define FUNC_OBJECTIVE "func_objective"

extern ConVar mp_roundlength;
extern ConVar mp_captureleeway;

enum ObjectiveState
{
	NEUTRAL = 0,
	ACTIVE,
	CAPTURED,
};

/**
* Class definition for a generic objective
* Basically manages state information
**/
class CObjective : public CBaseTrigger
{
	DECLARE_CLASS(CObjective, CBaseTrigger);
public:
	// constructor / destructor
	CObjective();
	~CObjective();

	// simple accessors
	int GetCash(void) { return m_iCash; }
	int GetXP(void) { return m_iXP; }
	int GetID(void) { return m_iID; }
	const char *GetTitle(void) { return m_szTitle; }
	void SetOwner(CSDKTeam *pOwner) { m_pOwner = pOwner; }
	CSDKTeam *GetOwner(void) { return m_pOwner; }
	int GetOwnerID(void);
	ObjectiveState GetState(void) { return m_eState; }
	ObjectiveState GetSafeState(void) { return (m_eState == ACTIVE ? (m_pOwner ? CAPTURED : NEUTRAL) : m_eState); }
	void SetCash(int iCash) { m_iCash = iCash; }
	void SetXP(int iXP) { m_iXP = iXP; }

	// (more)complicated accessors (these might need to be defined in subclasses)
	virtual void RoundReset(void); // resets for the round only
	virtual void Reset(void); // resets for the entire game
	virtual void ActivateObjective(void);
	virtual void DeactivateObjective(void);
	virtual void SetCaptured(void);
	virtual bool IsCaptured(void) { return false; }
	virtual bool IsPseudoCaptured(void) { return m_bPseudoCaptured; }
	virtual void SetPseudoCaptured(bool bCaptured) { m_bPseudoCaptured = bCaptured; }
	virtual bool RequiresValues(void) { return true; }
	virtual bool IsCapturableByTeam(CSDKTeam *pTeam) { return false; }
	virtual bool IsRelevantToTeam(CSDKTeam *pTeam) { return false; }
	virtual bool IsRequiredToWin(void) { return true; }
	virtual bool BankruptLosingTeam(void) { return false; }

	// inherited functions, hooks, etc
	virtual bool KeyValue(const char *szKeyName, const char *szValue);
	virtual bool ClassMatches(const char *szClassOrWildcard);
	virtual bool ClassMatches(string_t szNameStr);

protected:
	// boosts
	virtual void BoostPlayer(CSDKPlayer *pPlayer, bool bShouldBoost);
	virtual int GetBoostAmount(CSDKPlayer *pPlayer);
	bool RemoveFromBoostList(CSDKPlayer *pPlayer);
	void AddToBoostList(CSDKPlayer *pPlayer);

	// state information
	ObjectiveState m_eState;
	CSDKTeam *m_pOwner;
	CSDKTeam *m_pOffensiveTeam;
	char m_szTitle[256];
	bool m_bPseudoCaptured; // this is for stats and is pretty hacky. i don't recommend using for anything else

private:
	// generic info
	int m_iID;
	int	m_iCash;
	int	m_iXP;

	// list of players who are boosting
	CUtlVector<CSDKPlayer *> m_aBoostList;
};

/**
* Class definition for a zone objective
**/
class CObjectiveZone : public CObjective
{
	DECLARE_CLASS(CObjectiveZone, CObjective);
public:
	// inherited
	virtual void Spawn(void);
	
	// holdarea functionality
	virtual void StartTouch(CBaseEntity *pEntity);
	virtual void EndTouch(CBaseEntity *pEntity);
	virtual void RoundReset(void);
	virtual CSDKPlayer* GetPlayerInZone(int index) const;
	virtual int NumPlayersInZone(void) const;
	virtual void FakeFirstEntry(void);
	virtual bool PlayersInZone(void) const;

protected:
	// zones
	virtual void FirstPlayerEnteredZone(CSDKPlayer *pPlayer) { }
	virtual void LastPlayerLeftZone(CSDKPlayer *pPlayer) { }

private:
	CUtlVector<CSDKPlayer*> m_aOffensiveList;
};

/**
* Class definition for a holdarea objective
* Same as zone but with a timer
**/
class CObjectiveHoldarea : public CObjectiveZone
{
	DECLARE_CLASS(CObjectiveHoldarea, CObjectiveZone);

public:
	DECLARE_DATADESC();

	// capture
	virtual float GetCaptureTime(void) const;
	virtual float GetCaptureLeeway() const;
	virtual float GetRemainingCaptureTime(void) const;
	virtual bool IsCaptured(void);
	virtual bool IsCapturableByTeam(CSDKTeam *pTeam);
	virtual bool IsRelevantToTeam(CSDKTeam *pTeam);

	// reset
	virtual void RoundReset(void);
	virtual void Reset(void);
	virtual void ResetTimes(void);

	// thinking
	virtual void Think(void);
	virtual void LeewayThink();

protected:
	// zones
	virtual void FirstPlayerEnteredZone(CSDKPlayer *pPlayer);
	virtual void LastPlayerLeftZone(CSDKPlayer *pPlayer);

private:
	//float m_fCaptureStartTime;	// time when the first player entered the zone
	float m_fCaptureLeftTime;	// when the last player left
	float m_fSecondsRemaining;	// number of seconds until we capture the objective
	float m_fLastThink;
};

/**
* Class definition for a base holdarea
**/
class CObjectiveHoldareaBase : public CObjectiveHoldarea
{
	DECLARE_CLASS(CObjectiveHoldareaBase, CObjectiveHoldarea);
public:
	virtual bool KeyValue(const char *szKeyName, const char *szValue);
	virtual void Reset(void);
	virtual void SetCaptured(void);
	virtual void DeactivateObjective(void);
	virtual bool IsCapturableByTeam(CSDKTeam *pTeam) { return IsRelevantToTeam(pTeam); }
	virtual bool IsRelevantToTeam(CSDKTeam *pTeam);
	virtual bool IsRequiredToWin(void) { return false; }
	virtual bool BankruptLosingTeam(void) { return true; }

	// hooks
	virtual bool RequiresValues(void) { return false; }
};

#endif