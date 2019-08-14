//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WEAPON_RPG_H
#define WEAPON_RPG_H

#ifdef _WIN32
#pragma once
#endif

#include "Sprite.h"
#include "npcevent.h"
#include "beam_shared.h"

#ifdef CLIENT_DLL
	#include "c_basecombatcharacter.h"
	#include "c_smoke_trail.h"
	#include "buymenu.h"

	#define RocketTrail C_RocketTrail
	#define CWeaponRPG C_WeaponRPG
	#define CLaserDot C_LaserDot
#else
	#include "basecombatcharacter.h"
	#include "smoke_trail.h"
#endif

#define MISSLE_MAX_HEALTH 100

class CWeaponRPG;
class CLaserDot;
class RocketTrail;
 
//###########################################################################
//	>> CMissile		(missile launcher class is below this one!)
//###########################################################################
class CMissile : public CBaseCombatCharacter
{
	DECLARE_CLASS( CMissile, CBaseCombatCharacter );

public:
	CMissile();
	~CMissile();
	
	void	Spawn( void );
	void	Precache( void );
	void	MissileTouch( CBaseEntity *pOther );
	void	Explode(CBaseEntity *pOther);
	void	ShotDown( void );
	void	AccelerateThink( void );
	void	AugerThink( void );
	void	IgniteThink( void );
	void	SeekThink( void );
	void	DumbFire( void );
	void	SetGracePeriod( float flGracePeriod );
	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_RPG; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_ROCKET; }

#ifndef CLIENT_DLL
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void UpdateOnRemove(void);
#endif
	void	Event_Killed( const CTakeDamageInfo &info );
	
	virtual float	GetDamage() { return m_flDamage; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }

	unsigned int PhysicsSolidMaskForEntity( void ) const;

	CHandle<CWeaponRPG>		m_hOwner;

	static CMissile *Create( const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner = NULL );

protected:
#ifndef CLIENT_DLL
	virtual void DoExplosion();	
	virtual void ComputeActualDotPosition( CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed );
#endif

	virtual int AugerHealth() { return m_iMaxHealth - 20; }

#ifndef CLIENT_DLL
	// Creates the smoke trail
	void CreateSmokeTrail( void );
#endif

	// Gets the shooting position 
	void GetShootPosition( CLaserDot *pLaserDot, Vector *pShootPosition );

	CHandle<RocketTrail>	m_hRocketTrail;
	float					m_flAugerTime;		// Amount of time to auger before blowing up anyway
	float					m_flMarkDeadTime;
	float					m_flDamage;
	int						m_iMaxHealth;

private:
	float					m_flGracePeriodEndsAt;

#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif
};


//-----------------------------------------------------------------------------
// Laser dot 
//-----------------------------------------------------------------------------
CBaseEntity *CreateLaserDot( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot );
void SetLaserDotTarget( CBaseEntity *pLaserDot, CBaseEntity *pTarget );
void EnableLaserDot( CBaseEntity *pLaserDot, bool bEnable );

//-----------------------------------------------------------------------------
// RPG
//-----------------------------------------------------------------------------
class CWeaponRPG : public CWeaponSDKBase
{
public:
	DECLARE_CLASS(CWeaponRPG, CWeaponSDKBase);
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponRPG();
	~CWeaponRPG();

	void	Precache( void );

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_RPG; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_ROCKET; }

	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack(void);
	virtual float GetFireRate( void ) { return 1; };
	virtual void	ItemPostFrame( void );

	virtual void	DecrementAmmo( CBaseCombatCharacter *pOwner );

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool	Reload( void );
	virtual bool	Deploy(void);
	virtual void	WeaponIdle(void);
#ifndef CLIENT_DLL
	virtual void	OutOfAmmo(void);
#endif

	virtual void Drop( const Vector &vecVelocity );

#ifndef CLIENT_DLL
	virtual void UpdateOnRemove(void);
#endif

	int		GetMinBurst() { return 1; }
	int		GetMaxBurst() { return 1; }
	float	GetMinRestTime() { return 4.0; }
	float	GetMaxRestTime() { return 4.0; }

	void	NotifyRocketDied( void );

	bool	HasAnyAmmo( void );

	void StartPointer( void );
	void StopPointer( void );
	void TogglePointer( void );
	bool IsPointerOn(void) { return m_bPointerOn; }

	void	CreateLaserPointer( void );
	void	UpdateLaserPosition( Vector vecMuzzlePos = vec3_origin, Vector vecEndPos = vec3_origin );
	void	StartLaserEffects( void );
	void	StopLaserEffects( void );
	void	UpdateLaserEffects( void );

#ifndef CLIENT_DLL
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
#endif

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}
	
	CBaseEntity *GetMissile( void ) { return m_hMissile; }
	
protected:
	Vector GetMuzzlePoint();

	bool				m_bPointerOn;
	Vector				m_vecNPCLaserDot;
	CHandle<CLaserDot>	m_hLaserDot;
	CHandle<CMissile>	m_hMissile;
	CHandle<CSprite>	m_hLaserMuzzleSprite;
	CHandle<CBeam>		m_hLaserBeam;
};

#ifdef CLIENT_DLL
	ADDBUYABLEITEM(CWeaponRPG);
#endif

//-----------------------------------------------------------------------------
// Laser Dot
//-----------------------------------------------------------------------------
class CLaserDot : public CSprite 
{
	DECLARE_CLASS( CLaserDot, CSprite );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

public:

	CLaserDot( void );
	~CLaserDot( void );

#ifndef CLIENT_DLL
	static CLaserDot *Create( const Vector &origin, CBaseEntity *pOwner = NULL, bool bVisibleDot = true );
#endif

	void	SetTargetEntity( CBaseEntity *pTarget ) { m_hTargetEnt = pTarget; }
	CBaseEntity *GetTargetEntity( void ) { return m_hTargetEnt; }

	void	SetLaserPosition( const Vector &origin, const Vector &normal );
	Vector	GetChasePosition();
	void	TurnOn( void );
	void	TurnOff( void );
	bool	IsOn() const { return m_bIsOn; }

	void	Toggle( void );

	void	LaserThink( void );

#ifdef CLIENT_DLL
	virtual void ClientThink(void);
#endif

	int		ObjectCaps() { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }

	void	MakeInvisible( void );

protected:
	Vector				m_vecSurfaceNormal;
	EHANDLE				m_hTargetEnt;
	bool				m_bVisibleLaserDot;

public:
	CLaserDot			*m_pNext;

private:
	CNetworkVar(bool, m_bIsOn);
	CNetworkVectorForDerived(m_vecAbsOrigin);
};

#endif // WEAPON_RPG_H
