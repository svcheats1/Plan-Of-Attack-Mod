#ifndef _SNIPERSCOPE_H_
#define _SNIPERSCOPE_H_

#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "iclientmode.h"

#define SNIPERSCOPE_TEX "vgui/sniperscope"
#define SNIPERSCOPE_TEX_WIDTH 1024
#define SNIPERSCOPE_TEX_HEIGHT 1024
#define SNIPERSCOPE_RADIUS 394
#define CROSSHAIR_RADIUS 20

/**
* Class declaration for the sniper scope overlay
**/
class HudSniperScope : public CHudElement, public vgui::Panel, public CHudTexture
{
public:
	DECLARE_CLASS_SIMPLE(HudSniperScope, vgui::Panel);

	// constructor
	HudSniperScope(const char *szName);

	// inherited functions
	virtual bool ShouldDraw(void);
	virtual void Init(void);
	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void Paint(void);
	virtual void DrawSelf(int iX, int iY, int iW, int iH, Color& clr) const;

private:
	vgui::IScheme *m_pScheme;
	Color m_clr;
};

DECLARE_HUDELEMENT(HudSniperScope);
#endif