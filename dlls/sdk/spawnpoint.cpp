#include "cbase.h"
#include "spawnpoint.h"
#include "team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CSpawnPoint)
END_DATADESC()

LINK_ENTITY_TO_CLASS(info_player_team1,CSpawnPoint);
LINK_ENTITY_TO_CLASS(info_player_team2,CSpawnPoint);

// global Savedata for suppression start point
BEGIN_DATADESC(CSuppressionStartCoalition)
	// keyfields
	//DEFINE_KEYFIELD(m_iObjective, FIELD_STRING, "Objective"),
END_DATADESC()

// global Savedata for suppression start point
BEGIN_DATADESC(CSuppressionStartAmerican)
	// keyfields
	//DEFINE_KEYFIELD(m_iSpawnGroup, FIELD_STRING, "SpawnGroup"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(info_player_team1_suppression, CSuppressionStartAmerican);
LINK_ENTITY_TO_CLASS(info_player_team2_suppression, CSuppressionStartCoalition);

/**
* Determines if our class matches the specified string
*
* @param const char *szClassOrWildcard The string to match
* @return bool
**/
bool CSpawnPoint::ClassMatches(const char *szClassOrWildcard)
{
	// is this any type of spawn point?
	if(!strcmp(szClassOrWildcard, "spawn_point"))
		return true;

	return BaseClass::ClassMatches(szClassOrWildcard);
}

/**
* Determines if our class matches the specified string
*
* @param string_t szNameStr The string to match
* @return bool
**/
bool CSpawnPoint::ClassMatches(string_t szNameStr)
{
	// try the other one
	return ClassMatches(STRING(szNameStr));
}

/**
* Determines if there are any enemies fo the given team nearby
*
* @param CTeam *pTeam The team number to check for enemies of
* @return bool
**/
bool CSpawnPoint::EnemiesNearby(CTeam *pTeam)
{
	CBaseEntity *pEnts[16];
	int iEnts;
	Vector vecSource;
	Vector vecRadius(120, 120, 120);	// this could be adjusted to use the player's height as the z-component
										// what happens if someone is on the floor below me?

	// no enemies of non-playing teams
	if(!pTeam->IsPlayingTeam())
		return false;

	// see if we can find anyone
	vecSource = GetAbsOrigin();
	iEnts = UTIL_EntitiesInBox(pEnts, 16, vecSource - vecRadius, vecSource + vecRadius, FL_CLIENT);

	// run through the winners
	for(int i = 0; i < iEnts; ++i)
	{
		// is it a player who's on a real team and that team isn't mine
		if(pEnts[i] && pEnts[i]->IsPlayer() && pEnts[i]->GetTeam() && 
			pEnts[i]->GetTeam()->IsPlayingTeam() && pEnts[i]->GetTeamNumber() != pTeam->GetTeamNumber())
		{
			return true;
		}
	}

	return false;
}