#ifndef HUD_PUNISH_H
#define HUD_PUNISH_H

#include "hudelement.h"
#include "hud.h"
#include "hud_macros.h"
#include "filesystem.h"
#include "hilitebutton.h"

#include <vgui/ischeme.h>
#include <vgui_controls/Frame.h>
#include <cl_dll/iviewport.h>

using namespace vgui;

#define PUNISH_RES "resource/Punish.res"

/**
* Class definition for a punish menu
**/
class CPunishMenu : public vgui::Frame, public IViewPortPanel
{
public:
	DECLARE_CLASS_SIMPLE(CPunishMenu, vgui::Frame);

	// constructor / destructor
	CPunishMenu(IViewPort *pViewPort);
	~CPunishMenu();

	// inherited functions
	virtual const char *GetName(void) { return PANEL_PUNISH; }
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void SetData(KeyValues *pData) { }
	virtual void Reset(void);
	virtual void Update(void);
	virtual bool NeedsUpdate(void) { return false; }
	virtual bool HasInputElements(void) { return true; }
	virtual void ShowPanel(bool bState);
	virtual bool IsVisible(void) { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	virtual void OnCommand(const char *szCommand);

	// accessors
	void SetPlayer(int iPlayer) { m_iPlayer = iPlayer; }
	void SetPlayerName(const char *szName);

protected:
	virtual Panel *CreateControlByName(const char *controlName);

private:
	Label *m_pNameLabel;
	CHiliteTextButton *m_pStrikeButton;
	CHiliteTextButton *m_pForgiveButton;
	IScheme *m_pScheme;
	IViewPort *m_pViewPort;

	int m_iPlayer;
};

#endif