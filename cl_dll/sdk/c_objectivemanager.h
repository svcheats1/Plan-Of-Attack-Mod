#ifndef _C_OBJECTIVEMANAGER_H_
#define _C_OBJECTIVEMANAGER_H_

#include <UtlVector.h>
#include "usermessages.h"
#include "igameevents.h"
#include "objectiveproxy.h"

#define MAX_OBJECTIVES 10

/**
* Struct representing an objective area.  
* Didn't use the version derived from a trigger so that I didn't 
* have to move triggers to the client
**/
typedef struct SObjective_s
{
	int iID;
	int iOwner;
	char szName[256];
	int iXP;
	int iCash;
	bool bActive;
	bool bDirty;
	bool bIsBase;
	Vector vecPos;
} SObjective_t;

/**
* Class definition for the client side objective manager
* Stupidified version of the server side one
**/
class C_ObjectiveManager : public IGameEventListener2
{
public:
	// singleton access
	static C_ObjectiveManager *GetInstance(void);
	static void Term(void);

	// accessors
	bool GetObjectivesExist(void) { return m_iObjectivesCount > 0; }
	SObjective_t *GetObjective(int iIndex);
	int GetObjectiveCount(void) { return m_iObjectivesCount; }
	SObjective_t *GetNextObjective(SObjective_t *pObj = NULL);
	SObjective_t *GetActiveObjective(void);
	void Reset(void);
	void SetProxy(CObjectiveProxy *pProxy, CObjectiveProxy *pOldProxy = NULL);
	int GetPlayersInZone(void);

	// message handling
	static void MsgFunc_ObjectiveStatus(bf_read &msg);
	void FireGameEvent(IGameEvent *pEvent);

protected:

	// message handling
	void ReceivedObjective(bf_read &msg);
	void NotifyHUD(SObjective_t *pObj);

private:
	// constructor/destructor
	C_ObjectiveManager();
	~C_ObjectiveManager();

	CUtlVector<SObjective_t *> m_aObjectives;
	int m_iObjectivesCount;
	CObjectiveProxy *m_pProxy;

	// the singleton
	static C_ObjectiveManager *s_pInstance;
};

#define GET_OBJ_MGR C_ObjectiveManager::GetInstance

#endif