#ifndef HUD_CASH_H
#define HUD_CASH_H

#include "hud_numericdisplay.h"
#include "hudelement.h"
#include "hud.h"
#include "hud_macros.h"

#include <vgui_controls/Panel.h>

/**
* Class definition for a cash class
**/
class HudCash : public CHudElement, public CHudNumericDisplay
{
public:
	DECLARE_CLASS_SIMPLE( HudCash, CHudNumericDisplay );

	HudCash(const char *szName);

	// cash
	void UpdateCash(int iCash);

	// inherited functions
	virtual bool ShouldDraw(void);
	virtual void Init(void);
	virtual void LevelInit(void);
	void Paint(void);

private:

	char m_szLabel[2];
	int m_iCash;
	int m_iDifference;
};

DECLARE_HUDELEMENT(HudCash);

#endif