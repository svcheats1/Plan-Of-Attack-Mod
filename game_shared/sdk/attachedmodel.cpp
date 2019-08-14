#include "cbase.h"
#include "attachedmodel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_PREDICTION_DATA(CAttachedModel)
	DEFINE_PRED_FIELD(m_hEntAttached, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()

IMPLEMENT_NETWORKCLASS_ALIASED(AttachedModel, DT_AttachedModel);

BEGIN_NETWORK_TABLE(CAttachedModel, DT_AttachedModel)
	#ifndef CLIENT_DLL
		SendPropEHandle(SENDINFO(m_hEntAttached)),
	#else
		RecvPropEHandle(RECVINFO(m_hEntAttached)),
	#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS(attached_model, CAttachedModel);

// data descriptions are amazingly handy. they parse everything for you!
BEGIN_DATADESC(CAttachedModel)
	DEFINE_KEYFIELD(m_iModel, FIELD_STRING,	"model"),
	DEFINE_KEYFIELD(m_iAttachment, FIELD_STRING, "attachment"),
	DEFINE_FIELD(m_hEntAttached, FIELD_EHANDLE),
#ifndef CLIENT_DLL
	//DEFINE_THINKFUNC(WaitTillLand),
#endif
END_DATADESC()

#ifndef CLIENT_DLL

	/**
	* Destructor
	**/
	CAttachedModel::~CAttachedModel()
	{
		CBaseAnimating *pAttached;

		// if we're being destroyed and we're still attached to an entity we need to let the entity know
		if(m_hEntAttached != NULL)
		{
			// see if we can get the animating entity
			pAttached = dynamic_cast<CBaseAnimating *>((CBaseEntity *)m_hEntAttached);
			if(pAttached)
				pAttached->DestroyingAttachedModel(this);
		}
	}

	/**
	* Spawns the item on the given entity
	*
	* @param CBaseAnimating *pEntity The entity to spawn on
	* @param const char *szModel The model to use
	* @param const char *szAttachment The attachment to use
	* @return void
	**/
	void CAttachedModel::SpawnOnEntity(CBaseAnimating *pEntity, const char *szModel, const char *szAttachment)
	{
		Vector vecOrigin;
		QAngle qaAngle;

		// bail if we don't ahve an entity
		if(!pEntity)
			return;

		// see if we have a model and an attachment
		if(!szModel)
			szModel = STRING(m_iModel);
		if(!szAttachment)
			szAttachment = STRING(m_iAttachment);

		// make sure we have both
		if(!szModel || !szAttachment)
			return;

		// make sure i'm rendering normally
		SetRenderColorA( 255 );
		m_nRenderMode = kRenderNormal;
		m_nRenderFX = kRenderFxNone;
		
		// setup the entity info
		m_takedamage = DAMAGE_EVENTS_ONLY;
		SetSolid(SOLID_NONE);
		AddSolidFlags(FSOLID_NOT_STANDABLE);
		AddSolidFlags(FSOLID_NOT_SOLID);
		SetCollisionGroup(COLLISION_GROUP_DEBRIS);

		// set our model
		SetModel(szModel);

		// spawn ourselves
		DispatchSpawn(this);

		// reposition to the attachment
		m_iAttach = pEntity->LookupAttachment(szAttachment);
		if(m_iAttach > -1)
		{
			// grab all the positioning info
			pEntity->GetAttachment(m_iAttach, vecOrigin, qaAngle);

			// setup my position
			SetAbsOrigin(vecOrigin);
			SetAbsAngles(qaAngle);
			SetParent(pEntity, m_iAttach);
		}

		// set the guy to follow
		//SetOwnerEntity(pEntity); // @TRJ - crappy hits cleanup
		FollowEntity(pEntity);

		// no more thinking!
		SetTouch(NULL);
		SetThink(NULL);
		VPhysicsDestroyObject();

		// lastly, hang onto the guy we're attached to
		m_hEntAttached = pEntity;

		// don't draw us if we're attached to a player that isn't on a team
		if(pEntity->IsPlayer() && 
			(pEntity->GetTeamNumber() == TEAM_SPECTATOR || pEntity->GetTeamNumber() == TEAM_UNASSIGNED || !pEntity->IsAlive()))
		{
			AddEffects(EF_NODRAW);
		}
	}

	/**
	* Causes the attached model to release from whatever it is attached to
	*
	* @param const Vector &vecForce The vector on which to send the detached hat
	* @param const QAngle &qaAngle The new angular velocity for the hat
	* @param bool bFadeOut If true we will fade ourselves out
	* @return void
	**/
	void CAttachedModel::Detach(const Vector &vecForce, const QAngle &qaAngle, bool bFadeOut)
	{
		Vector vecOrigin;
		//CBaseAnimating *pEnt;
		IPhysicsObject *pObj;

		// set us up to be freed.  not solid and no velocity
		AddSolidFlags(FSOLID_NOT_SOLID);
		AddSolidFlags(FSOLID_NOT_STANDABLE);
		SetAbsVelocity(vecForce);

		// don't follow anyone
		SetOwnerEntity(NULL);
		FollowEntity(NULL);

		// don't rotate me
		SetAbsAngles(vec3_angle);

		// see if we can get the physics object
		if(VPhysicsInitNormal(SOLID_BBOX, 0, false))
		{
			// grab the object and set it up
			pObj = VPhysicsGetObject(); 
			if(pObj)
			{
				// let it go
				pObj->EnableMotion(true);
				pObj->EnableGravity(true);
			}
			SetMoveType(MOVETYPE_VPHYSICS);
			PhysicsSimulate(); 
		}

		// give me some angular velocity
		SetLocalAngularVelocity(qaAngle);

		// wait until we hit the ground before disappearing
		//SetNextThink(gpGlobals->curtime + 4.0f);
		//SetThink(&CAttachedModel::WaitTillLand);

		// are we fading out?
		if(bFadeOut)
		{
			// set us up for the fade
			SetRenderColorA(255);
			m_nRenderMode = kRenderTransTexture;

			// hang out a sec before the fade
			SetNextThink(gpGlobals->curtime + 20.0f);
			SetThink (&CAttachedModel::SUB_FadeOut);
		}

		// get rid of our attachment
		m_hEntAttached = NULL;
	}

	/**
	* Think function that hangs out until we hit the ground so we can delete ourselves
	*
	* @return void
	**/
	/*void CAttachedModel::WaitTillLand(void)
	{
		// if we don't exist, don't bother
		if(!IsInWorld())
		{
			UTIL_Remove(this);
			return;
		}

		// have we stopped?
		if(GetAbsVelocity() == vec3_origin)
		{
			// stop spinning
			SetLocalAngularVelocity(vec3_angle);

			// set us up for the fade
			SetRenderColorA(255);
			m_nRenderMode = kRenderTransTexture;

			// desolidify us
			if(GetMoveType() != MOVETYPE_VPHYSICS)
				AddSolidFlags(FSOLID_NOT_SOLID);

			// hang out a sec before the fade
			SetNextThink(gpGlobals->curtime + 10.0f);
			SetThink (&CAttachedModel::SUB_FadeOut);
		}
		else
		{
			// we haven't stopped rolling. check again in a minute
			SetNextThink(gpGlobals->curtime + 0.5f);
		}
	}*/

#else

	/**
	* Handles changes to our data.  In this case we want to know when we're created
	* so we can avoid drawing ourselves on the client to which we're attached
	*
	* @param DataUpdateType_t eUpdateType The type of update that took place
	* @return void
	**/
	void CAttachedModel::OnDataChanged(DataUpdateType_t eUpdateType)
	{
		// grab our parent
		C_BaseEntity *pEnt = m_hEntAttached;
		if(!pEnt || !pEnt->IsPlayer())
		{
			RemoveEffects(EF_NODRAW);
			return;
		}

		// if the entity we're attached to is this the local player make the model disappear
		if(pEnt == CBasePlayer::GetLocalPlayer())
			AddEffects(EF_NODRAW);

		// if the player we're attached to is a spectator then make the model disappear
		if(pEnt->GetTeamNumber() == TEAM_SPECTATOR || pEnt->GetTeamNumber() == TEAM_UNASSIGNED || !pEnt->IsAlive())
			AddEffects(EF_NODRAW);
	}

#endif