#ifndef _STRATEGYEDITORPOINT_H_
#define _STRATEGYEDITORPOINT_H_

#include <vgui_controls/Panel.h>
#include "strategyeditormap.h"
#include "strategyarrow.h"

#define SEPOINT_WIDE XRES(10)
#define SEPOINT_TALL YRES(10)
#define STRATEGY_EDITOR_POINT_POINTS 54 // must be multiple of 3!

#define RESIZE_PERIOD .05

// forward declarations
class CStrategyEditorMap;

/**
* Class declaration for a point on the strategy editor map
**/
class CStrategyEditorPoint : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(CStrategyEditorPoint, vgui::Panel);

	// constructor
	CStrategyEditorPoint(CStrategicArrow *pParentArrow, CStrategyEditorMap *pMap, int iXPos, int iYPos, int iWidth, Color sColor);

	// inherited methods
	virtual void Paint(void);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnMouseReleased(vgui::MouseCode sCode);
	virtual void OnMousePressed(vgui::MouseCode sCode);

	// accessors
	void InitPoint(int iWidth);
	bool IsValid(void) { return m_bIsValid; }

protected:
	// helpers
	virtual void MapToPanel(int &iXPos, int &iYPos);
	void ArrowPointToScreen(Vector &vecPoint);
	void DrawPoint(void);
	void Finalize(void);

private:
	CStrategicArrow *m_pArrow;
	CStrategyEditorMap *m_pMap;
	CStrategicArrow::SStrategicArrowPoint *m_pPoint;

	bool m_bIsDragging;
	bool m_bIsResizingArrow;
	bool m_bIsResizingPoint;
	bool m_bGrow;
	float m_fNextResizeTime;
	bool m_bIsValid;
	Color m_sColor;
	bool m_bFinalized;
	Vector m_vecPoints[STRATEGY_EDITOR_POINT_POINTS];
};

#endif