#ifndef _HUD_RADAR_H_
#define _HUD_RADAR_H_

#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "hudelement.h"
#include "hud.h"
#include "hud_macros.h"
#include "map.h"
#include "mathlib.h"

using namespace vgui;

#define RADAR_CIRCLE_POINTS 16
#define RADAR_MARKER_SCALE XRES(16)
#define RADAR_MARKER_TEXTURE "maps/radar_marker"
#define RADAR_DEAD_TEXTURE "vgui/xparent_black"
#define RADAR_RADIUS XRES(50)
#define RADAR_XOFFSET XRES(105)
#define RADAR_YOFFSET 0

/**
* Class declaration for the radar
**/
class HudRadar : public CHudElement, public CMap
{
public:
	DECLARE_CLASS_SIMPLE(HudRadar, CMap);

	// constructor / destructor
	HudRadar(const char *szName);
	~HudRadar();

	// inherited functions
	virtual bool ShouldDraw(void);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint(void);

	// painting stuff
	void DrawObjectiveMarker(void);
	void DrawMap(void);

protected:
	// inherited methods
	virtual void DrawMapTexture(void);
	virtual bool ShouldDrawItem(const Vector2D &vecPanelPos, int iSize);
	virtual Vector2D ComputeOffset(DrawType eType);
	virtual void MapReset(void);
	virtual CMapForeground *CreateForeground(void);

	// helpers
	void CalcVertices(const FootStep_t &sStep);

private:

	// drawing info
	int m_iRadius;
	int m_iMarkerTexture;
	int m_iDeadTexture;
	Vertex_t m_aVertices[RADAR_CIRCLE_POINTS];

	vgui::IScheme *m_pScheme;
};

/**
* Foreground for the radar
**/
class CRadarForeground : public CMapForeground
{
public:
	DECLARE_CLASS_SIMPLE(CRadarForeground, CMapForeground);

	// constructor
	CRadarForeground(vgui::Panel *pPanel, const char *szName)
		: BaseClass(pPanel, szName)
	{
		// ?
	}

	/**
	* Paints the panel
	*
	* @return void
	**/
	virtual void Paint(void)
	{
		// do nothing except update our following info
		m_pParent->UpdateFollowingInfo();
		return;
	}
};

#endif