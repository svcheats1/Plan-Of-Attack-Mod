#ifndef HUD_SKILL_H
#define HUD_SKILL_H

#include <vgui_controls/Panel.h>
#include "hudelement.h"
#include "hud.h"
#include "hud_macros.h"
#include "skillclass.h"
#include "iclientmode.h"
#include <vgui_controls/Label.h>
#include <vgui/ILocalize.h>
#include "c_sdk_player.h"
#include "bgpanel.h"

using namespace vgui;

#define NO_SKILL_STR "Girls want guys with skills: nunchaku skills, bow staff skills, computer hacking skills."
#define SKILLCLASS_LABEL_XPOS XRES(9)
#define SKILLCLASS_LABEL_YPOS YRES(3)
#define SKILLCLASS_LABEL_WIDTH XRES(110)
#define SKILLCLASS_LABEL_HEIGHT YRES(24)

#define SKILLCLASS_BAR_STARTX XRES(8)
#define SKILLCLASS_BAR_STARTY XRES(17)
#define SKILLCLASS_BAR_WIDTH XRES(125)
#define SKILLCLASS_BAR_HEIGHT XRES(10)

/**
* Class definition for a hudskill class
**/
class HudSkill : public CHudElement, public BGPanel
{
public:
	DECLARE_CLASS_SIMPLE(HudSkill, BGPanel);

	// constructor
	HudSkill(const char *szName);

	// inherited functions
	virtual bool ShouldDraw(void);
	virtual void Init(void);
	void ApplySchemeSettings(IScheme *pScheme);
	void Paint(void);
	virtual void OnScreenSizeChanged(int iOldWidth, int iOldHeight);

	// accessors
	void LocalSkillUpdated(void);

protected:
	// inherited methods
	void GetBackgroundTextureName(const char *szName, char *szStr, int iSize);

private:
	IScheme *m_pScheme;
	Label *m_pLabel;
	bool m_bShouldDraw;
	int m_iTextureID;
};

DECLARE_HUDELEMENT(HudSkill);

#endif