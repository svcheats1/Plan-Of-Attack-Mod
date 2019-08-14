//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sniper.h"
#include "ammodef.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	
	#include "c_sdk_player.h"
	#include "hud_stamina.h"

#else

	#include "sdk_player.h"

#endif

#include "movevars_shared.h"
#include "gamevars_shared.h"
#include "takedamageinfo.h"
#include "effect_dispatch_data.h"
#include "engine/ivdebugoverlay.h"

ConVar sv_showimpacts("sv_showimpacts", "0", FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point" );

BEGIN_PREDICTION_DATA( CSDKPlayer )
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_iShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ), 
	DEFINE_PRED_FIELD_TOL( m_flStamina, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 1.0f ),
END_PREDICTION_DATA()

void DispatchEffect( const char *pName, const CEffectData &data );

CWeaponSDKBase* CSDKPlayer::SDKAnim_GetActiveWeapon()
{
	return GetActiveSDKWeapon();
}

bool CSDKPlayer::SDKAnim_CanMove()
{
	return true;
}

void CSDKPlayer::FireBullet( 
						   Vector vecSrc,	// shooting postion
						   const QAngle &shootAngles,  //shooting angle
						   float vecSpread, // spread vector
						   int iDamage, // base damage
						   int iBulletType, // ammo type
						   CBaseEntity *pevAttacker, // shooter
						   bool bDoEffects,	// create impact effect ?
						   float x,	// spread x factor
						   float y	// spread y factor
						   )
{
	float fCurrentDamage = iDamage;   // damage of the bullet at it's current trajectory
	//float flCurrentDistance = 0.0;  //distance that the bullet has traveled so far

	Vector vecDirShooting, vecRight, vecUp;
	AngleVectors( shootAngles, &vecDirShooting, &vecRight, &vecUp );

	if ( !pevAttacker )
		pevAttacker = this;  // the default attacker is ourselves

	// add the spray 
	Vector vecDir = vecDirShooting +
		x * vecSpread * vecRight +
		y * vecSpread * vecUp;

	VectorNormalize( vecDir );

	float flMaxRange = 8000;

	Vector vecEnd = vecSrc + vecDir * flMaxRange; // max bullet range is 10000 units

	trace_t tr; // main enter bullet trace

	UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID|CONTENTS_DEBRIS|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0f )
			return; // we didn't hit anything, stop tracing shoot

	if ( sv_showimpacts.GetBool() )
	{
#ifdef CLIENT_DLL
		// draw red client impact markers
		debugoverlay->AddBoxOverlay( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), 255,0,0,127, 4 );

		if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			C_BasePlayer *player = ToBasePlayer( tr.m_pEnt );
			player->DrawClientHitboxes( 4, true );
		}
#else
		// draw blue server impact markers
		NDebugOverlay::Box( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), 0,0,255,127, 4 );

		if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			CBasePlayer *player = ToBasePlayer( tr.m_pEnt );
			player->DrawServerHitboxes( 4, true );
		}
#endif
	}

		//calculate the damage based on the distance the bullet travelled.
		/*
		flCurrentDistance += tr.fraction * flMaxRange;

		// damage get weaker of distance
		fCurrentDamage *= pow ( 0.85f, (flCurrentDistance / 500));
		*/

		int iDamageType = DMG_BULLET | DMG_NEVERGIB;

		if( bDoEffects )
		{
			// See if the bullet ended up underwater + started out of the water
			if ( enginetrace->GetPointContents( tr.endpos ) & (CONTENTS_WATER|CONTENTS_SLIME) )
			{	
				trace_t waterTrace;
				UTIL_TraceLine( vecSrc, tr.endpos, (MASK_SHOT|CONTENTS_WATER|CONTENTS_SLIME), this, COLLISION_GROUP_NONE, &waterTrace );

				if( waterTrace.allsolid != 1 )
				{
					CEffectData	data;
					data.m_vOrigin = waterTrace.endpos;
					data.m_vNormal = waterTrace.plane.normal;
					data.m_flScale = random->RandomFloat( 8, 12 );

					if ( waterTrace.contents & CONTENTS_SLIME )
					{
						data.m_fFlags |= FX_WATER_IN_SLIME;
					}

					DispatchEffect( "gunshotsplash", data );
				}
			}
			else
			{
				//Do Regular hit effects

				// Don't decal nodraw surfaces
				if ( !( tr.surface.flags & (SURF_SKY|SURF_NODRAW|SURF_HINT|SURF_SKIP) ) )
				{
					CBaseEntity *pEntity = tr.m_pEnt;
					if ( !( !friendlyfire.GetBool() && pEntity && pEntity->IsPlayer() && pEntity->GetTeamNumber() == GetTeamNumber() ) )
					{
						UTIL_ImpactTrace( &tr, iDamageType );
					}
				}
			}
		} // bDoEffects

		// add damage to entity that we hit

		ClearMultiDamage();

		CTakeDamageInfo info( pevAttacker, pevAttacker, fCurrentDamage, iDamageType );
		CalculateBulletDamageForce( &info, iBulletType, vecDir, tr.endpos, 0.01 );
		tr.m_pEnt->DispatchTraceAttack( info, vecDir, &tr );

#ifdef GAME_DLL
		TraceAttackToTriggers( info, tr.startpos, tr.endpos, vecDir );
#endif

		ApplyMultiDamage();

		// do we have a weapon?
		static int iTracerCount = 0;
		if(GetActiveWeapon())
		{
			if (iTracerCount++ % 2 != 0)
				return;

			int iAttachment;
			Vector vecTracerSrc;
			QAngle qaJunk;

			// figure out where to fire from
			// @TODO - add a muzzle attachment to weapons
			iAttachment = GetActiveWeapon()->LookupAttachment("muzzle");
			if(iAttachment == 0)
				iAttachment = 1;

			// try to get the attachment
			if(!GetActiveWeapon()->GetAttachment(iAttachment, vecTracerSrc, qaJunk))
				DevMsg("Couldn't find muzzle attachment point!\n");

			// set up the trace
			trace_t sBulletTracer;
			sBulletTracer = tr;
			sBulletTracer.startpos = vecTracerSrc;

			// fire the tracer
			MakeTracer(vecTracerSrc, sBulletTracer, 0);
		}
}

/**
* SDK implementation of MakeTracer.  Creates a tracer for a bullet that was fired
*
* @param const Vector &vecTracerSrc Where the tracer starts from
* @param const trace_t &tr The tracer to use.  Gives us the destination from fire bullet
* @param int iTracerType The type of tracer to use.  We're fucking with this
* @return void
**/
void CSDKPlayer::MakeTracer(const Vector &vecTracerSrc, const trace_t &tr, int iTracerType)
{		
	// compare the string
	iTracerType = TRACER_NONE;
	if(FStrEq(TRACER_TYPE_LINE, GetActiveWeapon()->GetTracerType()))
		iTracerType = TRACER_LINE;

	// if it's none, then don't bother
	if(iTracerType == TRACER_NONE)
		return;

	// call the base class
	BaseClass::MakeTracer(vecTracerSrc, tr, iTracerType);
}

bool CSDKPlayer::CanSprint()
{
	// need to have stamina and be on the ground
	return m_flStamina > 0 && (GetFlags() & FL_ONGROUND);
}

bool CSDKPlayer::IsSprinting()
{
	// i'm not ducked/ducking, i want to sprint, my buttons say i want to sprint, and i'm moving
	return 
		!(m_Local.m_bDucked && !m_Local.m_bDucking) 
		&& m_Local.m_bInSprint 
		&& (m_nButtons & IN_SPEED)
		&& IsWalking();
}

bool CSDKPlayer::IsWalking()
{
	return GetLocalVelocity().Length() > 40;
}

bool CSDKPlayer::IsDucking()
{
	return ((GetFlags() & (FL_ONGROUND | FL_DUCKING)) != 0);
}

PLAYER_ACTION CSDKPlayer::GetPlayerAction()
{
	if (GetMoveType() == MOVETYPE_LADDER)
		return PLAYER_ACTION_CLIMBING;

	if (CanSprint() && IsSprinting())
		return PLAYER_ACTION_SPRINTING;

	if (GetFlags() & FL_ONGROUND) {
        if (IsDucking())
			return PLAYER_ACTION_DUCKING;

		if (IsWalking())
			return PLAYER_ACTION_WALKING;
		
		return PLAYER_ACTION_STANDING;
	}
	else {
		if (m_Local.m_bInDuckJump)
			return PLAYER_ACTION_JUMPING;
		if (m_Local.m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
			return PLAYER_ACTION_FALLING_HURT;

		return PLAYER_ACTION_FALLING;
	}

	return PLAYER_ACTION_UNKNOWN;
}

float CSDKPlayer::GetAimRatio(CWeaponSDKBase *pWpn)
{
	PLAYER_ACTION curAction = GetPlayerAction();
	float flSkillRatio = (GetSkillClass() ? GetSkillClass()->GetAccuracyConeRatio(pWpn, curAction) : 1.0);
	float flActionRatio = 1.0;

	switch(curAction)
	{
	case PLAYER_ACTION_DUCKING:
		flActionRatio = 0.666666666;
		break;
	case PLAYER_ACTION_FALLING:
	case PLAYER_ACTION_WALKING:
		flActionRatio = 1.5;
		break;
	case PLAYER_ACTION_SPRINTING:
		flActionRatio = 3.0;
		break;
	case PLAYER_ACTION_CLIMBING:
	case PLAYER_ACTION_FALLING_HURT:
	case PLAYER_ACTION_JUMPING:
		flActionRatio = 2.0;
		break;
	case PLAYER_ACTION_STANDING:
	default:
		flActionRatio = 1.0;
	}

	return flSkillRatio * flActionRatio;
}

float CSDKPlayer::GetRecoilRatio(CWeaponSDKBase *pWpn)
{
	float flSkillRatio = 1.0;
	float flActionRatio;

	PLAYER_ACTION curAction = GetPlayerAction();
	if (GetSkillClass())
		flSkillRatio = GetSkillClass()->GetRecoilRatio(pWpn);

    switch(curAction)
	{
	case PLAYER_ACTION_DUCKING:
		flActionRatio = 0.5;
		break;
	case PLAYER_ACTION_STANDING:
		flActionRatio = 0.75;
		break;
	case PLAYER_ACTION_SPRINTING:
		if(pWpn->GetWeaponType()==WEAPON_TYPE_MACHINEGUN)
		{
			flActionRatio = 1.25;
		}
		else
		{
			flActionRatio = 0.8;
		}
		break;
	case PLAYER_ACTION_CLIMBING:
	case PLAYER_ACTION_FALLING_HURT:
	case PLAYER_ACTION_JUMPING:
	case PLAYER_ACTION_FALLING:
	case PLAYER_ACTION_WALKING:
	default:
		flActionRatio = 1.0;
	}

	return flActionRatio * flSkillRatio;
}

float CSDKPlayer::CalcMaxSpeed()
{
	float fBaseSpeed = mp_basespeed.GetFloat();

	// see if our weapon demands a new speed
	CWeaponSDKBase *pWpn = dynamic_cast<CWeaponSDKBase*>(GetActiveWeapon());
	if (pWpn) {
		float fWeaponSpeed = pWpn->CalcMaxSpeed();
		if (fWeaponSpeed >= 0)
			return fWeaponSpeed;
	}

	// pull the skill
	CSkillClass *pSkill = GetSkillClass();

	// did we get it?
	if(pSkill)
		fBaseSpeed *= pSkill->GetSpeedRatio();

	if(CanSprint() && IsSprinting())
		fBaseSpeed += SPRINT_BONUS;
	
	return fBaseSpeed;
}

/**
* Calculates the player's current view
*
* @param Vector &eyeOrigin Place where the eye is
* @param QAngle &eyeAngles Where the eye is looking
* @param float &fov The fov o' the eye
* @return void
**/
void CSDKPlayer::CalcPlayerView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	if ( !IsAlive() )
	{
#ifdef CLIENT_DLL
		Vector origin = EyePosition();			

		IRagdoll *pRagdoll = GetRepresentativeRagdoll();

		if ( pRagdoll )
		{
			origin = pRagdoll->GetRagdollOrigin();
			origin.z += VEC_DEAD_VIEWHEIGHT.z; // look over ragdoll, not through
		}

#endif
		BaseClass::CalcPlayerView( eyeOrigin, eyeAngles, fov );

#ifdef CLIENT_DLL
		eyeOrigin = origin;
		
		Vector vForward; 
		AngleVectors( eyeAngles, &vForward );

		VectorNormalize( vForward );
		VectorMA( origin, -CHASE_CAM_DISTANCE, vForward, eyeOrigin );

		Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
		Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );

		trace_t trace; // clip against world
		C_BaseEntity::EnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
		UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );
		C_BaseEntity::EnableAbsRecomputations( true );

		if (trace.fraction < 1.0)
		{
			eyeOrigin = trace.endpos;
		}
#endif
	}
	else {
		QAngle vecAngle(0, 0, 0);

		// call the base class
		BaseClass::CalcPlayerView(eyeOrigin, eyeAngles, fov);

		// figure out the sniper drift and add it on
		CalcSniperDrift(vecAngle);
		VectorAdd(eyeAngles, vecAngle, eyeAngles);
	}
}

/**
* Applies sniper drift to the player
*
* @return void
**/
void CSDKPlayer::CalcSniperDrift(QAngle &vecAngle)
{
	CBaseSniperRifle *pWpn;
	float fScale;
	const float SCALE_STEP = 1.0;

	// see if we have a sniper rifle
	pWpn = dynamic_cast<CBaseSniperRifle *>(GetActiveWeapon());

	// must be using a sniper rifle and be zoomed in
	if(pWpn && !pWpn->IsScopeHidden())
	{
		QAngle vecDrift;
		vecDrift.z = 0;

		// scale the "spiral-y thing" based on player action.
		PLAYER_ACTION state = GetPlayerAction();

		// multiply the scale times 4 to increase the drift size, and then by the sniper drift ratio
		// for this skill class, weapon, and state.
		fScale = 4.0 * GetSkillClass()->GetSniperDriftRatio(pWpn, state);

		if (m_fLastScale == 0)
			m_fLastScale = fScale;

		// if this is a change in scale, blend if necessary
		if (fScale != m_fLastScale) {
			float fStepSize = SCALE_STEP * gpGlobals->frametime;
			if (abs(m_fLastScale - fScale) > fStepSize)
			{
				if (fScale > m_fLastScale)
					fScale = m_fLastScale + fStepSize;
				else
					fScale = m_fLastScale - fStepSize;
			}

			m_fLastScale = fScale;
		}

		// make a spiral-y thing, see: http://mathworld.wolfram.com/LissajousCurve.html
		vecDrift.x = fScale * sin(gpGlobals->curtime + M_PI);
		vecDrift.y = fScale * cos(0.5 * gpGlobals->curtime);

		// increment by our angle
		vecAngle += vecDrift;
	}
}

/**
* Finds the sniper drift
*
* @return QAngle
**/
QAngle CSDKPlayer::GetSniperDrift(void)
{
	QAngle vecAngle(0, 0, 0);

	// figure it out
	CalcSniperDrift(vecAngle);
	return vecAngle;
}

// for debugging
int CSDKPlayer::GetAmmoCount( int iAmmoIndex ) const
{
	Assert(0);

	return BaseClass::GetAmmoCount(iAmmoIndex);
}

bool CSDKPlayer::Jump(void)
{
	if ( GetStamina() - REQ_STAMINA_JUMP < 0)
	{
		#ifdef CLIENT_DLL
			HudStamina *pStamina = GET_HUDELEMENT(HudStamina);
			if(pStamina)
				pStamina->SetOutOfStamina(true);
		#endif

		return false;
	}

	ReduceStamina(REQ_STAMINA_JUMP);

	return true;
}

float CSDKPlayer::GetStamina()
{
	return m_flStamina;
}

void CSDKPlayer::CalculateStamina()
{
	PLAYER_ACTION state = GetPlayerAction();
	switch(state)
	{
	case PLAYER_ACTION_SPRINTING:
		ReduceStamina(60.0 * gpGlobals->frametime);
		break;
	case PLAYER_ACTION_STANDING:
	case PLAYER_ACTION_DUCKING:
		// you will never be sprinting in these cases so its OK
		AddStamina(12.0 * gpGlobals->frametime);
		break;
	case PLAYER_ACTION_FALLING:
	case PLAYER_ACTION_WALKING:
		// you must not be pressing the stamina button
		if (!(m_nButtons & IN_SPEED))
			AddStamina(12.0 * gpGlobals->frametime);
		break;
	default:
		break;
	}
}

void CSDKPlayer::ReduceStamina(float flAmount)
{
	AddStamina(-flAmount);
}

void CSDKPlayer::AddStamina(float flAmount)
{
	m_flStamina += flAmount;
	m_flStamina = clamp(m_flStamina, 0.0, (float)m_pSkill->GetMaxStamina());
}

CBaseCombatWeapon* CSDKPlayer::GetWeaponBySlot(int iSlot, int *iStartNum)
{
	int *i;

	if (!iStartNum) {
		i = new int;
		*i = 0;
	}
	else
		i = iStartNum;

	for(; *i < WeaponCount(); ++*i) {
		CBaseCombatWeapon *pPlayerItem = GetWeapon( *i );
		if (pPlayerItem) {
			if (pPlayerItem->GetSlot() == iSlot)
				return pPlayerItem;
		}
	}

	if (!iStartNum)
		delete i;

	return NULL;
}

/**
 * Checks to see if we have a primary weapon.
 * @return bool
 */
CWeaponSDKBase* CSDKPlayer::GetPrimaryWeapon()
{
	for ( int i = 0 ; i < WeaponCount() ; i++ )
	{
		CBaseCombatWeapon *pPlayerItem = GetWeapon( i );

		if (pPlayerItem && pPlayerItem->HasWeaponExclusion()) {
			// see if this one of our weapons, or if its a grenade or something.
			CWeaponSDKBase *pWpn = dynamic_cast<CWeaponSDKBase*>(pPlayerItem);

			if (pWpn && pWpn->IsPrimaryWeapon())
				return pWpn;
		}
	}
	return NULL;
}

/**
 * Checks to see if we have a seconary weapon.
 * @return bool
 */
CWeaponSDKBase* CSDKPlayer::GetSecondaryWeapon()
{
	for ( int i = 0 ; i < WeaponCount() ; i++ )
	{
		CBaseCombatWeapon *pPlayerItem = GetWeapon( i );

		if (pPlayerItem && pPlayerItem->HasWeaponExclusion()) {
			// see if this one of our weapons, or if its a grenade or something.
			CWeaponSDKBase *pWpn = dynamic_cast<CWeaponSDKBase*>(pPlayerItem);

			if (pWpn && pWpn->IsSecondaryWeapon())
				return pWpn;
		}
	}
	return NULL;
}

/**
 * Checks to see if we have a melee weapon.
 * @return bool
 */
CWeaponSDKBase* CSDKPlayer::GetMeleeWeapon()
{
	for ( int i = 0 ; i < WeaponCount() ; i++ )
	{
		CBaseCombatWeapon *pPlayerItem = GetWeapon( i );

		if (pPlayerItem && pPlayerItem->HasWeaponExclusion()) {
			// see if this one of our weapons, or if its a grenade or something.
			CWeaponSDKBase *pWpn = dynamic_cast<CWeaponSDKBase*>(pPlayerItem);

			if (pWpn && pWpn->IsMeleeWeapon())
				return pWpn;
		}
	}

	return NULL;
}

/**
* Determines if the player can buy the item
* Note that this is different than whether or not they can have the item
* since a player cannot buy an item that they already own
* 
* @return bool
**/
bool CSDKPlayer::CanBuyItem(IBuyableItem *pItem)
{
	CWeaponSDKBase *pWeapon = dynamic_cast<CWeaponSDKBase*>(pItem);
	CWeaponSDKBase *pMyWeapon;
	char szStr[256];

	// check the cash
	if(GetCash() < pItem->GetPrice())
		return false;

	// check that our skill class will allow us to have the weapon
	if(m_pSkill && pItem)
	{
		// can we have it?
		if(!m_pSkill->CanHaveItem(pItem))
			return false;
	}

	// if i already own this weapon and i'm not full on ammo i can buy a new one
	Q_strncpy(szStr, pWeapon->GetClassname(), 256);
	pMyWeapon = (CWeaponSDKBase*)Weapon_OwnsThisType(szStr, pWeapon->GetSubType());
	if(pWeapon && pMyWeapon != NULL)
	{
		// if it's a primary, secondary or melee weapon we can't buy it
		if(pMyWeapon == GetPrimaryWeapon() || pMyWeapon == GetSecondaryWeapon() || pMyWeapon == GetMeleeWeapon())
			return false;

		// am I already filled up on primary and secondary ammo?
		return ((pMyWeapon->UsesPrimaryAmmo() && pMyWeapon->GetAmmo1() < pMyWeapon->GetMaxAmmo1())
			|| (pMyWeapon->UsesSecondaryAmmo() && pMyWeapon->GetAmmo2() < pMyWeapon->GetAmmo2()));
	}

	return true;
}

/**
 * Asks to see if the player can have a particular item.
 * @param CBaseCombatWeapon* pItem The item we're trying to accept
 * @return bool
 */
bool CSDKPlayer::CanHavePlayerItem( CBaseEntity *pItem, bool bRespectExclusion /* = true */)
{
	// check that our skill class will allow us to have the weapon
	IBuyableItem *pBItem = dynamic_cast<IBuyableItem *>(pItem);
	if(m_pSkill && pBItem)
	{
		// can we have it?
		if(!m_pSkill->CanHaveItem(pBItem))
			return false;
	}

	// if we have a combat weapon, see if we are respecting weapon exclusion
	if(bRespectExclusion)
	{
		CWeaponSDKBase *pWpn = dynamic_cast<CWeaponSDKBase *>(pItem);
		if (pWpn && pWpn->HasWeaponExclusion())
		{
			if (pWpn->IsPrimaryWeapon())
				return !GetPrimaryWeapon();
			if (pWpn->IsSecondaryWeapon())
				return !GetSecondaryWeapon();
			if (pWpn->IsMeleeWeapon())
				return !GetMeleeWeapon();
		}
	}

	char szStr[256];
	CWeaponSDKBase *pWeapon = dynamic_cast<CWeaponSDKBase*>(pItem);
	CWeaponSDKBase *pMyWeapon;

	if(pWeapon)
		Q_strncpy(szStr, pWeapon->GetClassname(), 256);

	// ok, so we don't care about exclusion, but am I a weapon?
	// does this weapon not use weapon excusion, and do I already have this weapon?
	if (pWeapon && !pWeapon->HasWeaponExclusion()
		&& (pMyWeapon = (CWeaponSDKBase*)Weapon_OwnsThisType(szStr, pWeapon->GetSubType())) != NULL)
	{
		// am I already filled up on primary and secondary ammo?
		return ((pMyWeapon->UsesPrimaryAmmo() && pMyWeapon->GetAmmo1() < pMyWeapon->GetMaxAmmo1())
			|| (pMyWeapon->UsesSecondaryAmmo() && pMyWeapon->GetAmmo2() < pMyWeapon->GetAmmo2()));
	}
	
	return true;
}
