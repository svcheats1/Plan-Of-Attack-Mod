#include "cbase.h"
#include "round.h"
#include "util.h"
#include "triggers.h"
#include "team.h"
#include "vstdlib/random.h"
#include "strategymanager.h"
#include <vgui/KeyCode.h>
#include <vgui/MouseCode.h>
#include <vgui/IInput.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar mp_chattime;

// this has to be a function (not a method) so it can be used as a callback
// get it into the round right away
void BuyTimeChanged(ConVar *cvar, const char* value)
{
	if (g_pGameRules && ((CSDKGameRules*)g_pGameRules)->GetCurrentRound())
		((CSDKGameRules*)g_pGameRules)->GetCurrentRound()->SendAllPlayersBuyTime();
}

/**
* Constructor for a round
**/
CRound::CRound()
{
	// setup the round info
	m_bInIntermission = false;
	m_fRoundEndTime = 0;

	// set the teams
	m_iOffensiveTeam = TEAM_A;
	m_iDefensiveTeam = TEAM_B;

	m_bIsFirstRound = true;
	m_bRoundStarted = false;

	// no freeze time
	m_bInOffensiveFreezeTime = false;	
	m_bInDefensiveFreezeTime = false;	
	m_bInDefensiveChooseTime = false;
	m_fOffensiveFreezeEndTime = 0;
	m_fDefensiveFreezeEndTime = 0;
	m_fDefensiveChooseEndTime = 0;

	// start the round
	StartRound();
}

/**
* Destructor
**/
CRound::~CRound()
{
	// blah!
}

/**
 * This is our think function. It gets called a lot.  Woot!
 */
void CRound::Think()
{
	// if the game is over we can bail immediately
	if(g_fGameOver)
		return;

	// are we in freeze time?
	if(InOffensiveFreezeTime() && OffensiveFreezeTimeOver())
	{
		// force the objective as long as we're not in the first round
		if(!IsFirstRound())
			GET_OBJ_MGR()->ForceObjectiveSelection();

		// unfreeze everyone
		EndOffensiveFreezeTime();
	}
	if(InDefensiveFreezeTime() && DefensiveFreezeTimeOver())
		EndDefensiveFreezeTime();

	if(InDefensiveChooseTime() && DefensiveChooseTimeOver())
		EndDefensiveChooseTime();

	// is the round going?
	if(!InIntermission() && IsRoundOver())
	{
		// start the round intermission
		GoToIntermission();
	}
	else if(InIntermission() && IntermissionOver())
		// restart the round
		Restart();
}

/**
* Starts the round
*
* @return void
**/
void CRound::StartRound(void)
{
	// clear our weapon list
	CGlobalWeaponManager::GetInstance()->Reset();

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
	m_fRoundEndTime = gpGlobals->curtime + mp_roundlength.GetFloat();

	// set the buy time
	m_fBuyStartTime = gpGlobals->curtime;
	
	// send it to the players so their menus and things are fantastic
	SendAllPlayersBuyTime();

	// switch the teams
	SwitchTeams();

	// are we in the first round?
	if(!IsFirstRound())
	{
		// start freeze time - MUST be before PickObjective
		StartFreezeTime();

		// reset the round in the objective manager
		GET_OBJ_MGR()->RoundReset();
		GetStrategyManager()->RoundReset();

		// do the objective picking
		GET_OBJ_MGR()->PickObjective(m_iOffensiveTeam);
		GetStrategyManager()->SetStrategyPicker(GetGlobalSDKTeam(m_iDefensiveTeam)->GetObjectivePicker());
		GetStrategyManager()->SendChooseStrategy(m_iDefensiveTeam);
	}
	else
	{
		// find all the objectives
		// @TODO - find a better place for this
		if(!GET_OBJ_MGR()->AreObjectivesEstablished())
			GET_OBJ_MGR()->EstablishObjectives();
	}

	// tell the clients about the time
	StartClientTimers(mp_roundlength.GetFloat());
}

/**
 * Switches the offensive and defensive teams.
 */
void CRound::SwitchTeams(void)
{
	// don't bother if we shouldn't be switching
	if(!ShouldSwitchTeams())
		return;

	int iTemp = m_iOffensiveTeam;
	m_iOffensiveTeam = m_iDefensiveTeam;
	m_iDefensiveTeam = iTemp;
	
	GetGlobalTeam( m_iOffensiveTeam )->SetOffensive(true);
	GetGlobalTeam( m_iDefensiveTeam )->SetOffensive(false);
}

/**
* Restarts the round.  Restarts the map and respawns players
*
* @return void
**/
void CRound::Restart(void /*bool bRespawnPlayers*/)
{
	bool bWasFirstRound;

	//  round has ended
	IGameEvent *pEvent = gameeventmanager->CreateEvent("round_ended");
	if (pEvent)
	{
		// set the winning team
		if(m_pWinningTeam)
			pEvent->SetInt("winner", m_pWinningTeam->GetTeamNumber());
		else
			pEvent->SetInt("winner", 0);

		// set the offensive team, note that this is the offensive team for the round 
		// that just ended
		if(IsFirstRound())
			pEvent->SetInt("offense", 0);
		else
			pEvent->SetInt("offense", m_iOffensiveTeam);

		gameeventmanager->FireEvent(pEvent);
	}

	// clean the map
	CSDKGameRules *pGameRules = dynamic_cast<CSDKGameRules *>(g_pGameRules);

	// check if we're in the first round, restart the round and check again
	// if we weren't and now are in the first round the gamerules is restarting the map
	// therefore we don't need to do anything else
	bWasFirstRound = IsFirstRound();
	pGameRules->RoundRestart();
	if(!bWasFirstRound && IsFirstRound())
		return;

	// reset the xp handicaps
	for(int i = 1; i <= MAX_TEAMS; ++i)
	{
		// see if we can get the team
		CSDKTeam *pTeam = GetGlobalSDKTeam(i);
		if(pTeam)
			// reset
			pTeam->ResetIndivXP();
	}

    // start the round
	StartRound();

	RespawnFunctor respawnFunc;
	ForEachPlayer(respawnFunc);

	// the round has started
	m_bRoundStarted = true;

	// send the buy tip and the attack tip to the attacking team
	// the defending team gets their stuff after their orders are sent out
	if(!IsFirstRound())
	{
		// send the buy tip and the attack tip
		UTIL_SendTipTeam("BuyTip", m_iOffensiveTeam);
		UTIL_SendTipTeam("RoundWinOffenseTip", m_iOffensiveTeam);
	}
}

/**
* Returns if we're in an end-of-round intermission
*
* @return bool
**/
bool CRound::InIntermission(void)
{
	return m_bInIntermission;
}

/**
* Determines if we are out of time
*
* @return bool
**/
bool CRound::RoundTimeExpired(void)
{
	// JD: extend the round if the objective is occupied
	return ( GET_OBJ_MGR()->IsActiveObjectiveEmpty() && gpGlobals->curtime > m_fRoundEndTime );
}

/**
* Starts the client timers by sending all clients the round length
*
* @param float fTime
* @return void
**/
void CRound::StartClientTimers(float fTime)
{
	// send to everyone
	CBroadcastRecipientFilter filter;
	filter.MakeReliable();

	// set the message up to send the round length
	UserMessageBegin(filter, "SyncRoundTimer");
		WRITE_FLOAT(fTime);
	MessageEnd();
}

/**
* Starts the timer for a client who joins late
*
* @param CSDKPlayer *pPlayer The player who joined
* @return void
**/
void CRound::StartNewClientTimer(CSDKPlayer *pPlayer)
{
	float fTime = m_fRoundEndTime - gpGlobals->curtime;
	if (fTime < 0.0)
		return;

	CRecipientFilter filter;
	filter.AddRecipient(pPlayer);
	filter.MakeReliable();

	UserMessageBegin(filter, "SyncRoundTimer");
		WRITE_FLOAT(fTime);
	MessageEnd();
}

/**
* Starts the round intermission
*
* @return void
**/
void CRound::GoToIntermission(void)
{
	CSDKTeam *pWinner;
	CSDKGameRules *pGameRules;

	// pull the gamerules
	pGameRules = dynamic_cast<CSDKGameRules *>(g_pGameRules);
	Assert(pGameRules);

	// deactivate the round timer
	StartClientTimers(-1.0);

	// figure out the winner
	pWinner = DetermineWinner();

	// see if the objective was captured
	if(pWinner && pWinner->IsOffensive() && GET_OBJ_MGR()->GetCurrentObjective())
		GET_OBJ_MGR()->GetCurrentObjective()->SetCaptured();
	
	// start the intermission
	if(GET_OBJ_MGR()->AllCaptured())
		// if everything was captured and we made it here it means we're playing
		// another game.  so we need to use the chat time break.
		m_fIntermissionEndTime = gpGlobals->curtime + mp_chattime.GetInt();
	else
		// otherwise, just do a regular intermission
		m_fIntermissionEndTime = gpGlobals->curtime + mp_roundintermissionlength.GetFloat();
	m_bInIntermission = true;
		
	// update the game state
	if(pGameRules) {
		pGameRules->UpdateGameState(pWinner);
	}

	// give everyone money and experience :) 
	// JD: Do this early-ish because it sets the objective as captured 
	// if we won by team deathmatch, and this is important for messaging and team scoring
	AwardTeams(pWinner);

	// now that objectives have been captured, tell game rules that it ended
	if (pGameRules) {
		pGameRules->RoundEnded();
	}

	// are we playing more than one game? and not following any other game finishing rules?
	if(mp_roundlimit.GetInt() <= 0 && mp_teamscorelimit.GetInt() == 3 && GET_OBJ_MGR()->AllCaptured())
		pGameRules->IncrementGamesPlayed();

	// see if we are just playing another game
	if(GET_OBJ_MGR()->AllCaptured() && pGameRules->MoreGamesToPlay())
	{
		// freeze all the players
		for(int i = 0; i < MAX_PLAYERS; i++)
		{
			// pull the player
			CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer *>(UTIL_PlayerByIndex( i ));
			if(!pPlayer)
				continue;

			// freeze them
			pPlayer->SetFreezeState(true);
		}

		// send the win/lose tip
		pGameRules->SendWinLoseTip();
	}
	else
		// notify everyone
		NotifyTeams(pWinner);

	// tell the objective manager the round ended
	GET_OBJ_MGR()->RoundEnded();

	// is this the first round?
	if(IsFirstRound())
	{
		// first round is over
		SetFirstRound(false);

		// set the defensive team to the winner so they 
		// become the offense when the round restarts
		if(pWinner && m_iDefensiveTeam != pWinner->GetTeamNumber())
			SwitchTeams();
		// if it was a draw, switch the teams half the time.
		else if (!pWinner && RandomInt(0, 1))
			SwitchTeams();
	}

	// round has ended
	m_bRoundStarted = false;

	// players can't die...
}

/**
* Determines if the round is over
*
* @return void
**/
bool CRound::IntermissionOver(void)
{
	return ( m_fIntermissionEndTime < gpGlobals->curtime );
}

/**
* Determines the winner of the round
*
* @return CSDKTeam *
**/
CSDKTeam *CRound::DetermineWinner(void)
{
	if (m_pWinningTeam)
		return m_pWinningTeam;

	m_pWinningTeam = DetermineWinnerByDeathmatch();
	if (m_pWinningTeam)
		return m_pWinningTeam;
	
	// may return null, but that's fine
    m_pWinningTeam = DetermineWinnerByObjective();
	return m_pWinningTeam;	
}

CSDKTeam* CRound::DetermineWinnerByDeathmatch(void)
{
	// Team Deathmatch Condition: We win because the other team is dead.
	// Stipulation: there must be someone on each team.
	CSDKTeam *pTeam, *pDeadTeam, *pAliveTeam;

	pAliveTeam = NULL;
	pDeadTeam = NULL;

	// see if we have a dead team and an alive team
	for(int i = 1; i <= MAX_TEAMS; ++i) {
		pTeam = dynamic_cast<CSDKTeam *>(GetGlobalTeam(i));
		if (pTeam) {
			if (pTeam->IsTeamAlive())
				pAliveTeam = pTeam;
			else
				pDeadTeam = pTeam;
		}
		else
			Assert(0);
	}

	// we must have a winner and a loser
	if (pAliveTeam && pDeadTeam && pAliveTeam->GetNumPlayers() && pDeadTeam->GetNumPlayers())
		return pAliveTeam;

	// no winner by deathmatch
	return NULL;
}

CSDKTeam* CRound::DetermineWinnerByObjective(void)
{	
	// has the objective been captured?
	if(!IsFirstRound() && GET_OBJ_MGR()->GetCurrentObjective() && GET_OBJ_MGR()->GetCurrentObjective()->IsCaptured())
	{
		// JD - had to put this here because it gets reset below
		CObjectiveZone *pObjZone = dynamic_cast<CObjectiveZone*>(GET_OBJ_MGR()->GetCurrentObjective());
		for(int i = 0; pObjZone && i < pObjZone->NumPlayersInZone(); ++i) {
			CSDKPlayer *pPlayer = pObjZone->GetPlayerInZone(i);
			pPlayer->AwardCap(pObjZone->GetID());
		}

		// @JD - I'm not sure if we should do this here, but I'll leave it for now
		// set everything up
		GET_OBJ_MGR()->GetCurrentObjective()->SetCaptured();
		GET_OBJ_MGR()->GetCurrentObjective()->RoundReset();

		// send back the team
		return dynamic_cast<CSDKTeam *>(GetGlobalTeam(m_iOffensiveTeam));
	}
	// we may have run out of time with folks still alive and no capture, defense wins
	else if(!IsFirstRound() && GET_OBJ_MGR()->GetCurrentObjective() && RoundTimeExpired())
	{
		// reset the objective
		GET_OBJ_MGR()->GetCurrentObjective()->RoundReset();

		// send back the defensive team
		return dynamic_cast<CSDKTeam *>(GetGlobalTeam(m_iDefensiveTeam));
	}

	return NULL;
}

/**
 * Check to see if this round is a draw.
 * Currently a draw is defined as everyone is dead. 
 * (i.e., In order for there to be a winner, a team has to be alive.)
 */
bool CRound::IsRoundADraw(void)
{
	int iNumPlayers = 0;

	// Get a count of how many players are in the game, and also see
	// if there are any alive.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer *>(UTIL_PlayerByIndex(i));
		if (pPlayer) {
			// If there are any alive, then its not a draw.
			if (pPlayer->IsAlive())
				return false;

			// increment if they are on a real team
			if(pPlayer->GetTeam() && pPlayer->GetTeam()->IsPlayingTeam())
				++iNumPlayers;
		}
	}

	// There is someone in the game, so lets say its a draw.
	// @TRJ - added the round started condition.  the first player to join the game is dead
	// when the round starts so it immediately ends.  m_bRoundStarted is true after spawn 
	// in Restart
	if(iNumPlayers > 0 && m_bRoundStarted)
		return true;

	// There's no one in the game, so avoid restarting the round every 10 seconds...
	return false;
}

/**
 * Checks to see if the round is over.
 */
bool CRound::IsRoundOver(void)
{
	return (RoundTimeExpired() || DetermineWinner() || IsRoundADraw());
}

/**
* Returns true if we're still in freeze time
*
* @return bool
**/
bool CRound::InOffensiveFreezeTime(void)
{
	return m_bInOffensiveFreezeTime && !IsFirstRound();
}

/**
* Returns true if we're still in freeze time
*
* @return bool
**/
bool CRound::InDefensiveFreezeTime(void)
{
	return m_bInDefensiveFreezeTime && !IsFirstRound();
}

bool CRound::InDefensiveChooseTime(void)
{
	return m_bInDefensiveChooseTime && !IsFirstRound();
}

bool CRound::IsPlayerFrozen(CSDKPlayer *pPlayer)
{
	// offensive or defensive?
	if(pPlayer->GetTeam() && pPlayer->GetTeamNumber() == m_iOffensiveTeam)
		return InOffensiveFreezeTime();
	else if(pPlayer->GetTeam() && pPlayer->GetTeamNumber() == m_iDefensiveTeam)
		return InDefensiveFreezeTime();

	return false;
}

/**
* Determines if freeze time is over
*
* @return bool
**/
bool CRound::OffensiveFreezeTimeOver(void)
{
	return (GetRemainingOffensiveFreezeTime() < 0.0);
}

/**
* Determines if freeze time is over
*
* @return bool
**/
bool CRound::DefensiveFreezeTimeOver(void)
{
	return (GetRemainingDefensiveFreezeTime() < 0.0);
}

bool CRound::DefensiveChooseTimeOver(void)
{
	return (GetRemainingDefensiveChooseTime() < 0.0);
}

/**
 * Determines how much freeze time is left
 * @ return float
 */
float CRound::GetRemainingOffensiveFreezeTime(void)
{
	return (m_fOffensiveFreezeEndTime - gpGlobals->curtime);
}

/**
 * Determines how much freeze time is left
 * @ return float
 */
float CRound::GetRemainingDefensiveFreezeTime(void)
{
	return (m_fDefensiveFreezeEndTime - gpGlobals->curtime);
}

float CRound::GetRemainingDefensiveChooseTime(void)
{
	return (m_fDefensiveChooseEndTime - gpGlobals->curtime);
}

/**
* Freezes all the players on a team
*
* @param int i
**/
void CRound::StartFreezeTime(void)
{
	// make sure our times are right
	if(mp_defensive_freezetime.GetFloat() > mp_offensive_freezetime.GetFloat())
		mp_defensive_freezetime.SetValue(mp_offensive_freezetime.GetFloat());

	// set the freeze time
	m_fOffensiveFreezeEndTime = gpGlobals->curtime + mp_offensive_freezetime.GetFloat();
	m_bInOffensiveFreezeTime = true;
	m_fDefensiveFreezeEndTime = gpGlobals->curtime + mp_defensive_freezetime.GetFloat();
	m_bInDefensiveFreezeTime = true;
	m_fDefensiveChooseEndTime = gpGlobals->curtime + mp_defensive_choosetime.GetFloat();
	m_bInDefensiveChooseTime = true;

	// freeze the players on the offensive team
	FreezeFunctor offensiveFreezeFunc(m_iOffensiveTeam, true);
	ForEachPlayer(offensiveFreezeFunc);

	// freeze everyone on the defensive team
	FreezeFunctor defensiveFreezeFunc(m_iDefensiveTeam, true);
	ForEachPlayer(defensiveFreezeFunc);
}

/**
* Unfreezes all the players on a team
*
* @param int iTeamIndex The team to unfreeze
* @return void
**/
void CRound::EndDefensiveFreezeTime(void)
{
	// no freeze time
	m_bInDefensiveFreezeTime = false;	

	// unfreeze all the players
	FreezeFunctor freezeFunc(m_iDefensiveTeam, false);
	ForEachPlayer(freezeFunc);

	// send the defend orders
	if(!IsFirstRound() && GetStrategyManager() && GetStrategyManager()->HasStrategy(m_iDefensiveTeam))
		GetStrategyManager()->SendStrategy(m_iDefensiveTeam);
}

void CRound::EndDefensiveChooseTime(void)
{
	m_bInDefensiveChooseTime = false;

	// we can bail if we're in an intermission
	if(InIntermission())
		return;

	// if its not the first round and there is no strategy for this team, then
	// send the old-school defend orders
	if(!IsFirstRound()) {
		if (!GetStrategyManager() || !GetStrategyManager()->HasStrategy(m_iDefensiveTeam))
			SendDefendOrders();
		else if (GetStrategyManager())
			GetStrategyManager()->SendStrategy(m_iDefensiveTeam);
	}
}

/**
* Unfreezes all the players on a team
*
* @param int iTeamIndex The team to unfreeze
* @return void
**/
void CRound::EndOffensiveFreezeTime(void)
{
	// no freeze time
	m_bInOffensiveFreezeTime = false;	

	// unfreeze all the players
	FreezeFunctor freezeFunc(m_iOffensiveTeam, false);
	ForEachPlayer(freezeFunc);

	// we can bail if we're in an intermission
	if(InIntermission())
		return;

	// send orders
	if(!IsFirstRound()) {
		if (!GetStrategyManager() || !GetStrategyManager()->HasStrategy(m_iOffensiveTeam))
			SendAttackOrders(GET_OBJ_MGR()->GetCurrentObjective());
		else if (GetStrategyManager())
			GetStrategyManager()->SendStrategy(m_iOffensiveTeam);
	}
}

/**
* Awards all the teams for the current round
*
* @param CSDKTeam *pWinner The team that won
* @return void
**/
void CRound::AwardTeams(CSDKTeam *pWinner)
{
	CSDKTeam *pTeam;
	CSDKGameRules *pGameRules;

	// get the game rules
	pGameRules = dynamic_cast<CSDKGameRules *>(g_pGameRules);
	Assert(pGameRules);
	if(!pGameRules)
		return;

	// go through our teams
	for(int i = 1; i <= MAX_TEAMS; ++i)
	{
		// grab the team
		pTeam = GetGlobalSDKTeam(i);
		if (pTeam)
		{
			// are we in the first round?
			if(IsFirstRound())
			{
				// award each of the teams the first round cash and no xp
				pTeam->AwardPlayers(END_OF_FIRST_ROUND_CASH, 0);
			}
			else
			{
				// get the current objective
				CObjective *pObj = GET_OBJ_MGR()->GetCurrentObjective();

				// if they don't have an objective, try to force one 
				// (this still may not work in strange cases, like no objectives on the map)
				if (!pObj) {
					GET_OBJ_MGR()->ForceObjectiveSelection();
					pObj = GET_OBJ_MGR()->GetCurrentObjective();
				}

				// did we win?
				if(pTeam == pWinner)
				{
					// offensive team won and we have an objective
					if(pTeam->IsOffensive() && pObj)
					{
						// give us our cash and XP
						pTeam->AwardPlayers(BASE_CASH_AWARD + pObj->GetCash(), 
											pObj->GetXP() + pGameRules->GetMoraleXPAward(pTeam));

						CObjectiveZone *pObjZone = dynamic_cast<CObjectiveZone*>(pObj);
						for(int i = 0; pObjZone && i < pObjZone->NumPlayersInZone(); ++i) {
							CSDKPlayer *pPlayer = pObjZone->GetPlayerInZone(i);
							pPlayer->AwardCap(pObj->GetID());
						}

						// @TRJ - move out into the intermission code
						// set the objective to captured
						//pObj->SetCaptured();
					}
					// defensive team won or no objective
					else
						pTeam->AwardPlayers(BASE_CASH_AWARD + TEAM_WIN_CASH_AWARD, pGameRules->GetMoraleXPAward(pTeam));
				}
				// this team lost
				else if(pTeam != pWinner && pWinner != NULL)
				{
					// should we bankrupt them?
					if (pObj && pObj->BankruptLosingTeam() && !pTeam->IsOffensive())
						pTeam->AwardPlayers(-1 * MAX_CASH, pGameRules->GetMoraleXPAward(pTeam));
					else
						pTeam->AwardPlayers(BASE_CASH_AWARD, pGameRules->GetMoraleXPAward(pTeam));
				}
				// draw
				else
					pTeam->AwardPlayers(BASE_CASH_AWARD, pGameRules->GetMoraleXPAward(pTeam));
			}
		}
		else
			Assert(0);
	}
}

/**
* Determines if players can still buy items
* 
* @return bool
**/
bool CRound::InBuyTime(void)
{
	return gpGlobals->curtime < (m_fBuyStartTime + mp_buytime.GetFloat());
}

void CRound::SendAllPlayersBuyTime()
{
	CBroadcastRecipientFilter filter;
	SendBuyTime(filter);
}

void CRound::SendPlayerBuyTime(CBasePlayer *pPlayer)
{
	CSingleUserRecipientFilter filter(pPlayer);
	SendBuyTime(filter);
}

void CRound::SendBuyTime(CRecipientFilter &filter)
{
	filter.MakeReliable();

	// send the message
	UserMessageBegin(filter, "BuyTime");
		WRITE_FLOAT(mp_buytime.GetFloat() - gpGlobals->curtime + m_fBuyStartTime);
	MessageEnd();
}

/**
* Determines if the specified player can spawn at this point in the round
*
* @param CBasePlayer *pPlayer The player who is attempting to spawn
* @return bool
**/
bool CRound::CanSpawn(CBasePlayer *pPlayer)
{
	// is this the first round?
	if(IsFirstRound() && gpGlobals->curtime < mp_firstroundspawntime.GetFloat())
		return true;
	else
		return InBuyTime();
}

/**
* Notifies each team of whether they won or lost the round
*
* @param CSDKTeam *pWinner The team that won
* @return void
**/
void CRound::NotifyTeams(CSDKTeam *pWinner)
{
	CSDKTeam *pTeam;
	char szCashStr[32], szXPStr[32], szTemp[64];
	CObjective *pObj;

	// pull the objective
	pObj = GET_OBJ_MGR()->GetCurrentObjective();

	// go through our teams
	for(int i = 1; i <= MAX_TEAMS; ++i)
	{
		// pull the team
		pTeam = GetGlobalSDKTeam(i);
		if(pTeam)
		{
			// setup the filter
			CTeamRecipientFilter filter(i);
			filter.MakeReliable();

			// set it up
			UserMessageBegin(filter, "HudDisplayMsg");

			// win?
			if(pTeam == pWinner)
				WRITE_STRING("RoundWin");
			// lose?
			else if(pWinner != NULL)
				WRITE_STRING("RoundLose");
			// draw
			else
				WRITE_STRING("RoundDraw");

			// no replacements
			WRITE_SHORT(0);

			// if the round was not a draw we need to send info on why each team won/lost
			if(pWinner != NULL)
			{
				// there is a label
				WRITE_BYTE(1);

				// win or lose?
				memset(szTemp, 0, sizeof(szTemp));
				Q_snprintf(szTemp, sizeof(szTemp), "%s", pTeam == pWinner ? "RoundWin" : "RoundLose");

				// all objectives captured?
				if (GET_OBJ_MGR()->AllCaptured()) {
					Q_snprintf(szTemp + Q_strlen(szTemp), sizeof(szTemp) - Q_strlen(szTemp), "%s", "AllObjectivesCaptured");
				}
				// death match?
				else if(DetermineWinnerByDeathmatch())
					Q_snprintf(szTemp + Q_strlen(szTemp), sizeof(szTemp) - Q_strlen(szTemp), "%s", "EliminatedSubLabel");
				// round time?
				else if(RoundTimeExpired())
					Q_snprintf(szTemp + Q_strlen(szTemp), sizeof(szTemp) - Q_strlen(szTemp), "%s", "TimeSubLabel");
				// the only thing left is objectives
				// BUT, FOR THE LOVE OF GOD, DON'T CALL THE FUNCTION THAT CHECKS THIS
				// AS IT WILL RESET ALL THE OBJECTIVE INFO AND WILL NOT RETURN THE 
				// CORRECT RESULT
				else
				{
					// was it a base?
					if(!pObj->IsRequiredToWin())
						Q_snprintf(szTemp + Q_strlen(szTemp), sizeof(szTemp) - Q_strlen(szTemp), "%s", "CaptureBaseSubLabel");
					else
						Q_snprintf(szTemp + Q_strlen(szTemp), sizeof(szTemp) - Q_strlen(szTemp), "%s", "CaptureSubLabel");
				}

				// send the string
				WRITE_STRING(szTemp);

				// two replacements if it was an objective capture and not a base
				if(!DetermineWinnerByDeathmatch() && !RoundTimeExpired() && pObj->IsRequiredToWin() && !GET_OBJ_MGR()->AllCaptured())
				{
					// write the info
					WRITE_SHORT(2);
					WriteObjectiveInfo(pObj);
				}
				else
					WRITE_SHORT(0);
			}

			// do we need a second sublabel?
			if(pTeam == pWinner && pObj && pWinner->IsOffensive() && !GET_OBJ_MGR()->AllCaptured())
			{
				// yup
				WRITE_BYTE(1);

				// figure out the two strings
				Q_snprintf(szCashStr, sizeof(szCashStr), "%d", pObj->GetCash());
				Q_snprintf(szXPStr, sizeof(szXPStr), "%d", pObj->GetXP());

				// no cash?
				if(pObj && pObj->GetCash() == 0 && pObj->GetXP() != 0)
					WRITE_STRING("RoundWinSubLabelXP");
				// no xp?
				else if(pObj && pObj->GetCash() != 0 && pObj->GetXP() == 0)
					WRITE_STRING("RoundWinSubLabelCash");
				// both?
				else if(pObj && pObj->GetCash() != 0 && pObj->GetXP() != 0)
					WRITE_STRING("RoundWinSubLabelCashXP");
				else if(pObj && pObj->BankruptLosingTeam())
					WRITE_STRING("RoundWinSubLabelBankrupt");

				// check what info we need to send
				if(pObj->GetCash() != 0 && pObj->GetXP() == 0)
				{
					// one replacement
					WRITE_SHORT(1);
					WRITE_STRING(szCashStr);
				}
				else if(pObj->GetCash() == 0 && pObj->GetXP() != 0)
				{
					// one replacement
					WRITE_SHORT(1);
					WRITE_STRING(szXPStr);
				}
				else if(pObj->BankruptLosingTeam())
					WRITE_SHORT(0);
				else
				{
					// two replacements
					WRITE_SHORT(2);
					WRITE_STRING(szCashStr);
					WRITE_STRING(szXPStr);
				}
			}
			else if(pTeam != pWinner && pWinner != NULL && pObj && pObj->BankruptLosingTeam() && !pTeam->IsOffensive() && !GET_OBJ_MGR()->AllCaptured())
			{
				// yup
				WRITE_BYTE(1);

				// send the loser message
				WRITE_STRING("RoundLoseSubLabelBankrupt");
				WRITE_SHORT(0);
			}
			else
				// nope, no sublabel
				WRITE_BYTE(0);

			// no winner, write the extra byte for the second sub label
			if(pWinner == NULL)
				WRITE_BYTE(0);

			// off it goes
			MessageEnd();
		}
	}
}

/**
* Sends a message to the client with some orders about the current objective
*
* @param CObjective *pObj The objective to send orders for
* @return void
**/
void CRound::SendAttackOrders(CObjective *pObj)
{
	EmitSound_t sSound;

	// make sure we got an objective
	if(!pObj)
	{
		DevMsg("SendAttackOrders needs a real objective!\n");
		return;
	}

	// flip through both teams to find the offensive one
	for(int i = 1; i <= MAX_TEAMS; ++i)
	{
		// pull the team
		CSDKTeam *pTeam = GetGlobalSDKTeam(i);
		if(pTeam && pTeam->IsOffensive())
		{
			// only send to this team
			CTeamRecipientFilter filter(i);
			filter.MakeReliable();

			// send the message
			UserMessageBegin(filter, "HudDisplayMsg");

			// if we are dealing with a base we only need to send one replacement
			if(!pObj->IsRequiredToWin())
			{
				WRITE_STRING("RoundAttackBase");
				WRITE_SHORT(1);
			}
			else
			{
				WRITE_STRING("RoundAttack");
				WRITE_SHORT(2);
			}

			// write out the objective info
			WriteObjectiveInfo(pObj);

			// no sublabel and no second sublabel
			WRITE_BYTE(0);
			WRITE_BYTE(0);

			// send it out
			MessageEnd();
		}
	}
}

/**
* Writes out the objective info
*
* @param CObjective *pObj The objective write out the info for
* @return void
**/
void CRound::WriteObjectiveInfo(CObjective *pObj)
{
	// add the title
	WRITE_STRING(pObj->GetTitle());

	// figure out what number to add if this isn't a base
	if(pObj->IsRequiredToWin())
	{
		switch(pObj->GetID())
		{
			case 0:
				WRITE_STRING("(I)");
				break;
			case 1:
				WRITE_STRING("(II)");
				break;
			case 2:
				WRITE_STRING("(III)");
				break;
			case 3:
				WRITE_STRING("(IV)");
				break;
			case 4:
				WRITE_STRING("(V)");
				break;
			case 5:
				WRITE_STRING("(VI)");
				break;
			case 6:
				WRITE_STRING("(VII)");
				break;
			case 7:
				WRITE_STRING("(VIII)");
				break;
			case 8:
				WRITE_STRING("(IX)");
				break;
			case 9:
				WRITE_STRING("(X)");
				break;
		}
	}
}

/**
* Sends orders to defend 
*
* @return void
**/
void CRound::SendDefendOrders(void)
{
	// only send to this team
	CTeamRecipientFilter filter(m_iDefensiveTeam);
	filter.MakeReliable();

	// send the message
	UserMessageBegin(filter, "HudDisplayMsg");

	// send the message with 1 replacement
	WRITE_STRING("RoundDefend");
	WRITE_SHORT(1);
	WRITE_STRING(GET_OBJ_MGR()->GetDefendableObjectives(m_iDefensiveTeam));
	WRITE_BYTE(1);
	WRITE_STRING("RoundDefendSubLabel");
	WRITE_SHORT(0);
	WRITE_BYTE(0);

	// send it out
	MessageEnd();

	// send the buy tip and the defense win tip
	UTIL_SendTipTeam("BuyTip", m_iDefensiveTeam);
	UTIL_SendTipTeam("RoundWinDefenseTip", m_iDefensiveTeam);
}