#ifndef _WORLDMAP_H_
#define _WORLDMAP_H_

#include "strategymap.h"
#include <cl_dll/iviewport.h>
#include <vgui_controls/Label.h>

#define WORLDMAP_PANEL_WIDTH XRES(360)
#define WORLDMAP_PANEL_HEIGHT YRES(400)
#define WORLDMAP_MAP_WIDTH WORLDMAP_PANEL_WIDTH
#define WORLDMAP_MAP_HEIGHT YRES(360)

#define WORLDMAP_LEGEND_YPOS YRES(380)
#define WORLDMAP_LEGEND_LABEL_WIDTH XRES(75)
#define WORLDMAP_LEGEND_LABEL_HEIGHT MAP_ICON_SCALE

/**
* Class declaration for the world map (the big one that pops up when
* you hit a tbd button
**/
class CWorldMap : public CStrategyMap, public IViewPortPanel
{
public:
	DECLARE_CLASS_SIMPLE(CWorldMap, CStrategyMap);

	// constructor/destructor
	CWorldMap(IViewPort*pViewPort);
	~CWorldMap();

	// inherited methods
	const char *GetName(void) { return PANEL_WORLDMAP; }
	virtual void SetData(KeyValues *pData);
	virtual void Reset(void) { BaseClass::Reset(); }
	virtual void Update(void) { BaseClass::Update(); }
	virtual bool NeedsUpdate(void) { return BaseClass::NeedsUpdate; }
	virtual bool HasInputElements(void) { return false; }
	virtual void ShowPanel(bool bState);
	virtual vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible(void) { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent( parent ); }
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint(void);
	virtual void SetMapSize(int wide, int tall);

	// accessors
	void UpdateLegend(void);

protected:
	// inherited methods
	virtual void MapReset(void);
	virtual void ResetRound(void);

	// helpers
	void DrawLegend(void);
	void InitLegend(void);

private:
	IViewPort *m_pViewPort;

	int m_iLastScreenWidth;
	int m_iLastScreenHeight;

	int m_iLabelsWidth;
	vgui::Label **m_aLegendLabels;
};

#endif

