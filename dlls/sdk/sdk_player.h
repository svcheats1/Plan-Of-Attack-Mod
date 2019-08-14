//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for SDK Game
//
// $NoKeywords: $
//=============================================================================//

#ifndef SDK_PLAYER_H
#define SDK_PLAYER_H
//#pragma once

#define HAT_ATTACHMENT "defusekit" // @TODO - rename this point!

#define MAX_CASH 10000
#define PLAYER_ALIVE_CASH_AWARD 0
#define PLAYER_ALIVE_XP_AWARD 1
#define PUNISH_TEAM_DAMAGE_THRESHOLD 20

#include "player.h"
#include "server_class.h"
#include "sdk_playeranimstate.h"
#include "sdk_shareddefs.h"
#include "skillclass.h"
#include "attachedmodel.h"

class CSkillClass;
class CSDKRagdoll;

//=============================================================================
// >> SDK Game player
//=============================================================================
class CSDKPlayer : public CBasePlayer, public ISDKPlayerAnimStateHelpers
{
public:
	DECLARE_CLASS( CSDKPlayer, CBasePlayer );
	DECLARE_SERVERCLASS();
	DECLARE_PREDICTABLE();

	CSDKPlayer();
	~CSDKPlayer();

	static CSDKPlayer *CreatePlayer( const char *className, edict_t *ed );
	static CSDKPlayer* Instance( int iEnt );

	// This passes the event to the client's and server's CPlayerAnimState.
	void DoAnimationEvent( PlayerAnimEvent_t event );

	// networking overrides
	virtual int UpdateTransmitState();
	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo );

	virtual void PreThink();
	virtual void PostThink();
	virtual void InitialSpawn();
	virtual void Spawn();
	void SetSpawnQueueTime(float fTime) { m_fLastQueue = fTime; }
	float GetSpawnQueueTime(void) { return m_fLastQueue; }
	virtual void Precache();
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles );
	void Reset(void);
	void PushawayThink();

	virtual void CheatImpulseCommands( int iImpulse );
	virtual bool CanHavePlayerItem( CBaseEntity *pItem, bool bRespectExclusion = true );

	virtual bool ClientCommand(const char *cmd);
	virtual void ForceChangeTeam();
	virtual bool ForceChangeTeamIfNecessary();
	virtual bool RequestChangeTeam(int iTeamNum);
	virtual void ProcessChangeTeamRequest(void);
	virtual int GetChangeTeamRequest() { return m_iRequestedTeamChange; }
    virtual void ChangeTeam(int iTeamNum, bool bDontKillOverride = false);
	void DeterminePlayerModel(void);
	void DeterminePlayerSkin(void);

	virtual void ForceChangeSkill(void);
	virtual void CommitSuicide(bool bForce = false);
	virtual void Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL);
	
	int GetAmmoCount( int iAmmoIndex ) const;	// temp

	CWeaponSDKBase* GetActiveSDKWeapon() const;
	virtual void	CreateViewModel( int viewmodelindex = 0 );

	CNetworkVar( int, m_iThrowGrenadeCounter );	// used to trigger grenade throw animations.
	CNetworkQAngle( m_angEyeAngles );	// Copied from EyeAngles() so we can send it to the client.
	CNetworkVector( m_vecEyePosition );
	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

	// Tracks our ragdoll entity.
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 

	// freezing
	void SetFreezeState(bool bFreeze);

	// cash and experience
	int GetCash(void);
	void ModifyCash(int iCash);
	float GetXP(void) { return m_fXP; }
	void ModifyXP(float fXP, bool bIgnoreFirstRound = false);

	int GetCaps(void) { return m_iCaps; }
	void AwardCap(int iObjective);

	// stamina
	void ReduceStamina(float flAmount);
	void AddStamina(float flAmount);
	float GetStamina();
	void CalculateStamina();

	virtual float CalcMaxSpeed();
	bool IsSprinting();
	bool CanSprint();
	bool IsWalking();
	bool IsDucking();

    // sound
	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );

	virtual float GetLastInputTime(void) { return m_flLastInputTime; }

	// weapon status
	CWeaponSDKBase* GetPrimaryWeapon();
	CWeaponSDKBase* GetSecondaryWeapon();
	CWeaponSDKBase* GetMeleeWeapon();
	CBaseCombatWeapon* GetWeaponBySlot(int iSlot, int *iStartNum = NULL);
	bool NeedsEquipment(void) { return m_bNeedsEquipment; }
	void SetNeedsEquipment(bool bNeedsEquipment) { m_bNeedsEquipment = bNeedsEquipment; }

	void DropPrimaryWeapon();
	void DropSecondaryWeapon();
	void DropMeleeWeapon();

	// attached models
	virtual void DestroyingAttachedModel(CBaseEntity *pAttached);
	void DropHat(const Vector &vecForce, const QAngle &qaAngle);

	virtual CBaseEntity	*GiveNamedItem( const char *szName, int iSubType = 0 );
	bool CanBuyItem(IBuyableItem *pItem);

	void MakeTracer(const Vector &vecTracerSrc, const trace_t &tr, int iTracerType);

// In shared code.
public:
	// ISDKPlayerAnimState overrides.
	virtual CWeaponSDKBase* SDKAnim_GetActiveWeapon();
	virtual bool SDKAnim_CanMove();
	
	virtual void AwardPlayer(int iAward, float fXP);
	virtual void Weapon_Equip( CBaseCombatWeapon *pBaseWeapon );
	virtual bool Jump(void);

	void FireBullet( 
		Vector vecSrc, 
		const QAngle &shootAngles, 
		float vecSpread, 
		int iDamage, 
		int iBulletType,
		CBaseEntity *pevAttacker,
		bool bDoEffects,
		float x,
		float y );

	// skillz
	void Boost(int iAmount);
	void UpdateWithSkillModifiers(void);
	CSkillClass *GetSkillClass(void) const { return m_pSkill; }
	PLAYER_ACTION GetPlayerAction();
	float GetAimRatio(CWeaponSDKBase *pWpn);
	float GetRecoilRatio(CWeaponSDKBase *pWpn);
	void UpdateBoostArmor(int iArmorBoost) { m_ArmorValue += iArmorBoost; }
	QAngle GetSniperDrift(void);

	// ladders
	LadderMove_t		*GetLadderMove() { return &m_Local.m_LadderMove; }

	virtual const char* GetPlayerName() { if (m_szName) return m_szName; return BaseClass::GetPlayerName(); }
	virtual void SetPlayerName(const char *szName) { Q_strncpy(m_szName.GetForModify(), szName, MAX_PLAYER_NAME_LENGTH); }
	virtual void SetRequestedName(const char *szName) { Q_strncpy(m_szNameChange, szName, MAX_PLAYER_NAME_LENGTH); }

	// team punish
	void AllowPunishAttacker(CSDKPlayer *pPlayer);
	void ForgivePlayer(int iPlayer);
	void StrikePlayer(int iPlayer);
	void GiveTDStrike(CSDKPlayer *pPlayer);

	// objective picking
	bool GetHasPickedObjective(void) { return m_bHasPickedObjective; }
	void SetHasPickedObjective(bool bHasPickedObjective) { m_bHasPickedObjective = bHasPickedObjective; }

protected:

	// view calculations
	virtual void CalcPlayerView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	void CalcSniperDrift(QAngle &vecAngle);

	// xp
	float GetXPHandicapModifier(float fXP);

	// messaging
	void SendTeamDifference(void);

private:

	bool MustChangeTeam();
	CSDKRagdoll *CreateRagdollEntity();

	// my hat
	CAttachedModel *m_pHat;

	// spawn points
	bool GetSpawnPointName(char szName[], int iLen);
	virtual CBaseEntity *EntSelectSpawnPoint(void);

	// buying
	void BuyNamedItem(const char *szName);

	// skillz
	void ChangeSkill(const char *szName);

	static CBaseEntity *s_pLastSpawnPoint;
	ISDKPlayerAnimState *m_PlayerAnimState;

	// cash and experience and stamina
	CNetworkVar(int, m_iCash);
	CNetworkVar(float, m_fXP);
	CNetworkVar(float, m_flStamina);
	CNetworkString( m_szName, MAX_PLAYER_NAME_LENGTH + 1 );
	char m_szNameChange[MAX_PLAYER_NAME_LENGTH + 1];
	int m_iMaxStamina;
	int m_iCaps;
	float m_flLastInputTime;

	// skillz
	CSkillClass *m_pSkill;
	bool m_bFirstSkillUpdate;

    int m_iRequestedTeamChange;
	float m_fLastScale;

	// in-game tips
	bool m_bInitialTips;

	// objective picks
	bool m_bHasPickedObjective;

	// team damage
	CNetworkVar(int, m_iTDStrikes);
	CUtlVector<int> m_aPunishable;

	// used for suppression, did we get a load out yet?
	bool m_bNeedsEquipment;

	// queueing
	float m_fLastQueue;
};


inline CSDKPlayer *ToSDKPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

#ifdef _DEBUG
	Assert( dynamic_cast<CSDKPlayer*>( pEntity ) != 0 );
#endif
	return static_cast< CSDKPlayer* >( pEntity );
}

class CSDKRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CSDKRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

void* SendProxy_SendSensitiveDataTable(const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID);

#endif	// SDK_PLAYER_H
