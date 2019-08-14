//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//

#ifndef SDK_TEAM_H
#define SDK_TEAM_H

#ifdef _WIN32
#pragma once
#endif


#include "utlvector.h"
#include "team.h"
#include "sdk_player.h"

//-----------------------------------------------------------------------------
// Purpose: Team Manager
//-----------------------------------------------------------------------------
class CSDKTeam : public CTeam
{
	DECLARE_CLASS( CSDKTeam, CTeam );
	DECLARE_SERVERCLASS();

public:
	// constructor and destructor
	CSDKTeam();
	~CSDKTeam();

	// Initialization
	virtual void Init( const char *pName, int iNumber );
	void Reset(void);
	void Precache(void);

	// accessors
	virtual bool IsTeamAlive(void);

	// rewards and xp
	virtual void AwardPlayers(int iAward, int iXP = 0);
	float GetAvgIndivXP(void);
	float GetXP(void) { return m_fXP; }
	void ResetIndivXP(void) { m_fAvgIndivXP = -1; }
	void ResetXP(void) { m_fXP = 0; }

	// objectives
	CSDKPlayer *GetObjectivePicker(void);

	// sound
	virtual void SendSoundToTeam(const char *szSound);
	virtual void SendSoundToTeam(const EmitSound_t & params);

	// transmissions
	virtual bool ShouldTransmitToPlayer(CBasePlayer* pRecipient, CBaseEntity* pEntity);

	// score
	virtual void AddPermanentScore( int iScore );
	virtual void SetScore( int iScore );

protected:
	// comparison function for checking order by xp
	static int CompareByXP(CSDKPlayer * const *ppLeft, CSDKPlayer * const *ppRight)
	{
		return (*ppRight)->GetXP() - (*ppLeft)->GetXP();
	}

private:

	// xp
	float m_fAvgIndivXP;
	float m_fXP;

	int m_iPermanentScore;
};


extern CSDKTeam *GetGlobalSDKTeam( int iIndex );

#endif // TF_TEAM_H
