#include "cbase.h"
#include "trigger_fog.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( trigger_fog, CTriggerFog );

// data descriptions are amazingly handy. they parse everything for you!
BEGIN_DATADESC( CTriggerFog )
	DEFINE_KEYFIELD( m_fog.colorPrimary,	FIELD_COLOR32,	"fogcolor" ),
	DEFINE_KEYFIELD( m_fog.colorSecondary,	FIELD_COLOR32,	"fogcolor2" ),
	DEFINE_KEYFIELD( m_fog.dirPrimary,		FIELD_VECTOR,	"fogdir" ),
	DEFINE_KEYFIELD( m_fog.enable,			FIELD_BOOLEAN,	"fogenable" ),
	DEFINE_KEYFIELD( m_fog.blend,			FIELD_BOOLEAN,	"fogblend" ),
	DEFINE_KEYFIELD( m_fog.start,			FIELD_FLOAT,	"fogstart" ),
	DEFINE_KEYFIELD( m_fog.end,				FIELD_FLOAT,	"fogend" ),
	DEFINE_KEYFIELD( m_fog.farz,			FIELD_FLOAT,	"farz" ),
END_DATADESC()

CTriggerFog::CTriggerFog()
{
}

void CTriggerFog::Spawn(void)
{
	// jump down
	BaseClass::Spawn();

	// turns on the trigger. apparently needed.
	BaseClass::InitTrigger();
}

void CTriggerFog::StartTouch(CBaseEntity *pEntity)
{
	if (!pEntity->IsPlayer())
		return;

	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>(pEntity);

	if (!pPlayer)
		return;

	pPlayer->m_Local.m_fog = m_fog;
}