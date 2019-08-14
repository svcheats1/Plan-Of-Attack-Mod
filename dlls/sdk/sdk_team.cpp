//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "sdk_team.h"
#include "entitylist.h"
#include "sdk_player.h"
#include "sdk_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Datatable
IMPLEMENT_SERVERCLASS_ST(CSDKTeam, DT_SDKTeam)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( sdk_team_manager, CSDKTeam );

/**
* Constructor
**/
CSDKTeam::CSDKTeam()
{
	// reset the xp
	ResetIndivXP();
	m_fXP = 0;
	m_iPermanentScore = 0;
}

/**
* Destructor
**/
CSDKTeam::~CSDKTeam()
{
	// ?
}

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified TF team manager
//-----------------------------------------------------------------------------
CSDKTeam *GetGlobalSDKTeam( int iIndex )
{
	return (CSDKTeam*)GetGlobalTeam( iIndex );
}


//-----------------------------------------------------------------------------
// Purpose: Needed because this is an entity, but should never be used
//-----------------------------------------------------------------------------
void CSDKTeam::Init( const char *pName, int iNumber )
{
	BaseClass::Init( pName, iNumber );

	// Only detect changes every half-second.
	NetworkProp()->SetUpdateInterval( 0.75f );
}

/**
* Precaches sounds for the team
*
* @return void
**/
void CSDKTeam::Precache(void)
{
	// cache our sounds
	PrecacheScriptSound("Radio.ObjectiveAttack");
	PrecacheScriptSound("Radio.ObjectiveDefend");
}

bool CSDKTeam::IsTeamAlive(void)
{
	int iSize = GetNumPlayers();
	for(int i = 0; i < iSize; ++i) {
		CBasePlayer *pPlayer = GetPlayer(i);
		// if at least one player is alive, then the team is alive
		if (pPlayer->IsAlive())
			return true;
	}

	// no one is alive, so they're dead
	return false;
}

/**
* Awards all the players on the team
*
* @param int iAward Amount of money to give the player so far
* @return void
**/
void CSDKTeam::AwardPlayers(int iAward, int iXP)
{
	// go through all my players
	for(int i = 0; i < GetNumPlayers(); ++i)
	{
		// grab the player
		CSDKPlayer *pPlayer = (CSDKPlayer *)GetPlayer(i);

		// did we get them?
		if(pPlayer)
			pPlayer->AwardPlayer(iAward, iXP);
	}
	
	// increment the xp
	m_fXP += iXP;
}

/**
* Picks the player to choose the objective
*
* @return CSDKPlayer *
**/
CSDKPlayer *CSDKTeam::GetObjectivePicker(void)
{
	CUtlVector<CBasePlayer *> *aPlayers;
	CSDKPlayer *pPlayer;

	// grab the players
	aPlayers = GetPlayers();

	// make sure we have some players
	if(!aPlayers->Count())
		return NULL;

	// start running through our members
	for(int i = 0; i < aPlayers->Count(); ++i)
	{
		// convert to an sdk player
		pPlayer = (CSDKPlayer *)(*aPlayers)[i];

		// have they picked yet?
		if(!pPlayer->GetHasPickedObjective())
		{
			// they picked
			pPlayer->SetHasPickedObjective(true);
			return pPlayer;
		}
	}

	// everyone has picked, reset and try again
	for(int i = 0; i < aPlayers->Count(); ++i)
	{
		// convert to an sdk player and set them to no pick
		pPlayer = (CSDKPlayer *)(*aPlayers)[i];
		pPlayer->SetHasPickedObjective(false);
	}

	// try again
	return GetObjectivePicker();
}

/**
* Determines the average experience of the top fifty percent
* of the players on this team
*
* @return float
**/
float CSDKTeam::GetAvgIndivXP(void)
{
	// is that one set yet?
	if(m_fAvgIndivXP == -1)
	{
		CUtlVector<CSDKPlayer *> aSortedPlayers;
		float fAvg = 0;
		
		// do we have players?
		if(!GetPlayers())
			return 0;

		// copy the players.  can't use = because of the cast
		// need sdk players for comparebyxp which takes const * so no dynamic cast
		// barf!
		for(int i = 0; i < GetPlayers()->Count(); ++i)
		{
			// pull the player, convert to sdk, and add to the list
			CSDKPlayer *pPlayer = ToSDKPlayer(GetPlayers()->Element(i));
			if(pPlayer)
				aSortedPlayers.AddToTail(pPlayer);
		}

		// sort the list
		aSortedPlayers.Sort(CompareByXP);

		// find the average of the top half
		if(aSortedPlayers.Count())
		{
			for(int i = 0; i < (int)ceil((float)aSortedPlayers.Count() / 2.0); ++i)
				fAvg += aSortedPlayers[i]->GetXP();
			fAvg /= ceil((float)aSortedPlayers.Count() / 2.0);
		}

		// set it for future use
		m_fAvgIndivXP = fAvg;
	}

	return m_fAvgIndivXP;
}

/**
* Resets all of the players on the team
*
* @return void
**/
void CSDKTeam::Reset(void)
{
	// reset the xp
	ResetXP();

	// iterate over the team members
	for(int i = 0; i < GetPlayers()->Count(); ++i)
	{
		// reset
		CSDKPlayer *pPlayer = ToSDKPlayer(GetPlayers()->Element(i));

		// did we get it?
		if(pPlayer)
			// reset
			pPlayer->Reset();
	}

	m_iPermanentScore = 0;
	m_iScore = 0;
}

/**
* Sends a sound message out to everyone on the team
*
* @param const char *szSound The sound to send
* @return void
**/
void CSDKTeam::SendSoundToTeam(const char *szSound)
{
	EmitSound_t sSound;

	// setup the sound
	sSound.m_nChannel = CHAN_STATIC;
	sSound.m_flVolume = VOL_NORM;
	sSound.m_SoundLevel = ATTN_TO_SNDLVL(ATTN_STATIC);
	sSound.m_pSoundName = szSound;

	// send it to the 
	SendSoundToTeam(sSound);
}

/**
* Sends a sound message out to everyone on the team
*
* @param EmitSound_t &sParams The sound info
* @return void
**/
void CSDKTeam::SendSoundToTeam(const EmitSound_t &sParams)
{
	// tell all of our players
	for(int i = 0; i < GetPlayers()->Count(); ++i)
	{
		// pull the player
		CSDKPlayer *pPlayer = ToSDKPlayer(GetPlayers()->Element(i));

		// did we get it?
		if(pPlayer)
		{
			// send it out
			CSingleUserRecipientFilter filter(pPlayer);
			filter.MakeReliable();
			pPlayer->EmitSound(filter, pPlayer->entindex(), sParams);
		}
	}
}

/**
* Determines if we should transmit to the given player
*
* @param CBasePlayer *pRecipient The player who we might send to
* @param CBaseEntity *pEntity The entity we might be sending
* @return bool True if we always want to send this entity
**/
bool CSDKTeam::ShouldTransmitToPlayer(CBasePlayer* pRecipient, CBaseEntity* pEntity)
{
	// IMPORTANT
	//////////////////////////////
	// JD: this should not be called anymore because CSDKPlayers now
	// always transmit. See CSDKPlayer::UpdateTransmitState
	//////////////////////////////
	Assert( false );

	// we only care about players, and we want real values
	if (!pRecipient || !pEntity || !pEntity->IsPlayer())
		return BaseClass::ShouldTransmitToPlayer(pRecipient, pEntity);

	// is it on our team?
	if(pEntity->GetTeamNumber() == pRecipient->GetTeamNumber())
		return true;

	// is it the first round?
	if(g_pGameRules && ((CSDKGameRules *)g_pGameRules)->IsFirstRound())
		return true;

	// are we dead and forcecamera is off?
	if (!pRecipient->IsAlive() && ((CSDKGameRules *)g_pGameRules)->GetForceCam() == OBS_ALLOW_ALL)
		return true;

	return BaseClass::ShouldTransmitToPlayer(pRecipient, pEntity);
}

void CSDKTeam::AddPermanentScore( int iScore )
{
	// subtract so we have the 'temporary' score
	m_iScore -= m_iPermanentScore;
	// add the perminent score
	m_iPermanentScore += iScore;
	// add back to the regular score
	m_iScore += m_iPermanentScore;
}

void CSDKTeam::SetScore( int iScore )
{
	m_iScore = m_iPermanentScore + iScore;
}
