#ifndef _HILITE_BUTTON_H_
#define _HILITE_BUTTON_H_

#include "mouseoverpanelbutton.h"
#include <vgui_controls/Button.h>
#include "hud.h"
#include "bgpanel.h"

// forward declarations
class CHiliteButton;
class CHiliteImageButton;

/**
* Enum describing when we should hilite
**/
enum HiliteCondition
{
	HILITE_CONDITION_NEIGHBOR = 1 << 0,
	HILITE_CONDITION_MOUSEENTERED = 1 << 1,
	HILITE_CONDITION_MOUSEEXITED = 1 << 2,
	HILITE_CONDITION_MOUSEDOWN = 1 << 3,
	HILITE_CONDITION_MOUSEUP = 1 << 4,
};

/**
* Enum describing paired events
* @NOTE - be sure to add new hilite events to GetEventForCondition
**/
enum HCEvent
{
	HC_NEIGHBOR_EVENT = HILITE_CONDITION_NEIGHBOR,
	HC_MOUSEOVER_EVENT = HILITE_CONDITION_MOUSEENTERED | HILITE_CONDITION_MOUSEEXITED,
	HC_MOUSECLICK_EVENT = HILITE_CONDITION_MOUSEDOWN | HILITE_CONDITION_MOUSEUP,
};

/**
* Class declaration for a hiliteable interface
**/
class IHiliteable
{
public:

	// all hiliteable items must define these methods
	virtual void Hilite(bool bState, IHiliteable *pSender, HiliteCondition eCondition) = 0;
	virtual bool ShouldHilite(void) = 0;

	/**
	* Constructor
	**/
	IHiliteable::IHiliteable()
	{
		// not hilited, no neighbor
		m_iHilite = 0;
		m_pNeighbor = NULL;
	}

	/**
	* Sets the neighbor that we should tell to hilite in the event of a mouseover
	*
	* @param CHiliteButton *pNeighbor The neighbor to tell about mouseovers
	* @return void
	**/
	void IHiliteable::SetNeighbor(IHiliteable *pNeighbor)
	{
		// set them
		m_pNeighbor = pNeighbor;

		// let them know of our state
		if(m_pNeighbor)
			m_pNeighbor->Hilite(m_iHilite, this, HILITE_CONDITION_NEIGHBOR);
	}

protected:
	int m_iHilite;
	IHiliteable *m_pNeighbor;
};

/**
* Class declaration for a hilite button
* Similar to a mouseover except that we alert a neighbor when we
* get moused over
**/
class CHiliteButton : public vgui::Button, public IHiliteable
{
public:
	DECLARE_CLASS_SIMPLE(CHiliteButton, vgui::Button);

	// constructor / destructor
	CHiliteButton(Panel *pParent, const char *szName, const char *szText);

	// accessors
	CHiliteButton *GetNeighbor(void);
	void SetHiliteConditions(int iConditions);
	virtual bool ShouldHilite(void) { return IsEnabled(); }
	void SetBackgroundColor(Color sColor) { m_sHiliteBackgroundColor = sColor; }

	// inherited methods
	virtual void OnCursorEntered(void);
	virtual void OnCursorExited(void);
	virtual void OnMousePressed(vgui::MouseCode sCode);
	virtual void Hilite(bool bState, IHiliteable *pSender, HiliteCondition eCondition);
	virtual void SetCommand(const char *szCommand)
	{
		// cache it because we can't get to it later...
		Q_strncpy(m_szCommand, szCommand, sizeof(szCommand));
		BaseClass::SetCommand(szCommand);
	}
	const char *GetCommand(void) { return m_szCommand; }
	virtual void ApplySettings(KeyValues *pData);

protected:
	// helpers
	HCEvent GetEventForCondition(HiliteCondition eCondition);

	// by default hilite on neighbor, mouseentered and mouseexited events
	CPanelAnimationVar(int, m_iHiliteConditions, "hiliteCondition", "7");
	CPanelAnimationVar(Color, m_sHiliteBackgroundColor, "HiliteBackgroundColor", "Blank");

	char m_szCommand[256];
};

/**
* Class declaration for a hilite text button
* The color of the text will be changed on mouseovers
* A background can be added using the BGPanel interface
**/
class CHiliteTextButton : public CHiliteButton
{
public:
	DECLARE_CLASS_SIMPLE(CHiliteTextButton, CHiliteButton);

	// constructor / destructor
	CHiliteTextButton(Panel *pParent, const char *szName, const char *szText);

	// inherited methods
	virtual void Paint(void);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual Color GetButtonBgColor(void);
	virtual void ApplySettings(KeyValues *pData);
	virtual void DrawFocusBorder(int tx0, int ty0, int tx1, int ty1) { /* i don't care */ }

private:
	CPanelAnimationVar(Color, m_sLoliteTextColor, "LoliteTextColor", "White");
	CPanelAnimationVar(Color, m_sHiliteTextColor, "HiliteTextColor", "Orange");
	CPanelAnimationVar(bool, m_bUseBorder, "UseHiliteBorder", "0");
};

/**
* Enum describing how we will draw the hilite
**/
enum
{
	HILITE_IMAGE_BUTTON_IMAGE = 0,
	HILITE_IMAGE_BUTTON_ALPHA,
};


/**
* Class declaration for a hilite image button class
* Displays a bitmap as the button
**/
class CHiliteImageButton : public CHiliteButton
{
public:
	DECLARE_CLASS_SIMPLE(CHiliteImageButton, CHiliteButton);

	// constructor / destructor
	CHiliteImageButton(Panel *pParent, const char *szName, const char *szTexture, int iHiliteType = 0);
	~CHiliteImageButton();

	// accessors
	void SetTexture(const char *szTexture);
	void SetImageCrop(int iX, int iY);
	void SetHiliteScale(float fX, float fY);
	void SetTextureSize(int iWidth, int iHeight);
	void SetHiliteTexture(int iTexture);
	void SetLoliteTexture(int iTexture);

	// inherited methods
	virtual void Paint(void);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void DrawFocusBorder(int tx0, int ty0, int tx1, int ty1) { /* i don't care */ }

protected:

	// helpers
	void LoadTexture(const char *szFile, int &iID);
	void SetTexture(int iTexture, CHudTexture *&pTexture);
	void InstallOverrides(void);
	virtual bool ShouldDrawHilite(void) { return true; }
	virtual Color GetButtonTextureColor(void) { return Color(255, 255, 255, 255); }
	virtual Color GetButtonHiliteColor(void);

	// textures
	CHudTexture *m_pTextureHilite;
	CHudTexture *m_pTextureLolite;
	CPanelAnimationStringVar(256, m_szTexture, "Texture", "");

	// overrides
	bool m_bOverridesInstalled;

	// drawing method
	CPanelAnimationVar(int, m_iHiliteTypeOverride, "hiliteType", "-1"); // 0 = _hilite, _lolite; 1 = alpha
	CPanelAnimationVar(float, m_fAlphaHiliteScaleXOverride, "alphaHiliteScaleX", "-1");
	CPanelAnimationVar(float, m_fAlphaHiliteScaleYOverride, "alphaHiliteScaleY", "-1");
	CPanelAnimationVar(Color, m_sLoliteTextColor, "LoliteTextColor", "255 255 255 255");
	CPanelAnimationVar(Color, m_sHiliteTextColor, "HiliteTextColor", "255 176 0 255");
	int m_iHiliteType;
	float m_fAlphaHiliteScaleX;
	float m_fAlphaHiliteScaleY;

	// crops
	CPanelAnimationVar(int, m_iCropXOverride, "imageCropX", "-1");
	CPanelAnimationVar(int, m_iCropYOverride, "imageCropY", "-1");
	int m_iCropX;
	int m_iCropY;

	// width and height
	CPanelAnimationVar(int, m_iWidthOverride, "imageWidth", "-1");
	CPanelAnimationVar(int, m_iHeightOverride, "imageHeight", "-1");
	int m_iWidth;
	int m_iHeight;
};

#endif