#ifndef _HUD_OBJECTIVETIMER_H_
#define _HUD_OBJECTIVETIMER_H_

#include "hud_selftimer.h"
#include "c_objectivemanager.h"
#include <keyvalues.h>
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/isurface.h>
#include <vgui/ILocalize.h>

#define OBJECTIVE_TIMER_TEXTURE "vgui/objective_timer"
#define OBJECTIVE_TIMER_RADIUS XRES(51.0)
#define OBJECTIVE_TIMER_XOFFSET (OBJECTIVE_TIMER_RADIUS + XRES(105))
#define OBJECTIVE_TIMER_YOFFSET (OBJECTIVE_TIMER_RADIUS + YRES(-1))

using namespace vgui;

class HudObjectiveTimer : public CHudSelfTimer
{
public:
	DECLARE_CLASS_SIMPLE( HudObjectiveTimer, CHudSelfTimer );

	HudObjectiveTimer(const char *szName);
	~HudObjectiveTimer();

	void Init(void);
	void LevelInit(void);
	void FireGameEvent(IGameEvent *event);
	virtual void Paint();
	virtual void OnThink();
	virtual bool ShouldDraw();
	void ApplySchemeSettings(IScheme *pScheme);
	void DrawTimerRing(void);

private:
	virtual int Round(float fl);
	int GetStringPixelWidth(wchar_t *pString, vgui::HFont hFont);
	int GetLocalTeamNumber();
	wchar_t* GetCountdownString(const wchar_t *wszNumber);
	wchar_t* GetFrozenString(const wchar_t *wszNumber);
	void DrawTimerText(void);

	bool m_bDeleteObjName;
	int m_iObjID;
	wchar_t *m_wszObjName;
	IScheme *m_pScheme;
	int m_iOffensiveTeam;
	OBJECTIVE_TIMER_STATE m_iState;
	int m_iTexture;
	float m_fPauseTime;
	float m_fSecondsRemaining;
};

#endif