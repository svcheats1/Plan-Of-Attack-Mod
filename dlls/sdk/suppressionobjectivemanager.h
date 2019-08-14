#ifndef _SUPPRESSIONOBJECTIVEMANAGER_H_
#define _SUPPRESSIONOBJECTIVEMANAGER_H_

#include "objectivemanager.h"

/**
* Class declaration for a suppression objective manager
**/
class CSuppressionObjectiveManager : public CObjectiveManager
{
public:
	DECLARE_CLASS(CSuppressionObjectiveManager, CObjectiveManager);

	// constructor
	// @NOTE - FOR THE LOVE ALL THAT IS HOLY, DON'T CALL US (new/delete)!!!!
	CSuppressionObjectiveManager();

	// accessors
	virtual void Finalize(CObjective *pObj);
	virtual void ObjectiveEntered(CObjectiveHoldarea *pObj);
	virtual void ObjectiveLeewayExpired(CObjectiveHoldarea *pObj);
	virtual void SetObjectiveCaptured(CObjective *pObj);
	virtual float GetCaptureTimeForObjective(const CObjectiveHoldarea *pObj) const;

protected:
	// inherited methods
	virtual void UpdateClientObjective(CObjective *pObj) { BaseClass::UpdateClientObjective(pObj); }
	virtual void UpdateClientObjective(CObjective *pObj, CRecipientFilter *pFilter, ObjectiveState eState);

	// helpers
	void CheckOccupiedObjectives(void);
};

#endif