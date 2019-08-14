//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "fx.h"
#include "c_gib.h"
#include "c_te_effect_dispatch.h"
#include "iefx.h"
#include "decals.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	NUM_ANTLION_GIBS_UNIQUE	3
const char *pszAntlionGibs_Unique[NUM_ANTLION_GIBS_UNIQUE] = {
"models/gibs/antlion_gib_large_1.mdl",
"models/gibs/antlion_gib_large_2.mdl",
"models/gibs/antlion_gib_large_3.mdl"
};

#define	NUM_ANTLION_GIBS_MEDIUM	3
const char *pszAntlionGibs_Medium[NUM_ANTLION_GIBS_MEDIUM] = {
"models/gibs/antlion_gib_medium_1.mdl",
"models/gibs/antlion_gib_medium_2.mdl",
"models/gibs/antlion_gib_medium_3.mdl"
};

#define	NUM_ANTLION_GIBS_SMALL	3
const char *pszAntlionGibs_Small[NUM_ANTLION_GIBS_SMALL] = {
"models/gibs/antlion_gib_small_1.mdl",
"models/gibs/antlion_gib_small_2.mdl",
"models/gibs/antlion_gib_small_3.mdl"
};

ConVar g_antlion_maxgibs( "g_antlion_maxgibs", "16", FCVAR_ARCHIVE );

void CAntlionGibManager::LevelInitPreEntity( void )
{
	m_LRU.Purge();
}

CAntlionGibManager s_AntlionGibManager;

void CAntlionGibManager::AddGib( C_BaseEntity *pEntity )
{
	m_LRU.AddToTail( pEntity );
}

void CAntlionGibManager::RemoveGib( C_BaseEntity *pEntity )
{
	m_LRU.FindAndRemove( pEntity );
}
	

//-----------------------------------------------------------------------------
// Methods of IGameSystem
//-----------------------------------------------------------------------------
void CAntlionGibManager::Update( float frametime )
{
	if ( m_LRU.Count() < g_antlion_maxgibs.GetInt() )
		 return;
	
	int i = 0;
	i = m_LRU.Head();

	if ( m_LRU[ i ].Get() )
	{
		 m_LRU[ i ].Get()->SetNextClientThink( gpGlobals->curtime );
	}

	m_LRU.Remove(i);
}

// Antlion gib - marks surfaces when it bounces

class C_AntlionGib : public C_Gib
{
	typedef C_Gib BaseClass;
public:
	
	static C_AntlionGib *C_AntlionGib::CreateClientsideGib( const char *pszModelName, Vector vecOrigin, Vector vecForceDir, AngularImpulse vecAngularImp, float m_flLifetime = DEFAULT_GIB_LIFETIME )
	{
		C_AntlionGib *pGib = new C_AntlionGib;

		if ( pGib == NULL )
			return NULL;

		if ( pGib->InitializeGib( pszModelName, vecOrigin, vecForceDir, vecAngularImp, m_flLifetime ) == false )
			return NULL;

		s_AntlionGibManager.AddGib( pGib );

		return pGib;
	}

	// Decal the surface
	virtual	void HitSurface( C_BaseEntity *pOther )
	{
		//JDW: Removed for the time being

		/*
		int index = decalsystem->GetDecalIndexForName( "YellowBlood" );
		
		if (index >= 0 )
		{
			effects->DecalShoot( index, pOther->entindex(), pOther->GetModel(), pOther->GetAbsOrigin(), pOther->GetAbsAngles(), GetAbsOrigin(), 0, 0 );
		}
		*/
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//-----------------------------------------------------------------------------
void FX_AntlionGib( const Vector &origin, const Vector &direction, float scale )
{
	Vector	offset;

	int numGibs = random->RandomInt( 1, NUM_ANTLION_GIBS_UNIQUE );

	// Spawn all the unique gibs
	for ( int i = 0; i < numGibs; i++ )
	{
		offset = RandomVector( -16, 16 ) + origin;

		C_AntlionGib::CreateClientsideGib( pszAntlionGibs_Unique[i], offset, ( direction + RandomVector( -0.8f, 0.8f ) ) * ( 150 * scale ), RandomAngularImpulse( -32, 32 ), 2.0f);
	}

	numGibs = random->RandomInt( 1, NUM_ANTLION_GIBS_MEDIUM );

	// Spawn all the medium gibs
	for ( i = 0; i < numGibs; i++ )
	{
		offset = RandomVector( -16, 16 ) + origin;

		C_AntlionGib::CreateClientsideGib( pszAntlionGibs_Medium[i], offset, ( direction + RandomVector( -0.8f, 0.8f ) ) * ( 250 * scale ), RandomAngularImpulse( -200, 200 ), 1.0f );
	}

	numGibs = random->RandomInt( 1, NUM_ANTLION_GIBS_SMALL );

	// Spawn all the small gibs
	for ( i = 0; i < NUM_ANTLION_GIBS_SMALL; i++ )
	{
		offset = RandomVector( -16, 16 ) + origin;

		C_AntlionGib::CreateClientsideGib( pszAntlionGibs_Small[i], offset, ( direction + RandomVector( -0.8f, 0.8f ) ) * ( 400 * scale ), RandomAngularImpulse( -300, 300 ), 0.5f );
	}

	//
	// Throw some blood
	//

	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "FX_AntlionGib" );
	pSimple->SetSortOrigin( origin );

	PMaterialHandle	hMaterial = pSimple->GetPMaterial( "effects/blood" );

	Vector	vDir;

	vDir.Random( -1.0f, 1.0f );

	for ( i = 0; i < 4; i++ )
	{
		SimpleParticle *sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, origin );
			
		if ( sParticle == NULL )
			return;

		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= random->RandomFloat( 0.5f, 0.75f );
			
		float	speed = random->RandomFloat( 16.0f, 64.0f );

		sParticle->m_vecVelocity	= vDir * -speed;
		sParticle->m_vecVelocity[2] += 16.0f;

		sParticle->m_uchColor[0]	= 255;
		sParticle->m_uchColor[1]	= 200;
		sParticle->m_uchColor[2]	= 32;
		sParticle->m_uchStartAlpha	= 255;
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= random->RandomInt( 16, 32 );
		sParticle->m_uchEndSize		= sParticle->m_uchStartSize * 2;
		sParticle->m_flRoll			= random->RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= random->RandomFloat( -1.0f, 1.0f );
	}

	hMaterial = pSimple->GetPMaterial( "effects/blood2" );

	for ( i = 0; i < 4; i++ )
	{
		SimpleParticle *sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, origin );
			
		if ( sParticle == NULL )
		{
			return;
		}

		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= random->RandomFloat( 0.5f, 0.75f );
			
		float	speed = random->RandomFloat( 16.0f, 64.0f );

		sParticle->m_vecVelocity	= vDir * -speed;
		sParticle->m_vecVelocity[2] += 16.0f;

		sParticle->m_uchColor[0]	= 255;
		sParticle->m_uchColor[1]	= 200;
		sParticle->m_uchColor[2]	= 32;
		sParticle->m_uchStartAlpha	= random->RandomInt( 64, 128 );
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= random->RandomInt( 16, 32 );
		sParticle->m_uchEndSize		= sParticle->m_uchStartSize * 2;
		sParticle->m_flRoll			= random->RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= random->RandomFloat( -1.0f, 1.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void AntlionGibCallback( const CEffectData &data )
{
	FX_AntlionGib( data.m_vOrigin, data.m_vNormal, data.m_flScale );
}

DECLARE_CLIENT_EFFECT( "AntlionGib", AntlionGibCallback );
