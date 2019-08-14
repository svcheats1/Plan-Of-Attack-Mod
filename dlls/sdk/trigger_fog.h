#ifndef _TRIGGER_FOG_H_
#define _TRIGGER_FOG_H_

#include "triggers.h"
#include "Color.h"

class CTriggerFog : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerFog, CBaseTrigger);
	DECLARE_DATADESC();

public:

	CTriggerFog();
	virtual void Spawn(void);
	virtual void StartTouch(CBaseEntity *pEntity);

private:
	fogparams_t	m_fog;
};

#endif