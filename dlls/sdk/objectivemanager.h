#ifndef _OBJECTIVEMANAGER_H_
#define _OBJECTIVEMANAGER_H_

#include "objective.h"
#include "objectiveproxy.h"

#define MAX_OBJECTIVES 10
#define FUNC_OBJECTIVE "func_objective"

/**
* Class definition for an objective manager
* Uses singleton pattern
* Manages everything to do with objectives
**/
class CObjectiveManager
{
public:
	DECLARE_CLASS_NOBASE(CObjectiveManager);

	// singleton accessors
	static CObjectiveManager *GetInstance(void);
	static void Term(void);

	// constructor / destructor
	// @NOTE - FOR THE LOVE ALL THAT IS HOLY, DON'T CALL US (new/delete)!!!!
	CObjectiveManager();
	~CObjectiveManager();

	// accessors
	void SetCurrentObjective(CSDKPlayer *pPlayer, int iObj, bool bForce = false);
	CObjective *GetCurrentObjective(void) { return m_pCurrentObj; }
	CObjective *GetObjective(int iIndex) { return (m_aObjectives.IsValidIndex(iIndex) ? m_aObjectives[iIndex] : NULL); }
	bool IsActiveObjectiveEmpty(void) { return !m_bObjectiveOccupied; }
	void PickObjective(int iTeam);
	void RoundEnded(void);
	void RoundReset(void);
	void ResetAllObjectives(void);
	static void Reset(void);
	void DisableTimerNotification(void);
	virtual void Finalize(CObjective *pObj) { /* just a hook */ }
	virtual void FinalizeObjectives(void);
	void EstablishObjectives(void);
	bool AreObjectivesEstablished(void) { return m_bObjectivesEstablished; }
	virtual void ObjectiveEntered(CObjectiveHoldarea *pObj);
	virtual void ObjectiveLeft(CObjectiveHoldarea *pObj);
	virtual void ObjectiveLeewayExpired(CObjectiveHoldarea *pObj);
	virtual void SetObjectiveCaptured(CObjective *pObj);
	void UpdatePlayer(CSDKPlayer *pPlayer);
	bool AllCaptured(void);
	CObjective *GetPlayersBase(CSDKPlayer *pPlayer);
	void ForceObjectiveSelection(void);
	void GetObjectiveCounts(int iCounts[MAX_TEAMS+1]);
	const char* GetDefendableObjectives(int iTeam);
	void PlayerLeft(CBasePlayer *pPlayer);
	virtual float GetCaptureTimeForObjective(const CObjectiveHoldarea *pObj) const;
	virtual void SetProxy(CObjectiveProxy *pProxy, CObjectiveProxy *pOldProxy = NULL);
	virtual void SetPlayersInZone(int iPlayers);
	virtual bool IsLastObjective(CObjective *pObj);

protected:

	// initialization
	void InitializeObjectives(void);
	virtual void UpdateClientObjective(CObjective *pObj);
	virtual void UpdateClientObjective(CObjective *pObj, CSDKPlayer *pPlayer, bool bSendSafe);
	virtual void UpdateClientObjective(CObjective *pObj, CSDKTeam *pTeam, bool bSendSafe);
	virtual void UpdateClientObjective(CObjective *pObj, CRecipientFilter *pFilter, ObjectiveState eState);
	void SendChooseObjective(CSDKPlayer *pPlayer);
	void UpdateTimerNotification(float fDuration, float fTime, OBJECTIVE_TIMER_STATE iState, float fLeewayTime = 0.0);
	void WriteObjectiveCaptureInfo(CObjective *pObj, float fDuration = -1);

	// helpers
	CObjective *FindObjective(CObjective *pObj);
	void AssignValues(CObjective *pObj);

protected:

	// objectives
	CUtlVector<CObjective *> m_aObjectives;
	CObjective *m_pCurrentObj;
	CSDKPlayer *m_pObjPicker;
	CSDKTeam *m_pObjPickerTeam;
	bool m_bObjectivesEstablished;
	bool m_bObjectiveOccupied;
	int m_iObjectiveCount;
	CObjectiveProxy *m_pProxy;

	// singleton
	static CObjectiveManager *s_pInstance;
};

#define GET_OBJ_MGR CObjectiveManager::GetInstance

/**
* Simple struct for storing value combinations
**/
typedef struct SValues_s
{
	int m_iCash;
	int m_iXP;
} SValues_t;

/**
* Class definition for ObjectiveValues
* Helper for the CObjectiveManager class that handles the values that
* need to be generated for each objective
**/
class CObjectiveValues
{
public:
	// singleton
	static CObjectiveValues *GetInstance(void);
	static void Term(void);

	// accessors
	void GenerateValues(int iNumValues);
	SValues_t *GetValues(void);

protected:
	static int Random(SValues_t * const *pLeft, SValues_t * const *pRight);
	void DestroyVector(CUtlVector<SValues_t *> &pVec);

	static int s_aCashValues[3];
	static int s_aXPValues[3];

private:
	// constructor / destructor
	CObjectiveValues();
	~CObjectiveValues();

	// value storage
	CUtlVector<SValues_t *> m_aFree;
	CUtlVector<SValues_t *> m_aUsed;

	// instance
	static CObjectiveValues *s_pInstance;
};

#endif