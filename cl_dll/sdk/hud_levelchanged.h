#ifndef HUD_LEVELCHANGED_H
#define HUD_LEVELCHANGED_H

#include "cbase.h"
#include "c_sdk_player.h"
#include "iclientmode.h"
#include "hud.h"
#include "hudelement.h"
#include <vgui_controls/Label.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HUDLVLCHANGE_DISPLAY_TIME 5.0
#define HUDLVLCHANGE_FADE_TIME 3.0

using namespace vgui;

/**
* Class declaration for a hud element that displays your stamina level
* Not sure what the actual behavior will be at this point
**/
class HudLevelChanged : public Label, public CHudElement
{
public:
	DECLARE_CLASS_SIMPLE(HudLevelChanged, Label);

	HudLevelChanged(const char *szName);
	virtual bool ShouldDraw(void);
	virtual void LevelChanged(int iNewLevel);
	virtual void Paint(void);
	virtual void ApplySchemeSettings(IScheme *pScheme);

private:
	int m_iCurrentLevel;
	float m_fEndTime;
	float m_fStartFadeTime;
	Color m_StartColor;
};

#endif