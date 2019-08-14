#ifndef _HUD_MOD_H
#define _HUD_MOD_H

#include <vgui_controls/Label.h>

using namespace vgui;

#include <igameevents.h>

#include "hud_numericdisplay.h"
#include "hudelement.h"
#include "hud.h"
#include "hud_macros.h"

#include <vgui_controls/Panel.h>

struct MODstruct
{
	int attackerid;
	int userid;
	char weapon[32];
	long starttime;
	bool headshot;
	bool levelup;
	Color color;
};

class HudMOD : public IGameEventListener2, public CHudElement, public Panel
{
public:
	DECLARE_CLASS_SIMPLE(HudMOD, Panel);

	HudMOD (const char *szName);
	~HudMOD();

	// inherited functions
	virtual void LevelInit(void);
	virtual bool ShouldDraw(void);
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void Paint(void);

	void FireGameEvent(IGameEvent * event);
	void MsgFunc_MODFunc(bf_read &msg);

protected:
	bool IsMyTeam(int iPlayerID);
	int GetStringPixelWidth( wchar_t *pString, vgui::HFont hFont );
	Color DetermineColor(int iPlayerID);
    void PrintDeathInConsole(const MODstruct &info);

private:
	void PrintString( wchar_t *pString );
	IScheme *m_pScheme;
	CUtlVector<MODstruct> m_Messages;
	KeyValues *m_pDeathChars;

};
#endif