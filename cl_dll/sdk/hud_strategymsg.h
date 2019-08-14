#ifndef _HUD_STRATEGYMSG_H_
#define _HUD_STRATEGYMSG_H_

#include "hudelement.h"
#include "hud.h"
#include "hud_macros.h"
#include "filesystem.h"
#include <vgui_controls/Panel.h>
#include "strategymenu.h"
#include "iclientmode.h"
#include "hud_displaymsg.h"

using namespace vgui;

/*
class CMyLabel : public vgui::Label
{
public:
	CMyLabel(Panel *pParent, const char* panelName, const char* text) : Label(pParent, panelName, text) { }
	CMyLabel(Panel *pParent, const char* panelName, const wchar_t* wszText) : Label(pParent, panelName, wszText) { }
	void SetWrap(bool bWrap) { vgui::Label::SetWrap(bWrap); }
};
*/

class HudStrategyMsg : public CHudElement, public vgui::Panel, public IGameEventListener2
{
public:
	DECLARE_CLASS_SIMPLE(HudStrategyMsg, vgui::Panel);

	// constructor / destructor
	HudStrategyMsg(const char *szName);
	~HudStrategyMsg();

	virtual void FireGameEvent(IGameEvent *event);
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void Init(void);
	virtual void LevelInit(void);
	virtual void Paint(void);
	virtual bool ShouldDraw(void);

	// accessors
	void DisplayMap(void);

	void MsgFunc_ChooseStrategy(bf_read &msg);
	void MsgFunc_StrategyMsg(bf_read &msg);
	void MsgFunc_StrategyMsgBlock(bf_read &msg);

private:
	void SetMessages();
	void EmitSound();
	void RoundReset();
	bool BehindPanels(void);

	// state and data
	char m_szObjectives[12], m_szObjectiveName[64];
	bool m_bIsBase, m_bAttacking;
	float m_fDisplayEndTime;
	bool m_bTransmit;
	int m_iCommandsToGo;
	CUtlBuffer m_sBuffer;

	// panels
	CStrategyMap *m_pMap;
	WrapLabel *m_pLabel, *m_pSubLabel;
	Label *m_pStrategyNameLabel;
};

#endif