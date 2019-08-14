#ifndef HUD_SKILL_H
#define HUD_SKILL_H

#include <vgui_controls/Panel.h>
#include "hudelement.h"
#include "hud.h"
#include "hud_macros.h"
#include "iclientmode.h"

using namespace vgui;

// FIXME
#define VECTOR_LENGTH_1DEGREE 1

#define CROSSHAIR_COLOR 255, 176, 0, 150 // green 0, 255, 0, 150
#define CROSSHAIR_LINE_LENGTH XRES(8)
/*#define CROSSHAIR_FONT "CrossHairIcons"
#define CROSSHAIR_CIRCLE L'o'
#define CROSSHAIR_DOT L'p'*/

/**
* Class definition for a dynamic crosshair class
**/
class HudDynamicCrosshair : public CHudElement, public Panel
{
public:
	DECLARE_CLASS_SIMPLE(HudDynamicCrosshair, Panel);

	// constructor
	HudDynamicCrosshair(const char *szName);

	// inherited functions
	virtual bool ShouldDraw(void);
	virtual void Init(void);
	void ApplySchemeSettings(IScheme *pScheme);
	virtual void Paint(void);

private:
	float GetAimAsPixels(void);
	float GetAimVector(void);

	IScheme *m_pScheme;
	bool m_bShouldDraw;
	float m_flCurrentRadius;
};

#endif