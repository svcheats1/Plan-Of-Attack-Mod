#ifndef _CSKILLCLASS_H_
#define _CSKILLCLASS_H_

#include "buyableitem.h"
#include "weapon_sdkbase.h"
#include "utlvector.h"

#ifdef CLIENT_DLL
	#include "c_sdk_player.h"
#else
	#include "sdk_player.h"
#endif

#define TEAM_A_MODEL "models/player/player_a_nohat.mdl"
#define TEAM_B_MODEL "models/player/player_b_nohat.mdl"

#define TEAM_A_SOLDIER_HAT_MODEL "models/player/a_soldier_hat.mdl"
#define TEAM_B_SOLDIER_HAT_MODEL "models/player/c_soldier_hat.mdl"
#define TEAM_A_SNIPER_HAT_MODEL "models/player/a_sniper_hat.mdl"
#define TEAM_B_SNIPER_HAT_MODEL "models/player/c_sniper_hat.mdl"
#define TEAM_A_SCOUT_HAT_MODEL "models/player/a_ranger_hat.mdl"
#define TEAM_B_SCOUT_HAT_MODEL "models/player/c_ranger_hat.mdl"
#define TEAM_A_HW_HAT_MODEL "models/player/a_gunner_hat.mdl"
#define TEAM_B_HW_HAT_MODEL "models/player/c_gunner_hat.mdl"

/*#ifdef _DEBUG
	#define SKILL_LEVEL_1 5
	#define SKILL_LEVEL_2 10
	#define SKILL_LEVEL_3 15
	#define SKILL_LEVEL_4 20
#else*/
	#define SKILL_LEVEL_1 25
	#define SKILL_LEVEL_2 55
	#define SKILL_LEVEL_3 90
	#define SKILL_LEVEL_4 130
//#endif

#define SKILL_LEVEL_COUNT 4

#define MAX_SKILLCLASS_NAME 20
#define MAX_XP 130
#define DEFAULT_STAMINA 120

#define PLAYER_KILL_IN_CLASS_XP 1

/**
* Skill classes
* THE ORDER OF THESE MUST CORRESPOND TO THE ORDER IN WHICH
* THE CLASSES ARE DECLARED IN InitSkillClasses
**/
typedef enum
{
	NONE_CLASS_INDEX = -1,
	SCOUT_CLASS_INDEX,
	SOLDIER_CLASS_INDEX,
	SNIPER_CLASS_INDEX,
	HW_CLASS_INDEX,
} SKILL_CLASS_INDEX;

// a little fudging for typedefs...
class CSkillClass;
typedef CUtlVector<CSkillClass *> SkillClassVec;

/**
* Declaration of CSkillClass
* Represents a series of skillz for the playa
*
* "Girls want guys with skills: nunchaku skills, bow staff skills, computer hacking skills."
**/
class CSkillClass 
{
public:
	// constructors
	CSkillClass(); // default
	CSkillClass(CSDKPlayer *pPlayer);

	// initialization
	static void InitSkillClasses(void);
	static void AddSkillClass(CSkillClass *pSkill);
	static void DeinitSkillClasses(void);

	// accessors
	static SkillClassVec *GetSkillClasses(void);
	static const CSkillClass *GetSkillClassModel(SKILL_CLASS_INDEX iIndex);
	bool SetPlayer(CSDKPlayer *pPlayer);

	// creation
	virtual CSkillClass *Clone(void) = 0; // sends back a pointer to the copy of this class
	static CSkillClass *CreateSkillByName(const char *szName); // sends back a new copy of the specified class
	static CSkillClass *CreateSkillByIndex(SKILL_CLASS_INDEX iIndex); // sends back a new copy based on the SKILL_CLASS_INDEX


	// class info accessors
	virtual const char *GetClassName(void) const = 0; // The name of this class, for display in menus, etc
	virtual const char *GetClassNameShort(void) const = 0; // Short version of the name of the class
	virtual const char *GetInternalClassName(void) const = 0; // An internal name for the class
	virtual const char *GetClassDesc(void) const = 0; // A textual description of this class, for menus
	virtual const char *GetClassLevelStr(void) const = 0; // The string specifying the characteristics that change at each level
	virtual const char *GetClassLevelIconStr(void) const = 0; // The string the icon string
	virtual SKILL_CLASS_INDEX GetClassIndex(void) const = 0; // The unique index/indentifier of this class
	virtual const char *GetHatModelForTeam(int iTeam) const = 0; // Sends back the name of the model to use for the given team

	// levels
	int GetSkillLevel(bool bIgnoreBoost = false);
	int GetLevelBoost(void) { return m_iBoostAmount; }
	bool IsBoosting(void) { return m_iBoostAmount != 0 && GetSkillLevel(true) != SKILL_LEVEL_COUNT; }
	void IncrementLevelBoost(int iAmount, bool bUpdateClient = true);
	void SetLevelBoost(int iBoost);
	float GetSkillLevelPercentage(void);
	float GetNextLevelPercentage(void);

	// modifiers
	virtual float GetSpeedRatio(void);														// done
	virtual float GetAccuracyConeRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction);	// done
	virtual float GetReloadTimeRatio(CWeaponSDKBase *pWeapon);
	virtual float GetRecoilRatio(CWeaponSDKBase *pWeapon);									// done
	virtual float GetNoiseLevelRatio(void);													// done
	virtual float GetArmorStrengthRatio(void);												// done
	virtual float GetRateOfFireRatio(CWeaponSDKBase *pWeapon);								// done
	virtual float GetSniperDriftRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction);	// done
	virtual float GetDamageRatio(CWeaponSDKBase *pWeapon);									// done
	virtual int GetKillXPModifier(CSDKPlayer *pVictim, CWeaponSDKBase *pWeapon);			// done
	virtual float GetMaxStamina(void);														// pants
	virtual bool ProvidesBonus(IBuyableItem *pItem) const { return false; }

	// permissions
	virtual bool CanHaveItem(IBuyableItem *pItem);											// done
	static int s_iSkillLevels[SKILL_LEVEL_COUNT]; // the skill level numbers in array form

protected:
	CSDKPlayer *m_pPlayer; // the player we're attached to

private:
	int m_iBoostAmount;
	static SkillClassVec *s_aSkillTypes; // all the different skill types
};

#define DECLARE_SKILLCLASS(className) CSkillClass::AddSkillClass(new className())

/**
* None skill class definition
**/
class CSkillClassNone : public CSkillClass
{
public:
	// Cloning
	virtual CSkillClass *Clone(void) { return new CSkillClassNone; }

	// CLASS INFO
	virtual const char *GetClassName(void) const { return "None"; }
	virtual const char *GetClassNameShort(void) const { return "None"; }
	virtual const char *GetInternalClassName(void) const { return "None"; }
	virtual const char *GetClassDesc(void) const { return "None"; }
	virtual const char *GetClassLevelStr(void) const { return "None"; }
	virtual const char *GetClassLevelIconStr(void) const { return ""; }
	virtual SKILL_CLASS_INDEX GetClassIndex(void) const { return NONE_CLASS_INDEX; }
	virtual const char *GetHatModelForTeam(int iTeam) const { return iTeam == TEAM_A ? TEAM_A_SOLDIER_HAT_MODEL : TEAM_B_SOLDIER_HAT_MODEL; }
};

/**
* Scout skill class definition
**/
class CSkillClassScout : public CSkillClass
{
public:
	// Cloning
	virtual CSkillClass *Clone(void);

	// CLASS INFO
	virtual const char *GetClassName(void) const { return "Ranger"; }
	virtual const char *GetClassNameShort(void) const { return "Ranger"; }
	virtual const char *GetInternalClassName(void) const { return "Scout"; }
	virtual const char *GetClassDesc(void) const { return "Increased speed, reduced noise"; }
	virtual const char *GetClassLevelStr(void) const { return "+ Speed, + Armor"; }
	virtual const char *GetClassLevelIconStr(void) const { return "A B"; }
	virtual SKILL_CLASS_INDEX GetClassIndex(void) const { return SCOUT_CLASS_INDEX; }
	virtual const char *GetHatModelForTeam(int iTeam) const { return iTeam == TEAM_A ? TEAM_A_SCOUT_HAT_MODEL : TEAM_B_SCOUT_HAT_MODEL; }

	// modifiers
	virtual float GetSpeedRatio(void);
	virtual float GetArmorStrengthRatio(void);
	virtual float GetNoiseLevelRatio(void);
	virtual int GetKillXPModifier(CSDKPlayer *pVictim, CWeaponSDKBase *pWeapon);
	virtual float GetDamageRatio(CWeaponSDKBase *pWeapon);
	virtual float GetAccuracyConeRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction);
	//virtual float GetMaxStamina(void);
	virtual bool ProvidesBonus(IBuyableItem *pItem) const;
};

/**
* Solider skill class definition
**/
class CSkillClassSoldier : public CSkillClass
{
public:
	// Cloning
	virtual CSkillClass *Clone(void);

	// CLASS INFO
	virtual const char *GetClassName(void) const { return "Rifleman"; }
	virtual const char *GetClassNameShort(void) const { return "Rifleman"; }
	virtual const char *GetInternalClassName(void) const { return "Soldier"; }
	virtual const char *GetClassDesc(void) const { return "Increased accuracy with rifles, faster reload"; }
	virtual const char *GetClassLevelStr(void) const { return "+ Accuracy"; }
	virtual const char *GetClassLevelIconStr(void) const { return "C"; }
	virtual SKILL_CLASS_INDEX GetClassIndex(void) const { return SOLDIER_CLASS_INDEX; }
	virtual const char *GetHatModelForTeam(int iTeam) const { return iTeam == TEAM_A ? TEAM_A_SOLDIER_HAT_MODEL : TEAM_B_SOLDIER_HAT_MODEL; }

	// modifiers
	virtual float GetAccuracyConeRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction);
	virtual float GetReloadTimeRatio(CWeaponSDKBase *pWeapon);
	virtual float GetRecoilRatio(CWeaponSDKBase *pWeapon);
	virtual int GetKillXPModifier(CSDKPlayer *pVictim, CWeaponSDKBase *pWeapon);
	//virtual float GetMaxStamina(void);
	virtual bool ProvidesBonus(IBuyableItem *pItem) const;
};

/**
* Sniper skill class definition
**/
class CSkillClassSniper : public CSkillClass
{
public:

	// Cloning
	virtual CSkillClass *Clone(void);

	// CLASS INFO
	virtual const char *GetClassName(void) const { return "Sniper"; }
	virtual const char *GetClassNameShort(void) const { return "Sniper"; }
	virtual const char *GetInternalClassName(void) const { return "Sniper"; }
	virtual const char *GetClassDesc(void) const { return "Increased accuracy with sniper rifles"; }
	virtual const char *GetClassLevelStr(void) const { return "+ Accuracy"; }
	virtual const char *GetClassLevelIconStr(void) const { return "C"; }
	virtual SKILL_CLASS_INDEX GetClassIndex(void) const { return SNIPER_CLASS_INDEX; }
	virtual const char *GetHatModelForTeam(int iTeam) const { return iTeam == TEAM_A ? TEAM_A_SNIPER_HAT_MODEL : TEAM_B_SNIPER_HAT_MODEL; }

	// modifiers
	virtual float GetSniperDriftRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction);
	virtual int GetKillXPModifier(CSDKPlayer *pVictim, CWeaponSDKBase *pWeapon);
	//virtual float GetMaxStamina(void);
	virtual bool ProvidesBonus(IBuyableItem *pItem) const;

	// permissions
	virtual bool CanHaveItem(IBuyableItem *pItem);
};

/**
* Heavy weapons skill class definition
**/
class CSkillClassHW : public CSkillClass
{
public:

	// Cloning
	virtual CSkillClass *Clone(void);

	// CLASS INFO
	virtual const char *GetClassName(void) const { return "Gunner"; }
	virtual const char *GetClassNameShort(void) const { return "Gunner"; }
	virtual const char *GetInternalClassName(void) const { return "HW"; }
	virtual const char *GetClassDesc(void) const { return "Increased armor, reduced recoil"; }
	virtual const char *GetClassLevelStr(void) const { return "+ Speed, + Armor"; }
	virtual const char *GetClassLevelIconStr(void) const { return "A B"; }
	virtual SKILL_CLASS_INDEX GetClassIndex(void) const { return HW_CLASS_INDEX; }
	virtual const char *GetHatModelForTeam(int iTeam) const { return iTeam == TEAM_A ? TEAM_A_HW_HAT_MODEL : TEAM_B_HW_HAT_MODEL; }

	// modifiers
	virtual float GetArmorStrengthRatio(void);
	virtual float GetSpeedRatio(void);
	virtual float GetRecoilRatio(CWeaponSDKBase *pWeapon);
	virtual int GetKillXPModifier(CSDKPlayer *pVictim, CWeaponSDKBase *pWeapon);
	virtual float GetAccuracyConeRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction);
	virtual float GetNoiseLevelRatio(void);
	//virtual float GetMaxStamina(void);
	virtual bool ProvidesBonus(IBuyableItem *pItem) const;
};

#endif
