#include "cbase.h"
#include "suppressionobjectivemanager.h"
#include "sdk_gamerulessuppression.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef SUPPRESSION_ENABLED

extern ConVar mp_suppressionholdareatimebase;

/**
* Constructor
**/
CSuppressionObjectiveManager::CSuppressionObjectiveManager()
	: CObjectiveManager()
{
	// ?
}

/**
* Finalizes teh objective after it has been established
*
* @param CObjective *pObj The objective to finalize
* @return void
**/
void CSuppressionObjectiveManager::Finalize(CObjective *pObj)
{
	// the coalition owns everything the americans can capture at the start
	if(pObj->IsCapturableByTeam(GetGlobalSDKTeam(TEAM_A)))
	{
		// set the owner to the coalition and make it active
		// round reset sets up the offensive team
		pObj->RoundReset();
		((CObjectiveHoldarea *)pObj)->ResetTimes();
		pObj->SetOwner(GetGlobalSDKTeam(TEAM_B));
		pObj->ActivateObjective();
	}
}

/**
* The objective has been entered by someone who can activate it
*
* @param CObjectiveHoldarea *pObj The objective that was entered
* @return void
**/
void CSuppressionObjectiveManager::ObjectiveEntered(CObjectiveHoldarea *pObj)
{
	// if we're doing suppression we only have a current objective for
	// as long as the objective is being captured.
	// if there's nothing else going on we can use whatever this one is
	if(m_pCurrentObj == NULL && pObj)
		m_pCurrentObj = pObj;

	// jump down
	BaseClass::ObjectiveEntered(pObj);
}

/**
* The other team completely left the objective for some reason
*
* @param CObjectiveHoldarea *pObj The objective that the team left
* @return void
**/
void CSuppressionObjectiveManager::ObjectiveLeewayExpired(CObjectiveHoldarea *pObj)
{
	CBroadcastRecipientFilter filter;

	// check that it's the current one
	if(pObj != m_pCurrentObj)
		return;

	// jump down
	BaseClass::ObjectiveLeewayExpired(pObj);

	// deactivate the current one.  note that we are cheating a bit by sending the state as
	// neutral.  the server still thinks its active so we can run into it at any time.
	// we just want the blinking to stop
	filter.MakeReliable();
	UpdateClientObjective(pObj, &filter, NEUTRAL);

	// reset the capture times
	pObj->ResetTimes();

	// we don't need our current objective anymore
	m_pCurrentObj = NULL;

	// see if there are any other occupied objectives
	CheckOccupiedObjectives();
}

/**
* Sets the fact that an objective has been captured
*
* @param CObjective *pObj The objective that was captured
* @return void
**/
void CSuppressionObjectiveManager::SetObjectiveCaptured(CObjective *pObj)
{
	// award the teams.  this should happen first
	((CSDKGameRules *)g_pGameRules)->GetCurrentRound()->AwardTeams(pObj->GetOwner());

	// jump down
	BaseClass::SetObjectiveCaptured(pObj);

	// deactivate it and reset the time
	m_pCurrentObj->DeactivateObjective();
	((CObjectiveHoldarea *)m_pCurrentObj)->ResetTimes();
	UpdateClientObjective(m_pCurrentObj);
	DisableTimerNotification();

	// no more current objective
	m_pCurrentObj = NULL;

	// update the team scores
	((CSDKGameRules *)g_pGameRules)->UpdateTeamScores();

	// check for additional objectives that are occupied
	CheckOccupiedObjectives();
}

/**
* Checks to see if any objectives are occupied already
*
* @return void
**/
void CSuppressionObjectiveManager::CheckOccupiedObjectives(void)
{
	// some other teammate may have been in an objective while we were capturing this one
	// see if any other objectives have player's in their lists
	for(int i = 0; i < m_aObjectives.Count(); ++i)
	{
		// make sure the americans care about it
		if(!m_aObjectives[i] || !m_aObjectives[i]->IsRelevantToTeam(GetGlobalSDKTeam(TEAM_A)) ||
			!m_aObjectives[i]->IsCapturableByTeam(GetGlobalSDKTeam(TEAM_A)))
			continue;

		// anyone in?
		if(((CObjectiveZone *)m_aObjectives[i])->NumPlayersInZone() > 0)
		{
			// pretend we just entered it
			((CObjectiveHoldarea *)m_aObjectives[i])->ResetTimes();
			((CObjectiveZone *)m_aObjectives[i])->FakeFirstEntry();
			break;
		}
	}
}

/**
* Sends out the objective data using the specified filter.  We are overriding the 
* base class method so that we always send the objective as neutral unless it 
* is the current objective
*
* @param CObjective *pObj The objective to tell clients about
* @param CRecipientFilter *pFilter The filter to use while sending
* @param int iActive The value to send for the active short
* @return void
**/
void CSuppressionObjectiveManager::UpdateClientObjective(CObjective *pObj, CRecipientFilter *pFilter, ObjectiveState eState)
{
	// send it out
	UserMessageBegin(*pFilter, "ObjectiveStatus");
		WRITE_SHORT(pObj->GetID());
		WRITE_SHORT(pObj->GetOwnerID());
		WRITE_SHORT(pObj->GetXP());
		WRITE_SHORT(pObj->GetCash());
		WRITE_BYTE(eState == ACTIVE && m_pCurrentObj == pObj);  // only active if we're the current objective
		WRITE_BYTE((dynamic_cast<CObjectiveHoldareaBase *>(pObj)) != NULL);
		WRITE_VEC3COORD(pObj->GetAbsOrigin());
		WRITE_STRING(pObj->GetTitle());
	MessageEnd();
}

/**
* Specifies how long it will take to capture the given holdarea
*
* @param const CObjectiveHoldarea *pObj The holdarea to get the capture time for
* @return float
**/
float CSuppressionObjectiveManager::GetCaptureTimeForObjective(const CObjectiveHoldarea *pObj) const
{
	int iCaptureCount;

	// figure out how many we've captured.  start at -1 because we always
	// own our base
	iCaptureCount = -1;
	for(int i = 0; i < m_aObjectives.Count(); ++i)
	{
		// skip the crap
		if(!m_aObjectives[i])
			continue;

		// if i own it incremnet the count
		if(m_aObjectives[i]->GetOwnerID() == TEAM_A)
			++iCaptureCount;
	}

	// knock off a third if we only have one left
	if(iCaptureCount == 2)
		return floor(mp_suppressionholdareatimebase.GetFloat() * (2.0f / 3.0f));

	return mp_suppressionholdareatimebase.GetFloat();
}

#endif