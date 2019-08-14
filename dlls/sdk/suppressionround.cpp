#include "cbase.h"
#include "suppressionround.h"
#include "sdk_gamerulessuppression.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef SUPPRESSION_ENABLED

/**
* Constructor
**/
CSuppressionRound::CSuppressionRound()
	: CRound()
{
	// initialize the spawn
	InitSpawn();

	// not spawning a wave
	m_bSpawningWave = false;

	// set the teams
	m_iOffensiveTeam = TEAM_A;
	m_iDefensiveTeam = TEAM_B;
	GetGlobalTeam(m_iOffensiveTeam)->SetOffensive(true);
	GetGlobalTeam(m_iDefensiveTeam)->SetOffensive(false);

	// not the first round
	m_bIsFirstRound = false;

	// store the last time limit
	m_fLastTimeLimit = mp_suppressionhalftimelimit.GetFloat();
}

/** 
* Destructor
**/
CSuppressionRound::~CSuppressionRound()
{
	// ?
}

/**
* Initializes the spawning info
*
* @return void
**/
void CSuppressionRound::InitSpawn(void)
{
	// set the time to the next spawn and clear the queue
	m_fNextSpawnTime = gpGlobals->curtime + 1.0; // @TRJ - we should spawn immediately mp_suppressionspawnperiod.GetFloat() + gpGlobals->curtime;
	m_aSpawnQueue.Purge();
	m_bSpawningWave = false;
}

/**
* Called at the start of the round (or in this case, the game)
* @NOTE - DON'T CALL THE BASECLASS!!
*
* @return void
**/
void CSuppressionRound::StartRound(void)
{
	float fRoundLength;

	// clear our weapon list
	CGlobalWeaponManager::GetInstance()->Reset();

	// initialize the spawn info
	InitSpawn();

	// definitely not in the first round...ever...
	SetFirstRound(false);

	// reset the spawn info
	((CSDKGameRules *)g_pGameRules)->ResetSpawns();

	// clear out everyone's hud messages
	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin(filter, "ResetHudDisplayMsg");
	MessageEnd();

	// the round is not over
	m_bInIntermission = false;

	// there is no winner
	m_pWinningTeam = NULL;

	// start the timer
	if(SDKGameRulesSuppression()->IsFirstRoundComplete())
	{
		// round length is the half time limit - the leftovers
		fRoundLength = (mp_suppressionhalftimelimit.GetFloat() * 60) - (m_fRoundEndTime - gpGlobals->curtime);
		if(fRoundLength > (mp_suppressionhalftimelimit.GetFloat() * 60))
			fRoundLength = mp_suppressionhalftimelimit.GetFloat() * 60;
	}
	else
		fRoundLength = mp_suppressionhalftimelimit.GetFloat() * 60;
	m_fRoundEndTime = gpGlobals->curtime + fRoundLength;

	/*// set the buy time
	m_fBuyStartTime = gpGlobals->curtime;
	
	// send it to the players so their menus and things are fantastic
	SendAllPlayersBuyTime();*/

	// reset the round in the objective manager
	GET_OBJ_MGR()->RoundReset();

	// find all the objectives
	// @TODO - find a better place for this
	if(!GET_OBJ_MGR()->AreObjectivesEstablished())
		GET_OBJ_MGR()->EstablishObjectives();

	// tell the clients about the time
	StartClientTimers(fRoundLength);
}

/**
* Restarts the round
*
* @return void
**/
void CSuppressionRound::Restart(void /*bool bRespawnPlayers = true*/)
{
	// initialize the spawn info
	InitSpawn();

	// pretend we're spawning a wave so we're allowed to spawn
	m_bSpawningWave = true;

	// jump down
	BaseClass::Restart();

	// done spawning the wave
	m_bSpawningWave = false;
}

/**
* Allows our round to do some deep pondering
*
* @return void
**/
void CSuppressionRound::Think(void)
{
	// if the game is over we can ignore everything
	if(((CSDKGameRules *)g_pGameRules)->IsGameOver())
		return;

	// spawn the next wave
	SpawnWave();

	// see if the current objective has been captured
	if(GET_OBJ_MGR()->GetCurrentObjective() && GET_OBJ_MGR()->GetCurrentObjective()->IsCaptured())
		GET_OBJ_MGR()->GetCurrentObjective()->SetCaptured();

	// @HACK - i'd rather something triggered this...
	// @TODO - we can trigger this.  convars allow a callback... talking to self via comments...
	// did the length of the game change?
	if(mp_suppressionhalftimelimit.GetFloat() != m_fLastTimeLimit)
	{
		// change the timer
		m_fRoundEndTime += (mp_suppressionhalftimelimit.GetFloat() - m_fLastTimeLimit) * 60;
		StartClientTimers(m_fRoundEndTime - gpGlobals->curtime);

		// set the last time
		m_fLastTimeLimit = mp_suppressionhalftimelimit.GetFloat();
	}
}

/**
 * Checks to see if the round is over.
 */
bool CSuppressionRound::IsRoundOver(void)
{
	// our round never ends! game rules takes care of this...
	return false;
}

/**
* Determines if the specified player can spawn at this point in the round
*
* @param CBasePlayer *pPlayer The player who is attempting to spawn
* @return bool
**/
bool CSuppressionRound::CanSpawn(CBasePlayer *pPlayer)
{
	// first player can always spawn
	if((GetGlobalSDKTeam(TEAM_A)->GetNumPlayers() == 1 && GetGlobalSDKTeam(TEAM_B)->GetNumPlayers() == 0) ||
		(GetGlobalSDKTeam(TEAM_B)->GetNumPlayers() == 1 && GetGlobalSDKTeam(TEAM_A)->GetNumPlayers() == 0))
		return true;

	// can always spawn at start of game
	if(gpGlobals->curtime < mp_initialsuppressionspawntime.GetFloat())
		return true;

	// if we're spawning a wave, go ahead
	if(m_bSpawningWave)
		return true;

	// otherwise, add them to the queue of folks to spawn
	QueuePlayerSpawn(pPlayer);

	return false;
}

/**
* Queues a dead player to be spawned
*
* @param CBasePlayer *pPlayer The dude to queue
* @return void
**/
void CSuppressionRound::QueuePlayerSpawn(CBasePlayer *pPlayer)
{
	CSDKPlayer *pSDKPlayer;

	// convert to sdk
	pSDKPlayer = ToSDKPlayer(pPlayer);
	if(!pSDKPlayer)
	{
		Assert(0);
		return;
	}

	// make sure they're not in the queue already
	if(m_aSpawnQueue.HasElement(pSDKPlayer))
		return;

	// set their queue time
	pSDKPlayer->SetSpawnQueueTime(gpGlobals->curtime);

	// add it to the queue
	m_aSpawnQueue.AddToTail(pSDKPlayer);
}

/**
* Spawns the next wave of minions!
*
* @return void
**/
void CSuppressionRound::SpawnWave(void)
{
	RespawnFunctor respawnFunc;
	int i;

	// has the proper time elapsed?
	if(m_fNextSpawnTime > gpGlobals->curtime)
		return;

	// reset all the spawns
	((CSDKGameRules *)g_pGameRules)->ResetSpawns();

	// we're spawning now
	m_bSpawningWave = true;

	// set the next spawn time
	m_fNextSpawnTime = mp_suppressionspawnperiod.GetFloat() + gpGlobals->curtime;

	// run through the queue and dump everyone out
	i = 0;
	while(i < m_aSpawnQueue.Count())
	{
		// make sure they've been dead long enough
		if((gpGlobals->curtime - m_aSpawnQueue[i]->GetSpawnQueueTime()) > 2.0f)
		{
			// spawn them and pull them from the list
			respawnFunc(m_aSpawnQueue[i]);
			m_aSpawnQueue.Remove(i);
		}
		else
			++i;
	}

	// all done
	m_bSpawningWave = false;
}

/**
* Awards teams xp points for suppression games
*
* @param CSDKTeam *pWinner The team that won the "round"
* @return void
**/
void CSuppressionRound::AwardTeams(CSDKTeam *pWinner)
{
	//CObjective *pObj;

	// make sure the winner is the american
	if(pWinner != GetGlobalSDKTeam(TEAM_A))
		Assert(0);

	// the team that won gets xp for the objective they captured
	//pObj = GET_OBJ_MGR()->GetCurrentObjective();
	pWinner->AwardPlayers(0, /*pObj->GetXP()*/ SUPPRESSION_CAPTURE_AWARD);
}

#endif