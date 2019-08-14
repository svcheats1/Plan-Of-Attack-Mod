#include "cbase.h"
#include "c_objectivemanager.h"
#include "hud_objectives.h"
#include "keyvalues.h"
#include "worldmap.h"

// Forward declarations
C_ObjectiveManager *C_ObjectiveManager::s_pInstance = NULL;

/**
* Handles the singleton
*
* @return C_ObjectiveManager *
**/
C_ObjectiveManager *C_ObjectiveManager::GetInstance(void)
{
	// do we have the instance yet?
	if(!s_pInstance)
		s_pInstance = new C_ObjectiveManager;

	return s_pInstance;
}

/**
* Destroys the singleton
*
* @return void
**/
void C_ObjectiveManager::Term(void)
{
	// did we create the instance yet?
	if(s_pInstance)
		delete(s_pInstance);
	s_pInstance = NULL;
}

/**
* Constructor
**/
C_ObjectiveManager::C_ObjectiveManager()
{
	// register message handler once
	usermessages->HookMessage("ObjectiveStatus", C_ObjectiveManager::MsgFunc_ObjectiveStatus);

	// reset
	Reset();

	// register for the events
	gameeventmanager->AddListener(this, "objective_notification", false);
	gameeventmanager->AddListener(this, "player_team", false);

	// no proxy
	m_pProxy = NULL;
}

/**
* Destructor
**/
C_ObjectiveManager::~C_ObjectiveManager()
{
	// die!
	m_aObjectives.PurgeAndDeleteElements();

	gameeventmanager->RemoveListener( this );
}

/**
* Resets the manager
*
* @return void
**/
void C_ObjectiveManager::Reset(void)
{
	// just reset the count and our list
	m_iObjectivesCount = 0;
	m_aObjectives.PurgeAndDeleteElements();
	
	// create space for the objectives
	m_aObjectives.SetSize(MAX_OBJECTIVES);

	// create a bunch of elements
	for(int i = 0; i < MAX_OBJECTIVES; ++i)
	{
		// create and configure
		m_aObjectives[i] = new SObjective_t;
		m_aObjectives[i]->bDirty = false;
		m_aObjectives[i]->iID = -1;
	}
}

/**
* Defines a handler for the ObjectiveStatus
*
* @param bf_read &msg The message to handle
* @return void
**/
void C_ObjectiveManager::MsgFunc_ObjectiveStatus(bf_read &msg)
{
	// pass it along to the instance
	GET_OBJ_MGR()->ReceivedObjective(msg);
}

/**
* Actually handles the message for the singleton
*
* @param bf_read &msg The message we received
* @return void
**/
void C_ObjectiveManager::ReceivedObjective(bf_read &msg)
{
	int iID;

	// read everything
	iID = msg.ReadShort();

	// make sure it's valid
	if(!m_aObjectives.IsValidIndex(iID))
		return;

	// is this a new one?
	if(m_aObjectives[iID]->iID == -1)
		++m_iObjectivesCount;

	// set the remaining attributes
	m_aObjectives[iID]->iID = iID;
	m_aObjectives[iID]->iOwner = msg.ReadShort();
	m_aObjectives[iID]->iXP = msg.ReadShort();
	m_aObjectives[iID]->iCash = msg.ReadShort();
	m_aObjectives[iID]->bActive = msg.ReadByte();
	m_aObjectives[iID]->bIsBase = msg.ReadByte();
	m_aObjectives[iID]->bDirty = true;
	msg.ReadBitVec3Coord(m_aObjectives[iID]->vecPos);
	msg.ReadString(m_aObjectives[iID]->szName, sizeof(m_aObjectives[iID]->szName));

	// tell the hud
	NotifyHUD(m_aObjectives[iID]);
}

/**
* Tells the hud elements about the objective
* @NOTE - By convention every hud element this notifies should have a OnObjectiveEvent
*
* @param SObjective_t *pObj The objective to notify everyone about
* @return void
**/
void C_ObjectiveManager::NotifyHUD(SObjective_t *pObj)
{
	// objective status
	HudObjectives *pHudObj = GET_HUDELEMENT(HudObjectives);
	pHudObj->OnObjectiveEvent(pObj);

	// find the worldmap and update its legend
	CWorldMap *pMap = (CWorldMap *)gViewPortInterface->FindPanelByName(PANEL_WORLDMAP);
	if(pMap)
		pMap->UpdateLegend();
}

/**
* Returns the objective identified by the index
*
* @param int iIndex The index of the objective
* @return SObjective_t *
**/
SObjective_t *C_ObjectiveManager::GetObjective(int iIndex)
{
	// do we have that one?
	if(!m_aObjectives.IsValidIndex(iIndex))
		return NULL;

	return m_aObjectives[iIndex];
}

/**
* Finds the next complete objective
*
* @param SObjective_t *pObj The last objective found.  If NULL, we'll start over
* @return SObjective_t *pObj Or NULL when no more could be found
**/
SObjective_t *C_ObjectiveManager::GetNextObjective(SObjective_t *pObj)
{
	int i;

	// did we get an objective?
	if(!pObj)
		i = 0;
	else
		i = pObj->iID + 1;

	// bounds check
	if(i < 0 || i >= m_aObjectives.Count())
		return NULL;

	// iterate until we find one with a real id
	while(i < m_aObjectives.Count())
	{
		// did we find one?
		if(m_aObjectives[i]->iID != -1)
			break;

		++i;
	}

	// did we get one?
	if(i < m_aObjectives.Count())
		return m_aObjectives[i];

	return NULL;
}

/**
* Handles events coming our way
*
* @param KeyValues *pEvent The event we need to handle
* @return void
**/
void C_ObjectiveManager::FireGameEvent(IGameEvent *pEvent)
{
	// what kind is it?
	if(FStrEq(pEvent->GetName(), "objective_notification"))
	{
		// grab the objective
		int iObjID = pEvent->GetInt("id");
		SObjective_t *pObj = GET_OBJ_MGR()->GetObjective(iObjID);

		// did we get it?
		if(!pObj || pObj->iID == -1)
			return;

		// are we out of time?
		if(pEvent->GetFloat("timer") > 0)
			// turn it on
			pObj->bActive = true;

		// tell the hud
		NotifyHUD(pObj);
	}
	// someone is changing teams....
	else if (FStrEq(pEvent->GetName(), "player_team"))
	{
		// who is it?
		int iPlayerID = pEvent->GetInt("userid");
		int iTeamID = pEvent->GetInt("team");

		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

		if (pPlayer && iPlayerID == pPlayer->GetUserID()) {
			// set the team id now, because otherwise it gets messed up
			pPlayer->m_iTeamNum = iTeamID;

			// go through our objectives
			for(int i = 0; i < MAX_OBJECTIVES; ++i)
			{
				// if there an objective here
				if (m_aObjectives[i]->iID != -1) {
					// set it as dirty so the menu updates
					m_aObjectives[i]->bDirty = true;

					// notify the HUD so it updates
					NotifyHUD(m_aObjectives[i]);
				}
			}
		}
	}
}

/**
* Finds the active objective if there is one
*
* @return SObjective_t * The objective that is currently active or NULL
**/
SObjective_t *C_ObjectiveManager::GetActiveObjective(void)
{
	// step through all of them
	for(int i = 0; i < MAX_OBJECTIVES; ++i)
	{
		// do we have this one?
		if(m_aObjectives[i]->iID != -1 && m_aObjectives[i]->bActive)
			return m_aObjectives[i];
	}

	return NULL;
}

/**
* Sets the proxy
*
* @param CObjectiveProxy *pProxy The proxy to use
* @param CObjectiveProxy *pOldProxy The old proxy
* @return void
**/
void C_ObjectiveManager::SetProxy(CObjectiveProxy *pProxy, CObjectiveProxy *pOldProxy/* = NULL*/)
{
	// if we have and old proxy and it's not the same as the current one, don't overwrite
	if(pOldProxy && pOldProxy != m_pProxy)
		return;

	// set it
	m_pProxy = pProxy;
}

/**
* Gets the number of players in teh active objective
*
* @return int
**/
int C_ObjectiveManager::GetPlayersInZone(void)
{
	// if we have a proxy get the count
	if(m_pProxy)
		return m_pProxy->GetPlayersInZone();

	return 0;
}