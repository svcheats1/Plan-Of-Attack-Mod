//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HUD_NUMERICDISPLAY_H
#define HUD_NUMERICDISPLAY_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include "bgpanel.h"

//-----------------------------------------------------------------------------
// Purpose: Base class for all the hud elements that are just a numeric display
//			with some options for text and icons
//-----------------------------------------------------------------------------
class CHudNumericDisplay : public BGPanel
{
	DECLARE_CLASS_SIMPLE( CHudNumericDisplay, BGPanel );

public:
	CHudNumericDisplay(vgui::Panel *parent, const char *name);

	void SetDisplayValue(int value);
	void SetSecondaryValue(int value);
	void SetShouldDisplayValue(bool state);
	void SetShouldDisplaySecondaryValue(bool state);
	void SetLabelText(const wchar_t *text);
	void SetIndent(bool state);
	wchar_t *GetLabelText(void);

	virtual void Reset();

protected:
	// vgui overrides
	virtual void Paint();
	virtual void PaintLabel();

	virtual void PaintNumbers(vgui::HFont font, int xpos, int ypos, int value);

	CPanelAnimationVar( float, m_flBlur, "Blur", "0" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
	CPanelAnimationVar( Color, m_Ammo2Color, "Ammo2Color", "FgColor" );

	CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudNumbers" );
	CPanelAnimationVar( vgui::HFont, m_hNumberGlowFont, "NumberGlowFont", "HudNumbersGlow" );
	CPanelAnimationVar( vgui::HFont, m_hSmallNumberFont, "SmallNumberFont", "HudNumbersSmall" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );

	CPanelAnimationVarAliasType( float, text_xpos, "text_xpos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_ypos, "text_ypos", "20", "proportional_float" );
	CPanelAnimationVarAliasType( float, digit_xpos, "digit_xpos", "50", "proportional_float" );
	CPanelAnimationVarAliasType( float, digit_ypos, "digit_ypos", "2", "proportional_float" );
	CPanelAnimationVarAliasType( float, digit2_xpos, "digit2_xpos", "98", "proportional_float" );
	CPanelAnimationVarAliasType( float, digit2_ypos, "digit2_ypos", "16", "proportional_float" );

private:

	int m_iValue;
	int m_iSecondaryValue;
	wchar_t m_LabelText[32];
	bool m_bDisplayValue, m_bDisplaySecondaryValue;
	bool m_bIndent;
};


#endif // HUD_NUMERICDISPLAY_H
