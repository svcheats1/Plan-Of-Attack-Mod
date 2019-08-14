//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "sdk_player.h"
#include "sdk_gamerules.h"
#include "weapon_sdkbase.h"
#include "predicted_viewmodel.h"
#include "iservervehicle.h"
#include "viewport_panel_names.h"
#include "buyableitem.h"
#include "movevars_shared.h"
#include "in_buttons.h"
#include "obstacle_pushaway.h"
#include "fogcontroller.h"
#include "strategymanager.h"
#include <typeinfo>

extern int gEvilImpulse101;
extern ConVar mp_basearmor;
extern ConVar mp_delayteamswitch;
extern ConVar mp_allowtkpunish;
extern ConVar mp_tkstrikesallowed;
extern ConVar mp_tkbantime;
extern ConVar mp_tkstrikesallowed;

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

					CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
					{
					}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
};

#define THROWGRENADE_COUNTER_BITS 3

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event )
{
	CPVSFilter filter( (const Vector&)pPlayer->EyePosition() );
	
	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( player, CSDKPlayer );
PRECACHE_REGISTER(player);

BEGIN_SEND_TABLE_NOBASE( CSDKPlayer, DT_SDKLocalPlayerExclusive )
	SendPropInt( SENDINFO( m_iShotsFired ), 8, SPROP_UNSIGNED ),
	SendPropInt(SENDINFO(m_iCash)),
	SendPropInt(SENDINFO(m_iTDStrikes)),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE(CSDKPlayer, DT_SDKPlayerSensitive)
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 11 ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 11 ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 2), 11 ),
	SendPropFloat(SENDINFO(m_fXP), 8, SPROP_UNSIGNED & SPROP_ROUNDDOWN, 0),
	SendPropVector(SENDINFO(m_vecEyePosition), -1, SPROP_COORD | SPROP_CHANGES_OFTEN),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST( CSDKPlayer, DT_SDKPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	
	// playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	// Data that only gets sent to the local player.
	SendPropDataTable( "sdklocaldata", 0, &REFERENCE_SEND_TABLE(DT_SDKLocalPlayerExclusive), SendProxy_SendLocalDataTable ),

	// Sensitive position data.. only send if ncessary
	SendPropDataTable("sensdata", 0, &REFERENCE_SEND_TABLE(DT_SDKPlayerSensitive), SendProxy_SendSensitiveDataTable),

	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
	SendPropFloat( SENDINFO( m_flStamina ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iThrowGrenadeCounter ), THROWGRENADE_COUNTER_BITS, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_szName ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( sdk_ragdoll, CSDKRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CSDKRagdoll, DT_SDKRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()


// -------------------------------------------------------------------------------- //

void cc_CreatePredictionError_f()
{
	CBaseEntity *pEnt = CBaseEntity::Instance( 1 );
	pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + Vector( 63, 0, 0 ) );
}

ConCommand cc_CreatePredictionError( "CreatePredictionError", cc_CreatePredictionError_f, "Create a prediction error", FCVAR_CHEAT );


CSDKPlayer::CSDKPlayer()
{
	m_PlayerAnimState = CreatePlayerAnimState( this, this, LEGANIM_9WAY, true );

	UseClientSideAnimation();
	m_angEyeAngles.Init();
	m_vecEyePosition.Init();

	SetViewOffset( SDK_PLAYER_VIEW_OFFSET );

	m_iThrowGrenadeCounter = 0;

	// no skill or xp
	m_pSkill = new CSkillClassNone;
	m_pSkill->SetPlayer(this);
	m_fXP = 0;
	m_iCaps = 0;
	m_iCash = 0;
	m_fLastScale = 0;
	m_iRequestedTeamChange = -1;
	m_flLastInputTime = gpGlobals->curtime;
	m_szNameChange[0] = NULL;

	// no strikes yet
	m_iTDStrikes = 0;

	// haven't picked yet
	m_bHasPickedObjective = false;

	// no hat
	m_pHat = NULL;
}


CSDKPlayer::~CSDKPlayer()
{
	m_PlayerAnimState->Release();

	// kill the skill
	if(m_pSkill)
		delete m_pSkill;
}


CSDKPlayer *CSDKPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CSDKPlayer *pPlayer;

	CSDKPlayer::s_PlayerEdict = ed;
	pPlayer = (CSDKPlayer*)CreateEntityByName( className );

	return pPlayer;
}

int CSDKPlayer::UpdateTransmitState()
{
	return BaseClass::UpdateTransmitState();

	// always send SDK Players
	//return SetTransmitState( FL_EDICT_ALWAYS );
}

int CSDKPlayer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	//return BaseClass::ShouldTransmit(pInfo);

	// always send SDK Players
	return FL_EDICT_ALWAYS;
}

void CSDKPlayer::LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles )
{
	BaseClass::LeaveVehicle( vecExitPoint, vecExitAngles );

	//teleoprt physics shadow too
	Vector newPos = GetAbsOrigin();
	QAngle newAng = GetAbsAngles();

	Teleport( &newPos, &newAng, &vec3_origin );
}

void CSDKPlayer::PreThink(void)
{
	// Riding a vehicle?
	if ( IsInAVehicle() )	
	{
		// make sure we update the client, check for timed damage and update suit even if we are in a vehicle
		UpdateClientData();		
		CheckTimeBasedDamage();

		// Allow the suit to recharge when in the vehicle.
		CheckSuitUpdate();
		
		WaterMove();	
		return;
	}

	SetMaxSpeed(CalcMaxSpeed());

	if (m_nButtons > 0)
		m_flLastInputTime = gpGlobals->curtime;

	BaseClass::PreThink();
}


void CSDKPlayer::PostThink()
{
	BaseClass::PostThink();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );
	
	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();
	m_vecEyePosition = EyePosition();

    m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	CalculateStamina();
}


void CSDKPlayer::Precache()
{
	PrecacheModel( TEAM_A_MODEL );
	PrecacheModel( TEAM_B_MODEL );
	PrecacheModel( TEAM_A_SNIPER_HAT_MODEL );
	PrecacheModel( TEAM_B_SNIPER_HAT_MODEL );
	PrecacheModel( TEAM_A_HW_HAT_MODEL );
	PrecacheModel( TEAM_B_HW_HAT_MODEL );
	PrecacheModel( TEAM_A_SCOUT_HAT_MODEL );
	PrecacheModel( TEAM_B_SCOUT_HAT_MODEL );
	PrecacheModel( TEAM_A_SOLDIER_HAT_MODEL );
	PrecacheModel( TEAM_B_SOLDIER_HAT_MODEL );

	BaseClass::Precache();
}

void CSDKPlayer::Spawn()
{
	GetWorldFogParams( m_Local.m_fog );

	// @NOTE - THESE LINES MUST GO FIRST SO THAT PLAYER'S BOOSTS GET RESET BEFORE THEY BECOME SOLID AND START
	// TOUCHING OUR ENTITIES
	// @TRJ - SKILLCLASS - set the base armor value
	if(m_pSkill)
	{
		m_ArmorValue = mp_basearmor.GetFloat() * m_pSkill->GetArmorStrengthRatio();

		// no boost
		m_pSkill->SetLevelBoost(0);

		m_flStamina = m_pSkill->GetMaxStamina();
	}
	else {
		m_flStamina = DEFAULT_STAMINA;
		m_ArmorValue = mp_basearmor.GetFloat();
	}

	// @NOTE - THESE LINES MUST GO SECOND SO WE CAN STOP PEOPLE FROM SPAWNING!!!
	// are we allowed to spawn?
	CSDKGameRules *pGameRules = dynamic_cast<CSDKGameRules *>(g_pGameRules);
	Assert(pGameRules);
	if(!pGameRules || !pGameRules->CanSpawn(this))
	{
		// just observe, don't spawn
		StartObserverMode(OBS_MODE_ROAMING);
		return;
	}
	// @TRJ - blame me!
	AddEffects(EF_NOINTERP);
	BaseClass::Spawn();

	RemoveSolidFlags(FSOLID_NOT_SOLID);
	
	// setup the model
	DeterminePlayerModel();

	SetMoveType( MOVETYPE_WALK );

	RemoveEffects( EF_NODRAW );
	// @TRJ - don't blame me!
	//RemoveEffects( EF_NOINTERP );

	m_hRagdoll = NULL;

	// are we in freeze time?
	if(pGameRules->IsPlayerFrozen(this))
		SetFreezeState(true);
	else
		SetFreezeState(false);

	m_Local.m_bInSprint = false;

	// let the active weapon know (!) that we are spawning
	if (m_hActiveWeapon) {
		CWeaponSDKBase* pWpn = dynamic_cast<CWeaponSDKBase*>((CBaseEntity*)m_hActiveWeapon);
		if (pWpn)
			pWpn->PlayerSpawn();
	}

	if (strlen(m_szNameChange)) {
		engine->ClientCommand( edict(), "name %s\n", m_szNameChange );
		m_szNameChange[0] = NULL;
	}

	SetContextThink( &CSDKPlayer::PushawayThink, gpGlobals->curtime + 0.05f, "PushawayThink" );
}

void CSDKPlayer::InitialSpawn( void )
{
	// no hat!
	m_pHat = NULL;

	GetWorldFogParams( m_Local.m_fog );

	m_takedamage = DAMAGE_NO;
	pl.deadflag = true;
	m_lifeState = LIFE_DEAD;
	AddEffects( EF_NODRAW );

	BaseClass::InitialSpawn();

	if (GetTeamNumber() == TEAM_A)
		SetModel( TEAM_A_MODEL );
	else
		SetModel( TEAM_B_MODEL );

	// pick a skin to use
	DeterminePlayerSkin();

	// no cash
	m_iCash = 0;

	// we haven't update our skill level yet
	m_bFirstSkillUpdate = true;
	m_bNeedsEquipment = true;

	CSDKGameRules *gameRules = (CSDKGameRules*)g_pGameRules;

	if (gameRules && gameRules->GetCurrentRound()) {
		// send them the current round time
		gameRules->GetCurrentRound()->StartNewClientTimer(this);
		// send them the current buy time
		gameRules->GetCurrentRound()->SendPlayerBuyTime(this);
	}	

	const ConVar *hostname = cvar->FindVar( "hostname" );
	const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

	// open info panel on client showing MOTD:
	KeyValues *data = new KeyValues("data");
	data->SetString( "title", title );		// info panel title
	data->SetString( "type", "1" );			// show userdata from stringtable entry
	data->SetString( "msg",	"motd" );		// use this stringtable entry
	data->SetString( "cmd", "changeteam" );	// exec this command if panel closed

	ShowViewPortPanel( PANEL_INFO, true, data );

	// we haven't received tips yet
	m_bInitialTips = false;
}

void CSDKPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	CBaseEntity *pKiller, *pInflictor;
	CSDKPlayer *pScorer;
	CWeaponSDKBase *pWpn;
	QAngle vecNewAngularVelocity;

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.

	// unfreeze so that we can drop our shit
	SetFreezeState(false);

	// tell the weapon that the player was killed
	pWpn = GetActiveSDKWeapon();
	if (pWpn)
		pWpn->PlayerKilled();

	BaseClass::Event_Killed( info );

	// create the ragdoll
	CreateRagdollEntity();

	// drop my hat
	vecNewAngularVelocity = GetLocalAngularVelocity();
	vecNewAngularVelocity.x = random->RandomFloat (100, 200);
	vecNewAngularVelocity.y = random->RandomFloat (100, 300);
	DropHat(info.GetDamageForce(), vecNewAngularVelocity);

	// who killed us? with what?
	pKiller = info.GetAttacker();
	pInflictor = info.GetInflictor();
	pScorer = ToSDKPlayer(((CSDKGameRules*)g_pGameRules)->GetDeathScorer(pKiller, pInflictor));
	// make sure we kille someone, that we didn't kill ourselves, and that we didn't team killer
	if(pScorer && this != pScorer && GetTeamNumber() != pScorer->GetTeamNumber())
	{
		pWpn = pScorer->GetActiveSDKWeapon();

		// award them for the kill with any modifiers
		pScorer->ModifyXP(((CSDKGameRules *)g_pGameRules)->GetPlayerKillXPReward(pScorer) + 
							(pScorer->GetSkillClass() && pWpn
								? pScorer->GetSkillClass()->GetKillXPModifier(this, pWpn)
								: 0));
	}

	// drop the primary weapon or the secondary in that order
	CWeaponSDKBase *pDroppedWeapon = NULL;
	pDroppedWeapon = GetPrimaryWeapon();

	// did we get it?
	// JD - HACK for RPG to fix dropping a RPG with no ammo - this only happens if you kill yourself
	// with the rocket launcher.
	if(!pDroppedWeapon || (pDroppedWeapon->GetWeaponID() == WEAPON_RPG && pDroppedWeapon->GetAmmo1() == 0))
		pDroppedWeapon = GetSecondaryWeapon();

	// are we dropping with physics?
	if(pDroppedWeapon && VPhysicsGetObject())
	{
		// calculate death force
		Vector forceVector = CalcDamageForceVector( info );

		// drop with some force
		Vector weaponForce = forceVector * VPhysicsGetObject()->GetInvMass();
		if (pDroppedWeapon == m_hActiveWeapon)
			m_hActiveWeapon = NULL;
		Weapon_Drop(pDroppedWeapon, NULL, &weaponForce);
	}
	else if(pDroppedWeapon) {
        // drop normal
		if (pDroppedWeapon == m_hActiveWeapon)
			m_hActiveWeapon = NULL;
		Weapon_Drop(pDroppedWeapon);
	}

	// if we dropped a weapon...and a bunch of other conditions...
	if(pDroppedWeapon && ShouldGib(info) == false && (((info.GetDamageType() & DMG_DISSOLVE) && CanBecomeRagdoll())))
	{
		// how are we dissolving?
		int nDissolveType = ENTITY_DISSOLVE_NORMAL;
		if ( info.GetDamageType() & DMG_SHOCK )
			nDissolveType = ENTITY_DISSOLVE_ELECTRICAL;

		// make it go away
		pDroppedWeapon->Dissolve(NULL, gpGlobals->curtime, false, nDissolveType);
	}

	// allow the player to punish the dude that killed him if he's on the same team
	if(pScorer && pScorer->GetTeamNumber() == GetTeamNumber() && pScorer != this)
		AllowPunishAttacker(pScorer);

	// stick them into death cam mode
	if(pScorer)
		SetObserverTarget(pScorer);
	SetObserverMode(OBS_MODE_DEATHCAM);

	// unless we killed ourselves we lose control of the camera
	if(pScorer != this)
		EnableControl(false);
}

/**
* Allows us to punsih someone who killed us
*
* @param CSDKPlayer *pPlayer The person who attacked us
* @return void
**/
void CSDKPlayer::AllowPunishAttacker(CSDKPlayer *pPlayer)
{
	// are we even using this system?
	if(!mp_allowtkpunish.GetBool())
		return;

	// add this guy to my list
	m_aPunishable.AddToTail(pPlayer->GetUserID());

	// setup the filter
	CSingleUserRecipientFilter filter(this);
	filter.MakeReliable();

	// send the message
	UserMessageBegin(filter, "AllowPunish");
		WRITE_SHORT(pPlayer->GetUserID());
		WRITE_STRING(pPlayer->GetPlayerName());
	MessageEnd();
}

/**
* Forgives a teammate who attacked us
* 
* @param int iPlayer The player who we're forgiving
* @return void
**/
void CSDKPlayer::ForgivePlayer(int iPlayer)
{
	int iPos;

	// are they in my list?
	iPos = m_aPunishable.Find(iPlayer);
	if(iPos == -1)
		return;

	// remove them from the list
	m_aPunishable.Remove(iPos);
}

/**
* Strikes a teammate who attacked us
*
* @param int iPlayer THe player who we're striking
* @return void
**/
void CSDKPlayer::StrikePlayer(int iPlayer)
{
	int iPos;
	CSDKPlayer *pAttacker;

	// are they in my list?
	iPos = m_aPunishable.Find(iPlayer);
	if(iPos == -1)
		return;

	// remove them from the list
	m_aPunishable.Remove(iPos);

	// pull the player
	pAttacker = (CSDKPlayer *)UTIL_PlayerByUserId(iPlayer);
	if(!pAttacker)
		return;

	// give them a strike
	pAttacker->GiveTDStrike(this);
}

/**
* Gives the user a team damage strike
*
* @param CSDKPlayer *pPlayer The player whose giving us a strike
* @return void
**/
void CSDKPlayer::GiveTDStrike(CSDKPlayer *pPlayer)
{
	char szStr[256];

	// increment my strike count
	++m_iTDStrikes;

	// notify them
	Q_snprintf(szStr, sizeof(szStr), "Received strike from %s. %d remaining!", pPlayer->GetPlayerName(), mp_tkstrikesallowed.GetInt() - m_iTDStrikes);
	UTIL_SayText(szStr, this);

	// am i over?
	if(mp_tkstrikesallowed.GetInt() - m_iTDStrikes <= 0)
	{
		// are we banning?
		if(mp_tkbantime.GetInt() >= 0)
		{
			// set the time
			Q_snprintf(szStr, sizeof(szStr), "banid %d %d kick\n", mp_tkbantime.GetInt(),GetUserID());
			engine->ServerCommand(szStr);
		}
		else
		{
			// kick them
			Q_snprintf(szStr, sizeof(szStr), "kickid %d\n", GetUserID());
			engine->ServerCommand(szStr);
		}
	}

}

CSDKRagdoll *CSDKPlayer::CreateRagdollEntity()
{
	// If we already have a ragdoll, don't make another one.
	CSDKRagdoll *pRagdoll = dynamic_cast< CSDKRagdoll* >( m_hRagdoll.Get() );

	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CSDKRagdoll* >( CreateEntityByName( "sdk_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = Vector(0,0,0);
		pRagdoll->m_nSkin = m_nSkin; // @TRJ - make sure the skins are the same
	}

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;

	return pRagdoll;
}

void CSDKPlayer::DoAnimationEvent( PlayerAnimEvent_t event )
{
	if ( event == PLAYERANIMEVENT_THROW_GRENADE )
	{
		// Grenade throwing has to synchronize exactly with the player's grenade weapon going away,
		// and events get delayed a bit, so we let CCSPlayerAnimState pickup the change to this
		// variable.
		m_iThrowGrenadeCounter = (m_iThrowGrenadeCounter+1) % (1<<THROWGRENADE_COUNTER_BITS);
	}
	else
	{
		m_PlayerAnimState->DoAnimationEvent( event );
		TE_PlayerAnimEvent( this, event );	// Send to any clients who can see this guy.
	}
}

CWeaponSDKBase* CSDKPlayer::GetActiveSDKWeapon() const
{
	return dynamic_cast< CWeaponSDKBase* >( GetActiveWeapon() );
}

void CSDKPlayer::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CPredictedViewModel *vm = ( CPredictedViewModel * )CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

void CSDKPlayer::CheatImpulseCommands( int iImpulse )
{
	if ( iImpulse != 101 )
	{
		BaseClass::CheatImpulseCommands( iImpulse );
		return;
	}
	if ( !sv_cheats->GetBool() )
	{
		return;
	}

	gEvilImpulse101 = true;

	EquipSuit();

	// Give weapons
	GiveNamedItem( "weapon_knife" );
	GiveNamedItem( "weapon_deagle" );
	GiveNamedItem( "weapon_mk48" );
	GiveNamedItem( "weapon_grenade" );

	ModifyCash( MAX_CASH );
	ModifyXP( 10.0 );
    
	if ( GetHealth() < 100 )
	{
		TakeHealth( 25, DMG_GENERIC );
	}

	GetPrimaryWeapon()->FillAmmo();
	GetSecondaryWeapon()->FillAmmo();

	gEvilImpulse101		= false;
}

/**
* Handles client commands
*
* @param const char *cmd The command we're getting
* @return bool True if the command was handled
**/
bool CSDKPlayer::ClientCommand(const char *cmd)
{
	// change teams
	if( stricmp( cmd, "changeteam" ) == 0)
	{
		int iTeamNumber;

		// how many args did we get?
		if( engine->Cmd_Argc() != 2)
			return ForceChangeTeamIfNecessary();

		// get the team number
		iTeamNumber = atoi( engine->Cmd_Argv(1) );

		// special teams. 6 is spectator, and switch them right away.
		if (iTeamNumber == 6)
			ChangeTeam( TEAM_SPECTATOR );
		// 5 is auto-assign...
		else if (iTeamNumber == 5) {
			CSDKGameRules *pGameRules = dynamic_cast<CSDKGameRules*>(g_pGameRules);
			Assert(pGameRules);
			if (pGameRules)
				RequestChangeTeam(pGameRules->TeamWithFewestPlayers());
		}
		// make sure the team exists and that we're not already on it
		else if( iTeamNumber < 0 || iTeamNumber > MAX_TEAMS || GetTeamNumber() == iTeamNumber )
			return ForceChangeTeamIfNecessary();
		// otherwise, do a normal switch
		else
			RequestChangeTeam( iTeamNumber );

		return ForceChangeTeamIfNecessary();
	}
	else if(Q_strstr(cmd, "changeskill"))
	{
		// make sure we have more than "changeskill"
		if(engine->Cmd_Argc() == 2)
			// change
			ChangeSkill(engine->Cmd_Argv(1));

		// make sure they actually picked one
		ForceChangeSkill();

		return true;
	}
	else if(Q_strstr(cmd, "buy"))
	{
		// make sure we have more than "buy"
		if(engine->Cmd_Argc() == 2)
			// buy it
			BuyNamedItem(engine->Cmd_Argv(1));

		return true;
	}
	else if(Q_strstr(cmd, "forgive"))
	{
		// make sure we have a player
		if(engine->Cmd_Argc() == 2)
			ForgivePlayer(atoi(engine->Cmd_Argv(1)));

		return true;
	}
	else if(Q_strstr(cmd, "strike"))
	{
		// make sure we have a player
		if(engine->Cmd_Argc() == 2)
			StrikePlayer(atoi(engine->Cmd_Argv(1)));

		return true;
	}
	else if(Q_strstr(cmd, "drop"))
	{
		CWeaponSDKBase *pWpn;
		char szStr[256];

		// do we have some more args?
		if(engine->Cmd_Argc() == 2)
		{
			// copy the string
			Q_strcpy(szStr, engine->Cmd_Argv(1));
			Q_strlower(szStr);

			// primary?
			if(Q_strstr(szStr, "primary"))
				pWpn = GetPrimaryWeapon();
			// secondary?
			else if(Q_strstr(szStr, "secondary"))
				pWpn = GetSecondaryWeapon();
			// accessory?
			else if(Q_strstr(szStr, "accessory"))
				pWpn = dynamic_cast<CWeaponSDKBase*>(GetWeaponBySlot(3));
			else
				pWpn = NULL;
		}
		// leftovers
		else
			pWpn = dynamic_cast<CWeaponSDKBase*>(GetActiveWeapon());

		// if we have a droppable weapon, get rid of it
		if(pWpn && pWpn->IsDroppable())
			Weapon_Drop(pWpn);

		return true;
	}

	return BaseClass::ClientCommand(cmd);
}

/** 
 * Acts as a helper for ClientCommand in order to re-display the menu if there is an error
 * and they still must change their team.
 */
bool CSDKPlayer::ForceChangeTeamIfNecessary()
{
	if(MustChangeTeam())
		ForceChangeTeam();
	
	return true;
}

bool CSDKPlayer::MustChangeTeam()
{
	return (GetTeamNumber() < 0);
}

/**
* Returns an entity to spawn for the player to spawn at
* GOTO = BAD!
*
* @void CBaseEntity * The entity to spawn at
**/
CBaseEntity *CSDKPlayer::EntSelectSpawnPoint(void)
{
	CBaseEntity *pSpot;

	// find somewhere to spawn
	pSpot = ((CSDKGameRules *)g_pGameRules)->GetValidSpawnPoint(this);

	// if we can't find one here, jump down
	if(!pSpot)
		pSpot = BaseClass::EntSelectSpawnPoint();

	// did we get one?
	if(!pSpot)
	{
		Warning( "PutClientInServer: no available spawn points on level\n");
		return CBaseEntity::Instance( INDEXENT( 0 ) );
	}

	// record the spawn point as the last on and move on
	//s_pLastSpawnPoint = pSpot;
	return pSpot;
}

/**
* Determines the name of the spawn point for this player
*
* @param char *szName Some memory to fill in with a name
* @param int iLen Size of the buffer
* @return bool
**/
/*bool CSDKPlayer::GetSpawnPointName(char szName[], int iLen)
{
	// am i on a team?
	if(!GetTeam())
		return false;

	// complete the name
	g_pGameRules->GetSpawnPointName(szName, iLen, GetTeamNumber());

	return true;
}*/

/**
 * Sends a message to the client to force the player to change their team.
 */
void CSDKPlayer::ForceChangeTeam( void )
{
	CSingleUserRecipientFilter filter ( this );
	filter.MakeReliable();

	// there's no data sent with the message.
	UserMessageBegin( filter, "ChangeTeam" );
	MessageEnd();
}

/**
* Forces the player to change skills
*
* @return void
**/
void CSDKPlayer::ForceChangeSkill(void)
{
	// do we have a skill yet?
	if(!m_pSkill || m_pSkill->GetClassIndex() == NONE_CLASS_INDEX 
			|| !((CSDKGameRules *)g_pGameRules)->CanJoinClass(this, m_pSkill->GetInternalClassName()))
	{
		// setup the filter to go to this player only
		CSingleUserRecipientFilter filter(this);
		filter.MakeReliable();

		// send it on
		UserMessageBegin(filter, "ChangeSkill");
		MessageEnd();
	}
}

bool CSDKPlayer::RequestChangeTeam(int iTeamNum)
{
	if (iTeamNum < 0 || iTeamNum > MAX_TEAMS) {
		UTIL_SayText("That is not a valid team\n", this);
		return false;
	}

	if (mp_delayteamswitch.GetBool() && ((CSDKGameRules *)g_pGameRules)->CanDelayTeamSwitch() && !MustChangeTeam() && 
		GetGlobalTeam(TEAM_A) && GetGlobalTeam(TEAM_B) &&
		(GetGlobalTeam(TEAM_A)->GetNumPlayers() + GetGlobalTeam(TEAM_B)->GetNumPlayers() > 0))
	{
		// can we join this team?
		if (((CSDKGameRules*)g_pGameRules)->CanJoinTeam(this, iTeamNum)) {
			// add our request to join it
			m_iRequestedTeamChange = iTeamNum;
			UTIL_SayText("You will change teams at the end of the current round\n", this);
			return true;
		}

		//UTIL_SayText("Can't change teams.  Exceeds player difference limit.\n", this);
		SendTeamDifference();
		return false;
	}
	// delay switch is off (or playing suppression), or I must change teams right away
	else {
		if (((CSDKGameRules*)g_pGameRules)->CanJoinTeam(this, iTeamNum)) {
			ChangeTeam(iTeamNum);
			return true;
		}
		//UTIL_SayText("Can't change teams.  Exceeds player difference limit.\n", this);
		SendTeamDifference();
		return false;
	}
}

/**
* Informs the team menu that the player can't switch
*
* @return void
**/
void CSDKPlayer::SendTeamDifference(void)
{
	// setup the filter
	CSingleUserRecipientFilter sFilter(this);
	sFilter.MakeReliable();

	// tell the menu that we can't switch
	UserMessageBegin(sFilter, "TeamDifference");
	MessageEnd();
}

void CSDKPlayer::ProcessChangeTeamRequest()
{
	if (m_iRequestedTeamChange > 0)
	{
		ChangeTeam(m_iRequestedTeamChange, true);
		m_iRequestedTeamChange = 0;
	}
}

/**
* Changes the team of this player
*
* @param int iTeamNum The team to change to
* @return void
**/
void CSDKPlayer::ChangeTeam( int iTeamNum, bool bDontKillOverride /*= false*/ )
{
	int iOldTeamNum;
	bool bFirstPlayer = false;
	CSDKGameRules *pGameRules;

	// don't bother if we're rejoining the same team
	if(iTeamNum == GetTeamNumber())
		return;

	// get the new team
	CSDKTeam *pTeam = (CSDKTeam *)GetGlobalTeam(iTeamNum);

	// did we get one?
	if(!pTeam) {
		BaseClass::ChangeTeam(iTeamNum);
		return;
	}

	// figure out a new skin
	DeterminePlayerSkin();

	// if i'm joining spectators then make my hat invisible
	if((iTeamNum == TEAM_SPECTATOR || iTeamNum == TEAM_UNASSIGNED || !IsAlive()) && m_pHat)
		m_pHat->AddEffects(EF_NODRAW);
	else if(m_pHat)
		m_pHat->RemoveEffects(EF_NODRAW);
		
	// grab the game rules and see if this is the first real player
	pGameRules = dynamic_cast<CSDKGameRules *>(g_pGameRules);
	if(pGameRules)
		bFirstPlayer = !pGameRules->TeamPlayersExist() && pTeam->IsPlayingTeam();
	else
		Assert(0);

	// store what we are now
	iOldTeamNum = GetTeamNumber();

	// are we on a real team? suicide before we switch. (Important for MOD and objectives)
	if ( iOldTeamNum > TEAM_SPECTATOR && !bDontKillOverride )
		CommitSuicide(true);

	// change to the new team
	BaseClass::ChangeTeam( iTeamNum );

	// let the game rules know
	pGameRules->PlayerChangedTeam(this);

	// if we're the first real player we need to restart
	if(bFirstPlayer)
	{
		// restart
		// @HACK - i am 733T!!  god help us
		CSDKGameRules::s_bFirstPlayer = true;
		pGameRules->RestartGame();
		CSDKGameRules::s_bFirstPlayer = false;
	}
	// otherwise, spawn normally
	else
	{
		// in the round.
		if ( (iOldTeamNum <= TEAM_SPECTATOR && iTeamNum > TEAM_SPECTATOR) ) {
			Spawn();	// it wont actually spawn them if they're not supossed to.
		
			// aka: did we spawn?
			if(IsAlive()) {
				CSDKGameRules *pRules = dynamic_cast<CSDKGameRules*>(g_pGameRules);
				if (pRules && pRules->GetCurrentRound() &&
					!pRules->GetCurrentRound()->IsFirstRound() &&
					(GetTeam()->IsOffensive() && !pRules->GetCurrentRound()->InOffensiveFreezeTime())
					|| (!GetTeam()->IsOffensive() && !pRules->GetCurrentRound()->InDefensiveChooseTime()))
				{
					GetStrategyManager()->SendStrategy(this);
				}
			}
		}

		// if the objectives have been established tell the player
		// if they aren't established this is the first player and they will get them in a second
		if(GET_OBJ_MGR()->AreObjectivesEstablished())
			GET_OBJ_MGR()->UpdatePlayer(this);
	}

	// are we joining a real team from unassigned or spectator?
	if(iOldTeamNum <= TEAM_SPECTATOR && iTeamNum > TEAM_SPECTATOR)
	{
		// reset, reequip and change skills
		Reset();
		if(g_pGameRules)
			((CSDKGameRules *)g_pGameRules)->PlayerSpawn(this);
		ForceChangeSkill();
	}
	else if (iTeamNum == TEAM_SPECTATOR)
	{
		StartObserverMode( m_iObserverLastMode );
	}

	// if they're the first player we need to send them some tips.  as soon as they spawn
	// they will reset the list in hud display message.  everyone else will be processed
	// normally
	if((bFirstPlayer || !m_bInitialTips) && (iTeamNum == TEAM_A || iTeamNum == TEAM_B))
	{
		// send them
		UTIL_SendTip("WinTip", this);
		//UTIL_SendTip("ExperienceTip", this);

		// we've seen the tips
		m_bInitialTips = true;
	}
}

/**
* Freezes or unfreezes the player
*
* @param bool bFreeze True if we should freeze the player
* @return void
**/
void CSDKPlayer::SetFreezeState(bool bFreeze)
{
	// abort if i'm dead
	if(IsDead())
		return;

	// do we need to send a message to the client?
	/*if((bFreeze && !(m_afPhysicsFlags & PFLAG_OBSERVER)) || 
			(!bFreeze && (m_afPhysicsFlags & PFLAG_OBSERVER)))
	{*/
		// tell the client
		CSingleUserRecipientFilter filter(this);
		filter.MakeReliable();

		// create the message and send
		UserMessageBegin(filter, "SetFreezeState");
			WRITE_BYTE(bFreeze);
		MessageEnd();
	//}

	// this takes care of the freezing on the server
	// freezing or unfreezing?
	if(bFreeze/* && !(m_afPhysicsFlags & PFLAG_OBSERVER)*/)
	{
		// no control
		SetMoveType(MOVETYPE_NONE);
		//m_afPhysicsFlags |= PFLAG_OBSERVER;
		EnableControl(false);
		AddFlag(FL_FROZEN_VIEW);
	}
	else if(!bFreeze/* && (m_afPhysicsFlags & PFLAG_OBSERVER)*/)
	{
		// if we didn't have control, send them a message
		/*UTIL_SayText("Go! Go! Go!\n", this);*/

		// lots o' control
		SetMoveType(MOVETYPE_WALK);
		//m_afPhysicsFlags &= ~PFLAG_OBSERVER;
		EnableControl(true);
		RemoveFlag(FL_FROZEN_VIEW);
	}
}

/**
 * Overwrite the suicide function to undo 5 second suicide timer.
 * @See CBasePlayer::CommitSuicide()
 */
void CSDKPlayer::CommitSuicide(bool bForce/* = false*/)
{
	// ask the game rules
	if(!bForce && !((CSDKGameRules *)g_pGameRules)->CanCommitSuicide(this))
	{
		// too bad!
		if(RandomInt(0, 10) == 10)
			UTIL_SayText("Why would you want to do that?", this);
		else
			UTIL_SayText("Suicide not permitted in this game mode!", this);
		return;
	}

	BaseClass::CommitSuicide();

	m_fNextSuicideTime = gpGlobals->curtime;
}

/**
* Returns the players current cash
*
* @return int
**/
int CSDKPlayer::GetCash(void)
{
	return m_iCash;
}

/**
* Modifies the cash value
*
* @param int iCash The amount to change by
* @return void
**/
void CSDKPlayer::ModifyCash(int iCash)
{
	IGameEvent *pEvent;

	// modify
	m_iCash += iCash;

	// check the bounds
	if(m_iCash > MAX_CASH)
		m_iCash = MAX_CASH;
	else if(m_iCash < 0)
		m_iCash = 0;

	// send out the local event
	if(iCash != 0)
	{
		// create the event
		pEvent = gameeventmanager->CreateEvent("money_changed");
		if(pEvent)
		{
			// set all the info
			pEvent->SetInt("userid", GetUserID());
			pEvent->SetInt("amount", iCash);

			// send it out
			gameeventmanager->FireEvent(pEvent, true);
		}
	}
}

/**
* 
*/
void CSDKPlayer::ModifyXP(float fXP, bool bIgnoreFirstRound/* = false*/)
{
	// just make sure it isn't the first round
	CSDKGameRules *pGameRules = dynamic_cast<CSDKGameRules *>(g_pGameRules);
	if(!pGameRules || !pGameRules->GetCurrentRound() || (pGameRules->GetCurrentRound()->IsFirstRound() && !bIgnoreFirstRound))
		return;

	int iOldLevel = m_pSkill->GetSkillLevel();

	m_fXP += fXP + GetXPHandicapModifier(fXP);

	if (m_fXP > MAX_XP)
		m_fXP = MAX_XP;
	else if (m_fXP < pGameRules->MinXPForGame())
		m_fXP = pGameRules->MinXPForGame();

	int iNewLevel = m_pSkill->GetSkillLevel();

	if(iOldLevel < iNewLevel)
	{
		// only update if our skill level has changed
		UpdateWithSkillModifiers();

        IGameEvent *event = gameeventmanager->CreateEvent( "player_levelup" );
		if (event) {
			event->SetInt("userid", GetUserID() );
			event->SetInt("level", iNewLevel);
			event->SetString("skill", m_pSkill->GetInternalClassName());
			gameeventmanager->FireEvent( event, true );
		}
	}
}

/**
* Awards the player at the end of a round ONLY
* 
* @param int iAward Amount to give to the player so far
* @return void
**/
void CSDKPlayer::AwardPlayer(int iAward, float iXP)
{
	// modify our cash
	ModifyCash(iAward + (!IsDead() ? PLAYER_ALIVE_CASH_AWARD : 0));
	ModifyXP(iXP + (!IsDead() ? PLAYER_ALIVE_XP_AWARD : 0));
}

/**
* Calculates the handicap modifier for the amount of experience 
* the player is receiving
*
* @param float fXP The experience the player is receiving
* @return float
**/
float CSDKPlayer::GetXPHandicapModifier(float fXP)
{
	float fIndex, fRatio, fResult;
	CSDKTeam *pTeam;

	// pull their team
	pTeam = GetGlobalSDKTeam(GetTeamNumber());
	if(!pTeam || !pTeam->IsPlayingTeam())
		return 0;

	// calculate the current xp index
	fIndex = pTeam->GetXP() + (pTeam->GetAvgIndivXP() / 2.0);

	// don't do handicaps at very low XP levels.. causes issues
	if (fIndex < 15)
		return 0;

	// play catch up
	fRatio = (fIndex - m_fXP) / fIndex;
	fResult = fXP * fRatio;

	// we will be less than the index after the modification?
	if((m_fXP + fResult) < fIndex)
		return fResult;

	return 0;
}

extern void FreeContainingEntity(edict_t *ed);

/**
* Gives the named item to the player if they can afford it
*
* @param const char *szName Name of the item to buy
* @return void
**/
void CSDKPlayer::BuyNamedItem(const char *szName)
{
	EHANDLE pent;
	IBuyableItem *pItem;
	CBaseEntity *pEntity;
	CSDKGameRules *pGameRules;

	// pull gamerules so we can check buy time
	pGameRules = dynamic_cast<CSDKGameRules *>(g_pGameRules);
	Assert(pGameRules);
	
	// did we get it?
	if(!pGameRules || !pGameRules->InBuyTime())
	{
		// tell the client
		ClientPrint( this, HUD_PRINTCONSOLE, "#BuyTimeExpired");
		return;
	}

	// create the item
	pent = CreateEntityByName(szName);
	if(pent == NULL)
	{
		Assert(0);
		return;
	}

	// convert it to an entity so we can precache
	// precaching will load the data file and get the price info, etc
	pEntity = (CBaseEntity *)pent;
	Assert(pEntity);

	// did we get it?
	if(!pEntity)
		return;

	// precache
	pEntity->Precache();

	// get the buyable version
	pItem = dynamic_cast<IBuyableItem *>((CBaseEntity *)pent);
	Assert(pItem);

	// did we get it?
	if(!pItem)
		return;

	// can we afford it?
	if(GetCash() >= pItem->GetPrice() && CanHavePlayerItem(pEntity, false) && 
		(pItem->GetBuyableTeam() == BI_TEAM_BOTH || pItem->GetBuyableTeam() == GetTeamNumber()))
	{
		// if this is a weapon, drop a weapon if we have one.
		if (pEntity->HasWeaponExclusion()) {
			CWeaponSDKBase *pWpn = (CWeaponSDKBase *) pEntity;
			if (pWpn->IsPrimaryWeapon())
				DropPrimaryWeapon();
			else if (pWpn->IsSecondaryWeapon())
				DropSecondaryWeapon();
			else if (pWpn->IsMeleeWeapon())
				DropMeleeWeapon();
		}

		// change our cash
		ModifyCash(-1 * pItem->GetPrice());

		// set the spawn stuff
		pEntity->SetLocalOrigin(GetLocalOrigin());
		pEntity->AddSpawnFlags(SF_NORESPAWN);

		// spawn
		DispatchSpawn(pEntity);

		// was it created in world?
		if(pEntity != NULL && !(pEntity->IsMarkedForDeletion())) 
			// give it
			pEntity->Touch(this);

		CWeaponSDKBase *pWpn = dynamic_cast<CWeaponSDKBase*>(pEntity);
		if (pWpn)
			pWpn->FillAmmo();
	}
	else
	{
		if (GetCash() < pItem->GetPrice())
			ClientPrint( this, HUD_PRINTCONSOLE, "#Not_Enough_Cash");
		else if(pItem->GetBuyableTeam() != BI_TEAM_BOTH && pItem->GetBuyableTeam() != GetTeamNumber())
			ClientPrint(this, HUD_PRINTCONSOLE, "#Incorrect_Team");
		else
			ClientPrint( this, HUD_PRINTCONSOLE, "#Cannot_Carry_More");

		// destroy the item
		FreeContainingEntity(pEntity->edict());
	}
}

/**
* Gives the player an experience boost for doing something with the objective
*
* @param int iAmount The amount to boost by
* @return void
**/
void CSDKPlayer::Boost(int iAmount)
{
	// have we selected a skill?
	if(m_pSkill)
		// set the boost
		m_pSkill->IncrementLevelBoost(iAmount);
}

/**
* Updates the player's stats to reflect a change in skill level
* 
* @return void
**/
void CSDKPlayer::UpdateWithSkillModifiers(void)
{
	// do we need to update any of the values that get set on spawn?
	if(m_bFirstSkillUpdate && m_pSkill && m_pSkill->GetClassIndex() != NONE_CLASS_INDEX)
	{
		// max out the armor value
		m_ArmorValue = mp_basearmor.GetFloat() * m_pSkill->GetArmorStrengthRatio();

		// all set
		m_bFirstSkillUpdate = false;
	}
	
	// cut the armor value off at their skill level
	if(m_pSkill && m_ArmorValue > (mp_basearmor.GetFloat() * m_pSkill->GetArmorStrengthRatio()))
		m_ArmorValue = mp_basearmor.GetFloat() * m_pSkill->GetArmorStrengthRatio();
	else if(!m_pSkill && m_ArmorValue > mp_basearmor.GetFloat())
		m_ArmorValue = mp_basearmor.GetFloat();

	// setup the event
	IGameEvent *pEvent = gameeventmanager->CreateEvent("skill_changed");
	if (pEvent) {
		pEvent->SetInt("userid", GetUserID());
		pEvent->SetInt("level", m_pSkill->GetSkillLevel(true));
		pEvent->SetInt("boost", m_pSkill->GetLevelBoost());
		pEvent->SetFloat("xp", GetXP());
		pEvent->SetString("class", m_pSkill->GetInternalClassName());

		// send it out
		gameeventmanager->FireEvent(pEvent);
	}
}

/**
* Changes the skill of the player
* 
* @param const char *szName The name of the skill to change to
* @return void
**/
void CSDKPlayer::ChangeSkill(const char *szName)
{
	CSkillClass *pSkill;
	int iBoost = 0;

	// don't create a new one if they want the same one
	if(m_pSkill && FStrEq(szName, m_pSkill->GetInternalClassName()))
		return;

	// make sure we can pick that class
	if(!((CSDKGameRules *)g_pGameRules)->CanJoinClass(this, szName))
	{
		UTIL_SayText("You cannot choose that class!", this);
		return;
	}

	// see if we're at our limit for this skill type
	if(((CSDKGameRules *)g_pGameRules)->AtMaxForSkill(this, szName))
	{
		UTIL_SayText("That class is full!", this);
		return;
	}

	// get the new skill
	pSkill = CSkillClass::CreateSkillByName(szName);

	// did we get it?
	if(!pSkill)
		return;

	// make sure it's a real skill
	if(pSkill->GetClassIndex() == NONE_CLASS_INDEX)
	{
		// destroy and bail
		delete(pSkill);
		return;
	}

	// do we have one already?
	if(m_pSkill)
	{
		// copy the boost
		iBoost = m_pSkill->GetLevelBoost();

		delete(m_pSkill);
	}

	// set it
	m_pSkill = pSkill;
	m_pSkill->SetPlayer(this);
	m_pSkill->SetLevelBoost(iBoost);

	// reset my XP
	ModifyXP(-1 * MAX_XP);

	// set the player model and the skin
	DeterminePlayerSkin();
	DeterminePlayerModel();

	// update with the new info
	UpdateWithSkillModifiers();

	// let game rules know
	((CSDKGameRules *)g_pGameRules)->PlayerChangedSkill(this);
}

/**
* Picks a skin to use for the player at random
*
* @return void
**/
void CSDKPlayer::DeterminePlayerSkin(void)
{
	// pick a skin
	m_nSkin = RandomInt(0, 3);
}

/**
* Determines the model to use for the player
*
* @return void
**/
void CSDKPlayer::DeterminePlayerModel(void)
{
	// check our class
	if((GetTeamNumber() == TEAM_A || GetTeamNumber() == TEAM_B)	&& IsAlive())
	{
		// if i have a hat, kill it
		if(m_pHat)
			UTIL_Remove(m_pHat);

		// gimme a new hat
		m_pHat = CREATE_ENTITY(CAttachedModel, "attached_model");
		if(m_pSkill && m_pSkill->GetClassIndex() != NONE_CLASS_INDEX)
			m_pHat->SpawnOnEntity(this, m_pSkill->GetHatModelForTeam(GetTeamNumber()), HAT_ATTACHMENT);
		else
			m_pHat->SpawnOnEntity(this, GetTeamNumber() == TEAM_A ? TEAM_A_SOLDIER_HAT_MODEL : TEAM_B_SOLDIER_HAT_MODEL, HAT_ATTACHMENT);

		// make sure we're drawing it
		m_pHat->RemoveEffects(EF_NODRAW);
	}
	// otherwise, make sure we're not drawing the hat
	else if(m_pHat)
		m_pHat->AddEffects(EF_NODRAW);

	// kill my bone caches so that everything updates
	//RecreateBoneCache();
	//if(m_pHat)
	//	m_pHat->RecreateBoneCache();

	// set it to our team model
	if(GetTeamNumber() == TEAM_A)
		SetModel(TEAM_A_MODEL);
	else if(GetTeamNumber() == TEAM_B)
		SetModel(TEAM_B_MODEL);
}

/**
* Hook that allows us to change the volume of step sounds base
* on skill class and skill level
*
* @param Vector &vecOrigin The spot where the sound started
* @param surfacedata_t *pSurface The surface we're standing on
* @param float fVol The volume to play the sound at
* @param bool bForce ?
* @return void
**/
void CSDKPlayer::PlayStepSound(Vector &vecOrigin, surfacedata_t *pSurface, float fVol, bool bForce)
{
	// do we have a skill class?
	if(m_pSkill)
	{
		// add the noise ratio
		fVol *= m_pSkill->GetNoiseLevelRatio();

		// cap it at the max
		if(fVol > 1.0)
			fVol = 1.0;
	}


	// call the base class
	BaseClass::PlayStepSound(vecOrigin, pSurface, fVol, bForce);
}

/**
 * Overriding the implementation of CBaseCombatCharacter to change how ammo works - basically
 * don't give them any ammo! Haha!
 * Most of this is copied from that function.
 */
void CSDKPlayer::Weapon_Equip( CBaseCombatWeapon *pBaseWeapon )
{
	CWeaponSDKBase *pWeapon = dynamic_cast<CWeaponSDKBase*>(pBaseWeapon);

	// This is not one of our weapons.
	if (!pWeapon) {
		// Shouldn't be the case...
		Assert(pWeapon);
		// Pass to the base class.
		BaseClass::Weapon_Equip(pBaseWeapon);
		// Get out of here.
		return;
	}

	// Add the weapon to my weapon inventory
	for (int i=0;i<MAX_WEAPONS;i++) 
	{
		if (!m_hMyWeapons[i]) 
		{
			m_hMyWeapons.Set( i, pWeapon );
			break;
		}
	}

	// Weapon is now on my team
	pWeapon->ChangeTeam( GetTeamNumber() );

	// Equip me.
	pWeapon->Equip( this );
	
	/*
	WeaponProficiency_t proficiency;
	proficiency = CalcWeaponProficiency( pWeapon );

	if( weapon_showproficiency.GetBool() != 0 )
	{
		Msg("%s equipped with %s, proficiency is %s\n", GetClassname(), pWeapon->GetClassname(), GetWeaponProficiencyName( proficiency ) );
	}

	SetCurrentWeaponProficiency( proficiency );
	*/
	
	// Pass the lighting origin over to the weapon if we have one
	pWeapon->SetLightingOrigin( GetLightingOrigin() );

	// should we switch to this item?
	if ( g_pGameRules->FShouldSwitchWeapon( this, pWeapon ) )
	{
		Weapon_Switch( pWeapon );
	}
}

void CSDKPlayer::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget /* = NULL */, const Vector *pVelocity /* = NULL */ )
{
	CWeaponSDKBase *pWpn = dynamic_cast<CWeaponSDKBase*>(pWeapon);
	if (pWpn && !pWpn->DropSpecial()) {
		BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );
	}
	else if (!pWpn) {
		BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );
	}
}

/**
* Gives the item of the specifed name to the player
*
* @param const char *szName Name of the entity to create and give
* @param int iSubType ?
* @return CBaseEntity *
**/
CBaseEntity *CSDKPlayer::GiveNamedItem(const char *szName, int iSubType /* = 0 */)
{
	CBaseEntity *pEntity;
	CWeaponSDKBase *pWpn;

	// give it in the base class
	pEntity = BaseClass::GiveNamedItem(szName, iSubType);

	// do we have an sdk weapon?
	pWpn = dynamic_cast<CWeaponSDKBase *>(pEntity);
	if(pWpn)
		pWpn->FillAmmo();

	return pEntity;
}

/**
* Resets the player so they are ready for the start of a game
*
* @return void
**/
void CSDKPlayer::Reset(void)
{
	BaseClass::Reset();

	// take all their stuff
	RemoveAllItems(false);

	// no cash, no xp
	ModifyCash(-1 * MAX_CASH);
	ModifyXP(-1 * MAX_XP);
	m_iCaps = 0;

	// update with the new info
	UpdateWithSkillModifiers();
}

void CSDKPlayer::DropPrimaryWeapon()
{
	Weapon_Drop( GetPrimaryWeapon() );
}

void CSDKPlayer::DropSecondaryWeapon()
{
	Weapon_Drop( GetSecondaryWeapon() );
}

void CSDKPlayer::DropMeleeWeapon()
{
	Weapon_Drop( GetMeleeWeapon() );
}

/**
* Determines who we should send our data to based on whether or not
* they are on my team and whether or not it is the first round
*
* @return NULL on failure (don't send), or the data on success
**/
void* SendProxy_SendSensitiveDataTable(const SendProp *pProp, 
									   const void *pStruct,			// who we're sending, i.e. this
									   const void *pVarData,		// what we're sending, i.e. this->...
									   CSendProxyRecipients *pRecipients,	// who we're sending to
									   int objectID)				// edict-ish of this ?
{
	CBaseEntity *pEntity;

	// if we're in the first round, send it to everyone
	if(g_pGameRules && ((CSDKGameRules *)g_pGameRules)->IsFirstRound())
		return (void *)pVarData;

	// get the entity
	pEntity = (CBaseEntity*)pStruct;

	if(pEntity)
	{
		// clear all the recipients
		pRecipients->ClearAllRecipients();

		// add the local client
		pRecipients->SetRecipient(objectID - 1);

		// if we can observe everyone, then send the data to dead people
		if (((CSDKGameRules *)g_pGameRules)->GetForceCam() == OBS_ALLOW_ALL) {
			CBasePlayer *pPlayer;
			for(int i = 1; i <= gpGlobals->maxClients; ++i) {
				pPlayer = UTIL_PlayerByIndex(i);
				if (pPlayer && !pPlayer->IsAlive())
					pRecipients->SetRecipient(i - 1);
			}
		}

		// pull their team
		CTeam *pTeam = pEntity->GetTeam();
		if(pTeam)
		{
			// add the team as the recipients
			for(int i=0; i < pTeam->GetNumPlayers(); i++)
				pRecipients->SetRecipient(pTeam->GetPlayer( i )->GetClientIndex());
		}

		// this code is taken roughly from the CPVSFilter code
		CBitVec< ABSOLUTE_PLAYER_LIMIT > playerbits;
		engine->Message_DetermineMulticastRecipients( false, pEntity->EyePosition(), playerbits );
		// add everyone who was in PVS
		int index = playerbits.FindNextSetBit( 0 );
		while ( index > -1 )
		{
			pRecipients->SetRecipient( index );
			index = playerbits.FindNextSetBit( index + 1 );
		}

		// finally, send back the data
		return (void*)pVarData;
	}

	// no data, don't send
	return NULL;
}

void CSDKPlayer::PushawayThink()
{
	// Push physics props out of our way.
	PerformObstaclePushaway( this );
	SetNextThink( gpGlobals->curtime + 0.05f, "PushawayThink" );
}

/**
* Awards a capture to a player
* 
* @param int iObjective The objective we captured
* @return void
**/
void CSDKPlayer::AwardCap(int iObjective)
{
	m_iCaps++;

	// send an event out so stats can grab the info
	IGameEvent *event = gameeventmanager->CreateEvent("player_captured");
	if(event)
	{
		event->SetInt("userid", GetUserID());
		event->SetInt("objective", iObjective);
		gameeventmanager->FireEvent(event, true);
	}
}

/**
* Causes the player to drop his hat, if he has one
*
* @param const Vector &vecForce The vector to use to throw the hat
* @param const QAngle &qaAngle The new angular velocity to use for the hat
* @return void
**/
void CSDKPlayer::DropHat(const Vector &vecForce, const QAngle &qaAngle)
{
	// see if i have one
	if(m_pHat)
	{
		// detach from ourselves
		m_pHat->Detach(vecForce, qaAngle, true);
		m_pHat = NULL;
	}
}

/**
* Alerts this entity that their attachment is about to be destroyed
*
* @param CBaseEntity *pAttached The guy that's attached to us
* @return void
**/
void CSDKPlayer::DestroyingAttachedModel(CBaseEntity *pAttached)
{
	// see if it's our hat
	if(pAttached == m_pHat)
		m_pHat = NULL;
}