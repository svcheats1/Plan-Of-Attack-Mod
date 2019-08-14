//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "weapon_sdkbase.h"
#include "ammodef.h"
#include "sdk_fx_shared.h"


#if defined( CLIENT_DLL )

	#define CBaseMachineGun C_BaseMachineGun
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ZOOM_FOV_RATIO 0.5
#define ZOOM_ZOOMIN_RATE 0.3
#define ZOOM_ZOOMOUT_RATE 0.3

// ----------------------------------------------------------------------------- //
// Global functions.
// ----------------------------------------------------------------------------- //

//--------------------------------------------------------------------------------------------------------
static const char * s_WeaponAliasInfo[] = 
{
	"none",		// WEAPON_NONE
	"knife",	// WEAPON_KNIFE
	"viking",	// WEAPON_VIKING
	"shotgun",	// WEAPON_SHOTGUN
	"mp5",		// WEAPON_MP5
	"ak107",	// WEAPON_AK107
	"mk48",		// WEAPON_MK48
	"grenade",	// WEAPON_GRENADE
	"m40",		// WEAPON_M40
	"deagle",	// WEAPON_DEAGLE
	"m16",		// WEAPON_M16
	"pp90m1",	// WEAPON_PP90M1
	"m25",		// WEAPON_M25
	"saiga12k", // WEAPON_SAIGA12K
	"ksvk",		// WEAPON_KSVK
	"svu",		// WEAPON_SVU
	"pkm",		// WEAPON_PKM
	"rpg",		// WEAPON_RPG
	NULL,		// WEAPON_MAX
};

//--------------------------------------------------------------------------------------------------------
//
// Given an alias, return the associated weapon ID
//
int AliasToWeaponID( const char *alias )
{
	if (alias)
	{
		for( int i=0; s_WeaponAliasInfo[i] != NULL; ++i )
			if (!Q_stricmp( s_WeaponAliasInfo[i], alias ))
				return i;
	}

	return WEAPON_NONE;
}

//--------------------------------------------------------------------------------------------------------
//
// Given a weapon ID, return its alias
//
const char *WeaponIDToAlias( int id )
{
	if ( (id >= WEAPON_MAX) || (id < 0) )
		return NULL;

	return s_WeaponAliasInfo[id];
}

// ----------------------------------------------------------------------------- //
// CWeaponSDKBase tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSDKBase, DT_WeaponSDKBase )

BEGIN_NETWORK_TABLE( CWeaponSDKBase, DT_WeaponSDKBase )
#ifdef GAME_DLL
	SendPropIntWithMinusOneFlag( SENDINFO(m_iAmmo1), 8 ),
	SendPropIntWithMinusOneFlag( SENDINFO(m_iAmmo2), 8 ),
	// world weapon models have no animations
  	SendPropExclude( "DT_AnimTimeMustBeFirst", "m_flAnimTime" ),
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
#else
	RecvPropIntWithMinusOneFlag( RECVINFO(m_iAmmo1 )),
	RecvPropIntWithMinusOneFlag( RECVINFO(m_iAmmo2 )),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSDKBase )
	DEFINE_PRED_FIELD( m_flTimeWeaponIdle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_iAmmo1, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			
	DEFINE_PRED_FIELD( m_iAmmo2, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_sdk_base, CWeaponSDKBase );

#ifdef GAME_DLL
	BEGIN_DATADESC( CWeaponSDKBase )
		DEFINE_FIELD( m_iAmmo1, FIELD_INTEGER ),
		DEFINE_FIELD( m_iAmmo2, FIELD_INTEGER ),
	END_DATADESC()
#endif

// ----------------------------------------------------------------------------- //
// CWeaponCSBase implementation. 
// ----------------------------------------------------------------------------- //
CWeaponSDKBase::CWeaponSDKBase()
{
	SetPredictionEligible( true );

	AddSolidFlags( FSOLID_TRIGGER ); // Nothing collides with these but it gets touches.

	m_bDoSecondaryZoom = false;
	m_bInZoom = false;
}

void CWeaponSDKBase::Drop(const Vector &velocity)
{
	SetZoom(false);

	BaseClass::Drop(velocity);
}

void CWeaponSDKBase::Equip( CBaseCombatCharacter *pOwner )
{
	SetZoom(false);

	BaseClass::Equip(pOwner);
}

bool CWeaponSDKBase::ShouldPredict()
{
#ifdef CLIENT_DLL
	if (GetOwner() && GetOwner() == CBasePlayer::GetLocalPlayer())
		return true;

	return false;
#else
	return false;
#endif
}

void CWeaponSDKBase::BI_Init()
{
	PrecacheWeaponScriptFile();
}

void CWeaponSDKBase::Precache()
{
#ifdef GAME_DLL
	char szFile[256];
	Q_snprintf(szFile, sizeof(szFile), "vgui/buymenu/weapons/%s", GetClassname());
	engine->PrecacheDecal(szFile, true);
#endif

	BaseClass::Precache();
}

const CSDKWeaponInfo &CWeaponSDKBase::GetSDKWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CSDKWeaponInfo *pSDKInfo;

	#ifdef _DEBUG
		pSDKInfo = dynamic_cast< const CSDKWeaponInfo* >( pWeaponInfo );
		Assert( pSDKInfo );
	#else
		pSDKInfo = static_cast< const CSDKWeaponInfo* >( pWeaponInfo );
	#endif

	return *pSDKInfo;
}

bool CWeaponSDKBase::PlayEmptySound()
{
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();

	EmitSound( filter, entindex(), "Default.ClipEmpty_Rifle" );
	
	return 0;
}

CSDKPlayer* CWeaponSDKBase::GetPlayerOwner() const
{
	return dynamic_cast< CSDKPlayer* >( GetOwner() );
}

void CWeaponSDKBase::SecondaryAttack()
{
#ifdef GAME_DLL
	if (m_bDoSecondaryZoom)
	{
		// can't hold me down
		CSDKPlayer *pPlayer = GetPlayerOwner();
		if (!pPlayer)
			return;

		if (!(pPlayer->m_afButtonPressed & IN_ATTACK2))
			return;

		// zoom in or out
		SetZoom(!m_bInZoom);
	}
#endif
}

void CWeaponSDKBase::SetZoom(bool bZoom)
{
#ifdef GAME_DLL
	if (m_bDoSecondaryZoom) 
	{
		m_bInZoom = bZoom;

		CSDKPlayer *pOwner = GetPlayerOwner();
		if (!pOwner)
			return;

		if(bZoom)
			pOwner->SetFOV(this, pOwner->GetDefaultFOV() * ZOOM_FOV_RATIO, ZOOM_ZOOMIN_RATE);
		else
			pOwner->SetFOV(this, pOwner->GetDefaultFOV(), ZOOM_ZOOMOUT_RATE);
	}
#endif
}

#ifdef GAME_DLL

void CWeaponSDKBase::SendReloadEvents()
{
	CSDKPlayer *pPlayer = dynamic_cast< CSDKPlayer* >( GetOwner() );
	if ( !pPlayer )
		return;

	// Send a message to any clients that have this entity to play the reload.
	/*
	CPASFilter filter( pPlayer->GetAbsOrigin() );
	filter.RemoveRecipient( pPlayer );

    UserMessageBegin( filter, "ReloadEffect" );
	WRITE_SHORT( pPlayer->entindex() );
	MessageEnd();
	*/

	// Make the player play his reload animation.
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
}

#endif

void CWeaponSDKBase::DoRecoil()
{
	CSDKPlayer *pPlayer = dynamic_cast< CSDKPlayer* >( GetOwner() );
	if ( !pPlayer )
		return;

	QAngle ang;
	ang.Init();

	if (GetSDKWpnData().m_iRecoilType == RT_SIMPLE) {
		ang = QAngle(GetSDKWpnData().m_aRecoilValues[0], GetSDKWpnData().m_aRecoilValues[1], GetSDKWpnData().m_aRecoilValues[2]);
	}
	else if (GetSDKWpnData().m_iRecoilType == RT_RANDOM)
	{
		ang = QAngle(
			random->RandomFloat(GetSDKWpnData().m_aRecoilValues[0], GetSDKWpnData().m_aRecoilValues[3]),
			random->RandomFloat(GetSDKWpnData().m_aRecoilValues[1], GetSDKWpnData().m_aRecoilValues[4]),
			random->RandomFloat(GetSDKWpnData().m_aRecoilValues[2], GetSDKWpnData().m_aRecoilValues[5]));
	}
	else if(GetSDKWpnData().m_iRecoilType == RT_RANDOM_GAUSSIAN)
	{
		float rand1, rand2, rand3;

		// @TRJ - changed to use a gaussian randomization function so we can make the punch more random
		rand1 = (GetSDKWpnData().m_aRecoilValues[0] + GetSDKWpnData().m_aRecoilValues[3]) / 2;
		rand1 = RandomGaussianFloat(rand1, .1 * (GetSDKWpnData().m_aRecoilValues[3] - GetSDKWpnData().m_aRecoilValues[0]));
		rand2 = (GetSDKWpnData().m_aRecoilValues[1] + GetSDKWpnData().m_aRecoilValues[4]) / 2;
		rand2 = RandomGaussianFloat(rand2, .1 * (GetSDKWpnData().m_aRecoilValues[4] - GetSDKWpnData().m_aRecoilValues[1]));
		rand3 = (GetSDKWpnData().m_aRecoilValues[2] + GetSDKWpnData().m_aRecoilValues[5]) / 2;
		rand3 = RandomGaussianFloat(rand3, .1 * (GetSDKWpnData().m_aRecoilValues[5] - GetSDKWpnData().m_aRecoilValues[2]));

		ang = QAngle(rand1, rand2, rand3);
	}

	ang *= pPlayer->GetRecoilRatio(this);

	pPlayer->ViewPunch(ang);
}

void CWeaponSDKBase::Spawn()
{
	BaseClass::Spawn();

	m_iClip1 = GetMaxClip1() > 0 ? 0 : -1;
	m_iClip2 = GetMaxClip2() > 0 ? 0 : -1;
	m_iAmmo1 = GetMaxAmmo1() > 0 ? 0 : -1;
	m_iAmmo2 = GetMaxAmmo2() > 0 ? 0 : -1;
}

void CWeaponSDKBase::GiveAmmo1(int iAmount)
{
	m_iAmmo1 = min(m_iAmmo1 + iAmount, GetMaxAmmo1());
}

void CWeaponSDKBase::GiveAmmo2(int iAmount)
{
	m_iAmmo2 = min(m_iAmmo2 + iAmount, GetMaxAmmo2());
}

int CWeaponSDKBase::GetAmmo1() const
{
	return m_iAmmo1;
}

int CWeaponSDKBase::GetAmmo2() const
{
	return m_iAmmo2;
}

int CWeaponSDKBase::GetMaxAmmo1() 
{
	return GetSDKWpnData().m_iMaxAmmo1;
}

int CWeaponSDKBase::GetMaxAmmo2()
{
	return GetSDKWpnData().m_iMaxAmmo2;
}

void CWeaponSDKBase::FillAmmo()
{
	m_iAmmo1 = GetMaxAmmo1();
	m_iAmmo2 = GetMaxAmmo2();
	m_iClip1 = GetMaxClip1();
	m_iClip2 = GetMaxClip2();
}

bool CWeaponSDKBase::UsesPrimaryAmmo()
{
	return (GetMaxAmmo1() > 0);
}

bool CWeaponSDKBase::UsesSecondaryAmmo()
{
	return (GetMaxAmmo2() > 0);
}

bool CWeaponSDKBase::HasPrimaryAmmo()
{
	return (m_iClip1 > 0 || m_iAmmo1 > 0);
}

bool CWeaponSDKBase::HasSecondaryAmmo()
{
	return (m_iClip2 > 0 || m_iAmmo2 > 0);
}

bool CWeaponSDKBase::HasAmmo()
{
	return (m_iClip1 > 0 || m_iAmmo1 > 0 || m_iClip2 > 0 || m_iAmmo2 > 0);
}

bool CWeaponSDKBase::CanBeSelected()
{
	return true;
}

bool CWeaponSDKBase::Reload( )
{
	CSDKPlayer *pPlayer = GetPlayerOwner();

	if (m_iAmmo1 <= 0)
		return false;

	int iResult = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( !iResult )
		return false;

	// This is redundant
	//pPlayer->SetAnimation( PLAYER_RELOAD );

#ifndef CLIENT_DLL
	if ((iResult) && (pPlayer->GetFOV() != pPlayer->GetDefaultFOV()))
	{
		m_bInZoom = false;
		pPlayer->SetFOV( pPlayer, pPlayer->GetDefaultFOV() );
	}
#endif

	pPlayer->m_iShotsFired = 0;

	return true;
}

/**
 * Overriding to use new ammo system.
 */
bool CWeaponSDKBase::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return false;

	// If I don't have any spare ammo, I can't reload
	if (m_iAmmo1 <= 0)
		return false;

	bool bReload = false;

	// If you don't have clips, then don't try to reload them.
	if ( UsesClipsForAmmo1() )
	{
		// need to reload primary clip?
		int primary	= min(iClipSize1 - m_iClip1, m_iAmmo1);
		if (primary != 0)
		{
			bReload = true;
		}
	}

	if ( UsesClipsForAmmo2() )
	{
		// need to reload secondary clip?
		int secondary = min(iClipSize2 - m_iClip2, m_iAmmo2);
		if ( secondary != 0 )
		{
			bReload = true;
		}
	}

	if ( !bReload )
		return false;

#ifdef CLIENT_DLL
	// Play reload
	WeaponSound( RELOAD );
#endif
	SendWeaponAnim( iActivity );

	// Play the player's reload animation
	if ( pOwner->IsPlayer() )
	{
		( ( CBasePlayer * )pOwner)->SetAnimation( PLAYER_RELOAD );
	}

#ifdef GAME_DLL
	SendReloadEvents();
#endif

	float flSequenceEndTime = gpGlobals->curtime + SequenceDuration();
	pOwner->SetNextAttack( flSequenceEndTime );
	m_flNextPrimaryAttack = flSequenceEndTime;

	m_bInReload = true;

	return true;
}

void CWeaponSDKBase::CheckReload( void )
{
	if ( m_bReloadsSingly )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( !pOwner )
			return;

		if ((m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			if ( pOwner->m_nButtons & (IN_ATTACK | IN_ATTACK2) && m_iClip1 > 0 )
			{
				m_bInReload = false;
				return;
			}

			// If out of ammo end reload
			if (m_iAmmo1 <= 0)
			{
				FinishReload();
				return;
			}
			// If clip not full reload again
			else if (m_iClip1 < GetMaxClip1())
			{
				// Add them to the clip
				++m_iClip1;
				--m_iAmmo1;

				Reload();
				return;
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				m_flNextPrimaryAttack	= gpGlobals->curtime;
				m_flNextSecondaryAttack = gpGlobals->curtime;
				return;
			}
		}
	}
	else
	{
		if ( (m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			FinishReload();
			m_flNextPrimaryAttack	= gpGlobals->curtime;
			m_flNextSecondaryAttack = gpGlobals->curtime;
			m_bInReload = false;
		}
	}
}

#ifdef CLIENT_DLL
void CWeaponSDKBase::ProcessMuzzleFlashEvent()
{
	// do not display two muzzle flashes (one from the view model and one from the weapon)
	// if the local player is observing this player from the in-eye cam.
	CSDKPlayer *pPlayer = CSDKPlayer::GetLocalSDKPlayer();
	if (pPlayer && pPlayer->IsObserver() && pPlayer->GetObserverMode() == OBS_MODE_IN_EYE 
		&& pPlayer->GetObserverTarget() && pPlayer->GetObserverTarget() == GetOwner()) {
		return;
	}

	BaseClass::ProcessMuzzleFlashEvent();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Reload has finished.
//-----------------------------------------------------------------------------
void CWeaponSDKBase::FinishReload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner)
	{
		// If I use primary clips, reload primary
		if ( UsesClipsForAmmo1() )
		{
			int primary	= min(GetMaxClip1() - m_iClip1, m_iAmmo1);	
			m_iClip1 += primary;
			m_iAmmo1 -= primary;
		}

		// If I use secondary clips, reload secondary
		if ( UsesClipsForAmmo2() )
		{
			int secondary = min(GetMaxClip2() - m_iClip2, m_iAmmo2);
			m_iClip2 += secondary;
			m_iAmmo2 -= secondary;
		}

		if ( m_bReloadsSingly )
		{
			m_bInReload = false;
		}
	}
}

/**
* Does stuff after we frame
*
* @return void
**/
void CWeaponSDKBase::ItemPostFrame(void)
{
	// are we out of ammo?
	if(HasAnyAmmo() == 0)
		OutOfAmmo();

	// go down
	BaseClass::ItemPostFrame();
}

const char *CWeaponSDKBase::GetDeathNoticeName( void )
{
	return WeaponIDToAlias( GetWeaponID() );
}


float	g_lateralBob;
float	g_verticalBob;

#define	HL2_BOB_CYCLE_MIN	1.0f
#define	HL2_BOB_CYCLE_MAX	0.45f
#define	HL2_BOB			0.002f
#define	HL2_BOB_UP		0.5f

void CWeaponSDKBase::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	// CLIENT ONLY!
#ifdef CLIENT_DLL
	static float flLastVerticalBob = 0.0;
	static float flLastLateralBob = 0.0;

	CBaseEntity *pOwner = GetOwner();
	// client only.. check to see if owner is us!
	if (!pOwner || !CBasePlayer::GetLocalPlayer() ||  pOwner != CBasePlayer::GetLocalPlayer())
		return;
    
	Vector	forward, right;
	AngleVectors( angles, &forward, &right, NULL );

	CalcViewmodelBob();

	// Apply bob, but scaled down to 40%
	VectorMA( origin, g_verticalBob * 0.08f, forward, origin ); // was 0.1
	
	// Z bob a bit more
	origin[2] += g_verticalBob * 0.08f;	// was 0.1

	// Do the lateral bob
	VectorMA( origin, g_lateralBob * 0.64f, right, origin ); // was 0.8

	// bob the angle of the viewmodel
	angles[ ROLL ]	+= g_verticalBob * 0.4f;	// was 0.5
	angles[ PITCH ]	-= g_verticalBob * 0.32f;	// was 0.4
	angles[ YAW ]	-= g_lateralBob  * 0.24f;	// was 0.3

	// bob the viewangle
	QAngle eyeAngles;
	engine->GetViewAngles(eyeAngles);
	eyeAngles[PITCH]	+= (g_verticalBob - flLastVerticalBob) * 0.6f;
	eyeAngles[YAW]		+= (g_lateralBob - flLastLateralBob) * 0.6f; //0.3f;
	flLastVerticalBob = g_verticalBob;
	flLastLateralBob = g_lateralBob;
	engine->SetViewAngles(eyeAngles);
#endif
}

float CWeaponSDKBase::CalcViewmodelBob( void )
{
	static	float bobtime;
	static	float lastbobtime;
	float	cycle;
	
	CBasePlayer *player = ToBasePlayer( GetOwner() );
	//Assert( player );

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	float speed = player->GetLocalVelocity().Length2D();

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	speed = clamp( speed, -320, 320 );

	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );
	
	bobtime += ( gpGlobals->curtime - lastbobtime ) * bob_offset;
	lastbobtime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = bobtime - (int)(bobtime/HL2_BOB_CYCLE_MAX)*HL2_BOB_CYCLE_MAX;
	cycle /= HL2_BOB_CYCLE_MAX;

	if ( cycle < HL2_BOB_UP )
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-HL2_BOB_UP)/(1.0 - HL2_BOB_UP);
	}
	
	g_verticalBob = speed*0.005f;
	g_verticalBob = g_verticalBob*0.3 + g_verticalBob*0.7*sin(cycle);

	g_verticalBob = clamp( g_verticalBob, -7.0f, 4.0f );

	//Calculate the lateral bob
	cycle = bobtime - (int)(bobtime/HL2_BOB_CYCLE_MAX*2)*HL2_BOB_CYCLE_MAX*2;
	cycle /= HL2_BOB_CYCLE_MAX*2;

	if ( cycle < HL2_BOB_UP )
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-HL2_BOB_UP)/(1.0 - HL2_BOB_UP);
	}

	g_lateralBob = speed*0.005f;
	g_lateralBob = g_lateralBob*0.3 + g_lateralBob*0.7*sin(cycle);
	g_lateralBob = clamp( g_lateralBob, -7.0f, 4.0f );
	
	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}



//////////////////////
// Implementation of IBuyableItem interface
//////////////////////
int CWeaponSDKBase::GetPrice(void) const
{
	/*
#ifdef _DEBUG
	return 100;
#else
	*/
	return GetSDKWpnData().m_iPrice;
//#endif;
}

int CWeaponSDKBase::GetPosition(void) const
{
	return GetSDKWpnData().m_iItemPosition;
}

BI_Type CWeaponSDKBase::GetType(void) const
{
	return GetSDKWpnData().m_iItemType;
}

const char* CWeaponSDKBase::GetItemName(void) const
{
	return GetSDKWpnData().szPrintName;
}

BI_Team CWeaponSDKBase::GetBuyableTeam(void) const
{
	return GetSDKWpnData().m_iBuyableTeam;
}

///////////////////////////////////////////////////
// CBaseMachineGun
///////////////////////////////////////////////////

IMPLEMENT_NETWORKCLASS_ALIASED( BaseMachineGun, DT_BaseMachineGun )

BEGIN_NETWORK_TABLE( CBaseMachineGun, DT_BaseMachineGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CBaseMachineGun )
END_PREDICTION_DATA()
 
LINK_ENTITY_TO_CLASS( weapon_base_machinegun, CBaseMachineGun );

CBaseMachineGun::CBaseMachineGun() { }

void CBaseMachineGun::Spawn()
{
	m_flCurrentAM = 2.0;
	m_flIdleTime = 5.0;

	BaseClass::Spawn();
}

bool CBaseMachineGun::Deploy( )
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	pPlayer->m_iShotsFired = 0;

	m_flCurrentAM = 2.0;

	return BaseClass::Deploy();
}

void CBaseMachineGun::PrimaryAttack( void )
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if (!IsAutomaticWeapon() && !(pPlayer->m_afButtonPressed & IN_ATTACK))
		return;

	const CSDKWeaponInfo &pWeaponInfo = GetSDKWpnData();
	float flCycleTime = pWeaponInfo.m_flCycleTime;

	if (pPlayer->GetSkillClass())
		flCycleTime *= pPlayer->GetSkillClass()->GetRateOfFireRatio(this);

	bool bPrimaryMode = true;

	pPlayer->m_iShotsFired++;

	// Out of ammo?
	if ( m_iClip1 <= 0 )
	{
		if (m_bFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
		}
	}

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	m_iClip1--;

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FX_FireBullets(
		pPlayer->entindex(),
		pPlayer->Weapon_ShootPosition(),
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle() + pPlayer->GetSniperDrift(),
		GetWeaponID(),
		bPrimaryMode ? Primary_Mode : Secondary_Mode,
		CBaseEntity::GetPredictionRandomSeed() & 255,
		GetSpread(pPlayer) );
	
	pPlayer->DoMuzzleFlash();

	DoRecoil();
	
	GrowSpread();

	pPlayer->m_flNextAttack = m_flNextPrimaryAttack = gpGlobals->curtime + flCycleTime;

	if (!m_iClip1 && m_iAmmo1 <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	// start idle animation in 5 seconds
	SetWeaponIdleTime( gpGlobals->curtime + m_flIdleTime );
}

float CBaseMachineGun::GetSpread(CSDKPlayer *pPlayer)
{
	// jumping/falling/etc
	return GetSDKWpnData().m_flBaseSpread * m_flCurrentAM * pPlayer->GetAimRatio(this);
}

void CBaseMachineGun::GrowSpread()
{
	m_flCurrentAM = min(m_flCurrentAM + GetSDKWpnData().m_flGrowRateAM, GetSDKWpnData().m_flMaxAM);
}

void CBaseMachineGun::ShrinkSpread()
{
	m_flCurrentAM = max(m_flCurrentAM - GetSDKWpnData().m_flShrinkRateAM * gpGlobals->frametime, GetSDKWpnData().m_flMinAM);
}

void CBaseMachineGun::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	ShrinkSpread();
}

void CBaseMachineGun::ItemBusyFrame()
{
	BaseClass::ItemBusyFrame();

	ShrinkSpread();
}

void CBaseMachineGun::WeaponIdle()
{
	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	// only idle if the slid isn't back
	if ( m_iClip1 != 0 )
	{
		SetWeaponIdleTime( gpGlobals->curtime + m_flIdleTime );
		SendWeaponAnim( ACT_VM_IDLE );
	}
}