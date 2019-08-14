//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "basecombatweapon_shared.h"
#include "baseviewmodel_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	TRACER_SPEED			5000 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector GetTracerOrigin( const CEffectData &data )
{
	Vector vecStart = data.m_vStart;
	QAngle vecAngles;

	int iEntIndex = data.m_nEntIndex;
	int iAttachment = data.m_nAttachmentIndex;

	// Attachment?
	if ( data.m_fFlags & TRACER_FLAG_USEATTACHMENT )
	{
		C_BaseViewModel *pViewModel = NULL;

		// If the entity specified is a weapon being carried by this player, use the viewmodel instead
		C_BaseEntity *pEnt = ClientEntityList().GetEnt( iEntIndex );
		if ( !pEnt )
			return vecStart;

#ifdef HL2MP
		if ( pEnt->IsDormant() )
			return vecStart;
#endif

		C_BaseCombatWeapon *pWpn = dynamic_cast<C_BaseCombatWeapon *>(pEnt);
		if ( pWpn && pWpn->IsCarriedByLocalPlayer() )
		{
			C_BasePlayer *player = ToBasePlayer( pWpn->GetOwner() );

			pViewModel = player ? player->GetViewModel( 0 ) : NULL;
			if ( pViewModel )
			{
				// Get the viewmodel and use it instead
				pEnt = pViewModel;
			}
		}

		// Get the attachment origin
		if ( !pEnt->GetAttachment( iAttachment, vecStart, vecAngles ) )
		{
			DevMsg( "GetTracerOrigin: Couldn't find attachment %d on model %s\n", iAttachment, pEnt->GetModelName() );
		}
	}

	return vecStart;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TracerCallback( const CEffectData &data )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	// Grab the data
	//Vector vecStart = GetTracerOrigin( data );
	//float flVelocity = data.m_flScale;
	bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	//int iEntIndex = data.m_nEntIndex;

	QAngle	vangles;
	Vector	vforward, vright, vup;
	Vector foo;
	engine->GetViewAngles( vangles );
	AngleVectors(vangles, &vforward, &vright, &vup);

	// @TRJ - a little bit of fudging to get it in the right place
	// @JDTRACER
	VectorMA(data.m_vStart, -10, vright, foo);
	VectorMA(foo, 26, vup, foo);
	//VectorMA(foo, 7, vforward, foo);
	
	// this version produces a smaller tracer effect than the one below
	// it looks better, i think
	//FX_PlayerTracer(foo, (Vector&)data.m_vOrigin);
	FX_Tracer( (Vector&)foo, (Vector&)data.m_vOrigin, 8000.0 /* flVelocity */, bWhiz );
	return;

	// @PERMANENT // @TEMP - TRJ
	/*if ( iEntIndex && iEntIndex == player->index )
	{
		Vector	foo = data.m_vStart;
		QAngle	vangles;
		Vector	vforward, vright, vup;

		engine->GetViewAngles( vangles );
		AngleVectors( vangles, &vforward, &vright, &vup );

		//VectorMA( data.m_vStart, 4, vright, foo );
		//foo[2] -= 0.5f;

		FX_PlayerTracer( foo, (Vector&)data.m_vOrigin );
		return;
	}*/
	
	// Use default velocity if none specified
	/*
	if ( !flVelocity )
	{
		flVelocity = TRACER_SPEED;
	}

	// Do tracer effect
	FX_Tracer( (Vector&)foo, (Vector&)data.m_vOrigin, flVelocity, bWhiz );
	*/
}

DECLARE_CLIENT_EFFECT( "Tracer", TracerCallback );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TracerSoundCallback( const CEffectData &data )
{
	// Grab the data
	Vector vecStart = GetTracerOrigin( data );
	
	// Do tracer effect
	FX_TracerSound( vecStart, (Vector&)data.m_vOrigin, data.m_fFlags );
}

DECLARE_CLIENT_EFFECT( "TracerSound", TracerSoundCallback );

