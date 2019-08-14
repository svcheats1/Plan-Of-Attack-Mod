//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_SDKBASE_H
#define WEAPON_SDKBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "sdk_playeranimstate.h"
#include "sdk_weapon_parse.h"
#include "buyableitem.h"

#if defined( CLIENT_DLL )
	#define CWeaponSDKBase C_WeaponSDKBase
	#define CBaseMachineGun C_BaseMachineGun
#endif

class CSDKPlayer;

// These are the names of the ammo types that the weapon script files reference.
#define AMMO_ROCKETS			"AMMO_ROCKETS"
#define AMMO_GRENADE			"AMMO_GRENADE"
#define AMMO_MACHINEGUN			"AMMO_MACHINEGUN"

//--------------------------------------------------------------------------------------------------------
//
// Weapon IDs for all SDK Game weapons
//
typedef enum
{
	WEAPON_NONE = 0,
	WEAPON_KNIFE,
	WEAPON_VIKING,
	WEAPON_SHOTGUN,
	WEAPON_MP5,
	WEAPON_AK107,
	WEAPON_MK48,
	WEAPON_GRENADE,
	WEAPON_M40,
	WEAPON_DEAGLE,
	WEAPON_M16,
	WEAPON_PP90M1,
	WEAPON_M25,
	WEAPON_SAIGA12K,
	WEAPON_KSVK,
	WEAPON_SVU,
	WEAPON_PKM,
	WEAPON_RPG,
	WEAPON_MAX,		// number of weapons weapon index
} SDKWeaponID;

/**
* Weapon types
**/
typedef enum
{
	WEAPON_TYPE_NONE = 0,
	WEAPON_TYPE_MELEE,
	WEAPON_TYPE_PISTOL,
	WEAPON_TYPE_SMG,
	WEAPON_TYPE_SHOTGUN,
	WEAPON_TYPE_RIFLE,
	WEAPON_TYPE_MACHINEGUN,
	WEAPON_TYPE_SNIPER,
	WEAPON_TYPE_GRENADE,
	WEAPON_TYPE_ROCKET,
} SDKWeaponType;

typedef enum
{
	Primary_Mode = 0,
	Secondary_Mode,
} SDKWeaponMode;

const char *WeaponIDToAlias( int id );

class CWeaponSDKBase : public CBaseCombatWeapon, public IBuyableItem
{
public:
	DECLARE_CLASS( CWeaponSDKBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponSDKBase();

	#ifdef GAME_DLL
		DECLARE_DATADESC();
	#endif

	// All predicted weapons need to implement and return true
	virtual bool	IsPredicted() const { return true; }
	virtual bool	ShouldPredict();
	virtual bool	IsDroppable() const { return true; }

	virtual void	Precache();
    virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_NONE; }
	virtual bool	IsPrimaryWeapon() const { return GetSlot() == 0; }
	virtual bool	IsSecondaryWeapon() const { return GetSlot() == 1; }
	virtual bool	IsMeleeWeapon() const { return GetSlot() == 2; }
	virtual bool	HasWeaponExclusion() const { return true; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_NONE; }
	virtual bool	EquipAmmoFromWeapon(CBaseCombatWeapon *pWeapon) { return false; }
	virtual const char*	GetDeathNoticeName( void );
	virtual const unsigned char *GetEncryptionKey( void ) { return (const unsigned char*)"BORKBORK"; }
	virtual float	CalcMaxSpeed( void ) const { return -1; }

	virtual void	WeaponThink() { }
	virtual bool	DropSpecial() { return false; }
	virtual void	Drop(const Vector &velocity);
	virtual void	Equip( CBaseCombatCharacter *pOwner );

	// zoom system
	virtual void SecondaryAttack();
	virtual void SetZoom(bool bZoom);
	virtual bool Holster(CBaseCombatWeapon *pSwitchingTo) 
	{ 
		SetZoom(false); 
		return BaseClass::Holster(pSwitchingTo); 
	}
	virtual bool Deploy() { SetZoom(false); return BaseClass::Deploy(); }

#ifdef CLIENT_DLL
	virtual void ProcessMuzzleFlashEvent(void);
#endif

	// New ammo system
	virtual void	GiveAmmo1(int iAmount);
	virtual void	GiveAmmo2(int iAmount);
	virtual int		GetAmmo1() const;
	virtual	int		GetAmmo2() const;
	virtual int		GetMaxAmmo1();
	virtual int		GetMaxAmmo2();
	virtual void	FillAmmo();
	virtual bool	UsesPrimaryAmmo();
	virtual bool	UsesSecondaryAmmo();
	virtual bool	HasAmmo();
	virtual bool	HasPrimaryAmmo();
	virtual bool	HasSecondaryAmmo();
	virtual bool	CanBeSelected();

	virtual bool DefaultReload(int iClipSize1, int iClipSize2, int iActivity);
	virtual void FinishReload(void);
	virtual void CheckReload(void);
	virtual bool Reload();
	virtual void Spawn();
	virtual void PlayerSpawn() { SetZoom(false); }	// called when the player is (re-)spawning
	virtual void PlayerKilled() { }

	virtual void AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual float CalcViewmodelBob( void );

	// Get SDK weapon specific weapon data.
	CSDKWeaponInfo const	&GetSDKWpnData() const;

	// Get a pointer to the player that owns this weapon
	CSDKPlayer* GetPlayerOwner() const;

	// override to play custom empty sounds
	virtual bool PlayEmptySound();

	virtual void ItemPostFrame(void);
	virtual void OutOfAmmo(void) { }

#ifdef GAME_DLL
	virtual void SendReloadEvents();
#endif

protected:
	virtual int GetPrice(void) const;
	virtual int GetPosition(void) const;
	virtual BI_Type GetType(void) const;
	virtual const char *GetItemName(void) const;
	virtual BI_Team GetBuyableTeam(void) const;
	virtual void BI_Init(void);
	virtual void DoRecoil(void);

	CNetworkVar(int, m_iAmmo1);
	CNetworkVar(int, m_iAmmo2);

	bool m_bDoSecondaryZoom;
	bool m_bInZoom;

private:
	CWeaponSDKBase( const CWeaponSDKBase & );
};

class CBaseMachineGun : public CWeaponSDKBase
{
public:
	DECLARE_CLASS( CBaseMachineGun, CWeaponSDKBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CBaseMachineGun();

	virtual void Spawn();
	virtual void PrimaryAttack();
	virtual bool Deploy();
	virtual void WeaponIdle();
	virtual void ItemPostFrame();
	virtual void ItemBusyFrame();
	virtual bool ShouldDrawCrosshair() const { return true; }
	virtual bool IsAutomaticWeapon() const { return true; }
	
	friend class HudDynamicCrosshair;

protected:
	float m_flIdleTime;
	float m_flBaseSpread;
	float m_flCurrentAM;
	float m_flMinAM;
	float m_flMaxAM;
	float m_flGrowRateAM;
	float m_flShrinkRateAM;
	virtual void GrowSpread();
	virtual void ShrinkSpread();
	virtual float GetSpread(CSDKPlayer *pPlayer);

private:
	CBaseMachineGun( const CBaseMachineGun & );
};

#endif // WEAPON_SDKBASE_H
