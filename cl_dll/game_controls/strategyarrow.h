#ifndef _STRATEGYARROW_H_
#define _STRATEGYARROW_H_

#include <vgui_controls/Frame.h>
#include <cl_dll/iviewport.h>
#include "map.h"
#include "hilitebutton.h"
#include <KeyValues.h>
#include <vgui/MouseCode.h>

#define MAX_POINTS 12
#define MIN_WIDTH 25
#define MAX_WIDTH 50
#define DEFAULT_ARROW_Z_VERTEX 1.0

enum eArrowType
{
	ARROW_PUSH,
	ARROW_HOLD,
	ARROW_NONE,
	ARROW_MAX
};

#define INITIAL_ARROW_WIDTH XRES(20)

class CStrategicArrow : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(CStrategicArrow, vgui::Panel);

	CStrategicArrow(vgui::Panel *pParent);
	~CStrategicArrow();

	/**
	* Struct declaration for a arrow point
	**/
	struct SStrategicArrowPoint
	{
		Vector m_vecPos;
		int m_iWidth;
	};

	// overwritten
	void SetPos(int x, int y);
	void SetSize(int wide, int tall);
	void SetZ(float z) { m_Z = z; m_bFinalized = false; }
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	// color of this arrow
	void SetColor(Color &color) { m_color = color; }
	// what kind of arrow head you want on it
	void SetArrowType(eArrowType type) { m_eArrowType = type; }

	// size
	void ModifyWidth(int iIncrement);
	int GetArrowWidth(void) { return m_iInitialWidth; }

	// doing these will force a re-calculation of the tri-points.
	// adds a control point given a vector
	SStrategicArrowPoint *AddPoint(Vector &point, int width = 10);
	// adds a control point as (x,y)
	SStrategicArrowPoint *AddPoint(float x, float y, int width = 10);
	void RemovePoint(const SStrategicArrowPoint *pPoint);
	void SetFinalized(bool bFinalized) { m_bFinalized = bFinalized; }
	
	// accessors
	CUtlVector<SStrategicArrowPoint *> &GetPoints(void) { return m_aPoints; }
	bool IsComplete(void) { return m_aPoints.Count() > 3; }
	virtual void PanelToMap(int &iXPos, int &iYPos);

	void Paint();

private:
	// finalizes data before painting
	void Finalize();
	// calculates the triangular points from the control points
	void CalculateTriPoints();
	// draws the triangles to the screen
	void DrawTriPoints();
	// draws the arrow head
	void DrawArrowHead();
	// mutates map coordiates [0, 1024] into the position and size specified for this object
	void MapToScreen(Vector &point, bool bDoParent = true);
	void MapToScreen(Vector2D &point, bool bDoParent = true);

	Color m_color;
	bool m_bFinalized;
	int m_iInitialWidth;

	// control points (not too many of these)
	CUtlVector<SStrategicArrowPoint *> m_aPoints;
	// our triangle strip points (quite a few of these)
	CUtlVector<Vector> m_aTriPoints;
	// what type of arrow are we?
	eArrowType m_eArrowType;

	float m_Z;
	CMeshBuilder m_MeshBuilder;
	Vector m_vEndPoint;
	Vector m_vEndTangent;
};

eArrowType AliasToArrowType( const char *alias );

#endif