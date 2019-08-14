#include "cbase.h"
#include "objectivemanager.h"
#include "suppressionobjectivemanager.h"
#include "vstdlib/random.h"
#include "sdk_gamerules.h"
#include "strategymanager.h"
#include <igameevents.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// foward declarations
CObjectiveManager *CObjectiveManager::s_pInstance = NULL;

/**
* Handles the instance
*
* @return CObjectiveManager *
**/
CObjectiveManager *CObjectiveManager::GetInstance(void)
{
	// if we don't have game rules we're fucked
	if(!g_pGameRules)
		return NULL;

	// do we have the instance yet?
	if(!s_pInstance)
		s_pInstance = ((CSDKGameRules *)g_pGameRules)->GetNewObjectiveManager();
		
	return s_pInstance;
}

/**
* Destorys the instance
*
* @return void
**/
void CObjectiveManager::Term(void)
{
	// has it been created yet?
	if(s_pInstance)
	{
		// kill the instance
		delete(s_pInstance);
		s_pInstance = NULL;
	}
}

/**
* Constructor
**/
CObjectiveManager::CObjectiveManager()
{
	// no current objective
	m_pCurrentObj = NULL;

	// no picker
	m_pObjPicker = NULL;
	m_pObjPickerTeam = NULL;

	// no objectives yet
	m_bObjectivesEstablished = false;
	m_iObjectiveCount = 0;

	// the objective is not occupied
	m_bObjectiveOccupied = false;

	// no proxy yet
	m_pProxy = NULL;
}

/**
* Destructor
**/
CObjectiveManager::~CObjectiveManager()
{
	// kill the proxy
	if(m_pProxy)
		UTIL_Remove(m_pProxy);

	// kill the vector
	m_aObjectives.Purge();

	// kill the values
	CObjectiveValues::Term();
}

/**
* Initializes the objectives
*
* @return void
**/
void CObjectiveManager::InitializeObjectives(void)
{
	// no count
	m_iObjectiveCount = 0;

	// create some space for the objectives
	m_aObjectives.SetSize(MAX_OBJECTIVES);

	// go through them all
	for(int i = 0; i < m_aObjectives.Count(); ++i)
		m_aObjectives[i] = NULL;
}

/**
* Finds the next objective of type FUNC_OBJECTIVE
*
* @param CObjective *pObj The objective to start at
* @return CObjective *
**/
CObjective *CObjectiveManager::FindObjective(CObjective *pObj)
{
	CBaseEntity *pEntity;

	// find the next one
	pEntity = gEntList.FindEntityByClassname(pObj, FUNC_OBJECTIVE);

	// did we get it?
	if(!pEntity)
		return NULL;

	// switch to an objective
	return dynamic_cast<CObjective *>(pEntity);
}

/**
* Finalizes each of the objectives
*
* @return void
**/
void CObjectiveManager::FinalizeObjectives(void)
{
	// reset everything
	for(int i = 0; i < m_iObjectiveCount; ++i)
		Finalize(m_aObjectives[i]);
}

/**
* Figures out which objectives exist
*
* @return void
**/
void CObjectiveManager::EstablishObjectives(void)
{
	CObjective *pObj;
	int iNumValues = 0;
	IGameEvent *pEvent;

	//spawn the proxy
	CreateEntityByName("objective_proxy");

	// initialize them
	InitializeObjectives();

	// start searching
	for(pObj = FindObjective(NULL); pObj; pObj = FindObjective(pObj))
	{
		// make sure it's valid
		if(!m_aObjectives.IsValidIndex(pObj->GetID()))
			continue;

		// reset it
		pObj->Reset();

		// increment the count if we don't have this already
		if(m_aObjectives[pObj->GetID()] != NULL)
		{
			Assert(0);
			DevMsg("You seem to have duplicate objectives in your map!");
		}
		else
			// increment the count
			++m_iObjectiveCount;

		// stick it in its spot
		m_aObjectives[pObj->GetID()] = pObj;

		// finalize it
		Finalize(pObj);

		// does this objective need values generated?
		if(pObj->RequiresValues())
			++iNumValues;
	}

	// make sure there are some objectives
	//Assert(m_iObjectiveCount != 0);
	if(m_iObjectiveCount == 0)
	{
		DevMsg("Add some objectives to your map!\n");
		return;
	}

	// generate the objective values
	CObjectiveValues::GetInstance()->GenerateValues(iNumValues);

	// assign values to each and update the objectives
	for(int i = 0; i < m_iObjectiveCount; ++i)
	{
		// does the objective need values? add them
		pObj = m_aObjectives[i];
		if(pObj->RequiresValues())
		{
			// get a value set for this objective
			SValues_t *pValues = CObjectiveValues::GetInstance()->GetValues();

			// set them
			pObj->SetCash(pValues->m_iCash);
			pObj->SetXP(pValues->m_iXP);
		}

		// update the client
		UpdateClientObjective(pObj);

		// send out the event
		pEvent = gameeventmanager->CreateEvent("objective_established");
		if(pEvent)
		{
			// set all the info
			pEvent->SetInt("id", pObj->GetID());
			pEvent->SetString("title", pObj->GetTitle());
			pEvent->SetInt("cash", pObj->GetCash());
			pEvent->SetInt("xp", pObj->GetXP());
			pEvent->SetInt("isbase", (short)(!pObj->IsRequiredToWin()));

			// send it out
			gameeventmanager->FireEvent(pEvent, true);
		}
	}

	// all set
	m_bObjectivesEstablished = true;
}

/**
* Sets the current objective
*
* @param CSDKPlayer *pPlayer The player trying to set the objective
* @param CObjective * The objective they want
* @return void
**/
void CObjectiveManager::SetCurrentObjective(CSDKPlayer *pPlayer, int iObj, bool bForce /* = false */)
{
	IGameEvent *pEvent;

	// has the objective been selected?
	if(m_pCurrentObj)
		return;

	// this allows us to get around time and picker constraints
	if (!bForce) {
		// is this the correct player?
		if(pPlayer != m_pObjPicker)
			return;

		// don't do anything if we're in an intermission
		if (((CSDKGameRules*)g_pGameRules)->GetCurrentRound()->InIntermission())
			return;

		// is our player still on the same team?
		if(pPlayer && pPlayer->GetTeam() != m_pObjPickerTeam) {
			// pick a new objective picker
			PickObjective(m_pObjPickerTeam->GetTeamNumber());
			return;
		}
	}

	// make sure we have a valid index
	if(!m_aObjectives.IsValidIndex(iObj)) {
		SendChooseObjective(m_pObjPicker);	// choose again
		return;
	}

	// make sure it exists
	if(!m_aObjectives[iObj]) {
		SendChooseObjective(m_pObjPicker);	// choose again
		return;
	}

	// is this objective currently capturable
	if (!m_aObjectives[iObj]->IsCapturableByTeam(m_pObjPickerTeam)) {
		SendChooseObjective(m_pObjPicker);	// choose again
		return;
	}

	// set it
	m_pCurrentObj = m_aObjectives[iObj];

	// reset then activate objective
	m_pCurrentObj->RoundReset();
	m_pCurrentObj->ActivateObjective();

	// tell everyone on the picker's team
	UpdateClientObjective(m_pCurrentObj, m_pObjPickerTeam, false);

	// send out the event
	pEvent = gameeventmanager->CreateEvent("objective_selected");
	if(pEvent)
	{
		// set all the info
		pEvent->SetInt("id", m_pCurrentObj->GetID());
		pEvent->SetInt("offteam", m_pObjPickerTeam->GetTeamNumber());
		pEvent->SetInt("userid", m_pObjPicker ? m_pObjPicker->GetUserID() : 0);

		// send it out
		gameeventmanager->FireEvent(pEvent, true);
	}

	// choose the strategy now
	if (m_pObjPicker)
		GetStrategyManager()->SendChooseStrategy(m_pObjPicker->GetTeamNumber());

	return;
}

/**
* Finds a player to pick the objective and signals the screen to be displayed
*
* @param int iTeam The team that should pick the objective
* @return void
**/
void CObjectiveManager::PickObjective(int iTeam)
{
	CSDKTeam *pTeam;

	// grab the offensive team
	pTeam = dynamic_cast<CSDKTeam *>(GetGlobalTeam(iTeam));
	if(!pTeam)
		return;

	m_pObjPickerTeam = pTeam;

	// pick a player
	m_pObjPicker = pTeam->GetObjectivePicker();
	if(!m_pObjPicker)
		return;

	GetStrategyManager()->SetStrategyPicker(m_pObjPicker);

	SendChooseObjective(m_pObjPicker);
}

/**
 * Sends the message to show the choose objective menu
 * @param CSDKPlayer *pPlayer The player to send it to
 * @return void
 */
void CObjectiveManager::SendChooseObjective(CSDKPlayer *pPlayer)
{
	float fFreezeTime;

	// check that we have everythign
	if(!g_pGameRules || !((CSDKGameRules *)g_pGameRules)->GetCurrentRound() || !pPlayer)
		return;

	// get the freeze time
	if(pPlayer->GetTeam() && pPlayer->GetTeam()->IsOffensive())
		fFreezeTime = ((CSDKGameRules *)g_pGameRules)->GetCurrentRound()->GetRemainingOffensiveFreezeTime();
	else
		fFreezeTime = ((CSDKGameRules *)g_pGameRules)->GetCurrentRound()->GetRemainingDefensiveFreezeTime();

	// show it to the player
	CSingleUserRecipientFilter filter(pPlayer);
	filter.MakeReliable();
	UserMessageBegin(filter, "ChooseObjective");
		WRITE_FLOAT(fFreezeTime);
	MessageEnd();
}

/** 
 * Called by a holdare objective when it is entered.
 * @param const CObjectiveHoldarea *pObj The objective that was entered
 * @return void
 */
void CObjectiveManager::ObjectiveEntered(CObjectiveHoldarea *pObj)
{
	// make sure its the current objective
	if(pObj != m_pCurrentObj)
		return;

	m_bObjectiveOccupied = true;

	// Tell the timer to start
	UpdateTimerNotification(pObj->GetCaptureTime(), pObj->GetRemainingCaptureTime(), TIMER_STATE_COUNTDOWN);
}

/** 
 * Called by a holdare objective when it is left.
 * @param const CObjectiveHoldarea *pObj The objective that was left
 * @return void
 */
void CObjectiveManager::ObjectiveLeft(CObjectiveHoldarea *pObj)
{
	// make sure its the current objective
	if (pObj != m_pCurrentObj)
		return;

	// Tell the timer to stop
	UpdateTimerNotification(pObj->GetCaptureTime(), pObj->GetRemainingCaptureTime(), TIMER_STATE_FROZEN, pObj->GetCaptureLeeway());
}

void CObjectiveManager::ObjectiveLeewayExpired(CObjectiveHoldarea *pObj)
{
	int iDefensive = 0;

	// don't care unless it's the current objective
	if(pObj != m_pCurrentObj)
		return;

	DisableTimerNotification();

	m_bObjectiveOccupied = false;

	// don't tell them if we don't have game rules and a round...
	if(!g_pGameRules || !((CSDKGameRules *)g_pGameRules)->GetCurrentRound())
		return;

	// don't tell them if the round is over
	if (((CSDKGameRules *)g_pGameRules)->GetCurrentRound()->IsRoundOver())
		return;

	// figure out the defensive team
	for(int i = 1; i <= MAX_TEAMS; ++i)
	{
		// pull the team
		CSDKTeam *pTeam = GetGlobalSDKTeam(i);
		if(pTeam && !pTeam->IsOffensive())
		{
			iDefensive = i;
			break;
		}
	}

	// couldn't find it? bail
	if(!iDefensive)
		return;

	// let the defensive team know
	CTeamRecipientFilter filter(iDefensive);
	filter.MakeReliable();

	// send the message
	UserMessageBegin(filter, "HudDisplayMsg");

	// send the leave message
	WRITE_STRING("ObjectiveAttackDefenseLeave");
	WriteObjectiveCaptureInfo(m_pCurrentObj);

	// no sublabels
	WRITE_BYTE(0);
	WRITE_BYTE(0);

	// let it go
	MessageEnd();
}

/**
 * Disables the timers on all clients.
 * @return void
 */
void CObjectiveManager::DisableTimerNotification()
{
	UpdateTimerNotification(0.0, 0.0, TIMER_STATE_DEACTIVATE);
}

/**
 * Sends an event to all client to update the timer.
 * @param float fTime The time to set the timer to.
 * @return void
 */
void CObjectiveManager::UpdateTimerNotification(float fDuration, float fTime, OBJECTIVE_TIMER_STATE iState, float fLeewayTime /* = 0 */)
{
	char szStr[256];
	CObjectiveHoldarea *pObj;

	// bail if we don't have an objective
	//pObj = (CObjectiveHoldarea *)m_pCurrentObj;
	//if(!m_pCurrentObj || !pObj)
	//	return;

	// create the new event
	IGameEvent *event = gameeventmanager->CreateEvent("objective_notification");

	if(event)
	{
		// set the data
		event->SetInt("id", m_pCurrentObj ? m_pCurrentObj->GetID() : -1);
		event->SetInt("state", (int)iState);
		event->SetInt("owner", m_pCurrentObj ? m_pCurrentObj->GetOwnerID() : -1);
		event->SetInt("offteam", (m_pObjPickerTeam ? m_pObjPickerTeam->GetTeamNumber() : -1));
		event->SetFloat("duration", fDuration);
		event->SetFloat("timer", fTime);
		event->SetFloat("leewaytimer", fLeewayTime);
		event->SetInt("captured", m_pCurrentObj ? (int)m_pCurrentObj->IsPseudoCaptured() : false);

		// it's not pseudo captured anymore
		if(m_pCurrentObj)
			m_pCurrentObj->SetPseudoCaptured(false);

		// fire the event
		gameeventmanager->FireEvent(event);
	}

	// now display our hud message
	if(m_pCurrentObj && iState != TIMER_STATE_DEACTIVATE)
	{
		// grab the holdarea
		pObj = (CObjectiveHoldarea *)m_pCurrentObj;

		// send something to both teams
		for(int i = 1; i <= MAX_TEAMS; ++i)
		{
			// pull the team
			CSDKTeam *pTeam = GetGlobalSDKTeam(i);
			if(pTeam && (pTeam->IsOffensive() || (iState != TIMER_STATE_FROZEN && pObj->GetCaptureTime() == pObj->GetRemainingCaptureTime())))
			{
				// only send to this team
				CTeamRecipientFilter filter(i);
				filter.MakeReliable();

				// send the message
				UserMessageBegin(filter, "HudDisplayMsg");

				// offense or defense?
				if(pTeam->IsOffensive())
				{
					// send the offense message
					Q_snprintf(szStr, sizeof(szStr), "ObjectiveAttackOffense%s", fLeewayTime > 0 ? "Leave" : "");
					WRITE_STRING(szStr);

					// write the base time if we're not in leeway time
					//if(fLeewayTime <= 0)
					//	WRITE_SHORT(fTime);

					// write the objective info
					if(fLeewayTime > 0)
						WriteObjectiveCaptureInfo(m_pCurrentObj);
					else
						WriteObjectiveCaptureInfo(m_pCurrentObj, fTime);
				}
				else if(iState != TIMER_STATE_FROZEN)
				{
					// send the defense message
					WRITE_STRING("ObjectiveAttackDefense");

					// send the objective info
					WriteObjectiveCaptureInfo(m_pCurrentObj, fDuration);
				}

				// no sublabels
				WRITE_BYTE(0);
				WRITE_BYTE(0);

				// let it go
				MessageEnd();
			}
		}
	}
}

/**
* Writes the info regarding an objective being captured
*
* @param CObjective *pObj The objective being captured
* @param float fDuration The duration to write.  -1 will not be displayed
* @return void
**/
void CObjectiveManager::WriteObjectiveCaptureInfo(CObjective *pObj, float fDuration/* = -1*/)
{
	char szStr[256];

	// write the number of replacements
	if(fDuration > 0)
		WRITE_SHORT(3);
	else
		WRITE_SHORT(2);

	// send the name if we're not a base
	if(m_pCurrentObj->IsRequiredToWin())
		WRITE_STRING(m_pCurrentObj->GetTitle());

	// next replacement is the number
	switch(m_pCurrentObj->GetID())
	{
		case 0:
			WRITE_STRING(" (I)");
			break;
		case 1:
			WRITE_STRING(" (II)");
			break;
		case 2:
			WRITE_STRING(" (III)");
			break;
		default:
			// only bases should get here
			WRITE_STRING("#The");
			break;
	}

	// if we're a base we need to send the name second
	if(!m_pCurrentObj->IsRequiredToWin())
		WRITE_STRING(m_pCurrentObj->GetTitle());

	// send out the duration
	if(fDuration > 0)
	{
		// last replacement is the amount of time
		Q_snprintf(szStr, sizeof(szStr), "%d", (int)fDuration);
		WRITE_STRING(szStr);
	}
}

/**
 * Called from CRound when the round has ended.
 * @return void
 */
void CObjectiveManager::RoundEnded(void)
{
	// deactivate the objective
	if (m_pCurrentObj)
	{
		// tell the client
		m_pCurrentObj->DeactivateObjective();
		UpdateClientObjective(m_pCurrentObj);
	}

	// deactivate any timer
	DisableTimerNotification();
}

/**
* Resets the manager for a new game
*
* @return void
**/
void CObjectiveManager::Reset(void)
{
	// terminate the instance
	CObjectiveManager::Term();
}

/**
* Resets the manager for a new round
*
* @return void
**/
void CObjectiveManager::RoundReset(void)
{
	// reset everything
	for(int i = 0; i < m_iObjectiveCount; ++i)
	{
		m_aObjectives[i]->RoundReset();

		// @TRJ - trying to fix the dual objective selection thing
		UpdateClientObjective(m_aObjectives[i]);
	}

	// if we have an objective, clear it out
	m_pCurrentObj = NULL;

	m_bObjectiveOccupied = false;
}

void CObjectiveManager::ResetAllObjectives(void)
{
	// reset everything
	for(int i = 0; i < m_iObjectiveCount; ++i)
		m_aObjectives[i]->Reset();

	// if we have an objective, clear it out
	m_pCurrentObj = NULL;

	m_bObjectiveOccupied = false;
}

/**
* Sends the updated objective information to everyone
*
* @param CObjective *pObj The objective to tell clients about
* @return void
**/
void CObjectiveManager::UpdateClientObjective(CObjective *pObj)
{
	// create the filter
	CBroadcastRecipientFilter filter;
	filter.MakeReliable();

	// go!
	UpdateClientObjective(pObj, &filter, pObj->GetSafeState());
}

/**
* Sends the updated objective information to the player
*
* @param CObjective *pObj The objective to tell clients about
* @param CSDKPlayer *pPlayer The player to send the message to
* @param bool bSendSafe Sends a version of the message that is safe for all players
* @return void
**/
void CObjectiveManager::UpdateClientObjective(CObjective *pObj, CSDKPlayer *pPlayer, bool bSendSafe)
{
	// did we get a real player?
	if(!pPlayer)
		return;

	// create the filter
	CSingleUserRecipientFilter filter(pPlayer);
	filter.MakeReliable();

	// go!
	UpdateClientObjective(pObj, 
						&filter,
						(!bSendSafe ? pObj->GetState() : pObj->GetSafeState()));
}

/**
* Sends the updated objective information to the team
*
* @param CObjective *pObj The objective to tell clients about
* @param CSDKTeam *pTeam The team to send the data to
* @return void
**/
void CObjectiveManager::UpdateClientObjective(CObjective *pObj, CSDKTeam *pTeam, bool bSendSafe)
{
	// did we get a real team?
	if(!pTeam)
		return;

	// create the filter
	CTeamRecipientFilter filter(pTeam->GetTeamNumber(), true);

	// go!
	UpdateClientObjective(pObj, 
						&filter, 
						(!bSendSafe
							? pObj->GetState() 
							: pObj->GetSafeState()));
}

/**
* Sends out the objective data using the specified filter
*
* @param CObjective *pObj The objective to tell clients about
* @param CRecipientFilter *pFilter The filter to use while sending
* @param int iActive The value to send for the active short
* @return void
**/
void CObjectiveManager::UpdateClientObjective(CObjective *pObj, CRecipientFilter *pFilter, ObjectiveState eState)
{
	// send it out
	UserMessageBegin(*pFilter, "ObjectiveStatus");
		WRITE_SHORT(pObj->GetID());
		WRITE_SHORT(pObj->GetOwnerID());
		WRITE_SHORT(pObj->GetXP());
		WRITE_SHORT(pObj->GetCash());
		WRITE_BYTE(eState == ACTIVE);
		WRITE_BYTE((dynamic_cast<CObjectiveHoldareaBase *>(pObj)) != NULL);
		WRITE_VEC3COORD(pObj->GetAbsOrigin());
		WRITE_STRING(pObj->GetTitle());
	MessageEnd();
}

/**
* The objective was captured
*
* @param CObjective *pObj The objective that was captured
* @return void
**/
void CObjectiveManager::SetObjectiveCaptured(CObjective *pObj)
{
	// tell everyone
	UpdateClientObjective(pObj);
}

/**
* Tells the player about all the objectives
*
* @param CSDKPlayer *pPlayer The player to tell
* @return void
**/
void CObjectiveManager::UpdatePlayer(CSDKPlayer *pPlayer)
{
	int iOffensiveTeam;

	// find the offensive team
	iOffensiveTeam = (GetGlobalTeam(TEAM_A)->IsOffensive() ? TEAM_A : TEAM_B);

	// run through the objectives
	for(int i = 0; i < m_iObjectiveCount; ++i)
	{
		// tell the player
		UpdateClientObjective(m_aObjectives[i], pPlayer, pPlayer->GetTeamNumber() != iOffensiveTeam);
	}
}

/**
* Determines if all the objectives have been captured
*
* @return bool
**/
bool CObjectiveManager::AllCaptured(void)
{
	CSDKTeam *pTeam = NULL;

	// do we have the objectives yet?
	if(!AreObjectivesEstablished())
		return false;

	// do we have any?
	if(!m_iObjectiveCount)
		return false;

	// go through all of them
	for(int i = 0; i < m_iObjectiveCount; ++i)
	{
		// is it required?
		if(!m_aObjectives[i]->IsRequiredToWin())
			continue;

		// is it captured?
		if (!pTeam)
			pTeam = m_aObjectives[i]->GetOwner();
		
		if (!m_aObjectives[i]->GetOwner() || m_aObjectives[i]->GetOwner() != pTeam)
			return false;
	}

	return true;
}

void CObjectiveManager::GetObjectiveCounts(int iCounts[MAX_TEAMS+1])
{
	iCounts[TEAM_A] = 0;
	iCounts[TEAM_B] = 0;   

	for(int i = 0; i < m_iObjectiveCount; ++i)
	{
		// is it required?
		if(!m_aObjectives[i]->IsRequiredToWin())
			continue;

		// is it captured?
		if (m_aObjectives[i]->GetOwner()) {
			// 1 for them
			iCounts[m_aObjectives[i]->GetOwner()->GetTeamNumber()]++;
		}
	}
}

/**
* Forces the objective selection by picking for the player
*
* @return void
**/
void CObjectiveManager::ForceObjectiveSelection(void)
{
	int i, iStart;

	// if we have an objective already, bail
	// also, if we don't have any objectives abort.  this happens in the case that
	// no one has joined. not sure why. this should fix it for the time being
	if(m_pCurrentObj || m_iObjectiveCount == 0)
		return;

	// start at a random objective and traverse
	i = RandomInt(0, m_iObjectiveCount);
	iStart = -1;

	// if we don't have a picker, don't bother
	// JD: pick anyway
	//if(!m_pObjPicker)
	//	return;

	// make sure we have a real index
	for(; i != iStart; ++i)
	{
		// set the start
		if(iStart == -1)
			iStart = i;

		// are we over?
		if(i >= m_iObjectiveCount)
			i = 0;

		// @TRJ - we are allowed to have a null picker
		// make sure we have a real index, real objective, we don't own it
		// and that it is required to win
		if(!m_aObjectives.IsValidIndex(i) || !m_aObjectives[i] || /*!m_pObjPicker || */
			m_aObjectives[i]->GetOwner() == m_pObjPickerTeam ||
			!m_aObjectives[i]->IsRequiredToWin())
			continue;

		// set the objective with force
		SetCurrentObjective(m_pObjPicker, i, true);
		return;
	}
}

// this returns which objectives the defensive team should defend
// the implementation is lame, but fast.
const char* CObjectiveManager::GetDefendableObjectives(int iTeam)
{
	int objectiveBits = 0;
	for(int i = 0; i < m_iObjectiveCount; ++i) {
		if (!m_aObjectives[i])
			continue;
		if (!m_aObjectives[i]->IsRequiredToWin())
			continue;
		// you cannot defend what you do not own
		if (m_aObjectives[i]->GetOwner() && m_aObjectives[i]->GetOwner()->GetTeamNumber() != iTeam)
			continue;

		objectiveBits |= (1 << i);
	}

	switch(objectiveBits) {
		case 0:
			return "";
		case 1:
			return "(I)";
		case 2:
			return "(II)";
		case 3:
			return "(I,II)";
		case 4:
			return "(III)";
		case 5:
			return "(I,III)";
		case 6:
			return "(II,III)";
		case 7:
			return "(I,II,III)";
		default:
			return "";
	}
}

/**
* Determines the action to take when a player leaves our game
*
* @param CBasePlayer *pPlayer The player that left
* @return void
**/
void CObjectiveManager::PlayerLeft(CBasePlayer *pPlayer)
{
	// is it our picker?
	if(pPlayer == m_pObjPicker)
		m_pObjPicker = NULL;
}

/**
* Specifies how long it will take to capture the given holdarea
*
* @param const CObjectiveHoldarea *pObj The holdarea to get the capture time for
* @return float
**/
float CObjectiveManager::GetCaptureTimeForObjective(const CObjectiveHoldarea *pObj) const
{
	return mp_holdareatime.GetFloat();
}

/**
* Sets our proxy
*
* @param CObjectiveProxy *pProxy The proxy to use
* @param CObjectiveProxy *pOldProxy The proxy that was in use before
* @return void
**/
void CObjectiveManager::SetProxy(CObjectiveProxy *pProxy, CObjectiveProxy *pOldProxy/* = NULL*/)
{
	// if we have and old proxy and it's not the same as the current one, don't overwrite
	if(pOldProxy && pOldProxy != m_pProxy)
		return;

	// set it
	m_pProxy = pProxy;
}

/**
* Sets the number of players in the zone
*
* @param int iPlayers
* @return void
**/
void CObjectiveManager::SetPlayersInZone(int iPlayers)
{
	// see if we have the proxy
	if(m_pProxy)
		m_pProxy->SetPlayersInZone(iPlayers);
}

/**
* Determines if the given objective is the last objective that needs
* to be captured by either team.  Does not include bases!
*
* @param CObjective *pObj The objective to check aginast
* @return bool
**/
bool CObjectiveManager::IsLastObjective(CObjective *pObj)
{
	int iTeamA, iTeamB;

	// in order for this to be the last objective one of the teams that doesn't own it
	// must own three other objectives

	// count the objectives owned by each team
	iTeamA = iTeamB = 0;
	for(int i = 0; i < m_aObjectives.Count(); ++i)
	{
		// mkae sure we have a real one
		if(!m_aObjectives[i])
			continue;

		// who gets it?
		if(m_aObjectives[i]->GetOwnerID() == TEAM_A)
			++iTeamA;
		else if(m_aObjectives[i]->GetOwnerID() == TEAM_B)
			++iTeamB;
	}

	// if either of them own three check that the objective is not owned by them
	if(iTeamA == 3)
		return pObj->GetOwnerID() != TEAM_A;
	else if(iTeamB == 3)
		return pObj->GetOwnerID() != TEAM_B;

	// otherwise, neither team owns enough
	return false;
}

/*****************************************************************************************/
/** CObjectiveValues *********************************************************************/
/*****************************************************************************************/

// Forward declarations
CObjectiveValues *CObjectiveValues::s_pInstance = NULL;
int CObjectiveValues::s_aCashValues[3] = {0, 1000, 2500};
int CObjectiveValues::s_aXPValues[3] = {25, 8, 0};

/**
* Handles the singleton for the class
*
* @return CObjectiveValues *
**/
CObjectiveValues *CObjectiveValues::GetInstance(void)
{
	// do we have one yet?
	if(!s_pInstance)
		s_pInstance = new CObjectiveValues();

	return s_pInstance;
}

/**
* Terminates the singleton
*
* @return void
**/
void CObjectiveValues::Term(void)
{
	// has it been created yet?
	if(s_pInstance)
	{
		// kill the instance
		delete(s_pInstance);
		s_pInstance = NULL;
	}
}

/**
* Constructor
**/
CObjectiveValues::CObjectiveValues()
{
	// set the vector size to zero
	m_aFree.SetSize(0);
	m_aUsed.SetSize(0);
}

/**
* Destructor
**/
CObjectiveValues::~CObjectiveValues()
{
	// destroy everything
	m_aFree.PurgeAndDeleteElements();
	m_aUsed.PurgeAndDeleteElements();
}

/**
* Generates values for the number of objectives specified
* @TODO - get this to load from a file on a per map basis or from a default
* 
* @param int iNumValues The number of value combinations to generate
* @return void
**/
void CObjectiveValues::GenerateValues(int iNumValues)
{
	// destroy everything
	m_aFree.PurgeAndDeleteElements();
	m_aUsed.PurgeAndDeleteElements();

	// generate each of the value sets
	for(int i = 0; i < iNumValues; ++i)
	{
		// create some room
		SValues_t *pValues = new SValues_t;

		// add it on
		m_aFree.AddToTail(pValues);

		// pick one
		m_aFree[i]->m_iCash = s_aCashValues[i % 3];
		m_aFree[i]->m_iXP = s_aXPValues[i % 3];
	}

	// sort them randomly
	m_aFree.Sort(Random);
}

/**
* Get the next free values
*
* @return SValues_t *
**/
SValues_t *CObjectiveValues::GetValues(void)
{
	// are there any free ones?
	// this should be a solid crash...
	if(m_aFree.Count() == 0)
	{
		Assert(0);
		return NULL;
	}

	// otherwise, move the last one into used
	m_aUsed.AddToTail(m_aFree[m_aFree.Count() - 1]);
	m_aFree.Remove(m_aFree.Count() - 1);

	// send back the one we just moved
	return m_aUsed[m_aUsed.Count() - 1];

}

/**
* Generates a random value for 'sorting'
*
* @param SValues_t *pLeft
* @param SValues_t *pRight
* @return int
**/
int CObjectiveValues::Random(SValues_t * const *pLeft, SValues_t * const *pRight)
{
	return RandomInt(-1, 1);
}