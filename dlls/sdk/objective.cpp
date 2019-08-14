#include "cbase.h"
#include "objective.h"
#include "objectivemanager.h"
#include "round.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar mp_holdareatime;

#ifdef SUPPRESSION_ENABLED
	extern ConVar mp_suppression;
	extern ConVar mp_suppressionholdareatimebase;
#endif

/**
* Constructor
**/
CObjective::CObjective(void)
{
	// ?
}

/**
* Destructor
**/
CObjective::~CObjective()
{
	// destroy our list
	m_aBoostList.Purge();
}

/**
* Pulls the basic objective info from the key value pairs 
*
* @param const char *szKeyName Name of the variable
* @param const char *szValue Value of the variable
* @return bool True on success
*/
bool CObjective::KeyValue(const char *szKeyName, const char *szValue)
{
	// copy any data we need
	if (FStrEq(szKeyName, "Title"))
		Q_strncpy(m_szTitle, szValue, sizeof(m_szTitle));
	else if (FStrEq(szKeyName, "Position"))
		m_iID = atoi(szValue);
	else
		// jump down
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

/**
* Causes the objective to become capturable
*
* @return void
*/
void CObjective::ActivateObjective(void)
{
	// turn it on
	m_eState = ACTIVE;
}

/**
* Causes the objective to become uncapturable
*
* @return void
**/
void CObjective::DeactivateObjective(void)
{
	// turn it off
	//m_eState = (m_pOwner ? CAPTURED : NEUTRAL);
	m_eState = NEUTRAL;
}

/**
* Resets the objective for the entire game
*
* @return void
*/
void CObjective::Reset()
{
	// no owner
	m_pOwner = NULL;

	// deactivate
	DeactivateObjective();

	// kill the boost list
	m_aBoostList.Purge();
}

/**
* Resets for the round
* @NOTE - This should always happen after the rounds have switched
*
* @return void
**/
void CObjective::RoundReset(void)
{
	// not active
	DeactivateObjective();

	// set the offensive team so we don't have to keep doing this
	m_pOffensiveTeam = GetGlobalSDKTeam(TEAM_A)->IsOffensive() ? GetGlobalSDKTeam(TEAM_A) : GetGlobalSDKTeam(TEAM_B);
}

/**
* Sends back the id of the team that owns this objective
*
* @return int -1 if not owned
**/
int CObjective::GetOwnerID(void)
{
	// do we have an owner?
	if(m_pOwner)
		return m_pOwner->GetTeamNumber();

	return -1;
}

/**
* Sets the owner for the objective to the current offensive team
*
* @return void
**/
void CObjective::SetCaptured(void)
{
	// we're pseudo captured
	m_bPseudoCaptured = true;

	// set the owner
	m_pOwner = m_pOffensiveTeam;

	// captured
	m_eState = CAPTURED;

	// let the manager know
	GET_OBJ_MGR()->SetObjectiveCaptured(this);
}

/**
* Relays the boost amount to the player
* 
* @param CSDKPlayer *pPlayer The player to boost
* @param bool bShouldBoost Are we turning the boost on or off?
* @return void
**/
void CObjective::BoostPlayer(CSDKPlayer *pPlayer, bool bShouldBoost)
{
	// boost
	// conditions: 
	//		player must own the objective
	//		player must be on defense
	//		objective cannot be captured
	//		we are decrementing and we're in the boostlist
	if(pPlayer->GetTeam() == m_pOwner && m_pOwner != m_pOffensiveTeam && 
		m_eState != CAPTURED && (bShouldBoost || RemoveFromBoostList(pPlayer)))
	{
		// are we boosting?
		// if so, add them to the boost list
		// note that if are not boosting we will have removed them above
		if(bShouldBoost)
			AddToBoostList(pPlayer);

		// set the boost
		pPlayer->Boost((bShouldBoost ? 1 : -1) * GetBoostAmount(pPlayer));
	}
}

/**
* Adds a player to the list of entities that have boosted
* Keep in mind we need to have duplicates
*
* @param CSDKPlayer *pPlayer The player to add to the list
* @return void
**/
void CObjective::AddToBoostList(CSDKPlayer *pPlayer)
{
	m_aBoostList.AddToTail(pPlayer);
}

/**
* Removes a player from the boost list
*
* @param CSDKPlayer *pPlayer The player to remove
* @return void
**/
bool CObjective::RemoveFromBoostList(CSDKPlayer *pPlayer)
{
	// trying to keep this fast...
	// flip through our list (shouldn't be long) and fast remove
	// note that order is not preserved
	for(int i = 0; i < m_aBoostList.Count(); ++i)
	{
		// is this it?
		if(m_aBoostList[i] == pPlayer)
		{
			// remove and break out
			m_aBoostList.FastRemove(i);
			return true;
		}
	}

	return false;
}

/**
* Determines the amount of boost to give to the player
*
* @param CSDKPlayer *pPlayer The player to give the boost to
* @return int
**/
int CObjective::GetBoostAmount(CSDKPlayer *pPlayer)
{
	return HOLDAREA_BOOST_AMOUNT;
}

/**
* Determines if the class of this objective matches the string
*
* @param const char *szClassOrWildcard
* @return bool
**/
bool CObjective::ClassMatches(const char *szClassOrWildcard)
{
	// is this any type of func_objective?
	if(!strcmp(szClassOrWildcard, FUNC_OBJECTIVE))
		return true;

	return BaseClass::ClassMatches(szClassOrWildcard);
}

/**
* Determines if the class of this objective matches the string
*
* @param string_t szNameStr
* @return bool
**/
bool CObjective::ClassMatches(string_t szNameStr)
{
	// try the other one
	return ClassMatches(STRING(szNameStr));
}

/*************************************************************************************/
/** Start CObjectiveZone *************************************************************/
/*************************************************************************************/

/**
* Called when the player spawns
*
* @return void
**/
void CObjectiveZone::Spawn(void)
{
	// jump down
	BaseClass::Spawn();

	// turns on the trigger. apparently needed.
	BaseClass::InitTrigger();
}

/**
* Called when an entity leaves the objective area
*
* @param CBaseEntity *pPlayer The player touching the area
* @return void
*/
void CObjectiveZone::EndTouch(CBaseEntity *pEntity)
{	
	// make sure we got a player
	CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer*>(pEntity);
	if(!pPlayer)
		return;

	// turn off any boost the player may have
	BoostPlayer(pPlayer, false);

	// @TRJ - i think this was causing problems with suppression
	//			feel free to uncomment if anything breaks (but please tell me!)
	// bail if we're not active
	//if(m_eState != ACTIVE)
	//	return;

	// decrement the count of the players in the area
	int index;
	if((index = m_aOffensiveList.Find(pPlayer)) >= 0) {
		m_aOffensiveList.Remove(index);

		if (!PlayersInZone())
			LastPlayerLeftZone(pPlayer);
	}
}

/**
* Called when an entity enters the trigger area
*
* @param CBaseEntity *pOther The entity entering the objective.
* @return void
**/
void CObjectiveZone::StartTouch(CBaseEntity *pEntity)
{
	// flip to an sdk player
	CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer *>(pEntity);

	// did we get them?
	if(!pPlayer)
		return;

	// give the player a bump
	BoostPlayer(pPlayer, true);

	// is the objective active and our current objective?
	if(m_eState != ACTIVE)
		return;

	// if this person is on the offensive team
	if(pPlayer->GetTeam() == m_pOffensiveTeam) {
		// increment the count before calling first player
		if (!PlayersInZone()) {
			m_aOffensiveList.AddToTail(pPlayer);
			// start the timer if it hasn't been started already
			FirstPlayerEnteredZone(pPlayer);
		}
		else
			m_aOffensiveList.AddToTail(pPlayer);
	}
}

CSDKPlayer* CObjectiveZone::GetPlayerInZone(int index) const
{
	if (!m_aOffensiveList.IsValidIndex(index))
		return NULL;

	return m_aOffensiveList[index];
}

int CObjectiveZone::NumPlayersInZone(void) const
{
	return m_aOffensiveList.Count();
}

/**
* Determines the number of players in the zone
* @NOTE - This exists to handle weird cases.  m_iPlayersInZone is not set in stone but it is simple
*
* @return bool
**/
bool CObjectiveZone::PlayersInZone(void) const
{
	return m_aOffensiveList.Count() > 0;
}

/**
 * Called when the round restarts
 * @return void
 */
void CObjectiveZone::RoundReset(void)
{
	m_aOffensiveList.Purge();
    
	BaseClass::RoundReset();
}

/**
* Pretends that the first person in our queue was the first person to enter our objective
*	And that they're doing so right now
*
* @return void
**/
void CObjectiveZone::FakeFirstEntry(void)
{
	// pull the first person from our list
	if(!PlayersInZone())
	{
		Assert(0);
		return;
	}

	// just pretend the first guy did it
	FirstPlayerEnteredZone(m_aOffensiveList[0]);
}

/*************************************************************************************/
/** Start CObjectiveHoldarea *********************************************************/
/*************************************************************************************/

LINK_ENTITY_TO_CLASS( func_objective_holdarea, CObjectiveHoldarea );

// global Savedata for base trigger
BEGIN_DATADESC( CObjectiveHoldarea )
	// keyfields
	DEFINE_KEYFIELD( m_iFilterName,	FIELD_STRING, "filtername" ),
	DEFINE_FIELD( m_hFilter, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_THINKFUNC(LeewayThink),
END_DATADESC()

/**
* Determines whether or not the objective has been captured 
*
* @return bool
*/
bool CObjectiveHoldarea::IsCaptured()
{
	// if there are players in the zone and the objective is active and we're the current objective
	if(GET_OBJ_MGR()->GetCurrentObjective() == this && PlayersInZone() && m_eState == ACTIVE)
	{
		// have we been in there long enough?
		if(GetRemainingCaptureTime() < 0)
			return true;
	}

	return false;
}

/**
 * Is this objective CURRENTLY capturable (i.e. selectable) by a team
 * @param CSDKTeam *pTeam the team we want to check against
 * @return bool
 */
bool CObjectiveHoldarea::IsCapturableByTeam(CSDKTeam *pTeam)
{
	// make sure we don't own it and gamerules says it's ok
	return ((CSDKGameRules *)g_pGameRules)->CanTeamCaptureObjective(pTeam, this) && GetOwner() != pTeam;
}

/**
* Determines if the objective is relevant to the team
*
* @param CSDKTeam *pTeam The team to check against
* @return bool
**/
bool CObjectiveHoldarea::IsRelevantToTeam(CSDKTeam *pTeam)
{
	return ((CSDKGameRules *)g_pGameRules)->CanTeamCaptureObjective(pTeam, this) || pTeam->GetTeamNumber() == TEAM_A;
}

float CObjectiveHoldarea::GetCaptureTime() const
{
	// ask the objective manager
	return GET_OBJ_MGR()->GetCaptureTimeForObjective(this);

	/*float fTime;

	// suppression or regular?
	if(mp_suppression.GetInt() > 0)
		fTime = mp_suppressionholdareatime.GetFloat();
	else
		fTime = mp_holdareatime.GetFloat();

	// check the range
	if (fTime < 1 || fTime > 120)
		return atof(CAPTURE_TIME_LENGTH);

	return fTime;*/
}

float CObjectiveHoldarea::GetRemainingCaptureTime() const
{
	return m_fSecondsRemaining;
	/*if (PlayersInZone())
		// players are in the zone: base it on the current time
		return GetCaptureTime() - gpGlobals->curtime + m_fCaptureStartTime;
	else
		// players have left: base it on when they left
		return GetCaptureTime() - m_fCaptureLeftTime + m_fCaptureStartTime;*/
}

/**
* Allows the objective to think
*
* @return void
**/
void CObjectiveHoldarea::Think(void)
{
	// jump down
	BaseClass::Think();

	// think every tenth
	SetNextThink(gpGlobals->curtime + 0.1f);

	// if we're active we need to subtract off the seconds since our last think
	if(m_eState == ACTIVE && NumPlayersInZone() > 0)
	{
		// figure out the new seconds remaining and tell the proxy
		m_fSecondsRemaining -= (gpGlobals->curtime - m_fLastThink) * (0.9f + (clamp((float)NumPlayersInZone(), 1.0f, 3.0f) * 0.1f));
		GET_OBJ_MGR()->SetPlayersInZone(NumPlayersInZone());
	}

	// set the last think
	m_fLastThink = gpGlobals->curtime;
}

/**
* Called when the first player enters the objective zone
* @NOTE - This will get called every time the entire team enters and leaves
*
* @param CSDKPlayer *pPlayer The player who entered
* @return void
**/
void CObjectiveHoldarea::FirstPlayerEnteredZone(CSDKPlayer *pPlayer)
{
	// set the capture time
	if(gpGlobals->curtime - m_fCaptureLeftTime > GetCaptureLeeway())
		m_fSecondsRemaining = GetCaptureTime();
	/*if (gpGlobals->curtime - m_fCaptureLeftTime < GetCaptureLeeway())
		m_fCaptureStartTime += gpGlobals->curtime - m_fCaptureLeftTime;
	else
		m_fCaptureStartTime = gpGlobals->curtime;*/

	m_fCaptureLeftTime = 0;

	// tell the objective manager
	GET_OBJ_MGR()->ObjectiveEntered(this);

	// set the think
	m_fLastThink = gpGlobals->curtime;
	SetNextThink(gpGlobals->curtime + 0.1f);
}

float CObjectiveHoldarea::GetCaptureLeeway() const
{
	float fTime = mp_captureleeway.GetFloat();
	if (fTime < 0 || fTime > 10)
		return atof(CAPTURE_LEEWAY_TIME_LENGTH);

	return fTime;
}

/**
* Called when the last player leaves the objective zone
*
* @param CSDKPlayer *pPlayer The player who left
* @return void
**/
void CObjectiveHoldarea::LastPlayerLeftZone(CSDKPlayer *pPlayer)
{
	m_fCaptureLeftTime = gpGlobals->curtime;

	// think when our leeway time is up
	//ThinkSet((void (CBaseEntity::*)(void))LeewayThink);
	SetThink(&CObjectiveHoldarea::LeewayThink);
	SetNextThink(gpGlobals->curtime + GetCaptureLeeway());

	// tell the objective manager
	GET_OBJ_MGR()->ObjectiveLeft(this);
}

/**
* Resets for the round
*
* @return void
**/
void CObjectiveHoldarea::RoundReset()
{
	//m_fCaptureStartTime = 0;
	m_fCaptureLeftTime = 0;
	m_fSecondsRemaining = GetCaptureTime();

	BaseClass::RoundReset();
}

/**
* Thinks during leeway time
*
* @return void
**/
void CObjectiveHoldarea::LeewayThink()
{
	if (m_fCaptureLeftTime != 0) {
		GET_OBJ_MGR()->ObjectiveLeewayExpired(this);
		m_fCaptureLeftTime = 0;

		// set the seconds
		m_fSecondsRemaining = GetCaptureTime();

		// move into the regular think
		m_fLastThink = gpGlobals->curtime;
	}
}

/**
* Resets the objective
* 
* @return void
**/
void CObjectiveHoldarea::Reset(void)
{
	BaseClass::Reset();

	//m_fCaptureStartTime = 0;
	m_fCaptureLeftTime = 0;
	m_fSecondsRemaining = GetCaptureTime();
}

/**
* Resets the capture times
*
* @return void
**/
void CObjectiveHoldarea::ResetTimes(void)
{
	// back at the beginning
	//m_fCaptureStartTime = 0;
	m_fCaptureLeftTime = 0;
	m_fSecondsRemaining = GetCaptureTime();
}

/*************************************************************************************/
/** Start CObjectiveHoldareaBase *****************************************************/
/*************************************************************************************/

LINK_ENTITY_TO_CLASS( func_objective_holdarea_base, CObjectiveHoldareaBase );

/**
* Pulls the basic objective info from the key value pairs 
*
* @param const char *szKeyName Name of the variable
* @param const char *szValue Value of the variable
* @return bool True on success
*/
bool CObjectiveHoldareaBase::KeyValue(const char *szKeyName, const char *szValue)
{
	// what key did we get?
	if  (FStrEq(szKeyName, "TeamOwner"))
	{
		// pull the team
		CSDKTeam *pTeam = dynamic_cast<CSDKTeam *>(GetGlobalTeam(atoi(szValue)));
		if (pTeam)
		{
			// set the owner to the team
			m_pOwner = pTeam;

			// make the title up based on the team; uses localization
			Q_strncpy(m_szTitle, pTeam->GetName(), sizeof(m_szTitle));
			strncat(m_szTitle, "Base", sizeof(m_szTitle) - strlen(m_szTitle));
		}
	}
	else
		// load the base settings
		return BaseClass::KeyValue(szKeyName, szValue);

	return true;
}

/**
* Suppossed to set the owner for the objective to the current offensive team
* but because we're a base we don't let people take us over
*
* @return void
**/
void CObjectiveHoldareaBase::SetCaptured(void)
{
	// we're pseudo captured
	m_bPseudoCaptured = true;

	// neutral again
	m_eState = NEUTRAL;

	// let the manager know. kind of.
	GET_OBJ_MGR()->SetObjectiveCaptured(this);
}

/**
* Causes the objective to become uncapturable
*
* @return void
**/
void CObjectiveHoldareaBase::DeactivateObjective(void)
{
	// always neutral
	m_eState = NEUTRAL;
}

/**
 * Determines if a team could ever capture this objective.
 * @param CSDKTeam *pTeam The team that we want to check
 */
bool CObjectiveHoldareaBase::IsRelevantToTeam(CSDKTeam *pTeam)
{
	// i care as long as i don't own it
	return pTeam != m_pOwner && ((CSDKGameRules *)g_pGameRules)->IsObjectiveRelevantToTeam(pTeam, this);
}

/**
* Resets the objective
* 
* @return void
**/
void CObjectiveHoldareaBase::Reset(void)
{
	CSDKTeam *pOwner;

	// cache the owner so we don't lose it when we reset
	pOwner = m_pOwner;
	BaseClass::Reset();
	m_pOwner = pOwner;
}