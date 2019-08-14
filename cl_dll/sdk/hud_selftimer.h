#ifndef HUD_SELFTIMER_H
#define HUD_SELFTIMER_H

#include "hud_basetimer.h"
#include "hudelement.h"
#include "hud_macros.h"
#include <vgui/IScheme.h>
#include "iconset.h"
#include "igameevents.h"

using namespace vgui;

/**
* Class definition for a hud timer
**/
class CHudSelfTimer : public CHudElement, public CHudBaseTimer
{
public:
	DECLARE_CLASS_SIMPLE(CHudSelfTimer, CHudBaseTimer);

	// constructor
	CHudSelfTimer(const char *szName);

	// accessors
	virtual void SetTimerDuration(float fDuration);
	virtual void SetEndTime(float fTime);

	// inherited functions
	virtual bool ShouldDraw(void);

	// helpers
	int DetermineMinutes();
	int DetermineSeconds();

protected:

	// inherited functions
	virtual void OnThink(void);
	virtual int Round(float fl);

	float m_fEndTime;
	float m_fDuration;
};

#define ROUND_TIMER_ICON_XOFFSET XRES(67)
#define ROUND_TIMER_ICON_YOFFSET YRES(-2)
#define ROUND_TIMER_ICON_WIDTH XRES(51)
#define ROUND_TIMER_ICON_HEIGHT YRES(51)

/**
* Enum for the round timer attack/defend icon
**/
enum RoundTimerState
{
	ROUND_TIMER_ATTACK = 0,
	ROUND_TIMER_DEFEND,
};

/**
* Declaration for a round timer
**/
class HudRoundTimer : public CHudSelfTimer, public IconSet
{
public:
	// constructor
	HudRoundTimer(const char *szName);

	// inherited methods
	virtual void Init(void);
	virtual void MsgFunc_SyncRoundTimer(bf_read &msg);
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void Paint(void);
	virtual void OnScreenSizeChanged(int iOldWidth, int iOldHeight);
	virtual bool ShouldDraw(void);
		
protected:
	// inherited methods
	virtual void GetBackgroundTextureName(const char *szName, char *szStr, int iSize);

private:
	IScheme *m_pScheme;
};

#endif