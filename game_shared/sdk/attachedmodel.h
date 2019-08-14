#ifndef _ATTACHEDMODEL_H_
#define _ATTACHEDMODEL_H_

#ifdef CLIENT_DLL
	#include "c_baseanimating.h"

	#define CAttachedModel C_AttachedModel
#else
	#include "BaseAnimating.h"
#endif

/**
* Class declaration for an entity that attaches to an arbitrary bone
* on the player model
**/
class CAttachedModel : public CBaseAnimating
{
public:
	DECLARE_CLASS(CAttachedModel, CBaseEntity);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

#ifndef CLIENT_DLL

	// destructor
	~CAttachedModel();

	// accessors
	virtual void SpawnOnEntity(CBaseAnimating *pEntity, const char *szModel = NULL, const char *szAttachment = NULL);
	virtual void Detach(const Vector &vecForce, const QAngle &qaAngle, bool bFadeOut);
	virtual bool IsAttached(void) { return m_hEntAttached != NULL; }

	// think functions
	//void WaitTillLand(void);

#else

	// client side inherited methods
	virtual void OnDataChanged(DataUpdateType_t eUpdateType);

#endif

private:
	string_t m_iModel;
	string_t m_iAttachment;
	int m_iAttach;

	CNetworkHandle(CBaseEntity, m_hEntAttached);
};

#endif