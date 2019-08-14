//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "obstacle_pushaway.h"
#include "props_shared.h"

//-----------------------------------------------------------------------------------------------------
ConVar sv_pushaway_force( "sv_pushaway_force", "30000", FCVAR_REPLICATED, "How hard physics objects are pushed away from the players on the server." );
ConVar sv_pushaway_min_player_speed( "sv_pushaway_min_player_speed", "75", FCVAR_REPLICATED, "If a player is moving slower than this, don't push away physics objects (enables ducking behind things)." );
ConVar sv_pushaway_max_force( "sv_pushaway_max_force", "1000", FCVAR_REPLICATED, "Maximum amount of force applied to physics objects by players." );
ConVar sv_pushaway_clientside( "sv_pushaway_clientside", "0", FCVAR_REPLICATED, "Clientside physics push away (0=off, 1=only localplayer, 1=all players)" );
ConVar sv_turbophysics( "sv_turbophysics", "0", FCVAR_REPLICATED, "Turns on turbo physics" );

ConVar sv_pushaway_player_force( "sv_pushaway_player_force", "200000", FCVAR_REPLICATED | FCVAR_CHEAT, "How hard the player is pushed away from physics objects (falls off with inverse square of distance)." );
ConVar sv_pushaway_max_player_force( "sv_pushaway_max_player_force", "10000", FCVAR_REPLICATED | FCVAR_CHEAT, "Maximum of how hard the player is pushed away from physics objects." );
ConVar sv_pushaway_expansion("sv_pushaway_expansion", "-5", FCVAR_REPLICATED | FCVAR_CHEAT, "Distance to search for entities to push away from.", true, -13, true, 13);
ConVar sv_pushaway_players("sv_pushaway_players", "0", FCVAR_REPLICATED, "If true bouncy player physics is used");

//-----------------------------------------------------------------------------------------------------
int GetPushawayEnts( CBaseCombatCharacter *pPushingEntity, CBaseEntity **ents, int nMaxEnts, float flPlayerExpand, int PartitionMask, CPushAwayEnumerator *enumerator )
{
	
	Vector vExpand( flPlayerExpand, flPlayerExpand, flPlayerExpand ), vForward;

	// grab the forward angles.  we want to move the origin slightly forward
	AngleVectors(pPushingEntity->EyeAngles(), &vForward);
	vForward.z = 0;
	VectorNormalize(vForward);

	Ray_t ray;
	ray.Init( pPushingEntity->GetAbsOrigin() + (vForward * 16.0f), pPushingEntity->GetAbsOrigin() + (vForward * 16.0f), pPushingEntity->GetCollideable()->OBBMins() - vExpand, pPushingEntity->GetCollideable()->OBBMaxs() + vExpand);

	CPushAwayEnumerator *physPropEnum = NULL;
	if  ( !enumerator )
	{
		physPropEnum = new CPushAwayEnumerator( ents, nMaxEnts );
		enumerator = physPropEnum;
	}

	partition->EnumerateElementsAlongRay( PartitionMask, ray, false, enumerator );

	int numHit = enumerator->m_nAlreadyHit;

	if ( physPropEnum )
		delete physPropEnum;

	return numHit;
}

//-----------------------------------------------------------------------------------------------------
void PerformObstaclePushaway( CBaseCombatCharacter *pPushingEntity )
{
	IPhysicsObject *pObj;
	Vector vPushAway, vPusherDirection;
	float flDist/*, fApproachAngle*/;

	if (  pPushingEntity->m_lifeState != LIFE_ALIVE )
		return;

	// Give a push to any barrels that we're touching.
	// The client handles adjusting our usercmd to push us away.
	CBaseEntity *props[256];

#ifdef CLIENT_DLL
	// if sv_pushaway_clientside is disabled, clientside phys objects don't bounce away
	if ( sv_pushaway_clientside.GetInt() == 0 )
		return;

	// if sv_pushaway_clientside is 1, only local player can push them
	CBasePlayer *pPlayer = pPushingEntity->IsPlayer() ? (dynamic_cast< CBasePlayer * >(pPushingEntity)) : NULL;
	if ( (sv_pushaway_clientside.GetInt() == 1) && (!pPlayer || !pPlayer->IsLocalPlayer()) )
		return;

	int nEnts = GetPushawayEnts( pPushingEntity, props, ARRAYSIZE( props ), sv_pushaway_expansion.GetFloat(), PARTITION_CLIENT_RESPONSIVE_EDICTS );
#else
	int nEnts = GetPushawayEnts( pPushingEntity, props, ARRAYSIZE( props ), sv_pushaway_expansion.GetFloat(), PARTITION_ENGINE_SOLID_EDICTS );
#endif
	
	for ( int i=0; i < nEnts; i++ )
	{
		// don't collide with myself
		if(pPushingEntity == props[i])
			continue;

		// If this entity uas PHYSICS_MULTIPLAYER_FULL set (ie: it's not just debris), and we're moving too slow, don't push it away.
		// Instead, let the client bounce off it. This allows players to get close to and duck behind things without knocking them over.
		IMultiplayerPhysics *pInterface = dynamic_cast<IMultiplayerPhysics*>( props[i] );

		/*
#ifdef GAME_DLL
		if ( pInterface->IsAsleep() && sv_turbophysics.GetBool() )
			continue;
#endif
		*/

		if ( pInterface && pInterface->GetMultiplayerPhysicsMode() == PHYSICS_MULTIPLAYER_SOLID )
		{
			if ( pPushingEntity->GetAbsVelocity().Length2D() < sv_pushaway_min_player_speed.GetFloat() )
				continue;
		}

		// get the vector between the two
		vPushAway = props[i]->WorldSpaceCenter() - pPushingEntity->WorldSpaceCenter();
		vPushAway.z = 0;
		flDist = VectorNormalize(vPushAway);

		// if the pushing entity is a player and we're pushing against 
		// another player, we need to push the pushing entity back
		if(pPushingEntity->IsPlayer() && props[i]->IsPlayer())
		{
			// only push the pusher if the pusher is actually pushing...
			// in other words, one player is running into the other.  if i'm not the one
			// doing the running then don't punish me

			// if i'm barely moving, don't push me
			if(pPushingEntity->GetAbsVelocity().Length2DSqr() < 0.1f)
				continue;

			// if i'm not moving generally in the direction of the other guy
			// then don't push me back
			/*vPusherDirection = pPushingEntity->GetAbsVelocity();
			vPusherDirection.z = 0;
			VectorNormalize(vPusherDirection);
			fApproachAngle = abs(acos(vPusherDirection.Dot(vPushAway)));
			if(fApproachAngle < (5.0f * M_PI / 8.0f) && fApproachAngle > (3.0 * M_PI / 8.0f))
				continue;*/

			// we're actually the jerk doing the pushing.  punish me
			pObj = pPushingEntity->VPhysicsGetObject();
		}
		else
			// grab the pushed object's physics object
			pObj = props[i]->VPhysicsGetObject();

		// if we have something to push, go ahead and do it
		if ( pObj )
		{		
			flDist = max( flDist, 1 );
			
			float flForce = sv_pushaway_force.GetFloat() / flDist;
			flForce = min( flForce, sv_pushaway_max_force.GetFloat() );

			pObj->ApplyForceOffset( vPushAway * flForce, pPushingEntity->WorldSpaceCenter() );
		}
	}
}

void AvoidPushawayProps( CBaseCombatCharacter *pPlayer, CUserCmd *pCmd )
{
	Vector vPusherDirection;
	//float fApproachAngle;

	// Figure out what direction we're moving and the extents of the box we're going to sweep 
	// against physics objects.
	Vector currentdir;
	Vector rightdir;
	AngleVectors( pCmd->viewangles, &currentdir, &rightdir, NULL );

	CBaseEntity *props[512];
#ifdef CLIENT_DLL
	int nEnts = GetPushawayEnts( pPlayer, props, ARRAYSIZE( props ), /*0.0f*/sv_pushaway_expansion.GetFloat(), PARTITION_CLIENT_SOLID_EDICTS, NULL );
#else
	int nEnts = GetPushawayEnts( pPlayer, props, ARRAYSIZE( props ), /*0.0f*/sv_pushaway_expansion.GetFloat(), PARTITION_ENGINE_SOLID_EDICTS, NULL );
#endif

	for ( int i=0; i < nEnts; i++ )
	{
		// don't push myself away
		if(props[i] == pPlayer)
			continue;

		// Don't respond to this entity on the client unless it has PHYSICS_MULTIPLAYER_FULL set.
		IMultiplayerPhysics *pInterface = dynamic_cast<IMultiplayerPhysics*>( props[i] );
		if ( pInterface && pInterface->GetMultiplayerPhysicsMode() != PHYSICS_MULTIPLAYER_SOLID )
			continue;

		const float minMass = 10.0f; // minimum mass that can push a player back
		const float maxMass = 30.0f; // cap at a decently large value
		float mass = maxMass;
		if ( pInterface )
		{
			mass = pInterface->GetMass();
		}
		mass = clamp( mass, minMass, maxMass );
		
		mass = max( mass, 0 );
		mass /= maxMass; // bring into a 0..1 range

		// Push away from the collision point. The closer our center is to the collision point,
		// the harder we push away.
		Vector vPushAway = (pPlayer->WorldSpaceCenter() - props[i]->WorldSpaceCenter());
		vPushAway.z = 0;
		float flDist = VectorNormalize( vPushAway );
		flDist = max( flDist, 1 );

		// if the pushing entity is a player and we're pushing against 
		// another player, we need to push the pushing entity back
		if(props[i]->IsPlayer())
		{
			// only push the pusher if the pusher is actually pushing...
			// in other words, one player is running into the other.  if i'm not the one
			// doing the running then don't punish me

			// if i'm barely moving, don't push me
			if(pPlayer->GetAbsVelocity().Length2DSqr() < 0.1f)
				continue;

			// if i'm not moving generally in the direction of the other guy
			// then don't push me back
			/*vPusherDirection = pPlayer->GetAbsVelocity();
			vPusherDirection.z = 0;
			VectorNormalize(vPusherDirection);
			fApproachAngle = abs(acos(vPusherDirection.Dot(vPushAway)));
			if(fApproachAngle < (5.0f * M_PI / 8.0f) && fApproachAngle > (3.0 * M_PI / 8.0f))
				continue;*/
		}

		float flForce = sv_pushaway_player_force.GetFloat() / flDist * mass;
		flForce = min( flForce, sv_pushaway_max_player_force.GetFloat() );
		vPushAway *= flForce;

		pCmd->forwardmove += vPushAway.Dot( currentdir );
		pCmd->sidemove    += vPushAway.Dot( rightdir );
	}
}