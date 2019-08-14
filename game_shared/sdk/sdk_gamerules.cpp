//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "sdk_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "weapon_sdkbase.h"

#ifdef CLIENT_DLL
	#include "hud_macros.h"
#else
	//#include "spawnpoint.h"
	#include "strategymanager.h"
	#include "mapentities.h"
	#include "player_resource.h"
	#include "world.h"
	#include "voice_gamemgr.h"
	#include "team.h"
	#include "weapon_grenade.h"
	#include "weapon_rpg.h"
	#include "gameinterface.h"
	#include "point_template.h"
#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar mp_limitteams;
extern ConVar mp_autoteambalance;
extern ConVar mp_delayteamswitch;
extern ConVar mp_autokick;
extern ConVar mp_autokickidletime;
extern ConVar mp_resetcashonrestart;
extern ConVar mp_docointossonrestart;
extern ConVar mp_roundlimit;
extern ConVar mp_teamscorelimit;
extern ConVar mp_resetxponrestart;
extern ConVar mp_delaynamechange;
extern bool g_bGameRestarted;
extern ConVar mp_limitriflemen;
extern ConVar mp_limitrangers;
extern ConVar mp_limitsnipers;
extern ConVar mp_limitgunners;
extern ConVar sv_pushaway_players;

REGISTER_GAMERULES_CLASS( CSDKGameRules );

BEGIN_NETWORK_TABLE_NOBASE(CSDKGameRules, DT_SDKGameRules)
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( sdk_gamerules, CSDKGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( SDKGameRulesProxy, DT_SDKGameRulesProxy )

#ifndef CLIENT_DLL
	extern CServerGameDLL g_ServerGameDLL;
	extern CPointTemplatePrecacher g_PointTemplatePrecacher;

	// forward declarations
	bool CSDKGameRules::s_bFirstPlayer = false;
#endif

#ifdef CLIENT_DLL
	void RecvProxy_SDKGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CSDKGameRules *pRules = SDKGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CSDKGameRulesProxy, DT_SDKGameRulesProxy )
		RecvPropDataTable( "sdk_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_SDKGameRules ), RecvProxy_SDKGameRules ),
	END_RECV_TABLE()
#else
	void *SendProxy_SDKGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CSDKGameRules *pRules = SDKGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();

		return pRules;
	}

	BEGIN_SEND_TABLE( CSDKGameRulesProxy, DT_SDKGameRulesProxy )
		SendPropDataTable( "sdk_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_SDKGameRules ), SendProxy_SDKGameRules ),
	END_SEND_TABLE()
#endif


#ifdef CLIENT_DLL
	void __MsgFunc_BuyTime(bf_read &msg)
	{
		((CSDKGameRules*)g_pGameRules)->SetBuyTime(gpGlobals->curtime + msg.ReadFloat());
	}
#else

	// --------------------------------------------------------------------------------------------------- //
	// Voice helper
	// --------------------------------------------------------------------------------------------------- //

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker )
		{
			// Dead players can only be heard by other dead team mates
			if ( pTalker->IsAlive() == false )
			{
				if ( pListener->IsAlive() == false )
					return ( pListener->InSameTeam( pTalker ) );

				return false;
			}

			return ( pListener->InSameTeam( pTalker ) );
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;



	// --------------------------------------------------------------------------------------------------- //
	// Globals.
	// --------------------------------------------------------------------------------------------------- //

	// NOTE: the indices here must match TEAM_TERRORIST, TEAM_CT, TEAM_SPECTATOR, etc.
	char *sTeamNames[] =
	{
		//"Unassigned",
		"#Spectators",
		"#TeamA",
		"#TeamB"
	};


	// --------------------------------------------------------------------------------------------------- //
	// Global helper functions.
	// --------------------------------------------------------------------------------------------------- //

	// Helper function to parse arguments to player commands.
	const char* FindEngineArg( const char *pName )
	{
		int nArgs = engine->Cmd_Argc();
		for ( int i=1; i < nArgs; i++ )
		{
			if ( stricmp( engine->Cmd_Argv(i), pName ) == 0 )
				return (i+1) < nArgs ? engine->Cmd_Argv(i+1) : "";
		}
		return 0;
	}


	int FindEngineArgInt( const char *pName, int defaultVal )
	{
		const char *pVal = FindEngineArg( pName );
		if ( pVal )
			return atoi( pVal );
		else
			return defaultVal;
	}

	
	// World.cpp calls this but we don't use it in SDK.
	void InitBodyQue()
	{
	}


	// --------------------------------------------------------------------------------------------------- //
	// CSDKGameRules implementation.
	// --------------------------------------------------------------------------------------------------- //
#endif
	CSDKGameRules::CSDKGameRules()
	{
#ifdef GAME_DLL

		// recover our teams
		RecoverTeams();

		// create the round
		m_pRound = NULL;
		m_iRoundCount = 0;
		m_iGamesPlayed = 0;

#else
		HOOK_MESSAGE(BuyTime);
		gameeventmanager->AddListener(this, "round_ended", false);

		m_fBuyEndTime = 0.0;
#endif
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	CSDKGameRules::~CSDKGameRules()
	{
#ifdef GAME_DLL
		// Note, don't delete each team since they are in the gEntList and will 
		// automatically be deleted from there, instead.
		g_Teams.Purge();

		// kill the round
		if (m_pRound) {
			delete(m_pRound);
			m_pRound = NULL;
		}

			// destroy the objectivemanager
		CObjectiveManager::Term();
		
		// kill teh weapon manager
		CGlobalWeaponManager::Term();

#else
		gameeventmanager->RemoveListener(this);
#endif
	}

#ifdef GAME_DLL
	/**
	* Recovers our team list.  The list will be purge but our
	* teams will not be destroyed on a restart
	*
	* @return void
	**/
	void CSDKGameRules::RecoverTeams(void)
	{
		CSDKTeam *pTeam;
		int iTeamsFound = 0;

		// fill the list with nulls
		g_Teams.Purge();
		for(int i = 0; i < ARRAYSIZE(sTeamNames); ++i)
			g_Teams.AddToTail(NULL);

		// if we restarted there are probably teams hanging around.  see if we can find
		// those first before creating new ones
		pTeam = (CSDKTeam *)gEntList.FindEntityByClassname(NULL, "sdk_team_manager");
		while(pTeam)
		{
			// set the team
			g_Teams[pTeam->GetTeamNumber()] = pTeam;

			// get the next one
			pTeam = (CSDKTeam *)gEntList.FindEntityByClassname(pTeam, "sdk_team_manager");
			++iTeamsFound;
		}

		// see if we have enough
		if(iTeamsFound == 0)
		{
			// clear the list
			g_Teams.Purge();

			// Create the team managers
			for ( int i = 0; i < ARRAYSIZE( sTeamNames ); i++ )
			{
				CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "sdk_team_manager" ));
				pTeam->Init( sTeamNames[i], i );

				g_Teams.AddToTail( pTeam );
			}
		}
		else if(iTeamsFound != ARRAYSIZE(sTeamNames))
			Assert(0);
	}

	//-----------------------------------------------------------------------------
	// Purpose: TF2 Specific Client Commands
	// Input  :
	// Output :
	//-----------------------------------------------------------------------------
	bool CSDKGameRules::ClientCommand( const char *pcmd, CBaseEntity *pEnt )
	{
		// get the player
		CSDKPlayer *pPlayer = (CSDKPlayer *) pEnt;

		// try to handle it
		if(!strcmp(pcmd, "chooseobjective"))
		{
			// check the args then tell the round
			if(engine->Cmd_Argc() == 2)
				GET_OBJ_MGR()->SetCurrentObjective(pPlayer, atoi(engine->Cmd_Argv(1)));

			return true;
		}
		else if (FStrEq(pcmd, "choosestrategy") || FStrEq(pcmd, "choosestrategyblock"))
		{
			bool bTransmit;

			// make sure we have enough arguments
			if (engine->Cmd_Argc() >= 3)
			{
				// send it to the strategy manager
				bTransmit = atoi(engine->Cmd_Argv(1)) == 0 ? false : true;
				GetStrategyManager()->ProcessChooseStrategy(pPlayer, 
															engine->Cmd_Args(), 
															bTransmit ? atoi(engine->Cmd_Argv(2)) : 0,
															FStrEq(pcmd, "choosestrategyblock"));
			}
		}
		else if (!pPlayer->ClientCommand( pcmd ))
			return BaseClass::ClientCommand( pcmd, pEnt );

		return true;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Player has just spawned. Equip them.
	//-----------------------------------------------------------------------------
	void CSDKGameRules::PlayerSpawn( CBasePlayer *pBasePlayer )
	{
		CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer*>(pBasePlayer);
		CWeaponSDKBase *pWpn;

		// Melee weapon?
		if (!pPlayer->GetMeleeWeapon())
			// Give them a knife
			pPlayer->GiveNamedItem( "weapon_knife" );

		// Secondary Weapon?
		pWpn = pPlayer->GetSecondaryWeapon();
		if (pWpn) {
			// Fill 'r up
			pWpn->FillAmmo();
		}
		else {
			// No secondary weapon, give 'm a glock
			pWpn = (CWeaponSDKBase*) pPlayer->GiveNamedItem( "weapon_viking" );
			// Fill 'r up
			if (pWpn)
				pWpn->FillAmmo();
		}

		// Primary weapon?
		pWpn = pPlayer->GetPrimaryWeapon();
		// @JD: this is a hack/fix for someone who tries to save their RPG from last round
		//		in order to get the ammo refilled for free.
		if (pWpn && pWpn->GetWeaponID() == WEAPON_RPG && pWpn->GetAmmo1() == 0) {
			pPlayer->Weapon_Drop(pWpn);
			UTIL_Remove(pWpn);
		}
		// Fill 'r up
		else if (pWpn)
			pWpn->FillAmmo();

		// tell them about the round
		if(GetCurrentRound()->IsFirstRound())
		{
			// only send to this player
			CSingleUserRecipientFilter filter(pPlayer);
			filter.MakeReliable();

			// send the message, no replacements, no sublabel
			UserMessageBegin(filter, "HudDisplayMsg");
			WRITE_STRING("CoinToss");
			WRITE_SHORT(0);
			WRITE_BYTE(0);
			WRITE_BYTE(0);
			MessageEnd();
		}

		// make sure we have enough experience based on the number of games we've played
		if(pPlayer->GetXP() < MinXPForGame())
			pPlayer->ModifyXP(MinXPForGame() - pPlayer->GetXP(), true);

		BaseClass::PlayerSpawn( pBasePlayer );
	}

	/**
	* Determines the minimum number of xp points a player should have in the current game
	*
	* @return int
	**/
	int CSDKGameRules::MinXPForGame(void)
	{
		// how many games have we played?
		if(m_iGamesPlayed == 0)
			return 0;
		else if(m_iGamesPlayed == 1)
			return SKILL_LEVEL_1;
		else if(m_iGamesPlayed == 2)
			return SKILL_LEVEL_2;
		else if(m_iGamesPlayed == 3)
			return SKILL_LEVEL_3;
		else
			return SKILL_LEVEL_4;
	}

	/**
	* Determines if the given player can spawn
	*
	* @param CBasePlayer *pPlayer The player who is trying to spawn
	* @return bool
	**/
	bool CSDKGameRules::CanSpawn(CBasePlayer *pPlayer)
	{
		// do we have a round yet?
		if(!GetCurrentRound())
			return false;
		
		// consult the round
		return GetCurrentRound()->CanSpawn(pPlayer);
	}

	void CSDKGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore )
	{
		RadiusDamage( info, vecSrcIn, flRadius, iClassIgnore, false );
	}

	// Add the ability to ignore the world trace
	void CSDKGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, bool bIgnoreWorld )
	{
		CBaseEntity *pEntity = NULL;
		trace_t		tr;
		float		flAdjustedDamage, falloff;
		Vector		vecSpot;
		Vector		vecToTarget;
		Vector		vecEndPos;

		Vector vecSrc = vecSrcIn;

		if ( flRadius )
			falloff = info.GetDamage() / flRadius;
		else
			falloff = 1.0;

		int bInWater = (UTIL_PointContents ( vecSrc ) & MASK_WATER) ? true : false;
		
		vecSrc.z += 1;// in case grenade is lying on the ground

		// iterate on all entities in the vicinity.
		for ( CEntitySphereQuery sphere( vecSrc, flRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
		{
			if ( pEntity->m_takedamage != DAMAGE_NO )
			{
				// UNDONE: this should check a damage mask, not an ignore
				if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
				{// houndeyes don't hurt other houndeyes with their attack
					continue;
				}

				// blast's don't tavel into or out of water
				if (bInWater && pEntity->GetWaterLevel() == 0)
					continue;
				if (!bInWater && pEntity->GetWaterLevel() == 3)
					continue;

				// radius damage can only be blocked by the world
				vecSpot = pEntity->BodyTarget( vecSrc );



				bool bHit = false;

				if( bIgnoreWorld )
				{
					vecEndPos = vecSpot;
					bHit = true;
				}
				else
				{
					UTIL_TraceLine( vecSrc, vecSpot, MASK_SOLID_BRUSHONLY, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

					if (tr.startsolid)
					{
						// if we're stuck inside them, fixup the position and distance
						tr.endpos = vecSrc;
						tr.fraction = 0.0;
					}

					vecEndPos = tr.endpos;

					if( tr.fraction == 1.0 || tr.m_pEnt == pEntity )
					{
						bHit = true;
					}
				}

				if ( bHit )
				{
					// the explosion can 'see' this entity, so hurt them!
					//vecToTarget = ( vecSrc - vecEndPos );
					vecToTarget = ( vecEndPos - vecSrc );

					// decrease damage for an ent that's farther from the bomb.
					flAdjustedDamage = vecToTarget.Length() * falloff;
					flAdjustedDamage = info.GetDamage() - flAdjustedDamage;
				
					if ( flAdjustedDamage > 0 )
					{
						CTakeDamageInfo adjustedInfo = info;
						adjustedInfo.SetDamage( flAdjustedDamage );

						Vector dir = vecToTarget;
						VectorNormalize( dir );

						// If we don't have a damage force, manufacture one
						if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
						{
							CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc, 1.5	/* explosion scale! */ );
						}
						else
						{
							// Assume the force passed in is the maximum force. Decay it based on falloff.
							float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
							adjustedInfo.SetDamageForce( dir * flForce );
							adjustedInfo.SetDamagePosition( vecSrc );
						}

						pEntity->TakeDamage( adjustedInfo );
			
						// Now hit all triggers along the way that respond to damage... 
						pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, vecEndPos, dir );
					}
				}
			}
		}
	}

	/**
	* Let the game rules do some major pondering
	*
	* @return void
	**/
	void CSDKGameRules::Think()
	{
		// hand it off to the base class
		BaseClass::Think();

		// is the game over?
		if(IsGameOver())
			GoToIntermission();

		// if the game is over just bail
		if(g_fGameOver)
			return;

		// let the round think
		GetCurrentRound()->Think();

		// clean up our list
		CGlobalWeaponManager::GetInstance()->CleanWeapons();
	}

	/**
	* Starts the intermission between levels
	*
	* @return void
	**/
	void CSDKGameRules::GoToIntermission(void)
	{
		// is the game over already?
		if(g_fGameOver)
			return;

		// go down
		BaseClass::GoToIntermission();

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

		SendWinLoseTip();
	}

	void CSDKGameRules::SendWinLoseTip(void)
	{
		int iCounts[MAX_TEAMS+1];
		int iWinner = 0;

		for(int i = 1; i <= MAX_TEAMS; ++i)
			iCounts[i] = GetGlobalTeam(i)->GetScore();

		if (iCounts[TEAM_A] > iCounts[TEAM_B])
			iWinner = TEAM_A;
		else if (iCounts[TEAM_B] > iCounts[TEAM_A])
			iWinner = TEAM_B;

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

				if (pTeam->GetTeamNumber() == iWinner) {
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

				// game ended because of round limit
				if (mp_roundlimit.GetInt() > 0 && m_iRoundCount >= mp_roundlimit.GetInt()) {
					// win
					if (pTeam->GetTeamNumber() == iWinner)
						WRITE_STRING("GameWinMoreCapsAtRoundLimitSublabel");
					// lose
					else if (iWinner > 0)
						WRITE_STRING("GameLostMoreCapsAtRoundLimit");
					// draw
					else
						WRITE_STRING("GameDrawEqualCapsAtRoundLimit");

					// 1 substitution
					WRITE_SHORT(1);

					char buf[4];
					Q_snprintf(buf, 3, "%d", mp_roundlimit.GetInt());

					// send it as the replacement
					WRITE_STRING(buf);
				}
				// default setting, and it was either team caps or time limit
				if (mp_teamscorelimit.GetInt() == 3) {
					// we won
					if (pTeam->GetTeamNumber() == iWinner) {
						// they had no objectives?
						if (iCounts[iWinner] == 3)
							WRITE_STRING("GameWinAllObjectivesSublabel");
						// they had an objective
						else
							WRITE_STRING("GameWinMoreObjectivesSublabel");
					}
					// we lost
					else if (iWinner > 0) {
						// we lost with no objectives
						if (iCounts[iWinner] == 3)
							WRITE_STRING("GameLoseAllObjectivesSublabel");
						// we lost with an objective
						else
							WRITE_STRING("GameLoseMoreObjectivesSublabel");
					}
					// no winner
					else {
						// no winner with no objectives
						if (iCounts[pTeam->GetTeamNumber()] == 0)
							WRITE_STRING("GameDrawNoObjectivesSublabel");
						// no winner with some objectives
						else
							WRITE_STRING("GameDrawEqualObjectivesSublabel");
					}

					// no substitutions
					WRITE_SHORT(0);
				}
				// game ended because of team caps
				else if (mp_teamscorelimit.GetInt() >= iCounts[iWinner]) {
					// we won
					if (pTeam->GetTeamNumber() == iWinner)
						WRITE_STRING("GameWinGotRequiredCapsSublabel");
					// we lost (there is no draw condition in this case)
					else
                        WRITE_STRING("GameLostGotRequiredCapsSublabel");
					
					// 1 substitution
					WRITE_SHORT(1);

					// write our team caps limit to a string buffer
					char buf[4];
					Q_snprintf(buf, 3, "%d", mp_teamscorelimit.GetInt());

					// send it as the replacement
					WRITE_STRING(buf);
				}
				// game ended because of time limit
				else {
					// win
					if (pTeam->GetTeamNumber() == iWinner)
						WRITE_STRING("GameWinMoreCapsAtTimeLimitSublabel");
					// lose
					else if (iWinner > 0)
						WRITE_STRING("GameLostMoreCapsAtTimeLimitSublabel");
					// draw
					else
						WRITE_STRING("GameDrawEqualCapsAtTimeLimitSublabel");

					// no substitutions
					WRITE_SHORT(0);
				}

				// no, there isn't a second sublabel
				WRITE_BYTE(0);

				MessageEnd();
			}
		}
	}

	/**
	* Determines if the game is over
	*
	* @return bool
	**/
	bool CSDKGameRules::IsGameOver(void)
	{
		// we're done if we've exceeded the number of games we intended to play
		// this is subordinate to any of the other over conditions
		if(mp_gamestoplay.GetInt() <= m_iGamesPlayed && mp_roundlimit.GetInt() <= 0 &&
			mp_teamscorelimit.GetInt() == 3)
		{
			return true;
		}

		if (mp_roundlimit.GetInt() > 0)
		{
			if (m_iRoundCount >= mp_roundlimit.GetInt())
				return true;
		}

		if (mp_teamscorelimit.GetInt() > 0 && mp_gamestoplay.GetInt() == 1)
		{
			for(int i = TEAM_A; i <= TEAM_B; ++i)
			{
				if (GetGlobalTeam(i)->GetScore() >= mp_teamscorelimit.GetInt())
					return true;
			}
		}

		return false;
	}

	/**
	* Restarts the map
	* 
	* @return void
	**/
	void CSDKGameRules::RestartMap(void)
	{
		CBaseEntity *pEnt;
		CCleanMapEntityFilter filter;

		// kill off everything
		for(pEnt = gEntList.FirstEnt(); pEnt; pEnt = gEntList.NextEnt(pEnt))
		{
			// clear out any decals
			//pEnt->RemoveAllDecals();

			// if we are recreating this entity we can remove it
			if(!FindInList(s_szPreserveList, pEnt->GetClassname()))
				UTIL_Remove(pEnt);
		}

		// clear out anything we don't need
		gEntList.CleanupDeleteList();

		// reload all the entities
		filter.m_iIterator = g_MapEntityRefs.Head();
     	MapEntity_ParseAllEntities(engine->GetMapEntitiesString(), &filter, true);

		// reset the objective manager
		CObjectiveManager::Term();
	}

	/**
	* Restarts the current game.
	*
	* @return void
	**/
	void CSDKGameRules::RestartGame(void)
	{
		IGameEvent *pEvent;

		// @EVIL - The order of this is extremely fragile. Handle with care.

		// restart the map as long as the game isn't over and we have people in the game
		// if the game is over then we are reloading the map so who cares?
		if(!g_fGameOver && !s_bFirstPlayer && !g_bGameRestarted)
		{
			// restart the map
			RestartMap();

			// remember that we're restarting the game so we can install
			// new game rules
			g_bGameRestarted = true;
		}

		// reset the spawn info
		ClearSpawns();

		// initialize the game state
		InitGameState();

		// resets the teams
		ResetTeams();

		// reset the score info
		g_pPlayerResource->Reset();

		// restart the objective manager
		GET_OBJ_MGR()->DisableTimerNotification();
		CObjectiveManager::Reset();

		// @TRJ - moved up from below !s_bFirstPlayer.  need to send reset before players spawn so
		// we can get coin toss.  we'll see if it breaks anything
		// tell the hud display message to reset
		CReliableBroadcastRecipientFilter filter;
		UserMessageBegin(filter, "ResetHudDisplayMsg");
		MessageEnd();

		// set the first round
		GetCurrentRound()->SetFirstRound(true);

		// start the round over
		GetCurrentRound()->Restart(/*false*/);

		// send out our event if this wasn't the result of the first player joining
		if(!s_bFirstPlayer)
		{
			pEvent = gameeventmanager->CreateEvent("game_start");
			if(pEvent)
				// send it out
				gameeventmanager->FireEvent(pEvent, true);
		}

		// go down
		BaseClass::RestartGame();
	}

	/**
	 * Resets all the objectives, allowing infinite replayability without a mapchange.
	 */
	void CSDKGameRules::ResetGame(void)
	{
		// reset all map entities
		if (!s_bFirstPlayer)
			RestartMap();

		// reset the spawn info
		ClearSpawns();

		InitGameState();

		// reset the obj mgr...
		CObjectiveManager::Reset();

		// ?
		GET_OBJ_MGR()->EstablishObjectives();
		GET_OBJ_MGR()->ResetAllObjectives();

		// do team stuff
		for(int i = FIRST_TEAM; i <= LAST_TEAM; ++i) 
		{
			if (mp_resetxponrestart.GetBool())
				GetGlobalSDKTeam(i)->ResetXP();
		}

		for(int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			CSDKPlayer *pPlayer = (CSDKPlayer*)UTIL_PlayerByIndex(i);

			if (pPlayer) {
				if (mp_resetxponrestart.GetBool()) {
                    pPlayer->ModifyXP(-1 * MAX_XP);
				}
				if (mp_resetcashonrestart.GetBool()) {
					pPlayer->ModifyCash(-1 * MAX_CASH);
					// take all their stuff
					pPlayer->RemoveAllItems(false);

					// give them their $2000 if we're not doing coin toss
					if (!mp_docointossonrestart.GetBool())
						pPlayer->ModifyCash(END_OF_FIRST_ROUND_CASH);
				}
			}
		}

		if (mp_docointossonrestart.GetBool())
			GetCurrentRound()->SetFirstRound(true);

		//GetCurrentRound()->Restart();
	}

	void CSDKGameRules::ChangeLevel(void)
	{
		// @JD: no need. We get deleted in a few ticks anyway
		//RestartGame();

		BaseClass::ChangeLevel();
	}

	/**
	* Initializes the game state
	*
	* @return void
	**/
	void CSDKGameRules::InitGameState(void)
	{
		// reset the gamestate and morale
		for(int i = 1; i <= MAX_TEAMS; ++i)
		{
			// set the gamestate
			for(int j = 0; j < 3; ++j)
				m_bGameState[i][j] = (j % 2 == 0 ? false : true);

			// start at five ( because 10 / 2 = 5 )
			m_iMorale[i - 1] = 5;
		}
	}

	/**
	* Resets the teams
	*
	* @return void
	**/
	void CSDKGameRules::ResetTeams(void)
	{
		CSDKTeam *pTeam;

		// go through our teams
		for(int i = 1; i <= MAX_TEAMS; ++i)
		{
			// get the team
			pTeam = GetGlobalSDKTeam(i);

			// did we get them?
			if(pTeam)
			{
				// reset the players
				pTeam->Reset();
			}
		}
	}

	/**
	* Returns whether or not we are in freeze time
	*
	* @return bool
	**/
	bool CSDKGameRules::IsPlayerFrozen(CSDKPlayer *pPlayer)
	{
		// do we have the round?
		if(!GetCurrentRound())
			return false;

		return GetCurrentRound()->IsPlayerFrozen(pPlayer);
	}

	bool CSDKGameRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
	{
		return false;
	}

	/**
	 * Returns true/false if we can have an item we find.
	 * This avoids having to reimplement CBasePlayer::BumpWeapon()
	 */
	bool CSDKGameRules::CanHavePlayerItem( CBasePlayer *pBasePlayer, CBaseCombatWeapon *pItem )
	{
		CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer*>(pBasePlayer);

		if (pPlayer)
			return (pPlayer->CanHavePlayerItem(pItem) && BaseClass::CanHavePlayerItem(pBasePlayer, pItem));
		else
			return BaseClass::CanHavePlayerItem(pBasePlayer, pItem);
	}

	/**
	* Determines if players can still buy items
	*
	* @return bool
	**/
	bool CSDKGameRules::InBuyTime(void)
	{
		// do we have a round?
		if(!GetCurrentRound())
			return false;

		// ask the round
		return GetCurrentRound()->InBuyTime();
	}

	/**
	* Updates team morale based on the current game state
	* The game state is maintained by UpdateGameState in CSDKGameRules
	*
	* @return void
	**/
	void CSDKGameRules::UpdateMorale(void)
	{
		for(int iTeam = 0; iTeam < MAX_TEAMS; ++iTeam) {
			int iMorale = m_iMorale[iTeam];

			// Won last 3 rounds
			if (IsGameState(iTeam, true, true, true)) {
				if (iMorale > 8)
					iMorale -= 2;
				else if (iMorale > 5)
					iMorale -= 1;
			}
			// Won last round and one other in past 3
			else if (IsGameState(iTeam, false, true, true) || IsGameState(iTeam, true, false, true))
				iMorale += 2;
			// Won this one after losing two
			else if (IsGameState(iTeam, false, false, true))
				iMorale += 3;
			// Lost last round after winning round before, or lost last two rounds
			else if (IsGameState(iTeam, false, true, false) || IsGameState(iTeam, true, true, false) || 
					IsGameState(iTeam, true, false, false)) 
			{
				if (iMorale > 8)
					iMorale -= 3;
				else if (iMorale > 4)
					iMorale -= 2;
				else
					iMorale -= 1;
			}
			// Lost three rounds in a row
			else if (IsGameState(iTeam, false, false, false)) {
				if (iMorale < 5)
					iMorale += 1;
				else
					iMorale -= 1;
			}

			// Cap it at [-1, 10]
			iMorale = clamp(iMorale, 1, 10);

			// save our changes
			m_iMorale[iTeam] = iMorale;
		}
	}

	/**
	* Determines if the game state is equivalent to the specified set
	*
	* @param int iTeam The team we need to check the state for
	* @param bool bWon1 Did we win the first game?
	* @param bool bWon2 Did we win the second game?
	* @param bool bWon3 Did we win the third game?
	* @return bool
	**/
	bool CSDKGameRules::IsGameState(int iTeam, bool bWon1, bool bWon2, bool bWon3)
	{
		return (m_bGameState[iTeam][0] == bWon1 &&
				m_bGameState[iTeam][1] == bWon2 &&
				m_bGameState[iTeam][2] == bWon3);
	}

	/**
	* Updates the current game state
	*
	* @param CSDKTeam *pTeam The team that won the current round
	* @return void
	**/
	void CSDKGameRules::UpdateGameState(CSDKTeam *pWinner)
	{
		// do we have a winner?
		if(!pWinner)
			return;

		// move forward the game state for each team
		for(int i = 0; i < MAX_TEAMS; ++i)
		{
			// did the current team win this round?
			bool bThisRound = (pWinner->GetTeamNumber() == i);

			// move the game state along
			m_bGameState[i][0] = m_bGameState[i][1];
			m_bGameState[i][1] = m_bGameState[i][2];
			m_bGameState[i][2] = bThisRound;
		}
	}

	/**
	* Determines the morale award for the current round for the given team
	*
	* @param CSDKTeam *pTeam The team to award
	* @return int
	**/
	int CSDKGameRules::GetMoraleXPAward(CSDKTeam *pTeam)
	{
		return (float)m_iMorale[pTeam->GetTeamNumber() - 1] * .75;
	}

	/**
	* Cleans the map of any left over entities (weapons, bodies, etc)
	*
	* @return void
	**/
	void CSDKGameRules::CleanMap(void)
	{
		CBaseEntity *pEntity;
		CWeaponSDKBase *pWpn;
		CSDKRagdoll *pDoll;

		// start searching for guns
		pEntity = NULL;
		while((pEntity = gEntList.FindEntityByClassname(pEntity, "weapon_*")) != NULL)
		{
			// try to get an sdk weapon
			pWpn = dynamic_cast<CWeaponSDKBase *>(pEntity);

			// did we get it?
			if(pWpn)
			{
				// If the weapon is not attached to a player
				if (!pWpn->GetPlayerOwner())
					// make it go away
					UTIL_Remove(pWpn);

				/*
				// do we have an owner?
				if(pWpn->GetPlayerOwner() && pWpn->GetPlayerOwner()->IsDead())
					// let go of the weapon
					pWpn->GetPlayerOwner()->Weapon_Drop(pWpn);
				*/
			}
		}

		// search for bodies
		pEntity = NULL;
		while((pEntity = gEntList.FindEntityByClassname(pEntity, "sdk_ragdoll")) != NULL)
		{
			// try to get a ragdoll
			pDoll = dynamic_cast<CSDKRagdoll *>(pEntity);

			// did we get it?
			if(pDoll)
			{
				// "crush, kill, destroy!" - gwar
				UTIL_Remove(pDoll);
			}
		}

		// search for hats that are on the ground
		pEntity = NULL;
		while((pEntity = gEntList.FindEntityByClassname(pEntity, "attached_model")) != NULL)
		{
			// make sure it's not attached to anyone
			if(!((CAttachedModel *)pEntity)->IsAttached())
				UTIL_Remove(pEntity);
		}

		// search for grenades
		pEntity = NULL;
		while((pEntity = gEntList.FindEntityByClassname(pEntity, "grenade_projectile")) != NULL)
			UTIL_Remove(pEntity);

		// search for rockets
		pEntity = NULL;
		while((pEntity = gEntList.FindEntityByClassname(pEntity, "rpg_missile")) != NULL)
			UTIL_Remove(pEntity);

		// and the decals?
		// @TODO - TRJ - anyone have an idea of how to do this?
	}

	/**
	* Determines if we are in the first round
	*
	* @return bool
	**/
	bool CSDKGameRules::IsFirstRound(void)
	{
		// do we have a round?
		if(GetCurrentRound())
			return GetCurrentRound()->IsFirstRound();

		return false;
	}

	/**
	* Determines if the listener can hear the speaker
	* Teamplay takes care of whether or not we're on the same team.
	* All I care about is that we are in the same life state
	*
	* @param CBasePlayer *pListener The ears
	* @param CBasePlayer *pSpeaker The mouth
	* @return bool Is is possible to get from mouth to ears?
	**/
	bool CSDKGameRules::PlayerCanHearChat(CBasePlayer *pListener, CBasePlayer *pSpeaker)
	{
		// check that they are in the same life state
		if(pListener->IsAlive() != pSpeaker->IsAlive())
			return false;

		return BaseClass::PlayerCanHearChat(pListener, pSpeaker);
	}

	/**
	* Creates a new round
	*
	* @return CRound *
	**/
	CRound *CSDKGameRules::GetNewRound(void)
	{
		return new CRound();
	}

#endif


bool CSDKGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
	}
	
	// @TEMP TRJ - making players non-solid to one another
	if(sv_pushaway_players.GetBool() && 
		(collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		(collisionGroup1 == COLLISION_GROUP_PLAYER || collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT))
	{
		return false;
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_PUSHAWAY )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS && collisionGroup1 == COLLISION_GROUP_PUSHAWAY )
	{
		// let debris and multiplayer objects collide
		return true;
	}

	//Don't stand on COLLISION_GROUP_WEAPON
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

//-----------------------------------------------------------------------------
// Purpose: Init CS ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

 	if ( !bInitted )
	{
		bInitted = true;
		
		// def.AddAmmoType( BULLET_PLAYER_50AE,		DMG_BULLET, TRACER_LINE, 0, 0, "ammo_50AE_max",		2400, 0, 10, 14 );
		//def.AddAmmoType( AMMO_GRENADE, DMG_BLAST, TRACER_LINE, 0, 0,	2/*max carry*/, 1, 0 );
		//def.AddAmmoType( AMMO_BULLETS, DMG_BULLET, TRACER_LINE, 0, 0,	200, 1, 0 );
	}

	return &def;
}


#ifndef CLIENT_DLL

/**
* Grabs the current round
*
* @return CRound *
**/
CRound *CSDKGameRules::GetCurrentRound(void)
{
	// grab the new round if we don't have one
	if(!m_pRound)
		m_pRound = GetNewRound();

	return m_pRound;
}

const char *CSDKGameRules::GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer )
{
	// ?
	if (!pPlayer)
		return "(Server)";

	if (bTeamOnly) {
		if (pPlayer->IsAlive())
			return "(TEAM)";
		else
			return "(DEAD) (TEAM)";
	}

	if(!pPlayer->IsAlive())
		return "(DEAD)";

	return "";
}

void CSDKGameRules::ClientSettingsChanged( CBasePlayer *pBasePlayer )
{
	CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer*>(pBasePlayer);

	if (!pPlayer)
		return;

	if (pPlayer->GetPlayerName() && !pPlayer->IsAlive() && mp_delaynamechange.GetBool()) {
		// get their name
		const char *name = engine->GetClientConVarValue( pPlayer->entindex(), "name" );
		if (strcmp(name, pPlayer->GetPlayerName())) {
			// save their request
			pPlayer->SetRequestedName(name);
			// set it back on the client, so that their name variable is still correct
			engine->ClientCommand( pPlayer->edict(), "name %s\n", pPlayer->GetPlayerName() );
			// tell the person that their request will go through
			ClientPrint( pPlayer, HUD_PRINTTALK, "Your name will be changed at the end of the round.\n" );
		}
	}
	else
		BaseClass::ClientSettingsChanged(pPlayer);
}

// Called when the round ends
void CSDKGameRules::RoundEnded(void)
{
	m_iRoundCount++;

    RemoveIdlePlayers();
	UpdateTeamScores();
	PrepareAutoTeamBalance();
	AnnounceAutoTeamBalance();
}

// Called when the round [re]starts
void CSDKGameRules::RoundRestart(void)
{
	bool bRestartingGame;

	// if all the objectives are captured we're done with this game
	bRestartingGame = false;
	if (GET_OBJ_MGR()->AllCaptured())
	{
		// are we playing more than one game? and not following any other game finishing rules?
		if(mp_roundlimit.GetInt() <= 0 &&
			mp_teamscorelimit.GetInt() == 3)
		{
			// do we need to play another game?
			if(mp_gamestoplay.GetInt() > m_iGamesPlayed)
			{
				int iCurTime = gpGlobals->curtime;

				// swap the teams and restart the game
				SwapTeams();
				RestartGame();
				bRestartingGame = true;

				// @HAX - DO NOT INSTALL NEW GAME RULES. Thanks!
				g_bGameRestarted = false;
				// @HAX - do not reset the time limit
				gpGlobals->curtime = iCurTime;
			}
			else
				// just bail, the game should be over
				return;
		}
		else
			ResetGame();
	}

	// clear out the map, allow players to switch teams, and then balance
	CleanMap();
	ProcessTeamChangeRequests();
	if(!bRestartingGame)
		ProcessAutoTeamBalance();

	// clear our weapon list
	CGlobalWeaponManager::GetInstance()->Reset();
}

/**
* Swaps the players between teams
*
* @return void
**/
void CSDKGameRules::SwapTeams(void)
{
	CSDKPlayer *pPlayer;
	CSDKTeam *pTeamA, *pTeamB;
	CUtlVector<CBasePlayer *> aTeamB;

	// stop any auto balance from occurring
	m_AutoBalancePlayers.Purge();

	// pull the team
	pTeamA = GetGlobalSDKTeam(TEAM_A);
	pTeamB = GetGlobalSDKTeam(TEAM_B);
	if(!pTeamB || !pTeamA)
	{
		Assert(0);
		return;
	}

	// pull the current list on B
	aTeamB = *(pTeamB->GetPlayers());

	// put everyone on A into B
	while(pTeamA->GetNumPlayers())
	{
		// pull the player and change teams
		pPlayer = ToSDKPlayer(pTeamA->GetPlayer(0));
		if(pPlayer)
			pPlayer->ChangeTeam(TEAM_B, true);
	}

	// now put everyone on B into A
	for(int i = 0; i < aTeamB.Count(); ++i)
	{
		// pull the player and change teams
		pPlayer = ToSDKPlayer(aTeamB[i]);
		if(pPlayer)
			pPlayer->ChangeTeam(TEAM_A, true);
	}
}

void CSDKGameRules::RemoveIdlePlayers()
{
	if (!mp_autokick.GetBool())
		return;

	int iIdleTime = mp_autokickidletime.GetInt();

	for(int i = 1; i <= gpGlobals->maxClients; ++i) {
		CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer*>(UTIL_PlayerByIndex(i));
		if (pPlayer) {
			// don't kick idle spectators
			if (pPlayer->GetTeamNumber() == TEAM_SPECTATOR)
				continue;

			if (pPlayer->GetLastInputTime() + iIdleTime < gpGlobals->curtime) {
				// kick me.
				char kickStr[81];
				Q_snprintf(kickStr, 80, "kickid %d Kicked for being idle %d seconds\n", pPlayer->GetUserID(), iIdleTime);
				engine->ServerCommand(kickStr);
			}
		}
	}
}

void CSDKGameRules::UpdateTeamScores()
{
	if (!GET_OBJ_MGR())
		return;

	int iCounts[3];
	GET_OBJ_MGR()->GetObjectiveCounts(iCounts);

	for(int i = TEAM_A; i <= TEAM_B; ++i) {
        CSDKTeam *pTeam = GetGlobalSDKTeam(i);
		if (pTeam) {
			if (iCounts[i] == 3) {
				pTeam->AddPermanentScore(3);
				pTeam->SetScore(0);
			}
			else
				pTeam->SetScore(iCounts[i]);
		}
	}
}

bool CSDKGameRules::CanJoinTeam(CSDKPlayer *pPlayer, int iTeamNum)
{
	// can always join the spectator team
	if (iTeamNum == TEAM_SPECTATOR)
		return true;

	Assert(!(iTeamNum > MAX_TEAMS || iTeamNum < 0));

	int iTeamLimit = mp_limitteams.GetInt();
	if (iTeamLimit > 0) {
		int iCounts[MAX_TEAMS + 1];
		GetTeamCounts(iCounts);

		// leaving my team
		if (pPlayer->GetTeam() && pPlayer->GetTeam()->IsPlayingTeam() && iCounts[pPlayer->GetTeamNumber()] > 0)
			iCounts[pPlayer->GetTeamNumber()]--;

		// joining new team
		iCounts[iTeamNum]++;

		if (abs(iCounts[TEAM_A] - iCounts[TEAM_B]) > iTeamLimit) {
			if (iCounts[TEAM_A] < iCounts[TEAM_B] && iTeamNum == TEAM_A)
				return true;
			else if (iCounts[TEAM_B] > iCounts[TEAM_A] && iTeamNum == TEAM_B)
				return true;
			else
				return false;
		}

		return true;
	}

	return true;
}

// make sure to pass a big enough array (MAX_TEAMS + 1)
void CSDKGameRules::GetTeamCounts(int iCounts[])
{
	iCounts[TEAM_A] = GetGlobalTeam(TEAM_A)->GetNumPlayers();
	iCounts[TEAM_B] = GetGlobalTeam(TEAM_B)->GetNumPlayers();

	// consider all requests before deciding what to do
	if (mp_delayteamswitch.GetBool())
	{
		// get each player
		for(int i = 1; i <= gpGlobals->maxClients; ++i) 
		{
			CSDKPlayer *pPlayer = (CSDKPlayer*)UTIL_PlayerByIndex(i);
			
			// did we get one?
			if (pPlayer && pPlayer->IsConnected()) {
				// get their request
				int iRequest = pPlayer->GetChangeTeamRequest();
				// if they have a request and its valid...
				if (iRequest > 0 && iRequest <= MAX_TEAMS) {
					// change our counts
					iCounts[iRequest]++;
					iCounts[pPlayer->GetTeamNumber()]--;
				}
			}
		}
	}

	if (m_AutoBalancePlayers.Count() > 0)
	{
		iCounts[m_iAutoBalanceTeam] += m_AutoBalancePlayers.Count();
		iCounts[GET_OTHER_TEAM(m_iAutoBalanceTeam)] -= m_AutoBalancePlayers.Count();
	}
}

// Rewriting this using TeamCounts since TeamCounts handles requests and autobalancing.
int CSDKGameRules::TeamWithFewestPlayers(void)
{
	int iCounts[3];
	GetTeamCounts(iCounts);

	if (iCounts[TEAM_A] == iCounts[TEAM_B])
		return (random->RandomInt(0, 1) == 0 ? TEAM_A : TEAM_B);

	return (iCounts[TEAM_A] > iCounts[TEAM_B] ? TEAM_B : TEAM_A);
}

void CSDKGameRules::PrepareAutoTeamBalance(void)
{
	if (!mp_autoteambalance.GetBool())
		return;

	// If autobalance is on, it will balance the teams. Don't care what mp_limitteams is.
	int iCounts[MAX_TEAMS + 1];
	GetTeamCounts(iCounts);

	int iDiff = abs(iCounts[TEAM_A] - iCounts[TEAM_B]);
	if (iDiff > 1) {
		CBasePlayer *pPlayer;
		CTeam *pTeam;
		int iNumToMove;

		// set the team we're going to switch players to
		m_iAutoBalanceTeam = (iCounts[TEAM_A] > iCounts[TEAM_B] ? TEAM_B : TEAM_A);
		// the team we're going to get players FROM is the other team
		pTeam = GetGlobalTeam(GET_OTHER_TEAM(m_iAutoBalanceTeam));

		// prevent a crash ??? don't know why this would ever happen...
		Assert(pTeam);
		if (!pTeam)
			return;

		iNumToMove = iDiff / 2;	// will be at least 1
		while(iNumToMove > 0)
		{
			// grab a random player
			pPlayer = pTeam->GetRandomPlayer();
            // check to see if we already have this player
			if (pPlayer && pPlayer->IsConnected() && !m_AutoBalancePlayers.HasElement(pPlayer->entindex()))
			{
				// we don't, so add this one
				m_AutoBalancePlayers.AddToTail(pPlayer->entindex());
                iNumToMove--;
			}
		}
	}
}

void CSDKGameRules::AnnounceAutoTeamBalance(void)
{	
	if (m_AutoBalancePlayers.Count() <= 0)
		return;

	char szMsg[128];
	Q_strcpy(szMsg, "You will be switched to the other team (Auto Team Balance)\n");

	for(int i = 0; i < m_AutoBalancePlayers.Count(); i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(m_AutoBalancePlayers[i]);
		if (pPlayer && pPlayer->IsConnected())
			UTIL_SayText(szMsg, pPlayer);
	}
}

void CSDKGameRules::ProcessAutoTeamBalance(void)
{
	for(int i = 0; i < m_AutoBalancePlayers.Count(); i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(m_AutoBalancePlayers[i]);
		if (pPlayer && pPlayer->IsConnected())
			((CSDKPlayer*)pPlayer)->ChangeTeam(m_iAutoBalanceTeam, true);
	}
	m_AutoBalancePlayers.Purge();
}

void CSDKGameRules::ProcessTeamChangeRequests(void)
{

	for(int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CSDKPlayer *pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex(i);
		if (pPlayer && pPlayer->IsConnected()) {
			pPlayer->ProcessChangeTeamRequest();
		}
	}
}

void CSDKGameRules::ClientDisconnected( edict_t *pClient )
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
	// this is unlikely, but best to be safe
	if ( pPlayer )
		m_AutoBalancePlayers.FindAndRemove(pPlayer->entindex());

	// tell teh objective manager so we don't crash!
	if(pPlayer)
	{
		GET_OBJ_MGR()->PlayerLeft(pPlayer);
		GetStrategyManager()->PlayerLeft(pPlayer);
	}

	BaseClass::ClientDisconnected(pClient);
}

/**
* Clears out the player spawn info
*
* @return void
**/
void CSDKGameRules::ClearSpawns(void)
{
	// reset everything
	ResetSpawns();

	// clear the list
	m_aSpawns.Purge();
}

/**
* Determines the name of the spawn point entity to use for a player on
* the specified team
*
* @param char szName[] The buffer to fill in
* @param int iLen The length of the buffer
* @param int iTeam The team the player is on
* @return bool
**/
bool CSDKGameRules::GetSpawnPointName(char szName[], int iLen, int iTeam)
{
	// set the string
	Q_snprintf(szName, iLen, "info_player_team%d", iTeam);

	return true;
}

/**
* Finds all the spawn points
*
* @return void
**/
void CSDKGameRules::FindSpawns(void)
{
	CBaseEntity *pSpot, *pFirstSpot;

	// do we have them already?
	if(m_aSpawns.Count() > 0)
		return;

	// find all the ones by the proper name
	pSpot = gEntList.FindEntityByClassname(NULL, "spawn_point");
	pFirstSpot = pSpot;
	do 
	{
		// add it to our list
		if(pSpot)
			m_aSpawns.AddToTail(pSpot);
		
		// move on to the next one
		pSpot = gEntList.FindEntityByClassname(pSpot, "spawn_point");

	} while(pSpot != pFirstSpot); // loop if we're not back to the start
}

/**
* Attempts to find a valid spawn point for the specified player
*
* @param CSDKPlayer *pPlayer The player to find a spawn point for
* @return CBaseEntity *
**/
CBaseEntity *CSDKGameRules::GetValidSpawnPoint(CSDKPlayer *pPlayer)
{
	// see if we have a valid team for the player
	if(pPlayer->GetTeamNumber() != TEAM_A && pPlayer->GetTeamNumber() != TEAM_B)
		return NULL;

	// find the spawns
	FindSpawns();

	// find the next valid spawn for the player
	for(int i = 0; i < m_aSpawns.Count(); ++i)
	{
		// is this one to your liking?
		if(IsSpawnPointValid(m_aSpawns[i], pPlayer))
			return m_aSpawns[i];
	}

	Assert(0);
	return NULL;
}

/**
* Determines if the specified spawn point is valid
*
* @param CBaseEntity *pSpot The potential spawn point
* @param CBasePlayer *pPlayer The player to check against
* @return bool
**/
bool CSDKGameRules::IsSpawnPointValid(CBaseEntity *pSpot, CBasePlayer *pPlayer)
{
	char szSpawnPointName[64];

	// complete the name
	if(!GetSpawnPointName(szSpawnPointName, sizeof(szSpawnPointName), pPlayer->GetTeamNumber()))
		return false;

	// is this the correct name?
	if(FStrEq(pSpot->GetClassname(), szSpawnPointName))
		return BaseClass::IsSpawnPointValid(pSpot, pPlayer);

	// the only other option is that if it's an info player start we have to be on a fake team
	if(FStrEq(pSpot->GetClassname(), "info_player_start") && 
		(!pPlayer->GetTeam() || !pPlayer->GetTeam()->IsPlayingTeam()))
	{
		return BaseClass::IsSpawnPointValid(pSpot, pPlayer);
	}

	// otherwise, the spawn point isn't valid
	return false;
}

/**
* Determines if we are at our maximum for a specific skill type
*
* @param CSDKPlayer *pPlayer The player whose team to check
* @param const char *szSkill The name of the skill to check
* @return bool
**/
bool CSDKGameRules::AtMaxForSkill(CSDKPlayer *pPlayer, const char *szSkill)
{
	CUtlVector<CBasePlayer *> *pPlayers;
	CSDKPlayer *pTeamPlayer;
	int iCount;

	// get the count for this skill type on the player's team
	if(!pPlayer->GetTeam() || !pPlayer->GetTeam()->IsPlayingTeam())
		return false; // i guess so...

	// make sure we have a real limit
	if(!stricmp(szSkill, "scout") && mp_limitrangers.GetInt() == -1)
		return false;
	else if(!stricmp(szSkill, "soldier") && mp_limitriflemen.GetInt() == -1)
		return false;
	else if(!stricmp(szSkill, "sniper") && mp_limitsnipers.GetInt() == -1)
		return false;
	else if(!stricmp(szSkill, "hw") && mp_limitgunners.GetInt() == -1)
		return false;

	// run through all the players
	pPlayers = pPlayer->GetTeam()->GetPlayers();
	for(int i = iCount = 0; i < pPlayers->Count(); ++i)
	{
		// pull the player
		pTeamPlayer = dynamic_cast<CSDKPlayer *>((*pPlayers)[i]);
		if(!pTeamPlayer)
			continue;

		// see if they're the right class
		if(pTeamPlayer && pTeamPlayer->GetSkillClass() && 
			FStrEq(szSkill, pTeamPlayer->GetSkillClass()->GetInternalClassName()))
		{
			++iCount;
		}
	}
	
	// what type should we compare to?
	if(!stricmp(szSkill, "scout"))
		return mp_limitrangers.GetInt() <= iCount;
	else if(!stricmp(szSkill, "soldier"))
		return mp_limitriflemen.GetInt() <= iCount;
	else if(!stricmp(szSkill, "sniper"))
		return mp_limitsnipers.GetInt() <= iCount;
	else if(!stricmp(szSkill, "hw"))
		return mp_limitgunners.GetInt() <= iCount;

	return false;
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Find the relationship between players (teamplay vs. deathmatch)
//-----------------------------------------------------------------------------
int CSDKGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	if (!pPlayer || !pTarget || !pPlayer->IsPlayer() || !pTarget->IsPlayer())
		return GR_NOTTEAMMATE;

	return (pPlayer->GetTeamNumber() == pTarget->GetTeamNumber() ? GR_TEAMMATE : GR_NOTTEAMMATE);
}

/**
* We need to listen for round ended events so we can clear
* decals among other things
*
* @param KeyValues *pEvent The event that occured
* @return void
**/
void CSDKGameRules::FireGameEvent(IGameEvent *pEvent)
{
#ifdef CLIENT_DLL
	// what type of event do we have?
	if(FStrEq(pEvent->GetName(), "round_ended"))
	{
		// clear out the decals
		modelrender->RemoveAllDecalsFromAllModels();
	}
#endif
}

#ifdef CLIENT_DLL
int CSDKGameRules::CanLocalPlayerBuy()
{
	if (GetRemainingBuyTime() > 0.0) {
        CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
		if (pPlayer)
			// got a player (99.9% of the time).. are we alive?
			return (pPlayer->IsAlive() ? 1 : -1);
		else
			// didn't get a player, but we're in buy time so I guess you can buy
			return true;
	}
	return 0;
}

void CSDKGameRules::SetBuyTime(float fTime)
{
	m_fBuyEndTime = fTime;
}

float CSDKGameRules::GetRemainingBuyTime()
{
	return (m_fBuyEndTime - gpGlobals->curtime);
}
#endif