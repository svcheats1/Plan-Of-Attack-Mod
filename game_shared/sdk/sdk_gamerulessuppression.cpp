#include "cbase.h"
#include "sdk_gamerulessuppression.h"

#ifdef CLIENT_DLL
	#include "c_team.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef SUPPRESSION_ENABLED

extern bool g_bGameRestarted;

REGISTER_GAMERULES_CLASS(CSDKGameRulesSuppression);

BEGIN_NETWORK_TABLE_NOBASE(CSDKGameRulesSuppression, DT_SDKGameRulesSuppression)
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( sdk_gamerulessuppression, CSDKGameRulesSuppressionProxy);
IMPLEMENT_NETWORKCLASS_ALIASED( SDKGameRulesSuppressionProxy, DT_SDKGameRulesSuppressionProxy)

#ifdef CLIENT_DLL
	void RecvProxy_SDKGameRulesSuppression(const RecvProp *pProp, void **pOut, void *pData, int objectID)
	{
		CSDKGameRulesSuppression *pRules = SDKGameRulesSuppression();
		Assert(pRules);
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE(CSDKGameRulesSuppressionProxy, DT_SDKGameRulesSuppressionProxy)
		RecvPropDataTable("sdk_gamerulessuppression_data", 0, 0, &REFERENCE_RECV_TABLE(DT_SDKGameRulesSuppression ), RecvProxy_SDKGameRulesSuppression),
	END_RECV_TABLE()
#else
	void *SendProxy_SDKGameRulesSuppression(const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID)
	{
		CSDKGameRulesSuppression *pRules = SDKGameRulesSuppression();
		Assert(pRules);
		pRecipients->SetAllRecipients();

		return pRules;
	}

	BEGIN_SEND_TABLE(CSDKGameRulesSuppressionProxy, DT_SDKGameRulesSuppressionProxy)
		SendPropDataTable("sdk_gamerulessuppression_data", 0, &REFERENCE_SEND_TABLE(DT_SDKGameRulesSuppression ), SendProxy_SDKGameRulesSuppression),
	END_SEND_TABLE()
#endif

ConVar mp_suppressionhalftimelimit("mp_suppressionhalftimelimit",
								"15",
								FCVAR_NOTIFY|FCVAR_REPLICATED,
								"Length of a half in suppression games" );
ConVar mp_suppressioncancommitsuicide("mp_suppressioncancommitsuicide",
										"0",
										FCVAR_NOTIFY | FCVAR_REPLICATED,
										"Determines if players can kill themselves in suppression games.",
										true,
										0,
										true,
										1);

#ifndef CLIENT_DLL

/**
* Constructor
**/
CSDKGameRulesSuppression::CSDKGameRulesSuppression()
	: BaseClass()
{
	// no spawn info yet
	m_iSpawnGroups = 0;
	m_iCurrentAmericanSpawnGroup = 0;
	m_iCurrentCoalitionSpawnObjective = 0;

	// no warnings please (somewhat counter intuitive)
	m_bFiveMinuteWarning = true;
	m_bOneMinuteWarning = true;

	// haven't captured them all yet
	m_bFirstRoundComplete = false;
	m_iFirstRoundAACaptures = 0;
}

/**
* Determines if the game is over
*	AA wins if all three objectives captured.
*	CF wins if time runs out (handled in multiplay)
*
* @return bool
**/
bool CSDKGameRulesSuppression::IsGameOver(void)
{
	// see if the aa has managed to capture all three objectives
	if(m_bFirstRoundComplete
			&& (GetCurrentRound()->GetEndTime() < gpGlobals->curtime
				|| GetGlobalTeam(TEAM_A)->GetScore() >= mp_teamscorelimit.GetInt()))
		return true;
	else if(GetCurrentRound()->GetEndTime() < gpGlobals->curtime 
				|| GetGlobalTeam(TEAM_A)->GetScore() >= mp_teamscorelimit.GetInt())
	{
		// first round is done so store the number of captures
		m_bFirstRoundComplete = true;
		m_iFirstRoundAACaptures = GetGlobalTeam(TEAM_A)->GetScore();

		// swap the teams and restart the map
		RestartMap();

		// reset the spawn info
		ClearSpawns();

		// initialize the game state
		InitGameState();

		// resets the teams
		ResetTeams();

		// re-finalize the objectives
		GET_OBJ_MGR()->ResetAllObjectives();
		GET_OBJ_MGR()->DisableTimerNotification();
		GET_OBJ_MGR()->FinalizeObjectives();

		// start the round over
		GetCurrentRound()->Restart(/*false*/);
		SwapTeams();

		// we can spawn immediately
		((CSuppressionRound *)GetCurrentRound())->SetNextSpawnTime(gpGlobals->curtime + 1.0f);
	}

	return false;
}

/**
* Creates our new round object
*
* @return CRound *
**/
CRound *CSDKGameRulesSuppression::GetNewRound(void)
{
	return new CSuppressionRound();
}

/**
* Determines what to do when a player dies
*
* @param CBasePlayer *pVictim The dude that died
* @param const CTakeDamageInfo &info How they died
* @return void
**/
void CSDKGameRulesSuppression::PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	// jump down
	BaseClass::PlayerKilled(pVictim, info);

	// tell teh round so they can be added to the queue
	GET_SUPPRESSION_ROUND()->QueuePlayerSpawn(pVictim);
}


/**
* Defines what will occur when a player spawns
*
* @param CBasePlayer *pPlayer The player whose spawning
* @return void
**/
void CSDKGameRulesSuppression::PlayerSpawn( CBasePlayer *pBasePlayer )
{
	CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer*>(pBasePlayer);

	// equip them
	EquipPlayer(pPlayer);

	// jump down past the regular sdk game rules
	CTeamplayRules::PlayerSpawn(pBasePlayer);
}

/**
* Equips the player with weapons appropriate to their skill
*
* @param CSDKPlayer *pPlayer The player to equip
* @return void
**/
void CSDKGameRulesSuppression::EquipPlayer(CSDKPlayer *pPlayer)
{
	CSkillClass *pSkill;
	CWeaponSDKBase *pWpn;

	// get rid of anything i have now
	pWpn = pPlayer->GetMeleeWeapon();
	if(pWpn)
	{
		// get rid of it
		pPlayer->Weapon_Drop(pWpn);
		UTIL_Remove(pWpn);
	}
	pWpn = pPlayer->GetSecondaryWeapon();
	if(pWpn)
	{
		// get rid of it
		pPlayer->Weapon_Drop(pWpn);
		UTIL_Remove(pWpn);
	}
	pWpn = pPlayer->GetPrimaryWeapon();
	if(pWpn)
	{
		// get rid of it
		pPlayer->Weapon_Drop(pWpn);
		UTIL_Remove(pWpn);
	}

	// everyone gets a knife
	pPlayer->GiveNamedItem("weapon_knife");

	// what's my skill?
	pSkill = pPlayer->GetSkillClass();
	if(FStrEq(pSkill->GetInternalClassName(), "Scout"))
	{
		// give them a deagle
		pPlayer->GiveNamedItem("weapon_deagle");

		// give them a shotgun
		if(pPlayer->GetTeamNumber() == TEAM_A)
		{
			// random shotty / mp5
			if(RandomInt(0, 1))
				pPlayer->GiveNamedItem("weapon_shotgun");
			else
				pPlayer->GiveNamedItem("weapon_mp5");
		}
		else
		{
			// random shotty / pp90m1
			if(RandomInt(0, 1))
				pPlayer->GiveNamedItem("weapon_saiga12k");
			else
				pPlayer->GiveNamedItem("weapon_pp90m1");
		}

		// give them a grenade too
		pPlayer->GiveNamedItem("weapon_grenade");
	}
	else if(FStrEq(pSkill->GetInternalClassName(), "Soldier"))
	{
		// you gets a viking!
		pPlayer->GiveNamedItem("weapon_viking");

		// given them the proper rifle
		if(pPlayer->GetTeamNumber() == TEAM_A)
			pPlayer->GiveNamedItem("weapon_m16");
		else
			pPlayer->GiveNamedItem("weapon_ak107");
	}
	else if(FStrEq(pSkill->GetInternalClassName(), "Sniper"))
	{
		// you gets a viking!
		pPlayer->GiveNamedItem("weapon_viking");

		// given them the proper sniper rifle
		if(pPlayer->GetTeamNumber() == TEAM_A)
			pPlayer->GiveNamedItem("weapon_m40");
		else
			pPlayer->GiveNamedItem("weapon_svu");
	}
	else if(FStrEq(pSkill->GetInternalClassName(), "HW"))
	{
		// you gets a viking!
		pPlayer->GiveNamedItem("weapon_viking");

		// how about a machine gun
		if(pPlayer->GetTeamNumber() == TEAM_A)
			pPlayer->GiveNamedItem("weapon_mk48");
		else
			pPlayer->GiveNamedItem("weapon_pkm");
	}

	// select the primary weapon
	pPlayer->Weapon_Switch(pPlayer->GetPrimaryWeapon());
}

/**
* Handles stuff when a player changes skillz
*
* @param CSDKPlayer *pPlayer The player who changed skillz
* @return void
**/
void CSDKGameRulesSuppression::PlayerChangedSkill(CSDKPlayer *pPlayer)
{
	// if we're alive and on a real team, and we need equipment we need to be equipped
	if(pPlayer->IsAlive() && pPlayer->GetTeam() && 
		pPlayer->GetTeam()->IsPlayingTeam() && pPlayer->NeedsEquipment())
	{
		// equip them
		EquipPlayer(pPlayer);
		pPlayer->SetNeedsEquipment(false);
	}
}

/**
* Sticks the player at the proper spawn spot
*
* @param CBasePlayer *pPlayer The player to spawn
* @return CBaseEntity *
**/
CBaseEntity *CSDKGameRulesSuppression::GetPlayerSpawnSpot(CBasePlayer *pPlayer)
{
	CBaseEntity *pSpot;
	CSuppressionSpawnPoint *pSpawn;

	// jump down
	pSpot = BaseClass::GetPlayerSpawnSpot(pPlayer);

	// seee if we have a suppression spawn
	pSpawn = dynamic_cast<CSuppressionSpawnPoint *>(pSpot);
	if(pSpawn)
		pSpawn->SetOccupied(true);

	return pSpot;
}

/**
* Resets all the spawn information
*
* @return void
**/
void CSDKGameRulesSuppression::ResetSpawns(void)
{
	CSuppressionSpawnPoint *pSpawn;
	CBaseEntity *pTemp;

	// run through the list
	for(int i = 0; i < m_aSpawns.Count(); ++i)
	{
		// convert to a suppression point
		pTemp = m_aSpawns[i];
		pSpawn = dynamic_cast<CSuppressionSpawnPoint *>(m_aSpawns[i]);
		if(pSpawn)
			pSpawn->SetOccupied(false);
	}

	// pick a new random spawn group
	m_iCurrentCoalitionSpawnObjective = random->RandomInt(0, 2);
	m_iCurrentAmericanSpawnGroup = random->RandomInt(0, m_iSpawnGroups);
}

/**
* Determines the name of the spawn point entity to use for a player on
* the specified team
*
* @param char szName[] The buffer to fill in
* @param int iLen The length of the buffer to fill in
* @param int iTeam The team the player is on
* @return bool
**/
bool CSDKGameRulesSuppression::GetSpawnPointName(char szName[], int iLen, int iTeam)
{
	// are we in the first spawn and on the american team
	// just use the regular spawn point which should be in/near our base
	if(GET_SUPPRESSION_ROUND()->InFirstSpawn() && iTeam == TEAM_A)
		Q_snprintf(szName, iLen, "info_player_team%d", iTeam);
	else
		// just pick one
		Q_snprintf(szName, iLen, "info_player_team%d_suppression", iTeam);

	return true;
}

/**
* Determines if the specified entity is a valid place for the player to spawn
*
* @param CBaseEntity *pSpot The point at which to spawn
* @param CBasePlayer *pPlayer The player to check against
* @return bool
**/
bool CSDKGameRulesSuppression::IsSpawnPointValid(CBaseEntity *pSpot, CBasePlayer *pPlayer)
{
	CObjectiveHoldarea *pObjective;
	int iObjectiveID;

	// if we're on the cf the spawn point is only valid if it is not under attack
	if(pPlayer->GetTeamNumber() == TEAM_B)
	{
		// see which objective we're attached to
		iObjectiveID = ((CSuppressionStartCoalition *)pSpot)->GetObjectiveID();
		pObjective = (CObjectiveHoldarea *)GET_OBJ_MGR()->GetObjective(iObjectiveID);

		// just check that we have an objective
		if(pObjective)
 			return !pObjective->PlayersInZone() 
					&& !GET_OBJ_MGR()->IsLastObjective(pObjective) 
					&& BaseClass::IsSpawnPointValid(pSpot, pPlayer);
		else
		{
			DevMsg("Couldn't find Coalition spawn! Objective groups?");
			return false;
		}
	}
	else
		// otherwise, do whatever
		return BaseClass::IsSpawnPointValid(pSpot, pPlayer);

	// just jump down
	return BaseClass::IsSpawnPointValid(pSpot, pPlayer);
}

/**
* Finds all the spawn points in the map
*
* @return void
**/
void CSDKGameRulesSuppression::FindSpawns(void)
{
	CSuppressionStartAmerican *pSpawn;
	int iLargestGroup;

	// do we have them already?
	if(m_aSpawns.Count() > 0)
		return;

	// jump down
	BaseClass::FindSpawns();

	// run through the list and find the different groups
	iLargestGroup = 0;
	for(int i = 0; i < m_aSpawns.Count(); ++i)
	{
		// see if we have an american spawn
		pSpawn = dynamic_cast<CSuppressionStartAmerican *>(m_aSpawns[i]);
		if(!pSpawn)
			continue;

		// is it bigger?
		if(pSpawn->GetSpawnGroup() > iLargestGroup)
			iLargestGroup = pSpawn->GetSpawnGroup();
	}

	// set the number of groups
	m_iSpawnGroups = iLargestGroup;
}

/**
* Finds a valid spawn point
*
* @param CSDKPlayer *pPlayer The player to find a spawn point for
* @return CBaseEntity *
**/
CBaseEntity *CSDKGameRulesSuppression::GetValidSpawnPoint(CSDKPlayer *pPlayer)
{
	CSpawnPoint *pOrigPoint, *pCurrPoint;

	// find all the spawn points
	FindSpawns();

	// american or coalition?
	if(pPlayer->GetTeamNumber() == TEAM_A || pPlayer->GetTeamNumber() == TEAM_B)
	{
		// grab a starting point
		pCurrPoint = pOrigPoint = (pPlayer->GetTeamNumber() == TEAM_A 
										? GetValidAmericanSpawnPoint(pPlayer)
										: GetValidCoalitionSpawnPoint(pPlayer));

		// keep trying points until we find a safe one or get back to where we started
		do
		{
			// bail if we have nothing...
			if(!pCurrPoint)
				return pCurrPoint;

			// is it safe?
			if(!pCurrPoint->EnemiesNearby(pPlayer->GetTeam()))
				return pCurrPoint;

			// move along
			IncrementSpawnGroup(pPlayer->GetTeamNumber());
			pCurrPoint = (pPlayer->GetTeamNumber() == TEAM_A 
										? GetValidAmericanSpawnPoint(pPlayer)
										: GetValidCoalitionSpawnPoint(pPlayer));
		} while(pCurrPoint != pOrigPoint);

		// couldn't find a good one, best of luck dude!
		return pOrigPoint;
	}
	else
		return BaseClass::GetValidSpawnPoint(pPlayer);
}

/**
* Restarts the current game.
*
* @return void
**/
void CSDKGameRulesSuppression::RestartGame(void)
{
	// we haven't captured everything
	m_bFirstRoundComplete = false;
	m_iFirstRoundAACaptures = 0;

	// let the base class take care of most of it
	BaseClass::RestartGame();

	// no warnings please (somewhat counter intuitive)
	m_bFiveMinuteWarning = true;
	m_bOneMinuteWarning = true;

	// re-finalize the objectives as long as the game isn't over and we have people in the game
	// if the game is over then we are reloading the map so who cares?
	if(!g_fGameOver && !s_bFirstPlayer)
	{
		// reset and finalize
		GET_OBJ_MGR()->ResetAllObjectives();
		GET_OBJ_MGR()->DisableTimerNotification();
		GET_OBJ_MGR()->FinalizeObjectives();
	}
}

/** 
* Finds a valid coalition spawn point
*
* @param CSDKPlayer *pPlayer The player to get a spawn point for
* @return CSpawnPoint *
**/
CSpawnPoint *CSDKGameRulesSuppression::GetValidCoalitionSpawnPoint(CSDKPlayer *pPlayer)
{
	CSuppressionStartCoalition *pSpawn;
	int iStartGroup;

	// keep going until we get back to where we started
	iStartGroup = m_iCurrentCoalitionSpawnObjective;
	do
	{
		// find any unoccupied spawn points in the current group
		for(int i = 0; i < m_aSpawns.Count(); ++i)
		{
			// grab the spawn
			pSpawn = dynamic_cast<CSuppressionStartCoalition *>(m_aSpawns[i]);
			if(pSpawn && pSpawn->GetObjectiveID() == m_iCurrentCoalitionSpawnObjective && 
				!pSpawn->IsOccupied() && IsSpawnPointValid(pSpawn, pPlayer))
			{
				return pSpawn;
			}
		}

		// increment
		IncrementSpawnGroup(TEAM_B);

	} while(m_iCurrentCoalitionSpawnObjective != iStartGroup);

	return NULL;
}

/**
* Finds a valid american spawn point
*
* @param CSDKPlayer *pPlayer The player to get a spawn point for
* @return CSpawnPoint *
**/
CSpawnPoint *CSDKGameRulesSuppression::GetValidAmericanSpawnPoint(CSDKPlayer *pPlayer)
{
	CSuppressionStartAmerican *pSpawn;
	int iStartGroup;

	// keep going until we get back to where we started
	iStartGroup = m_iCurrentAmericanSpawnGroup;
	do
	{
		// find any unoccupied spawn points in the current group
		for(int i = 0; i < m_aSpawns.Count(); ++i)
		{
			// if we're in the first spawn then we can spawn at any place with the right name
			if(GET_SUPPRESSION_ROUND()->InFirstSpawn() && IsSpawnPointValid(m_aSpawns[i], pPlayer))
				return (CSpawnPoint *)m_aSpawns[i];

			// grab the spawn
			pSpawn = dynamic_cast<CSuppressionStartAmerican *>(m_aSpawns[i]);
			if(pSpawn && pSpawn->GetSpawnGroup() == m_iCurrentAmericanSpawnGroup && 
				!pSpawn->IsOccupied() && IsSpawnPointValid(pSpawn, pPlayer))
			{
				return pSpawn;
			}
		}

		// increment
		IncrementSpawnGroup(TEAM_A);

	} while(m_iCurrentAmericanSpawnGroup != iStartGroup);

	return NULL;
}

/**
* Increments the current spawn group for the given team
*
* @param int iTeam The number of the team to index
* @return void
**/
void CSDKGameRulesSuppression::IncrementSpawnGroup(int iTeam)
{
	// which team?
	if(iTeam == TEAM_B)
	{
		// are we over?
		if(++m_iCurrentCoalitionSpawnObjective > 2)
			m_iCurrentCoalitionSpawnObjective = 0;
	}
	else if(iTeam == TEAM_A)
	{
		// are we over?
		if(++m_iCurrentAmericanSpawnGroup > m_iSpawnGroups)
			m_iCurrentAmericanSpawnGroup = 0;
	}
}

/**
* Clears out the spawn list
*
* @return void
**/
void CSDKGameRulesSuppression::ClearSpawns(void)
{
	// jump down
	BaseClass::ClearSpawns();

	// clear the spawn groups and randomize the start points
	m_iSpawnGroups = 0;
}

/**
* Determines the action to take whena  player switches teams
* Called after the switch takes place
*
* @param CSDKPlayer *pPlayer The player who switched
* @return void
**/
void CSDKGameRulesSuppression::PlayerChangedTeam(CSDKPlayer *pPlayer)
{
	// force them to change skills and make sure they get new equipment 
	// when they finally pick
	pPlayer->ForceChangeSkill();
	pPlayer->SetNeedsEquipment(true);
}

/**
* Sends the win/lose tip to everyone
*
* @return void
**/
void CSDKGameRulesSuppression::SendWinLoseTip(void)
{
	int iWinner;

	// if the aa won more objectives then they win
	if(GetGlobalTeam(TEAM_A)->GetScore() > m_iFirstRoundAACaptures)
		iWinner = TEAM_A;
	// if the first team managed to capture more the objectives then the coalition wins
	else if(m_iFirstRoundAACaptures > GetGlobalTeam(TEAM_A)->GetScore())
		iWinner = TEAM_B;
	// otherwise, it was a stalemate
	else
		iWinner = 0;
	
	// go through our teams
	for(int i = 1; i <= MAX_TEAMS; ++i)
	{
		// pull the team
		CSDKTeam *pTeam = GetGlobalSDKTeam(i);
		if(pTeam && pTeam->GetNumPlayers() > 0)
		{
			// setup the filter
			CTeamRecipientFilter filter(i);
			filter.MakeReliable();
		
			// set it up
			UserMessageBegin(filter, "HudDisplayMsg");

			// write out the winner
			if (pTeam->GetTeamNumber() == iWinner)
			{
				if (iWinner == TEAM_A)
                    WRITE_STRING("GameWin_TeamA");
				else
					WRITE_STRING("GameWin_TeamB");
			}
			else if (iWinner > 0) {
				if (iWinner == TEAM_A)
					WRITE_STRING("GameLose_TeamB");
				else
					WRITE_STRING("GameLose_TeamA");
			}
			else
				WRITE_STRING("GameDraw");

			// no substitutions
			WRITE_SHORT(0);
			// yes, there is a first sublabel
			WRITE_BYTE(1);

			// did one of the teams win by capturing all the obj in less time?
			if(GetGlobalTeam(TEAM_A)->GetScore() >= mp_teamscorelimit.GetInt() ||
				m_iFirstRoundAACaptures >= mp_teamscorelimit.GetInt())
			{
				// winner or loser?
				if(pTeam->GetTeamNumber() == iWinner)
					WRITE_STRING("GameWinCapturedInLessTime");
				else
					WRITE_STRING("GameLoseCapturedInLessTime");
			}
			// the other option is that the aa didn't manage to capture them in time
			// could be a stalemate if neither team managed to capture all the objectives
			else
			{
				// see if they captured the same number of objectives (stalemate)
				if(m_iFirstRoundAACaptures == GetGlobalTeam(TEAM_A)->GetScore())
				{
					// did anyone capture anything?
					if(m_iFirstRoundAACaptures > 0)
						WRITE_STRING("GameDrawEqualObjectivesSublabel");
					else
						WRITE_STRING("GameDrawNoObjectiveSublabel");
				}
				// neither team captured all but one team captured more
				else
				{
					// winner or loser?
					if(pTeam->GetTeamNumber() == iWinner)
						WRITE_STRING("GameWinMoreObjectivesSublabel");
					else
						WRITE_STRING("GameLoseMoreObjectivesSublabel");
				}
			}
			
			// no substitutions
			WRITE_SHORT(0);

			// no, there isn't a second sublabel
			WRITE_BYTE(0);

			MessageEnd();
		}
	}
}

#endif

/**
* Determines if the player can join the given class
*
* @param CSDKPlayer *pPlayer The player who is trying to join
* @param const char *szSkill The skill they're trying to pick
* @return bool
**/
bool CSDKGameRulesSuppression::CanJoinClass(CSDKPlayer *pPlayer, const char *szSkill)
{
	// check that we have a player
	if(!pPlayer)
		return true;

	// don't have a team? i guess so...
	if(!pPlayer->GetTeam() || !pPlayer->GetTeam()->IsPlayingTeam())
		return true;

	// american?  rangers/snipers
	if(pPlayer->GetTeamNumber() == TEAM_A && (!stricmp(szSkill, "scout") || !stricmp(szSkill, "sniper")))
		return true;
	else if(pPlayer->GetTeamNumber() == TEAM_B && (!stricmp(szSkill, "hw") || !stricmp(szSkill, "soldier")))
		return true;

	// tough luck!
	return false;
}

#endif