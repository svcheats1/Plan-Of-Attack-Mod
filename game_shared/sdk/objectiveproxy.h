#ifndef _OBJECTIVEPROXY_H_
#define _OBJECTIVEPROXY_H_

#ifdef CLIENT_DLL
	#define CObjectiveProxy C_ObjectiveProxy
	#include "c_baseentity.h"
#else
	#include "baseentity.h"
#endif

/**
* Class declaration for an objective proxy.  This is basically a complete hack.
* The suppression system requires that the number of players influence the rate
* at which the objective is captured.  Since objectives are not networked we have
* this proxy that performs this stupid task.
**/
class CObjectiveProxy : public CBaseEntity
{
public:
	DECLARE_CLASS(CObjectiveProxy, CBaseEntity);

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS();
#else
	DECLARE_SERVERCLASS();
#endif

	// constructor / destructor
	CObjectiveProxy();
	~CObjectiveProxy();

	// accessors
	void SetPlayersInZone(int iPlayers) { m_iPlayersInZone = iPlayers; }
	int GetPlayersInZone(void) { return m_iPlayersInZone; }

#ifndef CLIENT_DLL

	// accessors
	int UpdateTransmitState(void);

#endif

public:
	CNetworkVar(int, m_iPlayersInZone);
};

#endif