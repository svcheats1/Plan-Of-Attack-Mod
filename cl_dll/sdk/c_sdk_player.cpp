//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_sdk_player.h"
#include "weapon_sdkbase.h"
#include "c_basetempentity.h"
#include "usermessages.h"
#include "baseviewport.h"
#include "iclientmode.h"
#include "hud_skill.h"
#include "hud_levelchanged.h"
#include "movevars_shared.h"
#include "hud_macros.h"
#include "in_buttons.h"
#include "obstacle_pushaway.h"
#include "c_playerresource.h"

extern CUserMessages *usermessages;

#if defined( CSDKPlayer )
	#undef CSDKPlayer
#endif


/**
* Defines a handler for the SetFreezeStateMessage
*
* @param bf_read &msg The message to handle
* @return void
**/
void __MsgFunc_SetFreezeState(bf_read &msg)
{
	bool bState;

	// grab the player
	C_SDKPlayer *pPlayer = C_SDKPlayer::GetLocalSDKPlayer();

	// what are we doing?
	bState = (bool)msg.ReadByte();

	// did we get them? freeze them
	if(pPlayer)
		pPlayer->SetFreezeState(bState);

	// hide the objective panel if freeze time is over
	if(gViewPortInterface && !bState)
		gViewPortInterface->ShowPanel(PANEL_OBJECTIVES, false);
}

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		// Create the effect.
		C_SDKPlayer *pPlayer = dynamic_cast< C_SDKPlayer* >( m_hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get() );
		}	
	}

public:
	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_iEvent ) )
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_SDKPlayer, DT_SDKLocalPlayerExclusive )
	RecvPropInt( RECVINFO( m_iShotsFired ) ),
	RecvPropInt(RECVINFO(m_iCash)),
	RecvPropInt(RECVINFO(m_iTDStrikes)),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE(C_SDKPlayer, DT_SDKPlayerSensitive)
	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
	RecvPropFloat(RECVINFO( m_angEyeAngles[2])),
	RecvPropFloat(RECVINFO(m_fXP)),
	RecvPropVector(RECVINFO(m_vecEyePosition)),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_SDKPlayer, DT_SDKPlayer, CSDKPlayer )
	RecvPropDataTable( "sdklocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_SDKLocalPlayerExclusive) ),
	RecvPropDataTable("sensdata", 0, 0, &REFERENCE_RECV_TABLE(DT_SDKPlayerSensitive)),
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
	RecvPropFloat( RECVINFO( m_flStamina ) ),
	RecvPropInt( RECVINFO( m_iThrowGrenadeCounter ) ),
	RecvPropString(RECVINFO(m_szName)),
END_RECV_TABLE()

class C_SDKRagdoll : public C_BaseAnimatingOverlay
{
public:
	DECLARE_CLASS( C_SDKRagdoll, C_BaseAnimatingOverlay );
	DECLARE_CLIENTCLASS();

	C_SDKRagdoll();
	~C_SDKRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	int GetPlayerEntIndex() const;
	IRagdoll* GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );

private:

	C_SDKRagdoll( const C_SDKRagdoll & ) {}

	void Interp_Copy( VarMapping_t *pDest, CBaseEntity *pSourceEntity, VarMapping_t *pSrc );

	void CreateRagdoll();


private:

	EHANDLE	m_hPlayer;
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_SDKRagdoll, DT_SDKRagdoll, CSDKRagdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),
	RecvPropInt( RECVINFO(m_nForceBone) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) )
END_RECV_TABLE()


C_SDKRagdoll::C_SDKRagdoll()
{
}

C_SDKRagdoll::~C_SDKRagdoll()
{
	PhysCleanupFrictionSounds( this );
}

void C_SDKRagdoll::Interp_Copy( VarMapping_t *pDest, CBaseEntity *pSourceEntity, VarMapping_t *pSrc )
{
	if ( !pDest || !pSrc )
		return;

	if ( pDest->m_Entries.Count() != pSrc->m_Entries.Count() )
	{
		Assert( false );
		return;
	}

	int c = pDest->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		pDest->m_Entries[ i ].watcher->Copy( pSrc->m_Entries[i].watcher );
	}

	Interp_Copy( pDest->m_pBaseClassVarMapping, pSourceEntity, pSrc->m_pBaseClassVarMapping );
}

void C_SDKRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if ( iDamageType == DMG_BLAST )
	{
		dir *= 4000;  // adjust impact strenght

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  

		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	
	}
}


void C_SDKRagdoll::CreateRagdoll()
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_SDKPlayer *pPlayer = dynamic_cast< C_SDKPlayer* >( m_hPlayer.Get() );

	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer());			
		if ( bRemotePlayer )
		{
			Interp_Copy( varMap, pPlayer, pPlayer->C_BaseAnimatingOverlay::GetVarMapping() );

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( m_vecRagdollOrigin );

			SetAbsAngles( pPlayer->GetRenderAngles() );

			SetAbsVelocity( m_vecRagdollVelocity );

			int iSeq = LookupSequence( "walk_lower" );
			if ( iSeq == -1 )
			{
				Assert( false );	// missing walk_lower?
				iSeq = 0;
			}

			SetSequence( iSeq );	// walk_lower, basic pose
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}		
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin( m_vecRagdollOrigin );

		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );

	}

	SetModelIndex( m_nModelIndex );

	// Turn it into a ragdoll.
	// Make us a ragdoll..
	m_nRenderFX = kRenderFxRagdoll;

	// create ragdoll and set the skin from the player if they're available
	CBaseAnimating *pAnimating = BecomeRagdollOnClient( false );
	if(pPlayer && pAnimating)
		pAnimating->m_nSkin = pPlayer->m_nSkin;
}


void C_SDKRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		CreateRagdoll();
	
		IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

		if( pPhysicsObject )
		{
			AngularImpulse aVelocity(0,0,0);

			Vector vecExaggeratedVelocity = 3 * m_vecRagdollVelocity;

			pPhysicsObject->AddVelocity( &vecExaggeratedVelocity, &aVelocity );
		}
	}
}

IRagdoll* C_SDKRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

C_BaseAnimating * C_SDKPlayer::BecomeRagdollOnClient( bool bCopyEntity )
{
	// Let the C_CSRagdoll entity do this.
	// m_builtRagdoll = true;
	return NULL;
}


IRagdoll* C_SDKPlayer::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_SDKRagdoll *pRagdoll = (C_SDKRagdoll*)m_hRagdoll.Get();

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}

C_SDKPlayer::C_SDKPlayer() : 
	m_iv_angEyeAngles( "C_SDKPlayer::m_iv_angEyeAngles" )
{
	// register message handler once
	usermessages->HookMessage("SetFreezeState", __MsgFunc_SetFreezeState);

	// listen for skill changes
	gameeventmanager->AddListener(this, "skill_changed", false);

	m_PlayerAnimState = CreatePlayerAnimState( this, this, LEGANIM_9WAY, true );

	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

	m_vecEyePosition.Init();

	// no skill class
	m_pSkill = new CSkillClassNone;

	// no cash
	m_iCash = 0;
	m_fLastScale = 0;

	m_fNextThinkPushAway = 0.0f;
}

C_SDKPlayer::~C_SDKPlayer()
{
	m_PlayerAnimState->Release();

	// unregister for the event
	gameeventmanager->RemoveListener(this);

	// destroy the skill
	if(m_pSkill)
		delete m_pSkill;
}

void C_SDKPlayer::Spawn()
{
	m_Local.m_bInSprint = false;

	BaseClass::Spawn();

	// @HACK - kill my bone cache so that any changes to my model are reworked
	//RecreateBoneCache();
}

void C_SDKPlayer::ClientThink()
{
	BaseClass::ClientThink();

	if ( gpGlobals->curtime >= m_fNextThinkPushAway )
	{
		PerformObstaclePushaway( this );
		m_fNextThinkPushAway =  gpGlobals->curtime + 0.05f;
	}
}

C_SDKPlayer* C_SDKPlayer::GetLocalSDKPlayer()
{
	return ToSDKPlayer( C_BasePlayer::GetLocalPlayer() );
}

/**
* Handles any events we receive
*
* @param KeyValues *pEvent The event we need to handle
* @return void
**/
void C_SDKPlayer::FireGameEvent(IGameEvent *pEvent)
{
	// what type is it?
	if(!strcmp(pEvent->GetName(), "skill_changed"))
	{
		// is it mine?
		if(!GetLocalPlayer() || GetLocalPlayer()->GetUserID() != pEvent->GetInt("userid"))
			return;

		// pull the xp
		// JD: This is a network variable.
		m_fXP = pEvent->GetFloat("xp");

		// update the skill
		SkillChanged(pEvent);
	}
}

void C_SDKPlayer::ProcessMuzzleFlashEvent()
{
	// pass it off to the active weapon
	if (m_hActiveWeapon) {
		C_BaseAnimating *pWeapon = dynamic_cast<C_BaseAnimating*>((CBaseEntity*)m_hActiveWeapon);
		pWeapon->ProcessMuzzleFlashEvent();
	}
	else
		BaseClass::ProcessMuzzleFlashEvent();
}

/**
* Updates the skill with the new values
*
* @param KeyValues *pValues The new values
* @return void
**/
void C_SDKPlayer::SkillChanged(IGameEvent *pValues)
{
	CSkillClass *pSkill;

	// did our class change?
	if(!m_pSkill || 
		(strlen(pValues->GetString("class")) && !FStrEq(m_pSkill->GetInternalClassName(), pValues->GetString("class"))))
	{
		// create the new one
		pSkill = CSkillClass::CreateSkillByName(pValues->GetString("class"));

		// if we have a new one and our old one existed, destroy it
		if(pSkill && pSkill->GetClassIndex() != NONE_CLASS_INDEX && m_pSkill)
			delete(m_pSkill);

		// set the new skill
		if(pSkill)
		{
			m_pSkill = pSkill;
			m_pSkill->SetPlayer(this);
		}

		// @HACK - we need to kill the bone cache as we have a new model
		// this isn't the place to do this...
		//RecreateBoneCache();
	}

	// do we have a skill?
	if(m_pSkill)
	{
		// update the values
		m_pSkill->SetLevelBoost(pValues->GetInt("boost"));
	}

	// find the hudskill panel
	HudSkill *pHudSkill = (HudSkill*) GET_HUDELEMENT(HudSkill);

	// did we get it?
	if(pHudSkill)
	{
		// tell it we got an update
		pHudSkill->LocalSkillUpdated();
	}

	if (m_pSkill)
	{
		HudLevelChanged *pHudLevelChanged = (HudLevelChanged*)GET_HUDELEMENT(HudLevelChanged);
		if (pHudLevelChanged)
			pHudLevelChanged->LevelChanged(m_pSkill->GetSkillLevel(true) + 1);
	}
}

CSkillClass* C_SDKPlayer::GetSkillClass(bool bIgnoreObserver/* = false*/)
{
	C_SDKPlayer *fromPlayer = this;

	// if localplayer is in InEye spectator mode, return skill on chased player
	if (!bIgnoreObserver && fromPlayer == g_pLocalPlayer && g_pLocalPlayer->IsObserver() && GetObserverMode() == OBS_MODE_IN_EYE)
	{
		C_BaseEntity *target =  GetObserverTarget();

		if ( target && target->IsPlayer() )
		{
			fromPlayer = ToSDKPlayer( target );
			if ( fromPlayer && fromPlayer != g_pLocalPlayer ) {
				fromPlayer->UpdateSkill();
				return fromPlayer->GetSkillClass();
			}
			// in eye spectating yourself...
			else if ( fromPlayer ) {
				return m_pSkill;
			}
		}

		return NULL;
	}

	return m_pSkill;
}

void C_SDKPlayer::UpdateSkill()
{
	IGameResources *gr = GameResources();

	if (gr) {
		SKILL_CLASS_INDEX index = (SKILL_CLASS_INDEX)gr->GetPlayerSkillIndex(entindex());
		
		if (index > NONE_CLASS_INDEX && (!m_pSkill || m_pSkill->GetClassIndex() != index)) {
			CSkillClass *pSkill = CSkillClass::CreateSkillByIndex(index);

			if (pSkill) {
				if (m_pSkill)
					delete m_pSkill;
		
				m_pSkill = pSkill;
				m_pSkill->SetPlayer(this);
			}
		}
	}
}

float C_SDKPlayer::GetXP(void)
{
	return m_fXP;

	/*
	const C_SDKPlayer *fromPlayer = this;

	// if localplayer is in InEye spectator mode, return weapon on chased player
	if ( (fromPlayer == g_pLocalPlayer) && ( GetObserverMode() == OBS_MODE_IN_EYE) )
	{
		C_BaseEntity *target =  GetObserverTarget();

		if ( target && target->IsPlayer() )
		{
			fromPlayer = ToSDKPlayer( target );

			IGameResources *gr = GameResources();
			if (gr)
				return gr->GetPlayerXP(fromPlayer->entindex());
		}
		return 0;
	}
	*/
}


/**
* Sets the freeze state of the player
*
* @param bf_read &msg The message we're receiving
* @return void
**/
void C_SDKPlayer::SetFreezeState(bool bShouldFreeze)
{
	// what are we doing?
	if(bShouldFreeze)
	{
		// no control
		SetMoveType(MOVETYPE_NONE);
		m_bIsFrozen = true;
	}
	else
	{
		// lots o' control
		SetMoveType(MOVETYPE_WALK);
		m_bIsFrozen = false;
	}
}

const QAngle& C_SDKPlayer::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_PlayerAnimState->GetRenderAngles();
	}
}


void C_SDKPlayer::UpdateClientSideAnimation()
{
	// Update the animation data. It does the local check here so this works when using
	// a third-person camera (and we don't have valid player angles).
	if ( this == C_SDKPlayer::GetLocalSDKPlayer() )
		m_PlayerAnimState->Update( EyeAngles()[YAW], m_angEyeAngles[PITCH] );
	else
		m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	BaseClass::UpdateClientSideAnimation();
}


void C_SDKPlayer::PostDataUpdate( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );
	
	BaseClass::PostDataUpdate( updateType );
}

void C_SDKPlayer::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	UpdateVisibility();
}


void C_SDKPlayer::DoAnimationEvent( PlayerAnimEvent_t event )
{
	if ( event == PLAYERANIMEVENT_THROW_GRENADE )
	{
		// Let the server handle this event. It will update m_iThrowGrenadeCounter and the client will
		// pick up the event in CCSPlayerAnimState.
	}
	else	
	{
		m_PlayerAnimState->DoAnimationEvent( event );
	}
}

void C_SDKPlayer::PreThink()
{
	SetMaxSpeed(CalcMaxSpeed());

	BaseClass::PreThink();
}

void C_SDKPlayer::PostThink()
{
	BaseClass::PostThink();

	CalculateStamina();
}

bool C_SDKPlayer::ShouldDraw( void )
{
	// If we're dead, our ragdoll will be drawn for us instead.
	if ( !IsAlive() )
		return false;

	if( GetTeamNumber() == TEAM_SPECTATOR )
		return false;

	if( IsLocalPlayer() && IsRagdoll() )
		return true;

	return BaseClass::ShouldDraw();
}

CWeaponSDKBase* C_SDKPlayer::GetActiveSDKWeapon() const
{
	return dynamic_cast< CWeaponSDKBase* >( GetActiveWeapon() );
}


void C_SDKPlayer::CalcInEyeCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	BaseClass::CalcInEyeCamView(eyeOrigin, eyeAngles, fov);

	QAngle sniperDrift(0, 0, 0);
	CalcSniperDrift(sniperDrift);
	VectorAdd(eyeAngles, sniperDrift, eyeAngles);
	
	engine->SetViewAngles( eyeAngles );
}

void C_SDKPlayer::CalcDeathCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	// make sure we have an sdk player
	// @TODO - check that they're still connected
	C_SDKPlayer *pPlayer;
	pPlayer = ToSDKPlayer(GetObserverTarget());
	if(!pPlayer)
		SetObserverTarget(NULL);

	// jump down
	BaseClass::CalcDeathCamView(eyeOrigin, eyeAngles, fov);
}

// define the command to change teams
CON_COMMAND( changeteam, "Changes your team" )
{
	// how many args?
	if ( engine->Cmd_Argc() == 1 )
	{
		// just forward the command without parameters
		engine->ServerCmd( engine->Cmd_Argv(0) );
	}
	else if( engine->Cmd_Argc() == 2 )
	{
		// forward the command with parameter
		char command[128];
		Q_snprintf( command, sizeof(command), "%s %s", engine->Cmd_Argv(0), engine->Cmd_Argv(1) );
		engine->ServerCmd( command );
	}
}

CON_COMMAND(changeskill, "Changes your skill")
{
	if( engine->Cmd_Argc() == 2 )
	{
		// forward the command with parameter
		char command[128];
		Q_snprintf( command, sizeof(command), "%s %s", engine->Cmd_Argv(0), engine->Cmd_Argv(1) );
		engine->ServerCmd( command );
	}
}

CON_COMMAND( drop, "Drops your current weapon" )
{
	engine->ServerCmd( "drop" );
}
