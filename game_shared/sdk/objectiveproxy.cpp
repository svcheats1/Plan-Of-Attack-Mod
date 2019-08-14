#include "cbase.h"
#include "objectiveproxy.h"

#ifdef CLIENT_DLL
	#include "c_objectivemanager.h"
#else
	#include "objectivemanager.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(objective_proxy, CObjectiveProxy);
IMPLEMENT_NETWORKCLASS_ALIASED(ObjectiveProxy, DT_ObjectiveProxy)

BEGIN_NETWORK_TABLE(CObjectiveProxy, DT_ObjectiveProxy)

	#ifdef CLIENT_DLL
		RecvPropInt(RECVINFO(m_iPlayersInZone))
	#else
		SendPropInt(SENDINFO(m_iPlayersInZone), 6, 0),
	#endif

END_NETWORK_TABLE()

/**
* Constructor
**/
CObjectiveProxy::CObjectiveProxy()
{
	// add myself to the objective manager
	GET_OBJ_MGR()->SetProxy(this);
}

/**
* Destructor
**/
CObjectiveProxy::~CObjectiveProxy()
{
	// the proxy gets delete kind of late and we may not
	// have the game rules to get the objective manager
	// check for null
	if(GET_OBJ_MGR())
		// set the proxy to null
		GET_OBJ_MGR()->SetProxy(NULL, this);
}

#ifndef CLIENT_DLL

/**
* We should always transmit the proxy to the client
*
* @return int
**/
int CObjectiveProxy::UpdateTransmitState(void)
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

#endif