//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"
#include "gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "weapon_basesdkgrenade.h"
#include "in_buttons.h"	

#ifdef CLIENT_DLL
	#include "weapon_selection.h"
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"
	#include "items.h"

#endif


#define GRENADE_TIMER	1.5f //Seconds


IMPLEMENT_NETWORKCLASS_ALIASED( BaseSDKGrenade, DT_BaseSDKGrenade )

BEGIN_NETWORK_TABLE(CBaseSDKGrenade, DT_BaseSDKGrenade)
#ifndef CLIENT_DLL
	SendPropBool( SENDINFO(m_bRedraw) ),
	SendPropBool( SENDINFO(m_bPinPulled) ),
	SendPropBool( SENDINFO(m_bThrowing) ),
	SendPropFloat( SENDINFO(m_fThrowTime), 0, SPROP_NOSCALE ),
#else
	RecvPropBool( RECVINFO(m_bRedraw) ),
	RecvPropBool( RECVINFO(m_bPinPulled) ),
	RecvPropBool( RECVINFO(m_bThrowing) ),
	RecvPropFloat( RECVINFO(m_fThrowTime) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CBaseSDKGrenade )
	DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bPinPulled, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bThrowing, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_basesdkgrenade, CBaseSDKGrenade );


CBaseSDKGrenade::CBaseSDKGrenade()
{
	m_bThrowing = false;
	m_bRedraw = false;
	m_bPinPulled = false;
	m_fThrowTime = 0;
	m_iOldAmmo1 = 0;
}

void CBaseSDKGrenade::Spawn()
{
	BaseClass::Spawn();

	// Spawn as 1 grenade
	m_iAmmo1 = 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseSDKGrenade::Precache()
{
	BaseClass::Precache();
}

void CBaseSDKGrenade::FillAmmo()
{
	// do nothing.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseSDKGrenade::Deploy()
{
	m_bThrowing = false;
	m_bRedraw = false;
	m_bPinPulled = false;
	m_fThrowTime = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseSDKGrenade::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bThrowing = false;
	m_bRedraw = false;
	m_bPinPulled = false; // when this is holstered make sure the pin isn’t pulled.
	m_fThrowTime = 0;

#ifndef CLIENT_DLL
	// If they attempt to switch weapons before the throw animation is done, 
	// allow it, but kill the weapon if we have to.
	CSDKPlayer *pPlayer = GetPlayerOwner();
	
	if( pPlayer->IsAlive() && GetAmmo1() <= 0 )
	{
		CBaseCombatCharacter *pOwner = (CBaseCombatCharacter *)pPlayer;
		pOwner->Weapon_Drop( this );
		UTIL_Remove(this);
	}
#endif
	
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseSDKGrenade::PrimaryAttack()
{
	if ( m_bRedraw || m_bPinPulled || m_bThrowing )
		return;

	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer || GetAmmo1() <= 0 )
		return;

	// The pull pin animation has to finish, then we wait until they aren't holding the primary
	// attack button, then throw the grenade.
	SendWeaponAnim( ACT_VM_PULLPIN );
	m_bPinPulled = true;
	
	// Don't let weapon idle interfere in the middle of a throw!
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseSDKGrenade::SecondaryAttack()
{
	if ( m_bRedraw )
		return;

	CSDKPlayer *pPlayer = GetPlayerOwner();
	
	if ( pPlayer == NULL )
		return;

	//See if we're ducking
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		//Send the weapon animation
		SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	}
	else
	{
		//Send the weapon animation
		SendWeaponAnim( ACT_VM_HAULBACK );
	}

	// Don't let weapon idle interfere in the middle of a throw!
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseSDKGrenade::Reload()
{
	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();

		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseSDKGrenade::ItemPostFrame()
{
	CSDKPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	CBaseViewModel *vm = pPlayer->GetViewModel( m_nViewModelIndex );
	if ( !vm )
		return;

	// If they let go of the fire button, they want to throw the grenade.
	if ( m_bPinPulled && !(pPlayer->m_nButtons & IN_ATTACK) && gpGlobals->curtime >= m_flTimeWeaponIdle ) 
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_THROW_GRENADE );

		StartGrenadeThrow();
		
		// @TRJ - we need to decrement the ammo later so that we don't
		// switch weapons when we start the weapon idle again.
		// if we don't release the mouse button before then we will have 
		// switched weapons and never launched the grenade
		//DecrementAmmo( pPlayer );
	
		m_bPinPulled = false;
		m_bThrowing = true;
		SendWeaponAnim( ACT_VM_THROW );	
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}
	else if ((m_fThrowTime > 0) && (m_fThrowTime < gpGlobals->curtime))
	{
		// @TRJ - decrement the grenade ammo right before we throw
		DecrementAmmo( pPlayer );

#ifndef CLIENT_DLL
		// @TRJ - send out the weapon_fire event
		// @JD - moved this here, so a fire doesn't get called when you pull pin but dont let go
		IGameEvent *pEvent = gameeventmanager->CreateEvent("weapon_fire");
		if(pEvent)
		{
			// pull the skill
			CSkillClass *pSkill = pPlayer->GetSkillClass();

			// set all the data
			pEvent->SetString("weapon", "grenade_projectile");
			pEvent->SetString("skill", pSkill ? pSkill->GetInternalClassName() : "none");
			pEvent->SetInt("userid", pPlayer->GetUserID());

			// send it out
			gameeventmanager->FireEvent(pEvent, true);
		}
#endif	

		ThrowGrenade();
	}
	else if( m_bRedraw )
	{
		// Has the throw animation finished playing
		if( m_flTimeWeaponIdle < gpGlobals->curtime )
		{
#ifdef GAME_DLL
			// if we're officially out of grenades, ditch this weapon
			if( GetAmmo1() <= 0 )
			{
				pPlayer->Weapon_Drop( this, NULL, NULL );
				UTIL_Remove(this);
			}
			else
			{
				pPlayer->SwitchToNextBestWeapon( this );
			}
#endif
			return;	//don't animate this grenade any more!
		}	
	}
	else if( !m_bRedraw )
	{
		BaseClass::ItemPostFrame();
	}
}

void CBaseSDKGrenade::PlayerSpawn()
{
	if (m_bPinPulled) {
		m_bPinPulled = false;
		SendWeaponAnim( ACT_VM_DRAW );
	}

	BaseClass::PlayerSpawn();
}

void CBaseSDKGrenade::PlayerKilled()
{
	if (m_bPinPulled || m_bThrowing || (m_fThrowTime > 0 && m_fThrowTime < gpGlobals->curtime))
	{
		DropGrenade();
	}

	m_bRedraw = false;
}

#ifdef CLIENT_DLL

	void CBaseSDKGrenade::DecrementAmmo( CBaseCombatCharacter *pOwner )
	{
		m_iAmmo1--;
	}

	void CBaseSDKGrenade::DropGrenade()
	{
		m_bThrowing = false;
		m_bRedraw = true;
		m_fThrowTime = 0.0f;
	}

	void CBaseSDKGrenade::ThrowGrenade()
	{
		m_bThrowing = false;
		m_bRedraw = true;
		m_fThrowTime = 0.0f;
	}

	void CBaseSDKGrenade::StartGrenadeThrow()
	{
		m_fThrowTime = gpGlobals->curtime + 0.1f;
	}

#else

	BEGIN_DATADESC( CBaseSDKGrenade )
		DEFINE_FIELD( m_bRedraw, FIELD_BOOLEAN ),
	END_DATADESC()

	int CBaseSDKGrenade::CapabilitiesGet()
	{
		return bits_CAP_WEAPON_RANGE_ATTACK1; 
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *pOwner - 
	//-----------------------------------------------------------------------------
	void CBaseSDKGrenade::DecrementAmmo( CBaseCombatCharacter *pOwner )
	{
		m_iAmmo1--;
	}

	void CBaseSDKGrenade::StartGrenadeThrow()
	{
		m_fThrowTime = gpGlobals->curtime + 0.1f;
	}

	void CBaseSDKGrenade::ThrowGrenade()
	{
		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
		if ( !pPlayer )
		{
			Assert( false );
			return;
		}

		QAngle angThrow = pPlayer->LocalEyeAngles();

		Vector vForward, vRight, vUp;

		if (angThrow.x < 90 )
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);
		else
		{
			angThrow.x = 360.0f - angThrow.x;
			angThrow.x = -10 + angThrow.x * -((90 - 10) / 90.0);
		}

		float flVel = (90 - angThrow.x) * 6;

		if (flVel > 750)
			flVel = 750;

		AngleVectors( angThrow, &vForward, &vRight, &vUp );

		Vector vecSrc = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset();

		vecSrc += vForward * 16;
	
		Vector vecThrow = vForward * flVel + pPlayer->GetAbsVelocity();

		EmitGrenade( vecSrc, vec3_angle, vecThrow, AngularImpulse(600,random->RandomInt(-1200,1200),0), pPlayer );

		m_bRedraw = true;
		m_bThrowing = false;
		m_fThrowTime = 0.0f;
	}

	void CBaseSDKGrenade::DropGrenade()
	{
		if (GetAmmo1() <= 0)
			return;

		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
		if ( !pPlayer )
		{
			Assert( false );
			return;
		}

		Vector vForward;
		pPlayer->EyeVectors( &vForward );
		Vector vecSrc = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset() + vForward * 16; 

		Vector vecVel = pPlayer->GetAbsVelocity();

		EmitGrenade( vecSrc, vec3_angle, vecVel, AngularImpulse(600,random->RandomInt(-1200,1200),0), pPlayer );

		m_bRedraw = true;
		m_fThrowTime = 0.0f;

		DecrementAmmo(pPlayer);
	}

	void CBaseSDKGrenade::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer )
	{
		Assert( 0 && "CBaseSDKGrenade::EmitGrenade should not be called. Make sure to implement this in your subclass!\n" );
	}

	bool CBaseSDKGrenade::AllowsAutoSwitchFrom( void ) const
	{
		return !m_bPinPulled;
	}

#endif

void CBaseSDKGrenade::OnDataChanged( DataUpdateType_t updateType )
{
#ifdef CLIENT_DLL
	if (m_iOldAmmo1 < m_iAmmo1 && m_iAmmo1 > 1 && CBasePlayer::GetLocalPlayer() == GetOwner() && 
		ShouldDrawPickup() && WeaponState() != WEAPON_NOT_CARRIED)
	{
		// Tell the HUD this weapon's been picked up
		CBaseHudWeaponSelection *pHudSelection = GetHudWeaponSelection();
		if ( pHudSelection )
		{
			pHudSelection->OnWeaponPickup( this );
		}

		GetOwner()->EmitSound( "Player.PickupWeapon" );
	}

	m_iOldAmmo1 = m_iAmmo1;

	BaseClass::OnDataChanged(updateType);
#endif
}

bool CBaseSDKGrenade::EquipAmmoFromWeapon(CBaseCombatWeapon *pWeapon)
{
	int iOldAmmo = GetAmmo1();

	GiveAmmo1(1);	//increments the ammo ammount

	return (GetAmmo1() != iOldAmmo);
}

#ifndef CLIENT_DLL

bool CBaseSDKGrenade::DropSpecial()
{
	// we have more than one grenade. drop one and keep this weapon around.
	if (m_iAmmo1 > 1) {
		CBaseEntity *pEnt;
		CSDKPlayer *pPlayer = GetPlayerOwner();
		//CWeaponSDKBase *pWpn;

		// create the entity
		pEnt = CreateEntityByName(GetClassname());
		Assert(pEnt);
		if (!pEnt) return false;

		pEnt->SetLocalOrigin( pPlayer->GetLocalOrigin() );
		pEnt->AddSpawnFlags( SF_NORESPAWN );

		// where to send it off from
		Vector vThrowPos = pPlayer->Weapon_ShootPosition() - Vector(0,0,12);
		pEnt->SetAbsOrigin( vThrowPos );

		// angle to send it off from
		QAngle gunAngles;
		VectorAngles( pPlayer->BodyDirection2D(), gunAngles );
		pEnt->SetAbsAngles( gunAngles );

		pEnt->SetOwnerEntity( NULL );

		CBaseCombatWeapon *pWpn = (CBaseCombatWeapon *) pEnt;
		pWpn->m_iState = WEAPON_NOT_CARRIED;
		pWpn->SetOwner( NULL );

		// finally, spawn it
		if (DispatchSpawn( pEnt ) < 0) {
			Assert(0);
			return false;
		}

		// spawn messs this up, so put it after spawn
		// Determine the vector for its initial velocity
		Vector vecThrow = pPlayer->BodyDirection3D() * 400.0f; // 400.0f is throwForce

		// try to do it through the physics engine
		IPhysicsObject *pObj = pWpn->VPhysicsGetObject();
		if ( pObj != NULL )
		{
			AngularImpulse angImp( 200, 200, 200 );
			pObj->AddVelocity( &vecThrow, &angImp );
		}
		else
			pEnt->SetAbsVelocity( vecThrow );

		pEnt->SetGroundEntity( NULL );
		pEnt->SetThink( &CBaseCombatWeapon::SetPickupTouch );
		pEnt->SetTouch(NULL);
		pEnt->SetNextThink( gpGlobals->curtime + 1.0f );

		// decrement our ammo
        --m_iAmmo1;
		
		Reload();

		// tell them not to drop 
		return true;
	}

	// just one grenade, so we can drop this one using the normal drop system.
	return false;
}

#endif