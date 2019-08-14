#ifndef _BGPANEL_H_
#define _BGPANEL_H_

#include <vgui_controls/Panel.h>
#include "igameevents.h"
#include "keyvalues.h"

/**
* Class declaration for a panel with a custom background
* Most of our hud stuff should inherit from this at some point
**/
class BGPanel : public vgui::Panel, public IGameEventListener2
{
public:
	DECLARE_CLASS_SIMPLE(BGPanel, vgui::Panel);

	// constructor / destructor
	BGPanel(Panel *pParent, const char *szName);
	~BGPanel();
		
	// inherited methods
	virtual void Paint(void);
	virtual void FireGameEvent(IGameEvent *pEvent);
	virtual void ApplySettings(KeyValues *pData);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		
protected:

	// helpers
	bool LoadBackgroundTexture(const char *szName);
	virtual void GetBackgroundTextureName(const char *szName, char *szStr, int iSize);

	CPanelAnimationStringVar(256, m_szReloadOnEvent, "ReloadOnEvent", "");
	CPanelAnimationVarAliasType(float, m_iBgXOffset, "BackgroundXOffset", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, m_iBgYOffset, "BackgroundYOffset", "0", "proportional_float");
	CPanelAnimationStringVar(256, m_szBgTexture, "BackgroundTexture", "");
	CPanelAnimationVar(Color, m_sBackgroundColor, "PanelBackgroundColor", "Blank");
	CPanelAnimationVar(bool, m_bScaleToMin, "ScaleToMin", "0");

	CHudTexture *m_pTexture;

	bool m_bReloadTexture;
};

#endif