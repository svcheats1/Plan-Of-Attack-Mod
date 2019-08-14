#ifndef _WEAPON_SNIPER_H_
#define _WEAPON_SNIPER_H_

#include "weapon_sdkbase.h"
#include "sdk_fx_shared.h"

#ifdef CLIENT_DLL
	#define	CBaseSniperRifle C_BaseSniperRifle
	#include "c_sdk_player.h"
	#include "buymenu.h"
	#include "hud_sniperscope.h"
#else
	#include "sdk_player.h"
#endif

#define SCOPE_FOV_RATIO 0.25
#define SCOPE_ZOOMIN_RATE 0.4
#define SCOPE_ZOOMOUT_RATE 0.0

/**
* Class declaration for a base sniper rifle
**/
class CBaseSniperRifle : public CBaseMachineGun
{
public:
	DECLARE_CLASS(CBaseSniperRifle, CBaseMachineGun);
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	// constructor
	CBaseSniperRifle();

	// inherited methods
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
	virtual bool ShouldDrawCrosshair(void) const { return false; }
	virtual bool IsAutomaticWeapon(void) const { return false; }
	virtual float GetSpread(void) { return 0; }
	virtual bool Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	virtual void Drop(const Vector &velocity);
	virtual bool Deploy(void);
	virtual bool Reload(void);
	virtual void OutOfAmmo(void);
	virtual void PlayerSpawn(void);

	// accessors
	bool IsScopeHidden(void) { return !m_bDrawScope; }
	void ShowScope(bool bShouldDraw);

protected:

	// scope
#ifdef CLIENT_DLL
	virtual HudSniperScope *GetScope(void);
#endif
	virtual void BoltAction(void);

	bool m_bHasBoltAction;
	bool m_bZoomOutAfterFire;
	bool m_bDoBoltAction;

private:
	// scope actions
	CNetworkVar(bool, m_bDrawScope);
	bool m_bForcedZoomOut;
	
#ifdef CLIENT_DLL
	// scope
	HudSniperScope *m_pScope;
#endif

	// copy constructor
	CBaseSniperRifle(const CBaseSniperRifle &);
};

#endif