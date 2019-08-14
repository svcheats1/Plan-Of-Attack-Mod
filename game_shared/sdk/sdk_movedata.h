#ifndef SDK_MOVEDATA_H
#define SDK_MOVEDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "igamemovement.h"
#include "func_ladder.h"

class CReservePlayerSpot;

#define REQ_STAMINA_JUMP 30

//-----------------------------------------------------------------------------
// Purpose: Data related to automatic mounting/dismounting from ladders
//-----------------------------------------------------------------------------
struct LadderMove_t
{
	DECLARE_CLASS_NOBASE( LadderMove_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

	//	Are we forcing player movement during mount/dismount
	CNetworkVar(bool, m_bForceLadderMove);
	// Is the forced move getting on or off the ladder
	CNetworkVar(bool, m_bForceMount);
	
	// Simulation info for forcing the player move
	CNetworkVar(float, m_flStartTime);
	CNetworkVar(float, m_flArrivalTime);
	CNetworkVector(m_vecGoalPosition);
	CNetworkVector(m_vecStartPosition);

	// The ladder entity owning the forced move (for marking us "on" the ladder after automounting it)
	CNetworkHandle(CFuncLadder, m_hForceLadder);
	CNetworkHandle(CReservePlayerSpot, m_hReservedSpot);
};

#endif