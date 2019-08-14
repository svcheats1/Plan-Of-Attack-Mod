#include "cbase.h"
#include "skillclass.h"

// Forward declarations
int CSkillClass::s_iSkillLevels[SKILL_LEVEL_COUNT] = {SKILL_LEVEL_1, SKILL_LEVEL_2, SKILL_LEVEL_3, SKILL_LEVEL_4};
SkillClassVec* CSkillClass::s_aSkillTypes = NULL;

/**
* Initializes the skill classes
* @NOTE - THE ORDER OF THE CLASSES DECLARED HERE MUST BE THE SAME ORDER
* AS THE ENUM DEFINING THE SKILLCLASS INDICES
* 
* @return void
**/
void CSkillClass::InitSkillClasses(void)
{
	// declare all of them
	if (s_aSkillTypes == NULL) {
		s_aSkillTypes = new SkillClassVec;
	    
		DECLARE_SKILLCLASS(CSkillClassNone);
		DECLARE_SKILLCLASS(CSkillClassScout);
		DECLARE_SKILLCLASS(CSkillClassSoldier);
		DECLARE_SKILLCLASS(CSkillClassSniper);
		DECLARE_SKILLCLASS(CSkillClassHW);
	}
}

/**
* Destroys the instances in our list
*
* @return void
**/
void CSkillClass::DeinitSkillClasses(void)
{
	// kill it
	if (s_aSkillTypes) {
		s_aSkillTypes->PurgeAndDeleteElements();
		delete s_aSkillTypes;
		s_aSkillTypes = NULL;
	}
}

/**
* Adds a skill class to our cloneable list
*
* @param CSkillClass *pSkill The skill to add
* @return void
**/
void CSkillClass::AddSkillClass(CSkillClass *pSkill)
{
	// add it to the list
	s_aSkillTypes->AddToTail(pSkill);
}

/**
* Sends back the skill class list
*
* @return SkillClssVec *
**/
SkillClassVec *CSkillClass::GetSkillClasses(void)
{
	// that's it... that's all i do...
	return s_aSkillTypes;
}

/**
* Sends back the model of the skill 
*
* @param SKILL_CLASS_INDEX iIndex The index of the skill class
* @return const SkillClass *
**/
const CSkillClass *CSkillClass::GetSkillClassModel(SKILL_CLASS_INDEX iIndex)
{
	// make sure it's valid
	if(!s_aSkillTypes || !s_aSkillTypes->IsValidIndex(iIndex + 1))
		return NULL;

	return s_aSkillTypes->Element(iIndex + 1);
}

/**
* Default constructor
**/
CSkillClass::CSkillClass()
	: m_pPlayer(NULL), m_iBoostAmount(0)
{
}

/**
* Constructor for the skill class.
* All we need is a player
*
* @param CBasePlayer *pPlayer The player who has these skills
* @return void
**/
CSkillClass::CSkillClass(CSDKPlayer *pPlayer)
	: m_pPlayer(pPlayer), m_iBoostAmount(0)
{
}

/**
* Creates the skill using the name specified
* 
* @param const char *szName The name of the skill to create
* @return CSkillClass *
**/
CSkillClass *CSkillClass::CreateSkillByName(const char *szName)
{
	if (!s_aSkillTypes)
		return NULL;

	// iterate through the instances
	for(int i = 0; i < s_aSkillTypes->Count(); ++i)
	{
		// is this the one?
		if(FStrEq(s_aSkillTypes->Element(i)->GetInternalClassName(), szName))
			return s_aSkillTypes->Element(i)->Clone();
	}

	return NULL;
}

CSkillClass *CSkillClass::CreateSkillByIndex(SKILL_CLASS_INDEX iIndex)
{
	if (!s_aSkillTypes || !s_aSkillTypes->IsValidIndex(iIndex + 1)) 
		return NULL;

	return s_aSkillTypes->Element(iIndex + 1)->Clone();
}

/**
* Sets the player if no player has been set yet
* Returns true if the operation was successful
*
* @param CSDKPlayer *pPlayer The player to set
* @return bool
**/
bool CSkillClass::SetPlayer(CSDKPlayer *pPlayer)
{
	// do we have one?
	if(m_pPlayer != NULL)
		return false;

	// set it
	m_pPlayer = pPlayer;
	return true;
}

/**
* Increments the level boost level
* 
* @param int iAmount The amount to boost by
* @param bool bUpdateClient Should we update the client?
* @return void
**/
void CSkillClass::IncrementLevelBoost(int iAmount, bool bUpdateClient /* = true */)
{
	// JD: Since there are no boosts anymore we can save ourselves the trouble.
	/*
	float fOldArmorLevel, fNewArmorLevel;

	// store the old armor level, add the boost, get the new armor level
	fOldArmorLevel = GetArmorStrengthRatio();
	m_iBoostAmount += iAmount;
	fNewArmorLevel = GetArmorStrengthRatio();

#ifndef CLIENT_DLL
	// update the sarmor level for the client
	if(GetClassIndex() != NONE_CLASS_INDEX)
		m_pPlayer->UpdateBoostArmor((fNewArmorLevel - fOldArmorLevel) * mp_basearmor.GetFloat());

	// update the player's info
	if(bUpdateClient)
		m_pPlayer->UpdateWithSkillModifiers();
#endif
	*/
}

/**
* Determines the level of the player based on their experience
*
* @return int
**/
int CSkillClass::GetSkillLevel(bool bIgnoreBoost /* = false */)
{
	int i = 0;

	// do we have a player yet?
	if(!m_pPlayer)
		return 0;

	// which skill level are we in?
	for(; i < SKILL_LEVEL_COUNT; ++i)
	{
		// if we haven't passed this level, send the previous one back
		if(m_pPlayer->GetXP() < s_iSkillLevels[i])
			break;
	}

	// add the boost if we need to
	if(!bIgnoreBoost)
		i += m_iBoostAmount;

	// check the bounds
	if(i > SKILL_LEVEL_COUNT)
		i = SKILL_LEVEL_COUNT;
	else if(i < 0)
		i = 0;

	return i;
}

/**
* Determines the percent completion of the level the player will reach next
* So player is level 2, they are 50% of the way to level 3...
*
* @return float
**/
float CSkillClass::GetNextLevelPercentage(void)
{
	int iSL, iXPInLevel, iLevelSize;

	// get the current level
	iSL = GetSkillLevel(true);

	// how big is the level their working on?
	if(iSL == 0)
		iLevelSize = s_iSkillLevels[iSL];
	else
		iLevelSize = s_iSkillLevels[iSL] - s_iSkillLevels[iSL - 1];

	// how far are we through that level?
	if(iSL == 0)
		iXPInLevel = m_pPlayer->GetXP();
	else
		iXPInLevel = m_pPlayer->GetXP() - s_iSkillLevels[iSL - 1];

	return (float)iXPInLevel / (float)iLevelSize;
}

/**
 * Determines the skill level as a percentage of total skill levels
 *
 * @return float
 */
float CSkillClass::GetSkillLevelPercentage(void)
{
	return ((float)GetSkillLevel() / (float)SKILL_LEVEL_COUNT);
}

/**
* Determines whether or not the current player can have the given item
*
* @param CBasePlayerItem *pItem The item to check on
* @return bool
**/
bool CSkillClass::CanHaveItem(IBuyableItem* pItem)
{
	return true;
}

/**
* Determines the new ratio for the player's speed based on skill level and class
*
* @return float
**/
float CSkillClass::GetSpeedRatio(void)
{
	return 1.0;
}

/**
* Determines the new ratio for the accuracty cone base on skill level and class
*
* @param CWeaponSDKBase *pWeapon The weapon to get the cone ratio for
* @return float
**/
float CSkillClass::GetAccuracyConeRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction)
{
	return 1.0;
}

/**
* Determines the new reload time ratio based on skill level and class
*
* @param CWeaponSDKBase *pWeapon The weapon to get the reload ratio for
* @return float
**/
float CSkillClass::GetReloadTimeRatio(CWeaponSDKBase *pWeapon)
{
	return 1.0;
}

/**
* Determines the ratio of weapon recoil 
*
* @param CWeaponSDKBase *pWeapon The weapon to get the ratio for
* @return float
**/
float CSkillClass::GetRecoilRatio(CWeaponSDKBase *pWeapon)
{
	return 1.0;
}

/**
* Gets the noise level ratio for the player based on skill level and class
*
* @return float
**/
float CSkillClass::GetNoiseLevelRatio(void)
{
	return 1.0;
}

/**
* Determines the armor strength ratio for the player based on skill level and class
*
* @return float
**/
float CSkillClass::GetArmorStrengthRatio(void)
{
	return 1.0;
}

/**
* Determines the rate of fire ratio for the weapon given the player's skill level and class
*
* @param CWeaponSDKBase *pWeapon The weapon the player is using
* @return float
**/
float CSkillClass::GetRateOfFireRatio(CWeaponSDKBase *pWeapon)
{
	return 1.0;
}

/**
* Determines the ratio to use for sniper drift given the player's skill level and class
*
* @param CWeaponSDKBase *pWeapon The weapon to get the ratio for
* @return float
**/
float CSkillClass::GetSniperDriftRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction)
{
	float fScale;

	switch(curAction)
	{
	case PLAYER_ACTION_DUCKING:
		fScale = 0.6;
		break;
	case PLAYER_ACTION_SPRINTING:
	case PLAYER_ACTION_WALKING:
		fScale = 2.0;
		break;
	case PLAYER_ACTION_STANDING:
	default:
		fScale = 1.0;
		break;
	}

	return fScale;
}

/**
* Gets the damage ratio for the given weapon based on skill level and class
* 
* @param CWeaponSDKBase *pWeapon The weapon to get the ratio for
* @return float
**/
float CSkillClass::GetDamageRatio(CWeaponSDKBase *pWeapon)
{
	return 1.0;
}

/**
* Determines the modifier for killing the victim with the given weapon
*
* @param CSDKPlayer *pPlayer The player we killed
* @param CWeaponSDKBase *pWeapon The weapon used
* @return int
**/
int CSkillClass::GetKillXPModifier(CSDKPlayer *pVictim, CWeaponSDKBase *pWeapon)
{
	return 0;
}

/**
* Sets the current level boost for the player
*
* @param int iBoost The boost level to set
* @return void
**/
void CSkillClass::SetLevelBoost(int iBoost)
{ 
	// set the boost to zero
	m_iBoostAmount = 0;

	// increment by the boost
	IncrementLevelBoost(iBoost, true);
}

float CSkillClass::GetMaxStamina(void)
{
	return DEFAULT_STAMINA;
}

/*****************************************************************************/
/******************************** Scout **************************************/

/**
* Clones this instance and returns a pointer to the copy
*
* @return CSkillClass *
**/
CSkillClass* CSkillClassScout::Clone(void)
{
	return new CSkillClassScout(*this);
}

/**
* Determines the new ratio for the player's speed based on skill level and class
*
* @return float
**/
float CSkillClassScout::GetSpeedRatio(void)
{
	return 1.05 + (0.17 * (float)GetSkillLevelPercentage());
}

/**
* Gets the noise level ratio for the player based on skill level and class
*
* @return float
**/
float CSkillClassScout::GetNoiseLevelRatio(void)
{
	return 0.5 - ((float)GetSkillLevel() / ((float)SKILL_LEVEL_COUNT * 2));
}

/**
* Determines the armor strength ratio for the player based on skill level and class
*
* @return float
**/
float CSkillClassScout::GetArmorStrengthRatio(void)
{
	return .75 + (.05 * GetSkillLevel());
}

/**
* Gets the damage ratio for the given weapon based on skill level and class
* 
* @param CWeaponSDKBase *pWeapon The weapon to get the ratio for
* @return float
**/
float CSkillClassScout::GetDamageRatio(CWeaponSDKBase *pWeapon)
{
	// is it a knife?
	if(FClassnameIs(pWeapon, "weapon_knife"))
		return 1.0 + GetSkillLevelPercentage();

	return 1.0;
}

/**
* Determines if we have a bonus for a given weapon type
*
* @param IBuyableItem *pItem The item we want to see if we have a bonus for
* @return bool True if we get a bonus for this weapon
**/
bool CSkillClassScout::ProvidesBonus(IBuyableItem *pItem) const
{
	CWeaponSDKBase *pWeapon;

	// see if we can get a weapon
	pWeapon = dynamic_cast<CWeaponSDKBase *>(pItem);

	// is it a melee weapon?
	if(pWeapon && (pWeapon->GetWeaponType() == WEAPON_TYPE_MELEE || pWeapon->GetWeaponType() == WEAPON_TYPE_PISTOL))
		return true;

	return CSkillClass::ProvidesBonus(pItem);
}

/**
* Determines the modifier for killing the victim with the given weapon
*
* @param CSDKPlayer *pPlayer The player we killed
* @param CWeaponSDKBase *pWeapon The weapon used
* @return int
**/
int CSkillClassScout::GetKillXPModifier(CSDKPlayer *pVictim, CWeaponSDKBase *pWeapon)
{
	// did we get one?
	if(!pWeapon)
		return 0;

	// is it a scout weapon?
	if(pWeapon->GetWeaponType() == WEAPON_TYPE_MELEE || 
		pWeapon->GetWeaponType() == WEAPON_TYPE_PISTOL ||
		pWeapon->GetWeaponType() == WEAPON_TYPE_GRENADE)
		return PLAYER_KILL_IN_CLASS_XP;

	return 0;
}

/**
 * Determines an accuracy ratio for a particular weapon and action
 * @return float
 */
float CSkillClassScout::GetAccuracyConeRatio(CWeaponSDKBase *pWpn, PLAYER_ACTION curAction)
{
	// The normal penalty for walking is 1.5. We want to negate that by 0.1 per level.
	// As a ratio, this is 0.1 / 1.5 per level.
	// 0.1 / 1.5 is the same as 1 / 15, or Level / 15.
	// We then subtract from 1 in order to make it be the final value, not the "bonus" amount
	if (curAction == PLAYER_ACTION_WALKING)
	{
		return 1.0 - ((float)GetSkillLevel() / 15.0);
	}
	return 1.0;
}

/*
float CSkillClassScout::GetMaxStamina(void)
{
	return DEFAULT_STAMINA + 20;
}
*/

/*****************************************************************************/
/******************************** Soldier ************************************/

/**
* Clones this instance and returns a pointer to the copy
*
* @return CSkillClass *
**/
CSkillClass *CSkillClassSoldier::Clone(void)
{
	return new CSkillClassSoldier(*this);
}

/**
* Determines the new ratio for the accuracty cone base on skill level and class
*
* @param CWeaponSDKBase *pWeapon The weapon to get the cone ratio for
* @return float
**/
float CSkillClassSoldier::GetAccuracyConeRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction)
{
	// is this a soldier's weapon?
	if(pWeapon->GetWeaponType() == WEAPON_TYPE_SMG || pWeapon->GetWeaponType() == WEAPON_TYPE_RIFLE)
		return 1.0 - (GetSkillLevel() * 0.15);
	
	return 1.0;
}

/**
* Determines the new reload time ratio based on skill level and class
*
* @param CWeaponSDKBase *pWeapon The weapon to get the reload ratio for
* @return float
**/
float CSkillClassSoldier::GetReloadTimeRatio(CWeaponSDKBase *pWeapon)
{
	// is it a soldier weapon?
	if(pWeapon->GetWeaponType() == WEAPON_TYPE_SMG || pWeapon->GetWeaponType() == WEAPON_TYPE_RIFLE)
		return 1.0 - (GetSkillLevelPercentage() / 3.0);
	
	return 1.0;
}

/**
* Determines the ratio of weapon recoil 
*
* @param CWeaponSDKBase *pWeapon The weapon to get the ratio for
* @return float
**/
float CSkillClassSoldier::GetRecoilRatio(CWeaponSDKBase *pWeapon)
{
	// is it a soldier's weapon?
	if(pWeapon->GetWeaponType() == WEAPON_TYPE_SMG)
		return 1.0 - (GetSkillLevel() * 0.10);
	if(pWeapon->GetWeaponType() == WEAPON_TYPE_RIFLE)
		return 0.8 - (GetSkillLevel() * 0.10);
	
	return 1.0;
}

/**
* Determines the modifier for killing the victim with the given weapon
*
* @param CSDKPlayer *pPlayer The player we killed
* @param CWeaponSDKBase *pWeapon The weapon used
* @return int
**/
int CSkillClassSoldier::GetKillXPModifier(CSDKPlayer *pVictim, CWeaponSDKBase *pWeapon)
{
	// did we get one?
	if(!pWeapon)
		return 0;

	// is it a soldier weapon?
	if(pWeapon->GetWeaponType() == WEAPON_TYPE_SMG || 
		pWeapon->GetWeaponType() == WEAPON_TYPE_RIFLE)
		return PLAYER_KILL_IN_CLASS_XP;

	return 0;
}

/**
* Determines if we have a bonus for a given weapon type
*
* @param IBuyableItem *pItem The item we want to see if we have a bonus for
* @return bool True if we get a bonus for this weapon
**/
bool CSkillClassSoldier::ProvidesBonus(IBuyableItem *pItem) const
{
	CWeaponSDKBase *pWeapon;

	// see if we can get a weapon
	pWeapon = dynamic_cast<CWeaponSDKBase *>(pItem);

	// is it a melee weapon?
	if(pWeapon && (pWeapon->GetWeaponType() == WEAPON_TYPE_SMG || pWeapon->GetWeaponType() == WEAPON_TYPE_RIFLE))
		return true;

	return CSkillClass::ProvidesBonus(pItem);
}

/*
float CSkillClassSoldier::GetMaxStamina(void)
{
	return DEFAULT_STAMINA;
}
*/

/*****************************************************************************/
/******************************** Sniper *************************************/

/**
* Clones this instance and returns a pointer to the copy
*
* @return CSkillClass *
**/
CSkillClass* CSkillClassSniper::Clone(void)
{
	return new CSkillClassSniper(*this);
}

/**
* Determines if a sniper can use a weapon. A sniper can current use any weapon.
*
* @param IBuyableItem *pItem The item we want to buy
* @return bool
*/
bool CSkillClassSniper::CanHaveItem(IBuyableItem *pItem)
{
	return true;
}

/**
* Determines the ratio to use for sniper drift given the player's skill level and class
*
* @param CWeaponSDKBase *pWeapon The weapon to get the ratio for
* @return float
**/
float CSkillClassSniper::GetSniperDriftRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction)
{
	float fScale;

	switch(curAction)
	{
	case PLAYER_ACTION_DUCKING:
		fScale = 0.7;
		break;
	case PLAYER_ACTION_SPRINTING:
	case PLAYER_ACTION_WALKING:
		fScale = 2.0;
		break;
	case PLAYER_ACTION_STANDING:
	default:
		fScale = 1.0;
		break;
	}

	const float fValues[] = { 0.345, 0.259, 0.194, 0.146, 0.109 };
	return fScale * fValues[GetSkillLevel()];
}

/**
* Determines the modifier for killing the victim with the given weapon
*
* @param CSDKPlayer *pPlayer The player we killed
* @param CWeaponSDKBase *pWeapon The weapon used
* @return int
**/
int CSkillClassSniper::GetKillXPModifier(CSDKPlayer *pVictim, CWeaponSDKBase *pWeapon)
{
	// did we get one?
	if(!pWeapon)
		return 0;

	// is it a sniper weapon?
	if(pWeapon->GetWeaponType() == WEAPON_TYPE_SNIPER)
		return PLAYER_KILL_IN_CLASS_XP;

	return 0;
}

/**
* Determines if we have a bonus for a given weapon type
*
* @param IBuyableItem *pItem The item we want to see if we have a bonus for
* @return bool True if we get a bonus for this weapon
**/
bool CSkillClassSniper::ProvidesBonus(IBuyableItem *pItem) const
{
	CWeaponSDKBase *pWeapon;

	// see if we can get a weapon
	pWeapon = dynamic_cast<CWeaponSDKBase *>(pItem);

	// is it a melee weapon?
	if(pWeapon && pWeapon->GetWeaponType() == WEAPON_TYPE_SNIPER)
		return true;

	return CSkillClass::ProvidesBonus(pItem);
}

/*
float CSkillClassSniper::GetMaxStamina(void)
{
	return DEFAULT_STAMINA + 10;
}
*/

/*****************************************************************************/
/***************************** Heavy Weapons *********************************/

/**
* Clones this instance and returns a pointer to the copy
*
* @return CSkillClass *
**/
CSkillClass *CSkillClassHW::Clone(void)
{
	return new CSkillClassHW(*this);
}

/**
* Determines the new ratio for the player's speed based on skill level and class
*
* @return float
**/
float CSkillClassHW::GetSpeedRatio(void)
{
	// between 80% and 100% of normal speed
	return 1.0 - ((float)(SKILL_LEVEL_COUNT - GetSkillLevel()) * 0.05);
}

/**
* Determines the new ratio for the accuracty cone base on skill level and class
*
* @param CWeaponSDKBase *pWeapon The weapon to get the cone ratio for
* @return float
**/
float CSkillClassHW::GetAccuracyConeRatio(CWeaponSDKBase *pWeapon, PLAYER_ACTION curAction)
{
	// if this is a machine gun send back a better accuracy than normal
	if(pWeapon->GetWeaponType() == WEAPON_TYPE_MACHINEGUN)
		return 0.5 * (1.0 - 0.05 * (float)GetSkillLevel());

	return 1.0;
}

/**
* Determines the ratio of weapon recoil 
*
* @param CWeaponSDKBase *pWeapon The weapon to get the ratio for
* @return float
**/
float CSkillClassHW::GetRecoilRatio(CWeaponSDKBase *pWeapon)
{
	// is it a hw weapon?
	if (pWeapon->GetWeaponType() == WEAPON_TYPE_RIFLE ||
		pWeapon->GetWeaponType() == WEAPON_TYPE_MACHINEGUN)
		return 1.0 - (GetSkillLevelPercentage() / 3.0);
	
	return 1.0;
}

/**
* Determines if we have a bonus for a given weapon type
*
* @param IBuyableItem *pItem The item we want to see if we have a bonus for
* @return bool True if we get a bonus for this weapon
**/
bool CSkillClassHW::ProvidesBonus(IBuyableItem *pItem) const
{
	CWeaponSDKBase *pWeapon;

	// see if we can get a weapon
	pWeapon = dynamic_cast<CWeaponSDKBase *>(pItem);

	// is it a melee weapon?
	if(pWeapon && pWeapon->GetWeaponType() == WEAPON_TYPE_MACHINEGUN)
		return true;

	return CSkillClass::ProvidesBonus(pItem);
}

/**
* Determines the armor strength ratio for the player based on skill level and class
*
* @return float
**/
float CSkillClassHW::GetArmorStrengthRatio(void)
{
	return 1.0 + (.25 * GetSkillLevel());
}

/**
* Determines the modifier for killing the victim with the given weapon
*
* @param CSDKPlayer *pPlayer The player we killed
* @param CWeaponSDKBase *pWeapon The weapon used
* @return int
**/
int CSkillClassHW::GetKillXPModifier(CSDKPlayer *pVictim, CWeaponSDKBase *pWeapon)
{
	// did we get one?
	if(!pWeapon)
		return 0;

	// is it a hw weapon?
	if(pWeapon->GetWeaponType() == WEAPON_TYPE_MACHINEGUN || 
		pWeapon->GetWeaponType() == WEAPON_TYPE_ROCKET)
		return PLAYER_KILL_IN_CLASS_XP;

	return 0;
}

/**
* Gets the noise level ratio for the player based on skill level and class
*
* @return float
**/
float CSkillClassHW::GetNoiseLevelRatio(void)
{
	return 1.5 - ((float)GetSkillLevel() / ((float)SKILL_LEVEL_COUNT * 2));
}

/*
float CSkillClassHW::GetMaxStamina(void)
{
	return DEFAULT_STAMINA - 10;
}
*/
