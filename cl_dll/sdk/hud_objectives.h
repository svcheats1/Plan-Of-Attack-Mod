#ifndef _HUDOBJECTIVES_H_
#define _HUDOBJECTIVES_H_

#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include "c_objectivemanager.h"
#include "hudelement.h"
#include "hud.h"
#include "hud_macros.h"
#include "bgpanel.h"
#include "utlvector.h"
#include "hud_radar.h"
#include "iconset.h"
#include "hud_objectivetimer.h"

using namespace vgui;

#define OBJECTIVE_ICONSET_HEIGHT_SMALL XRES(27)
#define OBJECTIVE_ICONSET_WIDTH_SMALL YRES(27)
#define OBJECTIVE_ICONSET_HEIGHT_LARGE XRES(51)
#define OBJECTIVE_ICONSET_WIDTH_LARGE YRES(51)

class ObjectiveIconSet;

/**
* Class declaration for a hud element to display objective status information
**/
class HudObjectives : public CHudElement, public BGPanel
{
public:
	DECLARE_CLASS_SIMPLE(HudObjectives, BGPanel);

	// constructor / destructor
	HudObjectives(const char *szName);
	~HudObjectives();

	// inherited functions
	virtual bool ShouldDraw(void);
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void Paint(void);
	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);

	// handles objective events
	virtual void OnObjectiveEvent(SObjective_t *pObj);

	// Comparison function to check positioning
	static int ObjectiveIconSetCompare(ObjectiveIconSet * const *ppLeft, ObjectiveIconSet * const *ppRight);

protected:
	// helpers
	int FindIconSet(int iID);
	void UpdateIconSet(int i, SObjective_t *pObj);
	void CreateIconSet(SObjective_t *pObj);
	void PositionIconSets(void);
	static int GetLocalPlayerTeamNumber(void);

	// inherited methods
	void GetBackgroundTextureName(const char *szName, char *szStr, int iSize);

private:

	HudRadar *m_pRadar;
	HudObjectiveTimer *m_pTimer;

	CUtlVector<ObjectiveIconSet *> m_aIconSets;

	IScheme *m_pScheme;
};

#define OBJECTIVE_ICON_ACTIVE "vgui/hud_objectives_objactive"
#define OBJECTIVE_ICON_ACTIVE_PULSE 0.005

/**
* Enum for the states of the icons
**/
enum ObjectiveIconSetState
{
	OBJECTIVE_ICON_NEUTRAL = 0,
	OBJECTIVE_ICON_BLUE,
	OBJECTIVE_ICON_RED,
	OBJECTIVE_ICON_COUNT,
};

/**
* Class declaration for an ObjectiveIconSet
* Basically each icon set is a group of three hud textures (neutral, red, blue)
**/
class ObjectiveIconSet : public IconSet
{
public:
	DECLARE_CLASS_SIMPLE(ObjectiveIconSet, IconSet);

	// constructor/destructor
	ObjectiveIconSet(SObjective_t *pObj);
	~ObjectiveIconSet();

	// accessors
	int GetID(void) { return m_iID; }
	int GetOwner(void) { return m_iOwner; }
	void SetOwner(int iOwner) { m_iOwner = iOwner; }
	bool IsBase(void) { return m_bIsBase; }
	void SetIsBase(bool bIsBase) { m_bIsBase = bIsBase; }
	virtual void DrawSelf(void);

	// helpers
	void DetermineIcon(bool bActive, int iOwner);

	// sizing and positioning
	virtual void SetIconSize(int iWidth, int iHeight);

private:
	// objective status info
	int m_iID;
	bool m_bIsBase;
	int m_iOwner;

	// overlay info
	bool m_bDrawActiveOverlay;
	float m_fAlpha;
	float m_fLastUpdate;
	bool  m_bGrow;
	
	// icons
	CHudTexture *m_pActiveOverlay;
};

#endif