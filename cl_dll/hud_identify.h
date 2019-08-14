#ifndef _HUD_IDENTIFY_H
#define _HUD_IDENTIFY_H

#include <vgui_controls/Label.h>

using namespace vgui;

#include <igameevents.h>

#include "hud_numericdisplay.h"
#include "hudelement.h"
#include "hud.h"
#include "hud_macros.h"

#include <vgui_controls/Panel.h>

#define HUD_IDENTIFY_DISPLAY_TIME 1.0

class HudIdentify : public CHudElement, public Label
{
public:
	DECLARE_CLASS_SIMPLE(HudIdentify, Label);

	HudIdentify (const char *szName);
	~HudIdentify();

	// inherited functions
	virtual void LevelInit(void);
	virtual bool ShouldDraw(void);
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void Paint(void);

protected:
	Color DetermineColor(int iPlayerID);

private:
	void PrintString( wchar_t *pString );
	IScheme *m_pScheme;
	float m_fEndDisplay;
	int m_iLastUserID;
};
#endif