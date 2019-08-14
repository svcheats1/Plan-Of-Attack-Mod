//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "gamevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// some shared cvars used by game rules
ConVar mp_forcecamera( 
	"mp_forcecamera", 
	"0", 
	FCVAR_REPLICATED,
	"Restricts spectator modes for dead players" );

#ifdef SUPPRESSION_ENABLED
	ConVar mp_suppressionforcecamera(
		"mp_suppressionforcecamera",
		"1",
		FCVAR_REPLICATED,
		"Restrics spectator modes for dead players in suppression games");
#endif
	
ConVar mp_allowspectators(
	"mp_allowspectators", 
	"1.0", 
	FCVAR_REPLICATED,
	"toggles whether the server allows spectator mode or not" );

ConVar friendlyfire(
	"mp_friendlyfire",
	"0",
	FCVAR_REPLICATED | FCVAR_NOTIFY );

ConVar	mp_basespeed( "mp_basespeed", 
					 "160", 
					 FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_CHEAT, 
					 "sets the base speed for players");

// Specifies the length of a round
ConVar	mp_roundlength( "mp_roundlength",
					  ROUND_LENGTH,
					  FCVAR_NOTIFY | FCVAR_REPLICATED,
					  "Length of a single round in seconds", true, 5.0, true, 3600.0 );

// Specifies the length of a round intermission
ConVar	mp_roundintermissionlength( "mp_roundintermissionlength",
									ROUND_INTERMISSION_LENGTH,
									FCVAR_NOTIFY | FCVAR_REPLICATED,
									"Length of the round intermission in seconds", true, 1.0, false, 20.0 );

// Specifies the length of the freeze time for each round
ConVar	mp_offensive_freezetime( "mp_offensive_freezetime",
									OFFENSIVE_FREEZE_TIME_LENGTH,
									FCVAR_NOTIFY | FCVAR_REPLICATED,
									"Length of the offensive freeze time in seconds" );

ConVar	mp_defensive_freezetime( "mp_defensive_freezetime",
									DEFENSIVE_FREEZE_TIME_LENGTH,
									FCVAR_NOTIFY | FCVAR_REPLICATED,
									"Length of the offensive freeze time in seconds" );

ConVar mp_defensive_choosetime( "mp_defensive_choosetime",
									DEFENSIVE_CHOOSE_TIME_LENGTH,
									FCVAR_NOTIFY | FCVAR_REPLICATED,
									"Length of time the defensive team has to choose a strategy" );

// Specifies the length of time required to capture an objective
ConVar	mp_holdareatime( "mp_holdareatime",
									CAPTURE_TIME_LENGTH,
									FCVAR_NOTIFY | FCVAR_REPLICATED,
									"Length of time to capture an objective in seconds",
									true,
									1,
									true,
									120);

ConVar mp_captureleeway( "mp_captureleeway",
									CAPTURE_LEEWAY_TIME_LENGTH,
									FCVAR_NOTIFY | FCVAR_REPLICATED,
									"Number of seconds allowed before the objective timer resets" );

// Specifies the amount of time that players are allowed to spawn in the first round
ConVar mp_firstroundspawntime("mp_firstroundspawntime",
									FIRST_ROUND_SPAWN_TIME,
									FCVAR_NOTIFY | FCVAR_REPLICATED,
									"Length of time in which players may spawn during the first round");

#ifdef SUPPRESSION_ENABLED
	ConVar	mp_suppression("mp_suppression", "0", FCVAR_NOTIFY | FCVAR_REPLICATED);

	// Specifies the amount of time that players are allowed to spawn at the start of a suppression game

	ConVar mp_initialsuppressionspawntime("mp_initialsuppressionspawntime",
										SUPPRESSION_SPAWN_PERIOD,
										FCVAR_NOTIFY | FCVAR_REPLICATED,
										"Length of time in which players may spawn during the start of a suppression game");

	// Amount of time between spawns in suppression
	ConVar mp_suppressionspawnperiod("mp_suppressionspawnperiod",
										SUPPRESSION_SPAWN_PERIOD,
										FCVAR_NOTIFY | FCVAR_REPLICATED,
										"Time between spawns in a suppression game");

	// Specifies the length of time required to capture an objective in a suppression game
	ConVar	mp_suppressionholdareatimebase( "mp_suppressionholdareatimebase",
										SUPPRESSION_CAPTURE_TIME_LENGTH,
										FCVAR_NOTIFY | FCVAR_REPLICATED,
										"Length of time to capture an objective in seconds for suppression",
										true,
										1,
										true,
										120);
#endif

ConVar mp_limitteams("mp_limitteams",
					"1",
					FCVAR_NOTIFY | FCVAR_REPLICATED,
					"The Maximum number of players by which each team may differ");

ConVar mp_autoteambalance("mp_autoteambalance",
						"1",
						FCVAR_NOTIFY | FCVAR_REPLICATED,
						"Enables auto team balance");

ConVar mp_delayteamswitch("mp_delayteamswitch",
						"1",
						FCVAR_NOTIFY | FCVAR_REPLICATED,
						"Enables waiting until the end of the round to switch players");

ConVar mp_delaynamechange("mp_delaynamechange",
						  "1",
						  FCVAR_NOTIFY | FCVAR_REPLICATED,
						  "Enabled waiting until the end of the round to process name changes of dead players");

ConVar mp_canheardeadpeople("mp_canheardeadpeople",
							"0",
							FCVAR_NOTIFY | FCVAR_REPLICATED,
							"Allows alive people to hear dead people");

ConVar mp_autokick("mp_autokick", 
				   "1",
				   FCVAR_NOTIFY | FCVAR_REPLICATED,
				   "Kicks idle players.");

ConVar mp_autokickidletime("mp_autokickidletime",
						   "300",
						   FCVAR_NOTIFY | FCVAR_REPLICATED,
						   "How long in seconds to wait before kicking an idle player.", true, 60.0, false, 0.0);

ConVar mp_resetxponrestart("mp_resetxponrestart",
						   "0",
						   FCVAR_NOTIFY | FCVAR_REPLICATED,
						   "If 1, the game will reset XP on a game restart. Only used if mp_teamscorelimit > 3 or 0.");

ConVar mp_resetcashonrestart("mp_resetcashonrestart",
						     "1",
						     FCVAR_NOTIFY | FCVAR_REPLICATED,
						     "If 1, the game will reset cash and player items on a game restart. Only used if mp_teamscorelimit > 3 or 0.");

ConVar mp_docointossonrestart("mp_docointossonrestart",
						      "1",
						      FCVAR_NOTIFY | FCVAR_REPLICATED,
						      "If 1, the game does a coin toss round on a game restart. Only used if mp_teamscorelimit > 3 or 0.");

ConVar mp_roundlimit("mp_roundlimit",
					 "0",
					 FCVAR_NOTIFY | FCVAR_REPLICATED,
					 "Limits the game to a set number of rounds. 0 = no limit");

ConVar mp_teamscorelimit("mp_teamscorelimit",
						 "3",
						 FCVAR_NOTIFY | FCVAR_REPLICATED,
						 "Ends the game when any team reaches a certain number of caps. 0 = no limit");


ConVar mp_gamestoplay("mp_gamestoplay",
					  "1",
					  FCVAR_NOTIFY | FCVAR_REPLICATED,
					  "Specifies the number of games to string together in the same map");

ConVar mp_allowtkpunish("mp_allowtkpunish",
						"1",
						FCVAR_NOTIFY | FCVAR_REPLICATED,
						"If 1, players will be given the option to forgive/strike on TK");

ConVar mp_tkstrikesallowed("mp_tkstrikesallowed",
						"3",
						FCVAR_NOTIFY | FCVAR_REPLICATED,
						"Number of strikes to allow before kicking someone");

ConVar mp_tkbantime("mp_tkbantime",
					"15",
					FCVAR_NOTIFY | FCVAR_REPLICATED,
					"Amount of time (minutes) to ban a TKer for. (0 permanent, -1 no ban)");

ConVar mp_limitsnipers("mp_limitsnipers",
						"-1",
						FCVAR_NOTIFY | FCVAR_REPLICATED,
						"Maximum number of snipers to allow. (-1 = infinite)");

ConVar mp_limitriflemen("mp_limitriflemen",
						"-1",
						FCVAR_NOTIFY | FCVAR_REPLICATED,
						"Maximum number of snipers to allow. (-1 = infinite)");

ConVar mp_limitrangers("mp_limitrangers",
						"-1",
						FCVAR_NOTIFY | FCVAR_REPLICATED,
						"Maximum number of snipers to allow. (-1 = infinite)");

ConVar mp_limitgunners("mp_limitgunners",
						"-1",
						FCVAR_NOTIFY | FCVAR_REPLICATED,
						"Maximum number of snipers to allow. (-1 = infinite)");

// function declaration. static -> not global
extern void BuyTimeChanged(ConVar *cvar, const char* value);

// Specifies the amount of time in which players may buy stuff at the start of the round
ConVar mp_buytime("mp_buytime",
					BUY_TIME,
					FCVAR_NOTIFY | FCVAR_REPLICATED,
					"Length of time in which players can buy items", 
					true, 10.0, true, 3600.0, BuyTimeChanged);

// this makes the client DLL happy since we only use this on the server
#ifdef CLIENT_DLL
void BuyTimeChanged(ConVar *cvar, const char* value)
{
}
#endif