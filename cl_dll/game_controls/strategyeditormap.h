#ifndef _STRATEGYEDITORMAP_H_
#define _STRATEGYEDITORMAP_H_

#include "strategymap.h"
#include "strategyeditorpoint.h"

class CStrategyEditorPoint;

/**
* Class declaration for our strategy editor map.  Handles user input for 
* creating/editing strategy points
**/
class CStrategyEditorMap : public CStrategyMap
{
public:
	DECLARE_CLASS_SIMPLE(CStrategyEditorMap, CStrategyMap);

	// constructor
	CStrategyEditorMap(vgui::Panel *pParent);

	// inherited methods
	virtual void OnMouseReleased(vgui::MouseCode sCode);
	virtual void OnMousePressed(vgui::MouseCode sCode);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint(void);
	virtual void Reset(void);
	virtual void Init(void);
	virtual bool LoadFromKeyValues(KeyValues *pData);

	// accessors
	void RemovePoint(CStrategyEditorPoint *pPoint);
	void SetCurrentArrow(int iArrow);
	void ClearPoints(void);
	void SetActiveTeam(int iTeam);

protected:
	// helpers
	virtual void CreatePathPoint(int iXPos, int iYPos);
	virtual void AddArrowPoint(CStrategicArrow *pArrow, int iX, int iY, int iWidth);
	virtual void AddArrow(CStrategicArrow *pArrow);
	virtual int GetTeamRotation(void);
	Color GetPointColor(int iArrow);

private:
	CUtlVector<CStrategyEditorPoint *> m_aPoints[3];
	int m_iCurrentArrow;
	vgui::MouseCode m_sPressCode;
};

#endif