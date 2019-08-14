#ifndef _HUD_STAMINA_H_
#define _HUD_STAMINA_H_

#include "c_sdk_player.h"
#include "imagepanel.h"
#include "iclientmode.h"
#include "hud.h"
#include "hudelement.h"

/**
* Class declaration for a hud element that displays your stamina level
* Not sure what the actual behavior will be at this point
**/
class HudStamina : public CImagePanel, public CHudElement
{
public:
	DECLARE_CLASS_SIMPLE(HudStamina, CImagePanel);

	HudStamina(const char *szName);
	virtual bool ShouldDraw(void);
	void SetOutOfStamina(bool bOutOfStamina);

private:
	bool m_bOutOfStamina;
};

DECLARE_HUDELEMENT(HudStamina);

#endif