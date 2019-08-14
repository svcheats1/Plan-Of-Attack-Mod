#include "cbase.h"
#include "weapon_sdkbase.h"
#include "ai_activity.h"
#include "takedamageinfo.h"
#include "in_buttons.h"
#ifndef CLIENT_DLL
#endif

#if defined( CLIENT_DLL )
	#define CWeaponKnife C_WeaponKnife
	#include "c_sdk_player.h"
	#include "c_te_effect_dispatch.h"
#else
	#include "baseentity.h"
	#include "sdk_player.h"
	#include "te_effect_dispatch.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BLUDGEON_HULL_DIM		24

static const Vector g_bludgeonMins(-BLUDGEON_HULL_DIM,-BLUDGEON_HULL_DIM,-BLUDGEON_HULL_DIM);
static const Vector g_bludgeonMaxs(BLUDGEON_HULL_DIM,BLUDGEON_HULL_DIM,BLUDGEON_HULL_DIM);

class CWeaponKnife : public CWeaponSDKBase
{
	DECLARE_CLASS( CWeaponKnife, CWeaponSDKBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponKnife();
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual void ItemPostFrame();
	virtual bool IsDroppable() const { return false; }

	virtual SDKWeaponID GetWeaponID( void ) const { return WEAPON_KNIFE; }
	virtual SDKWeaponType GetWeaponType(void) const { return WEAPON_TYPE_MELEE; }

protected:
	virtual float GetRange( void ) { return 48.0f; }
	virtual void ImpactEffect( trace_t &traceHit );
	virtual void Hit( trace_t &traceHit, Activity nHitActivity, float flDamage );
	virtual void Swing(bool bIsSecondaryAttack);
	virtual bool ImpactWater( const Vector &start, const Vector &end );
	virtual Activity ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBasePlayer *pOwner );

private:

	CWeaponKnife( const CWeaponKnife & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponKnife, DT_WeaponKnife )

BEGIN_NETWORK_TABLE( CWeaponKnife, DT_WeaponKnife )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponKnife )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_knife, CWeaponKnife );
PRECACHE_WEAPON_REGISTER( weapon_knife );

CWeaponKnife::CWeaponKnife() { }

void CWeaponKnife::PrimaryAttack()
{
	Swing( false );
}

void CWeaponKnife::SecondaryAttack()
{
	Swing( true );
}

void CWeaponKnife::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		PrimaryAttack();
	} 
	else if ( (pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime) )
	{
		SecondaryAttack();
	}
	else 
	{
		WeaponIdle();
		return;
	}

	BaseClass::ItemPostFrame();
}

void CWeaponKnife::Hit( trace_t &traceHit, Activity nHitActivity, float flDamage )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	//Do view kick
	AddViewKick();

	//Make sound for the AI
	//CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, traceHit.endpos, 400, 0.2f, pPlayer );

	CBaseEntity	*pHitEntity = traceHit.m_pEnt;

	//Apply damage to a hit target
	if ( pHitEntity != NULL )
	{
		Vector hitDirection;
		pPlayer->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );

		CTakeDamageInfo info( GetOwner(), GetOwner(), flDamage, DMG_CLUB | DMG_NEVERGIB );

		CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos, 0.01 );

		pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit ); 
		ApplyMultiDamage();

		// Now hit all triggers along the ray that... 
#ifndef CLIENT_DLL
		TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );
#endif
	}

	// Apply an impact effect
	ImpactEffect( traceHit );
}

void CWeaponKnife::ImpactEffect( trace_t &traceHit )
{
	// See if we hit water (we don't do the other impact effects in this case)
	if ( ImpactWater( traceHit.startpos, traceHit.endpos ) )
		return;

	//FIXME: need new decals
	UTIL_ImpactTrace( &traceHit, DMG_SLASH );
}


//------------------------------------------------------------------------------
// Purpose : Starts the swing of the weapon and determines the animation
// Input   : bIsSecondary - is this a secondary attack?
//------------------------------------------------------------------------------
void CWeaponKnife::Swing( bool bIsSecondary )
{
	trace_t traceHit;

	// Try a ray
	CSDKPlayer *pOwner = ToSDKPlayer( GetOwner() );
	if ( !pOwner )
		return;

	Vector swingStart = pOwner->Weapon_ShootPosition( );
	Vector forward;

	pOwner->EyeVectors( &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange();
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );

	Activity nHitActivity = ACT_VM_HITCENTER;
	//Activity nHitActivity = (bIsSecondary ? ACT_VM_SECONDARYATTACK : ACT_VM_HITCENTER);

	// Like bullets, bludgeon traces have to trace against triggers.
	const CSDKWeaponInfo &pWeaponInfo = GetSDKWpnData();
	float flDamage = (float)pWeaponInfo.m_iDamage * (bIsSecondary ? 2.5 : 1.0);
	CTakeDamageInfo triggerInfo( GetOwner(), GetOwner(), flDamage, DMG_CLUB );
#ifndef CLIENT_DLL
	TraceAttackToTriggers( triggerInfo, traceHit.startpos, traceHit.endpos, vec3_origin );
#endif
	if ( traceHit.fraction == 1.0 )
	{
		float bludgeonHullRadius = 1.732f * BLUDGEON_HULL_DIM;  // hull is +/- 16, so use cuberoot of 2 to determine how big the hull is from center to the corner point

		// Back off by hull "radius"
		swingEnd -= forward * bludgeonHullRadius;

		UTIL_TraceHull( swingStart, swingEnd, g_bludgeonMins, g_bludgeonMaxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
		if ( traceHit.fraction < 1.0 && traceHit.m_pEnt )
		{
			Vector vecToTarget = traceHit.m_pEnt->GetAbsOrigin() - swingStart;
			VectorNormalize( vecToTarget );

			float dot = vecToTarget.Dot( forward );

			// YWB:  Make sure they are sort of facing the guy at least...
			if ( dot < 0.70721f )
			{
				// Force amiss
				traceHit.fraction = 1.0f;
			}
			else
			{
				ChooseIntersectionPointAndActivity( traceHit, g_bludgeonMins, g_bludgeonMaxs, pOwner );
			}
		}
	}

	// -------------------------
	//	Miss
	// -------------------------
	if ( traceHit.fraction == 1.0f )
	{
		//nHitActivity = bIsSecondary ? ACT_VM_MISSCENTER2 : ACT_VM_MISSCENTER;
		nHitActivity = ACT_VM_MISSCENTER; // seems we don't have a "miss center 2"

		// We want to test the first swing again
		Vector testEnd = swingStart + forward * GetRange();
		
		// See if we happened to hit water
		ImpactWater( swingStart, testEnd );
	}
	else
	{
		// this is a pseudo-hack for sound.  We did hit something, but not a player, so its a miss
		if ( !traceHit.m_pEnt || !traceHit.m_pEnt->IsPlayer() )
			nHitActivity = ACT_VM_MISSCENTER;
		
		Hit( traceHit, nHitActivity, flDamage );
	}

	// Send the anims
	SendWeaponAnim( nHitActivity );
#ifdef GAME_DLL
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_FIRE_GUN_PRIMARY);
	//pOwner->DoAnimationEvent(bIsSecondary ? PLAYERANIMEVENT_FIRE_GUN_SECONDARY : PLAYERANIMEVENT_FIRE_GUN_PRIMARY);
#endif
	pOwner->SetAnimation(PLAYER_ATTACK1);

	//DevMsg(1, "%f - Attack!\n", gpGlobals->curtime);

	//Setup our next attack times
	m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + (SequenceDuration() * (bIsSecondary ? 2.5 : 1.0));

	//Play swing sound
	WeaponSound( SINGLE );

	#ifndef CLIENT_DLL
	// @TRJ - send out the weapon_fire event
	IGameEvent *pEvent = gameeventmanager->CreateEvent("weapon_fire");
	if(pEvent)
	{
		// pull the skill
		CSkillClass *pSkill = pOwner->GetSkillClass();

		// set all the data
		pEvent->SetString("weapon", "weapon_knife");
		pEvent->SetString("skill", pSkill ? pSkill->GetInternalClassName() : "none");
		pEvent->SetInt("userid", pOwner->GetUserID());

		// send it out
		gameeventmanager->FireEvent(pEvent, true);
	}
	#endif	
}

bool CWeaponKnife::ImpactWater( const Vector &start, const Vector &end )
{
	//FIXME: This doesn't handle the case of trying to splash while being underwater, but that's not going to look good
	//		 right now anyway...
	
	// We must start outside the water
	if ( UTIL_PointContents( start ) & (CONTENTS_WATER|CONTENTS_SLIME))
		return false;

	// We must end inside of water
	if ( !(UTIL_PointContents( end ) & (CONTENTS_WATER|CONTENTS_SLIME)))
		return false;

	trace_t	waterTrace;

	UTIL_TraceLine( start, end, (CONTENTS_WATER|CONTENTS_SLIME), GetOwner(), COLLISION_GROUP_NONE, &waterTrace );

	if ( waterTrace.fraction < 1.0f )
	{
		CEffectData	data;

		data.m_fFlags  = 0;
		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = 8.0f;

		// See if we hit slime
		if ( waterTrace.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		DispatchEffect( "watersplash", data );			
	}

	return true;
}

Activity CWeaponKnife::ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBasePlayer *pOwner )
{
	int			i, j, k;
	float		distance;
	const float	*minmaxs[2] = {mins.Base(), maxs.Base()};
	trace_t		tmpTrace;
	Vector		vecHullEnd = hitTrace.endpos;
	Vector		vecEnd;

	distance = 1e6f;
	Vector vecSrc = hitTrace.startpos;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace );
	if ( tmpTrace.fraction == 1.0 )
	{
		for ( i = 0; i < 2; i++ )
		{
			for ( j = 0; j < 2; j++ )
			{
				for ( k = 0; k < 2; k++ )
				{
					vecEnd.x = vecHullEnd.x + minmaxs[i][0];
					vecEnd.y = vecHullEnd.y + minmaxs[j][1];
					vecEnd.z = vecHullEnd.z + minmaxs[k][2];

					UTIL_TraceLine( vecSrc, vecEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace );
					if ( tmpTrace.fraction < 1.0 )
					{
						float thisDistance = (tmpTrace.endpos - vecSrc).Length();
						if ( thisDistance < distance )
						{
							hitTrace = tmpTrace;
							distance = thisDistance;
						}
					}
				}
			}
		}
	}
	else
	{
		hitTrace = tmpTrace;
	}


	return ACT_VM_HITCENTER;
}
