//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_SDK_PLAYER_H
#define C_SDK_PLAYER_H
#ifdef _WIN32
#pragma once
#endif


#include "sdk_playeranimstate.h"
#include "c_baseplayer.h"
#include "sdk_shareddefs.h"
#include "baseparticleentity.h"
#include "skillclass.h"
#include "igameevents.h"
#include "keyvalues.h"
#include "sdk_movedata.h"

class CSkillClass;

class C_SDKPlayer : public C_BasePlayer, public ISDKPlayerAnimStateHelpers, public IGameEventListener2
{
public:
	DECLARE_CLASS( C_SDKPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_SDKPlayer();
	~C_SDKPlayer();

	static C_SDKPlayer* GetLocalSDKPlayer();

	virtual void ClientThink(void);
	virtual void PreThink(void);
	virtual void PostThink(void);
	virtual void Spawn(void);

	virtual const QAngle& GetRenderAngles();
	virtual void UpdateClientSideAnimation();
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ProcessMuzzleFlashEvent();
	virtual void CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

	// freeze time
	void SetFreezeState(bool bShouldFreeze);

	// accessors
	CSkillClass *GetSkillClass(bool bIgnoreObserver = false);
	void UpdateSkill(void);

	float GetXP(void);
	virtual bool CanHavePlayerItem( CBaseEntity *pItem, bool bRespectExclusion = true );
	bool CanBuyItem(IBuyableItem *pItem);

	// events
	void FireGameEvent(IGameEvent *pEvent);

// Called by shared code.
public:
	
	// ISDKPlayerAnimState overrides.
	virtual CWeaponSDKBase* SDKAnim_GetActiveWeapon();
	virtual bool SDKAnim_CanMove();
	virtual bool Jump(void);
	QAngle GetNetworkedEyeAngles(void) { return m_angEyeAngles; }
	Vector GetNetworkedEyePosition(void) { return m_vecEyePosition; }

	void DoAnimationEvent( PlayerAnimEvent_t event );
	bool ShouldDraw();

	ISDKPlayerAnimState *m_PlayerAnimState;

	QAngle	m_angEyeAngles;
	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	CNetworkVector(m_vecEyePosition);

	CNetworkVar( int, m_iThrowGrenadeCounter );	// used to trigger grenade throw animations.
	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

	EHANDLE	m_hRagdoll;

	CWeaponSDKBase *GetActiveSDKWeapon() const;

	C_BaseAnimating *BecomeRagdollOnClient( bool bCopyEntity);
	IRagdoll* C_SDKPlayer::GetRepresentativeRagdoll() const;

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

	void MakeTracer(const Vector &vecTracerSrc, const trace_t &tr, int iTracerType);

	// cash and experience
	int GetCash(void) { return m_iCash; }
	float GetAimRatio(CWeaponSDKBase *pWpn);
	float GetRecoilRatio(CWeaponSDKBase *pWpn);
	PLAYER_ACTION GetPlayerAction();

	// weapon status
	CWeaponSDKBase* GetPrimaryWeapon();
	CWeaponSDKBase* GetSecondaryWeapon();
	CWeaponSDKBase* GetMeleeWeapon();
	CBaseCombatWeapon* GetWeaponBySlot(int iSlot, int *iStartNum = NULL);

	QAngle GetSniperDrift(void);

	int GetAmmoCount( int iAmmoIndex ) const;	// temp

	// ladders
	LadderMove_t		*GetLadderMove() { return &m_Local.m_LadderMove; }

	// stamina
	float GetStamina();
	void ReduceStamina(float flAmount);
	void AddStamina(float flAmount);
	void CalculateStamina();

	virtual float CalcMaxSpeed();
	bool IsSprinting();
	bool CanSprint();
	bool IsWalking();
	bool IsDucking();

	virtual const char* GetPlayerName() { if (m_szName) return m_szName; return BaseClass::GetPlayerName(); }

protected:
	void SkillChanged(IGameEvent *pValues);

	virtual void CalcPlayerView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	virtual void CalcDeathCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void CalcSniperDrift(QAngle &vecAngle);

private:
	C_SDKPlayer( const C_SDKPlayer & );

	CNetworkVar(int, m_iCash);
	CNetworkVar(float, m_fXP);
	char m_szName[MAX_PLAYER_NAME_LENGTH + 1];
	CSkillClass *m_pSkill;
	int m_iMaxStamina;
	float m_flStamina;
	float m_fLastScale;
	float m_fNextThinkPushAway;
	CNetworkVar(int, m_iTDStrikes);
};


inline C_SDKPlayer* ToSDKPlayer( CBaseEntity *pPlayer )
{
	// JD: don't assert if we don't have a player to begin with.
	if ( !pPlayer )
		return NULL;

	Assert( dynamic_cast< C_SDKPlayer* >( pPlayer ) != NULL );
	return static_cast< C_SDKPlayer* >( pPlayer );
}


#endif // C_SDK_PLAYER_H
