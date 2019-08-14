//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side CBasePlayer.
//
//			- Manages the player's flashlight effect.
//
//=============================================================================//
#include "cbase.h"
#include "c_baseplayer.h"
#include "flashlighteffect.h"
#include "weapon_selection.h"
#include "history_resource.h"
#include "iinput.h"
#include "view.h"
#include "iviewrender.h"
#include "iclientmode.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "c_soundscape.h"
#include "usercmd.h"
#include "c_playerresource.h"
#include "iclientvehicle.h"
#include "view_shared.h"
#include "C_VguiScreen.h"
#include "movevars_shared.h"
#include "prediction.h"
#include "tier0/vprof.h"
#include "filesystem.h"
#include "bitbuf.h"
#include "KeyValues.h"
#include "particles_simple.h"
#include "fx_water.h"
#include "hltvcamera.h"
#include "takedamageinfo.h"
#include "c_sdk_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Don't alias here
#if defined( CBasePlayer )
#undef CBasePlayer	
#endif

#define FLASHLIGHT_DISTANCE		1000
#define MAX_VGUI_INPUT_MODE_SPEED 30
#define MAX_VGUI_INPUT_MODE_SPEED_SQ (MAX_VGUI_INPUT_MODE_SPEED*MAX_VGUI_INPUT_MODE_SPEED)

static Vector WALL_MIN(-WALL_OFFSET,-WALL_OFFSET,-WALL_OFFSET);
static Vector WALL_MAX(WALL_OFFSET,WALL_OFFSET,WALL_OFFSET);

extern ConVar default_fov;
extern ConVar sensitivity;

static ConVar	cl_customsounds ( "cl_customsounds", "0", 0, "Enable customized player sound playback" );
static ConVar	spec_track		( "spec_track", "0", 0, "Tracks an entity in spec mode" );
static ConVar	cl_autowepswitch( "cl_autowepswitch", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "Turns on/off auto weapon switching" );

#ifdef CS_BETA
	static ConVar	cl_smooth		( "cl_smooth", "0", 0, "Smooth view/eye origin after prediction errors" );
#else
	static ConVar	cl_smooth		( "cl_smooth", "1", 0, "Smooth view/eye origin after prediction errors" );
#endif

static ConVar	cl_smoothtime	( 
	"cl_smoothtime", 
	"0.1", 
	0, 
	"Smooth client's view after prediction error over this many seconds",
	true, 0.01,	// min/max is 0.01/2.0
	true, 2.0
	 );

ConVar zoom_sensitivity_ratio( "zoom_sensitivity_ratio", "1.0", 0, "Additional mouse sensitivity scale factor applied when FOV is zoomed in." );

void RecvProxy_FOV( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_DefaultFOV( const CRecvProxyData *pData, void *pStruct, void *pOut );

void RecvProxy_LocalVelocityX( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_LocalVelocityY( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_LocalVelocityZ( const CRecvProxyData *pData, void *pStruct, void *pOut );

// -------------------------------------------------------------------------------- //
// RecvTable for CPlayerState.
// -------------------------------------------------------------------------------- //

	BEGIN_RECV_TABLE_NOBASE(CPlayerState, DT_PlayerState)
		RecvPropInt		(RECVINFO(deadflag)),
	END_RECV_TABLE()


BEGIN_RECV_TABLE_NOBASE( CPlayerLocalData, DT_Local )
	RecvPropArray3( RECVINFO_ARRAY(m_chAreaBits), RecvPropInt(RECVINFO(m_chAreaBits[0]))),
	RecvPropInt(RECVINFO(m_iHideHUD)),

	// View
	RecvPropInt(RECVINFO(m_iFOV), 0, RecvProxy_FOV),
	RecvPropFloat(RECVINFO(m_flFOVRate)),
	RecvPropInt(RECVINFO(m_iDefaultFOV), 0, RecvProxy_DefaultFOV),

	RecvPropInt		(RECVINFO(m_bDucked)),
	RecvPropInt		(RECVINFO(m_bDucking)),
	RecvPropInt		(RECVINFO(m_bInDuckJump)),
	RecvPropBool	(RECVINFO(m_bInSprint)),
	RecvPropFloat	(RECVINFO(m_flDucktime)),
	RecvPropFloat	(RECVINFO(m_flDuckJumpTime)),
	RecvPropFloat	(RECVINFO(m_flJumpTime)),
	RecvPropFloat	(RECVINFO(m_flFallVelocity)),
//	RecvPropInt		(RECVINFO(m_nOldButtons)),
	RecvPropVector	(RECVINFO(m_vecClientBaseVelocity)),
	RecvPropVector	(RECVINFO(m_vecPunchAngle)),
	RecvPropVector	(RECVINFO(m_vecPunchAngleVel)),
	RecvPropInt		(RECVINFO(m_bDrawViewmodel)),
	RecvPropInt		(RECVINFO(m_bWearingSuit)),
	RecvPropBool	(RECVINFO(m_bPoisoned)),
	RecvPropFloat	(RECVINFO(m_flStepSize)),
	RecvPropInt		(RECVINFO(m_bAllowAutoMovement)),

	// 3d skybox data
	RecvPropInt(RECVINFO(m_skybox3d.scale)),
	RecvPropVector(RECVINFO(m_skybox3d.origin)),
	RecvPropInt(RECVINFO(m_skybox3d.area)),

	// 3d skybox fog data
	RecvPropInt( RECVINFO( m_skybox3d.fog.enable ) ),
	RecvPropInt( RECVINFO( m_skybox3d.fog.blend ) ),
	RecvPropVector( RECVINFO( m_skybox3d.fog.dirPrimary ) ),
	RecvPropInt( RECVINFO( m_skybox3d.fog.colorPrimary ) ),
	RecvPropInt( RECVINFO( m_skybox3d.fog.colorSecondary ) ),
	RecvPropFloat( RECVINFO( m_skybox3d.fog.start ) ),
	RecvPropFloat( RECVINFO( m_skybox3d.fog.end ) ),

	// fog data
	RecvPropInt( RECVINFO( m_fog.enable ) ),
	RecvPropInt( RECVINFO( m_fog.blend ) ),
	RecvPropVector( RECVINFO( m_fog.dirPrimary ) ),
	RecvPropInt( RECVINFO( m_fog.colorPrimary ) ),
	RecvPropInt( RECVINFO( m_fog.colorSecondary ) ),
	RecvPropFloat( RECVINFO( m_fog.start ) ),
	RecvPropFloat( RECVINFO( m_fog.end ) ),
	RecvPropFloat( RECVINFO( m_fog.farz ) ),

	// audio data
	RecvPropVector( RECVINFO( m_audio.localSound[0] ) ),
	RecvPropVector( RECVINFO( m_audio.localSound[1] ) ),
	RecvPropVector( RECVINFO( m_audio.localSound[2] ) ),
	RecvPropVector( RECVINFO( m_audio.localSound[3] ) ),
	RecvPropVector( RECVINFO( m_audio.localSound[4] ) ),
	RecvPropVector( RECVINFO( m_audio.localSound[5] ) ),
	RecvPropVector( RECVINFO( m_audio.localSound[6] ) ),
	RecvPropVector( RECVINFO( m_audio.localSound[7] ) ),
	RecvPropInt( RECVINFO( m_audio.soundscapeIndex ) ),
	RecvPropInt( RECVINFO( m_audio.localBits ) ),
	RecvPropEHandle( RECVINFO( m_audio.ent ) ),

	// ladders
	RecvPropEHandle(RECVINFO(m_hLadder)),
	RecvPropBool(RECVINFO(m_LadderMove.m_bForceLadderMove)),
	RecvPropBool(RECVINFO(m_LadderMove.m_bForceMount)),
	RecvPropFloat(RECVINFO(m_LadderMove.m_flStartTime)),
	RecvPropFloat(RECVINFO(m_LadderMove.m_flArrivalTime)),
	RecvPropVector(RECVINFO(m_LadderMove.m_vecGoalPosition)),
	RecvPropVector(RECVINFO(m_LadderMove.m_vecStartPosition)),
	RecvPropEHandle(RECVINFO(m_LadderMove.m_hForceLadder)),
	RecvPropEHandle(RECVINFO(m_LadderMove.m_hReservedSpot)),
END_RECV_TABLE()

// -------------------------------------------------------------------------------- //
// This data only gets sent to clients that ARE this player entity.
// -------------------------------------------------------------------------------- //

	BEGIN_RECV_TABLE_NOBASE( C_BasePlayer, DT_LocalPlayerExclusive )

		RecvPropDataTable	( RECVINFO_DT(m_Local),0, &REFERENCE_RECV_TABLE(DT_Local) ),

		RecvPropFloat		( RECVINFO(m_vecViewOffset[0]) ),
		RecvPropFloat		( RECVINFO(m_vecViewOffset[1]) ),
		RecvPropFloat		( RECVINFO(m_vecViewOffset[2]) ),
		RecvPropFloat		( RECVINFO(m_flFriction) ),

		RecvPropArray3		( RECVINFO_ARRAY(m_iAmmo), RecvPropInt( RECVINFO(m_iAmmo[0])) ),
		
		RecvPropInt			( RECVINFO(m_fOnTarget) ),

		RecvPropInt			( RECVINFO( m_nTickBase ) ),
		RecvPropInt			( RECVINFO( m_nNextThinkTick ) ),

		RecvPropEHandle		( RECVINFO( m_hLastWeapon ) ),
		RecvPropEHandle		( RECVINFO( m_hGroundEntity ) ),

 		RecvPropFloat		( RECVINFO(m_vecVelocity[0]), 0, RecvProxy_LocalVelocityX ),
 		RecvPropFloat		( RECVINFO(m_vecVelocity[1]), 0, RecvProxy_LocalVelocityY ),
 		RecvPropFloat		( RECVINFO(m_vecVelocity[2]), 0, RecvProxy_LocalVelocityZ ),

		RecvPropVector		( RECVINFO( m_vecBaseVelocity ) ),

		RecvPropArray		( RecvPropEHandle( RECVINFO( m_hViewModel[0] ) ), m_hViewModel ),
		
		RecvPropEHandle		( RECVINFO( m_hConstraintEntity)),
		RecvPropVector		( RECVINFO( m_vecConstraintCenter) ),
		RecvPropFloat		( RECVINFO( m_flConstraintRadius )),
		RecvPropFloat		( RECVINFO( m_flConstraintWidth )),
		RecvPropFloat		( RECVINFO( m_flConstraintSpeedFactor )),

		RecvPropFloat		( RECVINFO( m_flDeathTime )),

		RecvPropInt			( RECVINFO( m_nWaterLevel ) ),
		RecvPropFloat		( RECVINFO( m_flLaggedMovementValue )),

	END_RECV_TABLE()

	
// -------------------------------------------------------------------------------- //
// DT_BasePlayer datatable.
// -------------------------------------------------------------------------------- //
	IMPLEMENT_CLIENTCLASS_DT(C_BasePlayer, DT_BasePlayer, CBasePlayer)
		// We have both the local and nonlocal data in here, but the server proxies
		// only send one.
		RecvPropDataTable( "localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalPlayerExclusive) ),

		RecvPropDataTable(RECVINFO_DT(pl), 0, &REFERENCE_RECV_TABLE(DT_PlayerState), DataTableRecvProxy_StaticDataTable),

		RecvPropEHandle( RECVINFO(m_hVehicle) ),
		RecvPropInt		(RECVINFO(m_MoveType) ),

		RecvPropInt		(RECVINFO(m_iHealth)),
		RecvPropInt		(RECVINFO(m_ArmorValue)),
		RecvPropInt		(RECVINFO(m_lifeState)),

		RecvPropFloat	(RECVINFO(m_flMaxspeed)),
		RecvPropInt		(RECVINFO(m_fFlags)),

		RecvPropInt		(RECVINFO(m_iObserverMode) ),
		RecvPropEHandle	(RECVINFO(m_hObserverTarget) ),
		RecvPropArray	(RecvPropEHandle( RECVINFO( m_hObserverViewModel[0] ) ), m_hObserverViewModel ),

		RecvPropString( RECVINFO(m_szLastPlaceName) ),

	END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( CPlayerState )

	DEFINE_PRED_FIELD(  deadflag, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	// DEFINE_FIELD( netname, string_t ),
	// DEFINE_FIELD( fixangle, FIELD_INTEGER ),
	// DEFINE_FIELD( anglechange, FIELD_FLOAT ),
	//DEFINE_FIELD( v_angle, FIELD_VECTOR ),

END_PREDICTION_DATA()	

BEGIN_PREDICTION_DATA_NO_BASE( CPlayerLocalData )

	// DEFINE_PRED_TYPEDESCRIPTION( m_skybox3d, sky3dparams_t ),
	// DEFINE_PRED_TYPEDESCRIPTION( m_fog, fogparams_t ),
	// DEFINE_PRED_TYPEDESCRIPTION( m_audio, audioparams_t ),

	DEFINE_FIELD( m_nStepside, FIELD_INTEGER ),

	DEFINE_PRED_FIELD( m_iHideHUD, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iFOV, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_vecPunchAngle, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.125f ),
	DEFINE_PRED_FIELD_TOL( m_vecPunchAngleVel, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.125f ),
	DEFINE_PRED_FIELD( m_bDrawViewmodel, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bWearingSuit, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bPoisoned, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bAllowAutoMovement, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_bDucked, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDucking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bInDuckJump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flDucktime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flDuckJumpTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flJumpTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flFallVelocity, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.5f ),
//	DEFINE_PRED_FIELD( m_nOldButtons, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( m_nOldButtons, FIELD_INTEGER ),
	DEFINE_PRED_FIELD( m_vecClientBaseVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flStepSize, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),

	// ladders
	DEFINE_PRED_FIELD( m_hLadder, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()	

BEGIN_PREDICTION_DATA( C_BasePlayer )

	DEFINE_PRED_TYPEDESCRIPTION( m_Local, CPlayerLocalData ),
	DEFINE_PRED_TYPEDESCRIPTION( pl, CPlayerState ),

	DEFINE_PRED_FIELD( m_hVehicle, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flMaxspeed, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.5f ),
	DEFINE_PRED_FIELD( m_iHealth, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_ArmorValue, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fOnTarget, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nNextThinkTick, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_lifeState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nWaterLevel, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	
	DEFINE_PRED_FIELD_TOL( m_vecBaseVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.05 ),

	DEFINE_FIELD( m_nButtons, FIELD_INTEGER ),
	DEFINE_FIELD( m_flWaterJumpTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_nImpulse, FIELD_INTEGER ),
	DEFINE_FIELD( m_flStepSoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSwimSoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecLadderNormal, FIELD_VECTOR ),
	DEFINE_FIELD( m_flPhysics, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_szAnimExtension, FIELD_CHARACTER ),
	DEFINE_FIELD( m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonReleased, FIELD_INTEGER ),
	// DEFINE_FIELD( m_vecOldViewAngles, FIELD_VECTOR ),

	// DEFINE_ARRAY( m_iOldAmmo, FIELD_INTEGER,  MAX_AMMO_TYPES ),

	//DEFINE_FIELD( m_hOldVehicle, FIELD_EHANDLE ),
	// DEFINE_FIELD( m_pModelLight, dlight_t* ),
	// DEFINE_FIELD( m_pEnvironmentLight, dlight_t* ),
	// DEFINE_FIELD( m_pBrightLight, dlight_t* ),
	DEFINE_PRED_FIELD( m_hLastWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_nTickBase, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_hGroundEntity, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_ARRAY( m_hViewModel, FIELD_EHANDLE, MAX_VIEWMODELS, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( player, C_BasePlayer );


// -------------------------------------------------------------------------------- //
// Functions.
// -------------------------------------------------------------------------------- //
C_BasePlayer::C_BasePlayer() : m_iv_vecViewOffset( "C_BasePlayer::m_iv_vecViewOffset" )
{
	CONSTRUCT_PREDICTABLE( C_BasePlayer );

	AddVar( &m_vecViewOffset, &m_iv_vecViewOffset, LATCH_SIMULATION_VAR );
	

#ifdef _DEBUG																
	m_vecLadderNormal.Init();
	m_vecOldViewAngles.Init();
#endif

	m_pFlashlight = NULL;

	m_pCurrentVguiScreen = NULL;
	m_pCurrentCommand = NULL;

	m_flPredictionErrorTime = -100;
	m_StuckLast = 0;
	m_bWasFrozen = false;
	m_bIsFrozen = false;

	m_bResampleWaterSurface = true;
	
	ResetObserverMode();

	m_vecPredictionError.Init();
	m_flPredictionErrorTime = 0;

	m_surfaceProps = 0;
	m_pSurfaceData = NULL;
	m_surfaceFriction = 1.0f;
	m_chTextureType = 0;
}

C_BasePlayer *g_pLocalPlayer = NULL;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BasePlayer::~C_BasePlayer()
{
	DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
	if ( this == g_pLocalPlayer )
	{
		g_pLocalPlayer = NULL;
	}

	if (m_pFlashlight)
	{
		delete m_pFlashlight;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BasePlayer::Spawn( void )
{
	// Clear all flags except for FL_FULLEDICT
	ClearFlags();
	AddFlag( FL_CLIENT );

	int effects = GetEffects() & EF_NOSHADOW;
	SetEffects( effects );

	m_Local.m_iFOV	= 0;	// init field of view.

    SetModel( "models/player.mdl" );

	Precache();

	SetThink(NULL);

	SharedSpawn();
}

bool C_BasePlayer::IsHLTV() const
{
	return ( IsLocalPlayer() && engine->IsHLTV() );	
}

CBaseEntity	*C_BasePlayer::GetObserverTarget() const	// returns players targer or NULL
{
	if ( engine->IsHLTV() && IsLocalPlayer() )
	{
		return HLTVCamera()->GetObserverTarget();
	}

	if ( GetObserverMode() == OBS_MODE_ROAMING )
	{
		return NULL;	// no target in roaming mode
	}
	else
	{
		return m_hObserverTarget;
	}
}

int C_BasePlayer::GetObserverMode() const 
{ 
	if ( engine->IsHLTV() && IsLocalPlayer() )
	{
		return HLTVCamera()->GetObserverMode();
	}

	return m_iObserverMode; 
}

bool C_BasePlayer::ViewModel_IsTransparent( void )
{
	return IsTransparent();
}

//-----------------------------------------------------------------------------
// Used by prediction, sets the view angles for the player
//-----------------------------------------------------------------------------
void C_BasePlayer::SetLocalViewAngles( const QAngle &viewAngles )
{
	pl.v_angle = viewAngles;
}

surfacedata_t* C_BasePlayer::GetGroundSurface()
{
	//
	// Find the name of the material that lies beneath the player.
	//
	Vector start, end;
	VectorCopy( GetAbsOrigin(), start );
	VectorCopy( start, end );

	// Straight down
	end.z -= 64;

	// Fill in default values, just in case.
	
	Ray_t ray;
	ray.Init( start, end, GetPlayerMins(), GetPlayerMaxs() );

	trace_t	trace;
	UTIL_TraceRay( ray, MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );

	if ( trace.fraction == 1.0f )
		return NULL;	// no ground
	
	return physprops->GetSurfaceData( trace.surface.surfaceProps );
}


//-----------------------------------------------------------------------------
// returns the player name
//-----------------------------------------------------------------------------
const char * C_BasePlayer::GetPlayerName()
{
	return g_PR ? g_PR->GetPlayerName( entindex() ) : "";
}

//-----------------------------------------------------------------------------
// Is the player dead?
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsPlayerDead()
{
	return pl.deadflag == true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_BasePlayer::SetVehicleRole( int nRole )
{
	if ( !IsInAVehicle() )
		return;

	// HL2 has only a player in a vehicle.
	if ( nRole > VEHICLE_DRIVER )
		return;

	char szCmd[64];
	Q_snprintf( szCmd, sizeof( szCmd ), "vehicleRole %i\n", nRole );
	engine->ServerCmd( szCmd );
}

//-----------------------------------------------------------------------------
// Purpose: Store original ammo data to see what has changed
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BasePlayer::OnPreDataChanged( DataUpdateType_t updateType )
{
	for (int i = 0; i < MAX_AMMO_TYPES; ++i)
	{
		m_iOldAmmo[i] = GetAmmoCount(i);
	}

	BaseClass::OnPreDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_BasePlayer::PostDataUpdate( DataUpdateType_t updateType )
{
	// This has to occur here as opposed to OnDataChanged so that EHandles to the player created
	//  on this same frame are not stomped because prediction thinks there
	//  isn't a local player yet!!!
	if ( IsLocalPlayer() )
	{
		// Make sure g_pLocalPlayer is correct
		g_pLocalPlayer = this;
		SetSimulatedEveryTick( true );
	}
	else
	{
		SetSimulatedEveryTick( false );

		// estimate velocity for non local players
		float flTimeDelta = m_flSimulationTime - m_flOldSimulationTime;
		if ( flTimeDelta > 0  && !IsEffectActive(EF_NOINTERP) )
		{
			Vector newVelo = (GetNetworkOrigin() - GetOldOrigin()  ) / flTimeDelta;
			SetAbsVelocity( newVelo);
		}
	}

	BaseClass::PostDataUpdate( updateType );

	// Only care about this for local player
	if ( IsLocalPlayer() )
	{
		default_fov.SetValue( m_Local.m_iDefaultFOV );

		//Update our FOV, including any zooms going on
		int iDefaultFOV = default_fov.GetInt();
		int	localFOV	= GetFOV();
		int min_fov		= GetMinFOV();

		// Don't let it go too low
		localFOV = max( min_fov, localFOV );

		if ( gHUD.m_flMouseSensitivityFactor )
		{
			gHUD.m_flMouseSensitivity = sensitivity.GetFloat() * gHUD.m_flMouseSensitivityFactor;
		}
		else
		{
			// No override, don't use huge sensitivity
			if ( localFOV == iDefaultFOV )
			{  
				// reset to saved sensitivity
				gHUD.m_flMouseSensitivity = 0;
			}
			else
			{  
				// Set a new sensitivity that is proportional to the change from the FOV default and scaled
				//  by a separate compensating factor
				gHUD.m_flMouseSensitivity = 
					sensitivity.GetFloat() * // regular sensitivity
					((float)localFOV / (float)iDefaultFOV) * // linear fov downscale
					zoom_sensitivity_ratio.GetFloat(); // sensitivity scale factor
			}
		}

		if ( updateType == DATA_UPDATE_CREATED )
		{
			QAngle angles;
			engine->GetViewAngles( angles );
			SetLocalViewAngles( angles );
			m_flOldPlayerZ = GetLocalOrigin().z;
		}
	}
	else
	{
// FIXME: BUG BUG: HACK HACK:  okay, you get the point
// when we finally fix the delta encoding relative to multiple
// players and the baseline, we can remove this
#if defined( CLIENT_DLL )
		for ( int i = 0 ; i < MAX_VIEWMODELS; i++ )
		{
			m_hViewModel[ i ] = NULL;
		}
#endif
	}
}

void C_BasePlayer::ReceiveMessage( int classID, bf_read &msg )
{
	if ( classID != GetClientClass()->m_ClassID )
	{
		// message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	int messageType = msg.ReadByte();

	switch( messageType )
	{
		case PLAY_PLAYER_JINGLE:
			PlayPlayerJingle();
			break;
	}
}

void C_BasePlayer::OnRestore()
{
	BaseClass::OnRestore();

	if ( IsLocalPlayer() )
	{
		// debounce the attack key, for if it was used for restore
		input->ClearInputButton( IN_ATTACK | IN_ATTACK2 );
		// GetButtonBits() has to be called for the above to take effect
		input->GetButtonBits( 0 );
	}

	// For ammo history icons to current value so they don't flash on level transtions
	for ( int i = 0; i < MAX_AMMO_TYPES; i++ )
	{
		m_iOldAmmo[i] = GetAmmoCount(i);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Process incoming data
//-----------------------------------------------------------------------------
void C_BasePlayer::OnDataChanged( DataUpdateType_t updateType )
{
	if ( IsLocalPlayer() )
	{
		SetPredictionEligible( true );
	}

	BaseClass::OnDataChanged( updateType );

	// Only care about this for local player
	if ( IsLocalPlayer() )
	{
		// Reset engine areabits pointer
		( *render->GetAreaBits() ) = m_Local.m_chAreaBits;

		// Check for Ammo pickups.
		for ( int i = 0; i < MAX_AMMO_TYPES; i++ )
		{
			if ( GetAmmoCount(i) > m_iOldAmmo[i] )
			{
				// Don't add to ammo pickup if the ammo doesn't do it
				const FileWeaponInfo_t *pWeaponData = gWR.GetWeaponFromAmmo(i);

				if ( !pWeaponData || !( pWeaponData->iFlags & ITEM_FLAG_NOAMMOPICKUPS ) )
				{
					// We got more ammo for this ammo index. Add it to the ammo history
					CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );
					if( pHudHR )
					{
						pHudHR->AddToHistory( HISTSLOT_AMMO, i, abs(GetAmmoCount(i) - m_iOldAmmo[i]) );
					}
				}
			}
		}

		Soundscape_Update( m_Local.m_audio );
	}
}


//-----------------------------------------------------------------------------
// Did we just enter a vehicle this frame?
//-----------------------------------------------------------------------------
bool C_BasePlayer::JustEnteredVehicle()
{
	if ( !IsInAVehicle() )
		return false;

	return ( m_hOldVehicle == m_hVehicle );
}

//-----------------------------------------------------------------------------
// Are we in VGUI input mode?.
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsInVGuiInputMode() const
{
	return (m_pCurrentVguiScreen.Get() != NULL);
}


//-----------------------------------------------------------------------------
// Check to see if we're in vgui input mode...
//-----------------------------------------------------------------------------
void C_BasePlayer::DetermineVguiInputMode( CUserCmd *pCmd )
{
	// If we're dead, close down and abort!
	if ( !IsAlive() )
	{
		DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
		m_pCurrentVguiScreen.Set( NULL );
		return;
	}

	// If we're in vgui mode *and* we're holding down mouse buttons,
	// stay in vgui mode even if we're outside the screen bounds
	if (m_pCurrentVguiScreen.Get() && (pCmd->buttons & (IN_ATTACK | IN_ATTACK2)) )
	{
		SetVGuiScreenButtonState( m_pCurrentVguiScreen.Get(), pCmd->buttons );

		// Kill all attack inputs if we're in vgui screen mode
		pCmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);
		return;
	}

	// We're not in vgui input mode if we're moving, or have hit a key
	// that will make us move...

	// Not in vgui mode if we're moving too quickly
	// ROBIN: Disabled movement preventing VGUI screen usage
	//if (GetVelocity().LengthSqr() > MAX_VGUI_INPUT_MODE_SPEED_SQ)
	if ( 0 )
	{
		DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
		m_pCurrentVguiScreen.Set( NULL );
		return;
	}

	// Don't enter vgui mode if we've got combat buttons held down
	bool bAttacking = false;
	if ( ((pCmd->buttons & IN_ATTACK) || (pCmd->buttons & IN_ATTACK2)) && !m_pCurrentVguiScreen.Get() )
	{
		bAttacking = true;
	}

	// Not in vgui mode if we're pushing any movement key at all
	// Not in vgui mode if we're in a vehicle...
	// ROBIN: Disabled movement preventing VGUI screen usage
	//if ((pCmd->forwardmove > MAX_VGUI_INPUT_MODE_SPEED) ||
	//	(pCmd->sidemove > MAX_VGUI_INPUT_MODE_SPEED) ||
	//	(pCmd->upmove > MAX_VGUI_INPUT_MODE_SPEED) ||
	//	(pCmd->buttons & IN_JUMP) ||
	//	(bAttacking) )
	if ( bAttacking || IsInAVehicle() )
	{ 
		DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
		m_pCurrentVguiScreen.Set( NULL );
		return;
	}

	// Not in vgui mode if there are no nearby screens
	C_BaseEntity *pOldScreen = m_pCurrentVguiScreen.Get();

	m_pCurrentVguiScreen = FindNearbyVguiScreen( EyePosition(), pCmd->viewangles, GetTeamNumber() );

	if (pOldScreen != m_pCurrentVguiScreen)
	{
		DeactivateVguiScreen( pOldScreen );
		ActivateVguiScreen( m_pCurrentVguiScreen.Get() );
	}

	if (m_pCurrentVguiScreen.Get())
	{
		SetVGuiScreenButtonState( m_pCurrentVguiScreen.Get(), pCmd->buttons );

		// Kill all attack inputs if we're in vgui screen mode
		pCmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);
	}
}

extern void SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);

void C_BasePlayer::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	if ( m_takedamage )
	{
		CTakeDamageInfo info = inputInfo;

		// Prevent team damage here so blood doesn't appear
		if ( info.GetAttacker()->IsPlayer() )
		{
			if (info.GetAttacker() != this) {
				ConVar *pFrFire = cvar->FindVar( "mp_friendlyfire" );
				if (!pFrFire || !pFrFire->GetBool()) {
					if (GetTeamNumber() == info.GetAttacker()->GetTeamNumber() && info.GetAttacker() != this)
						return;
				}
			}
		}

		switch ( ptr->hitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			info.ScaleDamage( 3.0 );
			break;
		case HITGROUP_CHEST:
			info.ScaleDamage( 1.15 );
			break;
		case HITGROUP_STOMACH:
			info.ScaleDamage( 1.0 );
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			info.ScaleDamage( 1.0 );
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			info.ScaleDamage( 0.5 );
			break;
		default:
			break;
		}

		int blood = BloodColor();		
		if ( blood != DONT_BLEED )
		{
			Vector vecOrigin = ptr->endpos - vecDir * 4;
			SpawnBlood( vecOrigin, vecDir, blood, info.GetDamage() );// a little surface blood.
			TraceBleed( info.GetDamage(), vecDir, ptr, info.GetDamageType() );
		}

		AddMultiDamage( info, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handling
//-----------------------------------------------------------------------------
void C_BasePlayer::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{
	// Allow the vehicle to clamp the view angles
	if ( IsInAVehicle() )
	{
		IClientVehicle *pVehicle = m_hVehicle.Get()->GetClientVehicle();
		if ( pVehicle )
		{
			pVehicle->UpdateViewAngles( this, pCmd );
			engine->SetViewAngles( pCmd->viewangles );
		}
	}

	// If the frozen flag is set, prevent view movement (server prevents the rest of the movement)
	if ( (GetFlags() & FL_FROZEN) && !m_bIsFrozen )
	{
		// Don't stomp the first time we get frozen
		if ( m_bWasFrozen )
		{
			// Stomp the new viewangles with old ones
			pCmd->viewangles = m_vecOldViewAngles;
			engine->SetViewAngles( pCmd->viewangles );
		}
		else
		{
			m_bWasFrozen = true;
		}
	}
	else
	{
		m_bWasFrozen = false;
	}

	m_vecOldViewAngles = pCmd->viewangles;
	
	// Check to see if we're in vgui input mode...
	DetermineVguiInputMode( pCmd );

}


//-----------------------------------------------------------------------------
// Purpose: Player has changed to a new team
//-----------------------------------------------------------------------------
void C_BasePlayer::TeamChange( int iNewTeam )
{
	// Base class does nothing
}


//-----------------------------------------------------------------------------
// Purpose: Creates, destroys, and updates the flashlight effect as needed.
//-----------------------------------------------------------------------------
void C_BasePlayer::UpdateFlashlight()
{
	// The dim light is the flashlight.
	if ( IsEffectActive( EF_DIMLIGHT ) )
	{
		if (!m_pFlashlight)
		{
			// Turned on the headlight; create it.
			m_pFlashlight = new CFlashlightEffect(index);

			if (!m_pFlashlight)
				return;

			m_pFlashlight->TurnOn();
		}

		Vector vecForward, vecRight, vecUp;
		EyeVectors( &vecForward, &vecRight, &vecUp );

		// Update the light with the new position and direction.		
		m_pFlashlight->UpdateLight( EyePosition(), vecForward, vecRight, vecUp, FLASHLIGHT_DISTANCE );
	}
	else if (m_pFlashlight)
	{
		// Turned off the flashlight; delete it.
		delete m_pFlashlight;
		m_pFlashlight = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates player flashlight if it's ative
//-----------------------------------------------------------------------------
void C_BasePlayer::Flashlight( void )
{
	UpdateFlashlight();

	// Check for muzzle flash and apply to view model
	C_BaseAnimating *ve = this;
	if ( GetObserverMode() == OBS_MODE_IN_EYE )
	{
		ve = dynamic_cast< C_BaseAnimating* >( GetObserverTarget() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Engine is asking whether to add this player to the visible entities list
//-----------------------------------------------------------------------------
void C_BasePlayer::AddEntity( void )
{
	// FIXME/UNDONE:  Should the local player say yes to adding itself now 
	// and then, when it ges time to render and it shouldn't still do the render with
	// STUDIO_EVENTS set so that its attachment points will get updated even if not
	// in third person?

	// Add in water effects
	if ( IsLocalPlayer() )
	{
		CreateWaterEffects();
	}

	// If set to invisible, skip. Do this before resetting the entity pointer so it has 
	// valid data to decide whether it's visible.
	if ( !IsVisible() || !g_pClientMode->ShouldDrawLocalPlayer( this ) )
	{
		return;
	}

	// Server says don't interpolate this frame, so set previous info to new info.
	if ( IsEffectActive(EF_NOINTERP) || 
		Teleported() )
	{
		ResetLatched();
	}

	// Add in lighting effects
	CreateLightEffects();
}

extern float UTIL_WaterLevel( const Vector &position, float minz, float maxz );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BasePlayer::CreateWaterEffects( void )
{
	// Must be completely submerged to bother
	if ( GetWaterLevel() < 3 )
	{
		m_bResampleWaterSurface = true;
		return;
	}

	// Do special setup if this is our first time back underwater
	if ( m_bResampleWaterSurface )
	{
		// Reset our particle timer
		m_tWaterParticleTimer.Init( 32 );
		
		// Find the surface of the water to clip against
		m_flWaterSurfaceZ = UTIL_WaterLevel( WorldSpaceCenter(), WorldSpaceCenter().z, WorldSpaceCenter().z + 256 );
		m_bResampleWaterSurface = false;
	}

	// Make sure the emitter is setup
	if ( m_pWaterEmitter == NULL )
	{
		if ( ( m_pWaterEmitter = WaterDebrisEffect::Create( "splish" ) ) == NULL )
			return;
	}

	Vector vecVelocity;
	GetVectors( &vecVelocity, NULL, NULL );

	Vector offset = WorldSpaceCenter();

	m_pWaterEmitter->SetSortOrigin( offset );

	PMaterialHandle	hMaterial[2];
	hMaterial[0] = g_ParticleMgr.GetPMaterial( "effects/fleck_cement1" );
	hMaterial[1] = g_ParticleMgr.GetPMaterial( "effects/fleck_cement2" );

	SimpleParticle	*pParticle;

	float curTime = gpGlobals->frametime;

	// Add as many particles as we need
	while ( m_tWaterParticleTimer.NextEvent( curTime ) )
	{
		offset = WorldSpaceCenter() + ( vecVelocity * 128.0f ) + RandomVector( -128, 128 );

		// Make sure we don't start out of the water!
		if ( offset.z > m_flWaterSurfaceZ )
		{
			offset.z = ( m_flWaterSurfaceZ - 8.0f );
		}

		pParticle = (SimpleParticle *) m_pWaterEmitter->AddParticle( sizeof(SimpleParticle), hMaterial[random->RandomInt(0,1)], offset );

		if (pParticle == NULL)
			continue;

		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= random->RandomFloat( 2.0f, 4.0f );

		pParticle->m_vecVelocity = RandomVector( -2.0f, 2.0f );

		//FIXME: We should tint these based on the water's fog value!
		float color = random->RandomInt( 32, 128 );
		pParticle->m_uchColor[0] = color;
		pParticle->m_uchColor[1] = color;
		pParticle->m_uchColor[2] = color;

		pParticle->m_uchStartSize	= 1;
		pParticle->m_uchEndSize		= 1;
		
		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;
		
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -0.5f, 0.5f );
	}
}

//-----------------------------------------------------------------------------
// Called when not in tactical mode. Allows view to be overriden for things like driving a tank.
//-----------------------------------------------------------------------------
void C_BasePlayer::OverrideView( CViewSetup *pSetup )
{
}

bool C_BasePlayer::ShouldInterpolate()
{
	// always interpolate myself
	if ( IsLocalPlayer() )
		return true;

	// always interpolate entity if followed by HLTV
	if ( HLTVCamera()->GetCameraMan() == this )
		return true;

	return BaseClass::ShouldInterpolate();
}


bool C_BasePlayer::ShouldDraw()
{
	return ( !IsLocalPlayer() || input->CAM_IsThirdPerson() || (GetObserverMode() == OBS_MODE_DEATHCAM ) ) &&
		   BaseClass::ShouldDraw();
}

int C_BasePlayer::DrawModel( int flags )
{
	// if local player is spectating this player in first person mode, don't draw it
	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player && player->IsObserver() )
	{
		if ( player->GetObserverMode() == OBS_MODE_IN_EYE &&
			 player->GetObserverTarget() == this )
			return 0;
	}

	return BaseClass::DrawModel( flags );
}

void C_BasePlayer::CalcChaseCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_BaseEntity *target = GetObserverTarget();

	if ( !target || target->GetHealth() <= 0) 
	{
		// just copy a save in-map position
		VectorCopy( EyePosition(), eyeOrigin );
		VectorCopy( EyeAngles(), eyeAngles );
		return;
	}

	// QAngle tmpangles;

	Vector forward, viewpoint;

	// GetRenderOrigin() returns ragdoll pos if player is ragdolled
	Vector origin = target->GetRenderOrigin();

	C_BasePlayer *player = ToBasePlayer( target );
	
	if ( player && player->IsAlive() )
	{
		if( player->GetFlags() & FL_DUCKING )
		{
			VectorAdd( origin, VEC_DUCK_VIEW, origin );
		}
		else
		{
			VectorAdd( origin, VEC_VIEW, origin );
		}
	}
	else
	{
		// assume it's the players ragdoll
		VectorAdd( origin, VEC_DEAD_VIEWHEIGHT, origin );
	}

	QAngle viewangles;

	if ( IsLocalPlayer() )
	{
		engine->GetViewAngles( viewangles );
	}
	else
	{
		viewangles = EyeAngles();
	}

	m_flObserverChaseDistance += gpGlobals->frametime*48.0f;
	m_flObserverChaseDistance = clamp( m_flObserverChaseDistance, 16, CHASE_CAM_DISTANCE );
	
	AngleVectors( viewangles, &forward );

	VectorNormalize( forward );

	VectorMA(origin, -m_flObserverChaseDistance, forward, viewpoint );

	trace_t trace;
	C_BaseEntity::EnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( origin, viewpoint, WALL_MIN, WALL_MAX, MASK_SOLID, target, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::EnableAbsRecomputations( true );

	if (trace.fraction < 1.0)
	{
		viewpoint = trace.endpos;
		m_flObserverChaseDistance = VectorLength(origin - eyeOrigin);
	}
	
	VectorCopy( viewangles, eyeAngles );
	VectorCopy( viewpoint, eyeOrigin );

	fov = GetFOV();
}

void C_BasePlayer::CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_BaseEntity *target = GetObserverTarget();
	
	if ( !target ) 
	{
		target = this;
	}

	m_flObserverChaseDistance = 0.0;

	eyeOrigin = target->EyePosition();
	eyeAngles = target->EyeAngles();
	
	if ( spec_track.GetInt() > 0 )
	{
		C_BaseEntity *target =  ClientEntityList().GetBaseEntity( spec_track.GetInt() );

		if ( target )
		{
			Vector v = target->GetAbsOrigin(); v.z += 54;
			QAngle a; VectorAngles( v - eyeOrigin, a );

			NormalizeAngles( a );
			eyeAngles = a;
			engine->SetViewAngles( a );
		}
	}

	// Apply a smoothing offset to smooth out prediction errors.
	Vector vSmoothOffset;
	GetPredictionErrorSmoothingVector( vSmoothOffset );
	eyeOrigin += vSmoothOffset;

	fov = GetFOV();
}

void C_BasePlayer::CalcInEyeCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_SDKPlayer *target = (C_SDKPlayer*)GetObserverTarget();

	if ( !target ) 
	{
		// just copy a save in-map position
		VectorCopy( EyePosition(), eyeOrigin );
		VectorCopy( EyeAngles(), eyeAngles );
		return;
	};

	if ( !target->IsAlive() )
	{
		// if dead, show from 3rd person
		CalcChaseCamView( eyeOrigin, eyeAngles, fov );
		return;
	}

	fov = GetFOV();	// TODO use tragets FOV

	m_flObserverChaseDistance = 0.0;

	eyeAngles = target->GetNetworkedEyeAngles();
	eyeOrigin = target->GetAbsOrigin();

	if( engine->IsHLTV() )
	{
		if ( GetFlags() & FL_DUCKING )
		{
			eyeOrigin += VEC_DUCK_VIEW;
		}
		else
		{
			eyeOrigin += VEC_VIEW;
		}
	}
	else
	{
		eyeOrigin += m_vecViewOffset;
	}

	// Apply punch angle
	VectorAdd( eyeAngles, m_Local.m_vecPunchAngle, eyeAngles );

	// JD: SDK will now set them.
	//engine->SetViewAngles( eyeAngles );
}

void C_BasePlayer::CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	CBaseEntity	* killer = GetObserverTarget();

	float interpolation = ( gpGlobals->curtime - m_flDeathTime ) / DEATH_ANIMATION_TIME;
	interpolation = clamp( interpolation, 0.0f, 1.0f );

	m_flObserverChaseDistance += gpGlobals->frametime*48.0f;
	m_flObserverChaseDistance = clamp( m_flObserverChaseDistance, 16, CHASE_CAM_DISTANCE );

	QAngle aForward = eyeAngles = EyeAngles();
	Vector origin = EyePosition();			

	IRagdoll *pRagdoll = GetRepresentativeRagdoll();
	if ( pRagdoll )
	{
		origin = pRagdoll->GetRagdollOrigin();
		origin.z += VEC_DEAD_VIEWHEIGHT.z; // look over ragdoll, not through
	}
	
	if ( killer && killer->IsPlayer() && (killer != this) ) 
	{
		Vector vKiller = killer->EyePosition() - origin;
		QAngle aKiller; 
		VectorAngles( vKiller, aKiller );
		InterpolateAngles( aForward, aKiller, eyeAngles, interpolation );
	};

	Vector vForward; AngleVectors( eyeAngles, &vForward );

	VectorNormalize( vForward );

	VectorMA( origin, -m_flObserverChaseDistance, vForward, eyeOrigin );

	trace_t trace; // clip against world
	C_BaseEntity::EnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID, this, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::EnableAbsRecomputations( true );

	if (trace.fraction < 1.0)
	{
		eyeOrigin = trace.endpos;
		m_flObserverChaseDistance = VectorLength(origin - eyeOrigin);
	}

	fov = GetFOV();
}



//-----------------------------------------------------------------------------
// Purpose: Return the weapon to have open the weapon selection on, based upon our currently active weapon
//			Base class just uses the weapon that's currently active.
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *C_BasePlayer::GetActiveWeaponForSelection( void )
{
	return GetActiveWeapon();
}

C_BaseAnimating* C_BasePlayer::GetRenderedWeaponModel()
{
	// Attach to either their weapon model or their view model.
	if ( input->CAM_IsThirdPerson() || this != C_BasePlayer::GetLocalPlayer() )
	{
		return GetActiveWeapon();
	}
	else
	{
		return GetViewModel();
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsLocalPlayer( void ) const
{
	return ( index == engine->GetLocalPlayer() ? true : false );
}

int	C_BasePlayer::GetUserID( void )
{
	player_info_t pi;

	if ( !engine->GetPlayerInfo( entindex(), &pi ) )
		return -1;

	return pi.userID;
}


// For weapon prediction
void C_BasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	// FIXME
}

void C_BasePlayer::UpdateClientData( void )
{
	// Update all the items
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		if ( GetWeapon(i) )  // each item updates it's successors
			GetWeapon(i)->UpdateClientData( this );
	}
}

// Prediction stuff
void C_BasePlayer::PreThink( void )
{
	ItemPreFrame();

	UpdateClientData();

	if (m_lifeState >= LIFE_DYING)
		return;

	//
	// If we're not on the ground, we're falling. Update our falling velocity.
	//
	if ( !( GetFlags() & FL_ONGROUND ) )
	{
		m_Local.m_flFallVelocity = -GetAbsVelocity().z;
	}
}

void C_BasePlayer::PostThink( void )
{
	if ( IsAlive())
	{
		// do weapon stuff
		ItemPostFrame();

		if ( GetFlags() & FL_ONGROUND )
		{		
			m_Local.m_flFallVelocity = 0;
		}

		// Don't allow bogus sequence on player
		if ( GetSequence() == -1 )
		{
			SetSequence( 0 );
		}

		StudioFrameAdvance();
	}

	// Even if dead simulate entities
	SimulatePlayerSimulatedEntities();
}

//-----------------------------------------------------------------------------
// Purpose: Simulate the player for this frame
//-----------------------------------------------------------------------------
void C_BasePlayer::Simulate()
{
	//Frame updates
	if ( this == C_BasePlayer::GetLocalPlayer() )
	{
		//Update the flashlight
		Flashlight();
	}
	else
	{
		// update step sounds for all other players
		Vector vel;
		EstimateAbsVelocity( vel );
		UpdateStepSound( GetGroundSurface(), GetAbsOrigin(), vel );
	}

	BaseClass::Simulate();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseViewModel
//-----------------------------------------------------------------------------
C_BaseViewModel *C_BasePlayer::GetViewModel( int index /*= 0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	C_BaseViewModel *vm;
	
	if ( GetObserverMode() == OBS_MODE_IN_EYE )
	{
		vm = m_hObserverViewModel[ index ];
	}
	else
	{
		vm = m_hViewModel[ index ];
	}

	return vm;
}

C_BaseCombatWeapon	*C_BasePlayer::GetActiveWeapon( void ) const
{
	const C_BasePlayer *fromPlayer = this;

	// if localplayer is in InEye spectator mode, return weapon on chased player
	if ( (fromPlayer == g_pLocalPlayer) && ( GetObserverMode() == OBS_MODE_IN_EYE) )
	{
		C_BaseEntity *target =  GetObserverTarget();

		if ( target && target->IsPlayer() )
		{
			fromPlayer = ToBasePlayer( target );
		}
	}

	return fromPlayer->C_BaseCombatCharacter::GetActiveWeapon();
}

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector C_BasePlayer::GetAutoaimVector( float flDelta )
{
	// Never autoaim a predicted weapon (for now)
	Vector	forward;
	AngleVectors( GetAbsAngles() + m_Local.m_vecPunchAngle, &forward );
	return	forward;
}

void C_BasePlayer::PlayPlayerJingle()
{
	// Find player sound for shooter
	player_info_t info;
	engine->GetPlayerInfo( entindex(), &info );

	if ( !cl_customsounds.GetBool() )
		return;

	// Doesn't have a jingle sound
	 if ( !info.customFiles[1] )	
		return;

	char soundhex[ 16 ];
	Q_binarytohex( (byte *)&info.customFiles[1], sizeof( info.customFiles[1] ), soundhex, sizeof( soundhex ) );

	// See if logo has been downloaded.
	char fullsoundname[ 512 ];
	Q_snprintf( fullsoundname, sizeof( fullsoundname ), "sound/temp/%s.wav", soundhex );

	if ( !filesystem->FileExists( fullsoundname ) )
	{
		char custname[ 512 ];
		Q_snprintf( custname, sizeof( custname ), "downloads/%s.dat", soundhex );
		// it may have been downloaded but not copied under materials folder
		if ( !filesystem->FileExists( custname ) )
			return; // not downloaded yet

		// copy from download folder to materials/temp folder
		// this is done since material system can access only materials/*.vtf files

		if ( !engine->CopyFile( custname, fullsoundname) )
			return;
	}

	Q_snprintf( fullsoundname, sizeof( fullsoundname ), "temp/%s.wav", soundhex );

	CLocalPlayerFilter filter;

	EmitSound_t ep;
	ep.m_nChannel = CHAN_VOICE;
	ep.m_pSoundName =  fullsoundname;
	ep.m_flVolume = VOL_NORM;
	ep.m_SoundLevel = SNDLVL_NORM;

	C_BaseEntity::EmitSound( filter, GetSoundSourceIndex(), ep );
}

// Stuff for prediction
void C_BasePlayer::SetSuitUpdate(char *name, int fgroup, int iNoRepeat)
{
	// FIXME:  Do something here?
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BasePlayer::ResetAutoaim( void )
{
#if 0
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = QAngle( 0, 0, 0 );
		engine->CrosshairAngle( edict(), 0, 0 );
	}
#endif
	m_fOnTarget = false;
}

bool C_BasePlayer::ShouldPredict( void )
{
	// Do this before calling into baseclass so prediction data block gets allocated
	if ( IsLocalPlayer() )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Special processing for player simulation
// NOTE: Don't chain to BaseClass!!!!
//-----------------------------------------------------------------------------
void C_BasePlayer::PhysicsSimulate( void )
{
	VPROF( "C_BasePlayer::PhysicsSimulate" );
	// If we've got a moveparent, we must simulate that first.
	CBaseEntity *pMoveParent = GetMoveParent();
	if (pMoveParent)
	{
		pMoveParent->PhysicsSimulate();
	}

	// Make sure not to simulate this guy twice per frame
	if (m_nSimulationTick == gpGlobals->tickcount)
		return;

	m_nSimulationTick = gpGlobals->tickcount;

	if ( g_pLocalPlayer != this )
		return;

	C_CommandContext *ctx = GetCommandContext();
	Assert( ctx );
	Assert( ctx->needsprocessing );
	if ( !ctx->needsprocessing )
		return;

	ctx->needsprocessing = false;

	// Handle FL_FROZEN.
	if(GetFlags() & FL_FROZEN)
	{
		ctx->cmd.forwardmove = 0;
		ctx->cmd.sidemove = 0;
		ctx->cmd.upmove = 0;
		ctx->cmd.buttons = 0;
		ctx->cmd.impulse = 0;
		//VectorCopy ( pl.v_angle, ctx->cmd.viewangles );
	}

	// Run the next command
	prediction->RunCommand( 
		this, 
		&ctx->cmd, 
		MoveHelper() );
}

const QAngle& C_BasePlayer::GetPunchAngle()
{
	return m_Local.m_vecPunchAngle.Get();
}


void C_BasePlayer::SetPunchAngle( const QAngle &angle )
{
	m_Local.m_vecPunchAngle = angle;
}


float C_BasePlayer::GetWaterJumpTime() const
{
	return m_flWaterJumpTime;
}

void C_BasePlayer::SetWaterJumpTime( float flWaterJumpTime )
{
	m_flWaterJumpTime = flWaterJumpTime;
}

float C_BasePlayer::GetSwimSoundTime() const
{
	return m_flSwimSoundTime;
}

void C_BasePlayer::SetSwimSoundTime( float flSwimSoundTime )
{
	m_flSwimSoundTime = flSwimSoundTime;
}


//-----------------------------------------------------------------------------
// Purpose: Return true if this object can be +used by the player
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps )
{
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float C_BasePlayer::GetFOV( void )
{
	float fFOV = m_Local.m_iFOV;

	// Allow our vehicle to override our FOV if it's currently at the default FOV.
	IClientVehicle *pVehicle = GetVehicle();
	if ( pVehicle && ( fFOV == 0 ) )
	{
		pVehicle->GetVehicleFOV( fFOV );
	}

	if ( fFOV == 0 )
	{
		fFOV = default_fov.GetInt();
	}

	//See if we need to lerp the values
	if ( ( fFOV != m_Local.m_iFOVStart ) && ( m_Local.m_flFOVRate > 0.0f ) )
	{
		float deltaTime = (float)( gpGlobals->curtime - m_Local.m_flFOVTime ) / m_Local.m_flFOVRate;

		if ( deltaTime >= 1.0f )
		{
			//If we're past the zoom time, just take the new value and stop lerping
			m_Local.m_iFOVStart = fFOV;
		}
		else
		{
			fFOV = SimpleSplineRemapVal( deltaTime, 0.0f, 1.0f, m_Local.m_iFOVStart, fFOV );
		}
	}

	return fFOV;
}


//-----------------------------------------------------------------------------
// Purpose: Called when the FOV changes from the server-side
// Input  : *pData - 
//			*pStruct - 
//			*pOut - 
//-----------------------------------------------------------------------------
void RecvProxy_FOV( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CPlayerLocalData *pPlayerData = (CPlayerLocalData *) pStruct;

	//Hold onto the old FOV as our starting point
	int iNewFov = pData->m_Value.m_Int;
	if ( pPlayerData->m_iFOV == iNewFov )
		return;

	// Get the true current FOV of the player at this point
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer != NULL )
	{
		pPlayerData->m_iFOVStart = pPlayer->GetFOV();
	}
	else
	{
		// At connect time, it can get in here b/c it's receiving baselines for other players.
		pPlayerData->m_iFOVStart = ( pPlayerData->m_iFOV == 0 ) ? default_fov.GetInt() : pPlayerData->m_iFOV;
	}

	//Get our start time for the zoom
	pPlayerData->m_flFOVTime = gpGlobals->curtime;
	pPlayerData->m_iFOV = iNewFov;
}

//-----------------------------------------------------------------------------
// Purpose: Called when the default FOV changes from the server-side
// Input  : *pData - 
//			*pStruct - 
//			*pOut - 
//-----------------------------------------------------------------------------
void RecvProxy_DefaultFOV( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CPlayerLocalData *pPlayerData = (CPlayerLocalData *) pStruct;

	//Hold onto the old FOV as our starting point
	int iNewFov = pData->m_Value.m_Int;
	
	if ( pPlayerData->m_iDefaultFOV != iNewFov )
	{
		// Don't lerp
		pPlayerData->m_flFOVRate = 0.0f;

		pPlayerData->m_iDefaultFOV = iNewFov;
	}
}

void RecvProxy_LocalVelocityX( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BasePlayer *pPlayer = (C_BasePlayer *) pStruct;

	Assert( pPlayer );

	float flNewVel_x = pData->m_Value.m_Float;

	Vector vecVelocity = pPlayer->GetLocalVelocity();

	if( vecVelocity.x != flNewVel_x )	// Should this use an epsilon check?
	{
		vecVelocity.x = flNewVel_x;
		pPlayer->SetLocalVelocity( vecVelocity );
	}
}

void RecvProxy_LocalVelocityY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BasePlayer *pPlayer = (C_BasePlayer *) pStruct;

	Assert( pPlayer );

	float flNewVel_y = pData->m_Value.m_Float;

	Vector vecVelocity = pPlayer->GetLocalVelocity();

	if( vecVelocity.y != flNewVel_y )
	{
		vecVelocity.y = flNewVel_y;
		pPlayer->SetLocalVelocity( vecVelocity );
	}
}

void RecvProxy_LocalVelocityZ( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BasePlayer *pPlayer = (C_BasePlayer *) pStruct;
	
	Assert( pPlayer );

	float flNewVel_z = pData->m_Value.m_Float;

	Vector vecVelocity = pPlayer->GetLocalVelocity();

	if( vecVelocity.z != flNewVel_z )
	{
		vecVelocity.z = flNewVel_z;
		pPlayer->SetLocalVelocity( vecVelocity );
	}
}
//-----------------------------------------------------------------------------
// Purpose: Remove this player from a vehicle
//-----------------------------------------------------------------------------
void C_BasePlayer::LeaveVehicle( void )
{
	if ( NULL == m_hVehicle.Get() )
		return;

// Let server do this for now
#if 0
	IClientVehicle *pVehicle = GetVehicle();
	Assert( pVehicle );

	int nRole = pVehicle->GetPassengerRole( this );
	Assert( nRole >= 0 );

	SetParent( NULL );

	// Find the first non-blocked exit point:
	Vector vNewPos = GetAbsOrigin();
	QAngle qAngles = GetAbsAngles();
	pVehicle->GetPassengerExitPoint( nRole, &vNewPos, &qAngles );
	OnVehicleEnd( vNewPos );
	SetAbsOrigin( vNewPos );
	SetAbsAngles( qAngles );

	m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
	RemoveEffects( EF_NODRAW );

	SetMoveType( MOVETYPE_WALK );
	SetCollisionGroup( COLLISION_GROUP_PLAYER );

	qAngles[ROLL] = 0;
	SnapEyeAngles( qAngles );

	m_hVehicle = NULL;
	pVehicle->SetPassenger(nRole, NULL);

	Weapon_Switch( m_hLastWeapon );
#endif
}


float C_BasePlayer::GetMinFOV()	const
{
	if ( gpGlobals->maxClients == 1 )
	{
		// Let them do whatever they want, more or less, in single player
		return 5;
	}
	else
	{
		return 5;
	}
}

float C_BasePlayer::GetFinalPredictedTime() const
{
	return ( m_nFinalPredictedTick * TICK_INTERVAL );
}

void C_BasePlayer::NotePredictionError( const Vector &vDelta )
{
	Vector vOldDelta;

	GetPredictionErrorSmoothingVector( vOldDelta );

	// sum all errors within smoothing time
	m_vecPredictionError = vDelta + vOldDelta;

	// remember when last error happened
	m_flPredictionErrorTime = gpGlobals->curtime;
 
	ResetLatched(); 
}

void C_BasePlayer::GetPredictionErrorSmoothingVector( Vector &vOffset )
{
	if ( engine->IsPlayingDemo() || !cl_smooth.GetInt() || !cl_predict.GetBool() )
	{
		vOffset.Init();
		return;
	}

	float errorAmount = ( gpGlobals->curtime - m_flPredictionErrorTime ) / cl_smoothtime.GetFloat();

	if ( errorAmount >= 1.0f )
	{
		vOffset.Init();
		return;
	}
	
	errorAmount = 1.0f - errorAmount;

	vOffset = m_vecPredictionError * errorAmount;
}


IRagdoll* C_BasePlayer::GetRepresentativeRagdoll() const
{
	return m_pRagdoll;
}

