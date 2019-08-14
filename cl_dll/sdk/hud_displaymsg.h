#ifndef HUD_DISPLAYMSG_H
#define HUD_DISPLAYMSG_H

#include "hudelement.h"
#include "hud.h"
#include "hud_macros.h"
#include "filesystem.h"

#include <vgui_controls/Label.h>
#include <vgui/ischeme.h>

#define DEFAULT_TIMER_LENGTH 6
#define DISPLAY_MSG_RES "resource/DisplayMessages.res"
#define DISPLAY_MSG_X_POS XRES(5)
#define DISPLAY_MSG_Y_POS YRES(5)
#define DISPLAY_MSG_SUBLABEL1_X_POS XRES(5)
#define DISPLAY_MSG_SUBLABEL1_Y_POS YRES(20)
#define DISPLAY_MSG_SUBLABEL2_X_POS XRES(5)
#define DISPLAY_MSG_SUBLABEL2_Y_POS YRES(40)

/**
* Message type enum
**/
enum DMType
{
	DMTYPE_NOTIFICATION = 0,
	DMTYPE_TIP,
};

/**
* Class definition for a msg in the center of the screen
**/
class HudDisplayMsg : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(HudDisplayMsg, vgui::Panel);

	// constructor / destructor
	HudDisplayMsg(const char *szName);
	~HudDisplayMsg();

	// msg
	void MsgFunc_HudDisplayMsg(bf_read &msg);
	void MsgFunc_ResetHudDisplayMsg(bf_read &msg);

	// inherited functions
	virtual bool ShouldDraw(void);
	virtual void Init(void);
	virtual void LevelInit(void);
	virtual void Paint(void);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	void PaintBackground(void);

protected:
	// helpers
	void PerformReplacements(wchar_t *szStr, bf_read &msg);
	void GetMsgString(bf_read &msg, wchar_t *wszStr);
	void LoadMsgs(void);
	KeyValues *ExtractMsg(bf_read &msg);
	wchar_t *GetMsgText(bf_read &msg, const char *szStr);
	void DisplayHead(void);
	void PositionLabel(vgui::Label *pLabel, KeyValues *pValues);
	void RemoveInterrupts(KeyValues *pInterrupts);
	void AddTimers(void);
	void SubstituteTimer(vgui::Label *pLabel, const wchar_t *wszText, const wchar_t *wszSub);
	bool BehindPanels(void);
	void ShowMsgs(bool bShow);
	bool ShouldDisplayMsg(KeyValues *pMsg);
	bool AtMaxForMsg(KeyValues *pMsg);
	void ResetData(void);

private:
	CUtlVector<KeyValues *> m_aQ;
	float m_fEndDisplayTime;
	int m_iTimeLeft;
	int m_iLastTimeLeft;
	bool m_bQHeadChanged;
	bool m_bHidden;

	vgui::IScheme *m_pScheme;
	vgui::Label *m_pLabel;
	vgui::Label *m_pSubLabel1;
	vgui::Label *m_pSubLabel2;
	KeyValues *m_pMsgs;
	KeyValues *m_pMsgCounts;
};

/**
* A wrap label class as the stuff that exists doesn't seem to be working
**/
class WrapLabel : public vgui::Label
{
public:
	DECLARE_CLASS_SIMPLE(WrapLabel, vgui::Label);

	// constructor
	WrapLabel(vgui::Panel *pParent, const char *szName);

	// accessors
	virtual void SetText(const wchar_t *wszStr);
};

#endif