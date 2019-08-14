//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CS_GAMEVARS_SHARED_H
#define CS_GAMEVARS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

#define ROUND_LENGTH "150"
#define CAPTURE_TIME_LENGTH "45"
#define CAPTURE_LEEWAY_TIME_LENGTH "5"
#define FIRST_ROUND_SPAWN_TIME "45"
#define SUPPRESSION_SPAWN_PERIOD "15"
#define SUPPRESSION_CAPTURE_TIME_LENGTH "60"
#define BUY_TIME "30"

#define ROUND_INTERMISSION_LENGTH "5"
#define OFFENSIVE_FREEZE_TIME_LENGTH "16"
#define DEFENSIVE_FREEZE_TIME_LENGTH "6"
#define DEFENSIVE_CHOOSE_TIME_LENGTH "10"

extern ConVar mp_forcecamera;
#ifdef SUPPRESSION_ENABLED
	extern ConVar mp_suppressionforcecamera;
#endif
extern ConVar mp_allowspectators;
extern ConVar friendlyfire;
extern ConVar mp_basespeed;

extern ConVar mp_roundlength;
extern ConVar mp_roundintermissionlength;
extern ConVar mp_offensive_freezetime;
extern ConVar mp_defensive_freezetime;
extern ConVar mp_holdareatime;
extern ConVar mp_captureleeway;
extern ConVar mp_firstroundspawntime;
extern ConVar mp_limitteams;
extern ConVar mp_autoteambalance;
extern ConVar mp_delayteamswitch;
extern ConVar mp_delaynamechange;
extern ConVar mp_canheardeadpeople;
extern ConVar mp_autokick;
extern ConVar mp_autokickidletime;
extern ConVar mp_resetxponrestart;
extern ConVar mp_resetcashonrestart;
extern ConVar mp_docointossonrestart;
extern ConVar mp_roundlimit;
extern ConVar mp_teamscorelimit;
extern ConVar mp_buytime;
extern ConVar mp_defensive_choosetime;
#ifdef SUPPRESSION_ENABLED
	extern ConVar mp_initialsuppressionspawntime;
	extern ConVar mp_suppressionspawnperiod;
#endif

#endif // CS_GAMEVARS_SHARED_H
