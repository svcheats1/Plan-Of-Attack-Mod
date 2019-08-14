#include "cbase.h"
#include "HiliteButton.h"
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/***************************************************************************************/
/** CHiliteButton ***********************************************************************/
/***************************************************************************************/

/**
* Constructor
*
* @param Panel *pParent The guy who we report to
* @param const char *szName The name of the button
* @param const char *szText The text to display on the button
**/
CHiliteButton::CHiliteButton(Panel *pParent, const char *szName, const char *szText)
	: BaseClass(pParent, szName, szText)
{
	// ?
}

/**
* Sets our hilite state and updates our neighbor
*
* @param bool bState The state to set the button to
* @param IHiliteable *pSender The guy who told us to hilite
* @param HiliteCondition eCondition The condition that caused us to hilite
* @return void
**/
void CHiliteButton::Hilite(bool bState, IHiliteable *pSender, HiliteCondition eCondition)
{
	// turn the bit on or off based on conditions
	if(bState)
		m_iHilite |= GetEventForCondition(eCondition);
	else if(m_iHilite & GetEventForCondition(eCondition))
		m_iHilite ^= GetEventForCondition(eCondition);

	// set our state
	SetArmed(m_iHilite);

	// don't send messages back to the guy who sent us the message. this should prevent recursion
	if(m_pNeighbor && m_pNeighbor != pSender)
		m_pNeighbor->Hilite(bState, this, (HiliteCondition)(HILITE_CONDITION_NEIGHBOR | eCondition));
}

/**
* Finds the condition for a given event
*
* @param HiliteCondition eCondition The condition to find an event for
* @return HCEvent The associated event
**/
HCEvent CHiliteButton::GetEventForCondition(HiliteCondition eCondition)
{
	// what type of event do we have?
	if(eCondition & HC_NEIGHBOR_EVENT)
		return HC_NEIGHBOR_EVENT;
	else if(eCondition & HC_MOUSEOVER_EVENT)
		return HC_MOUSEOVER_EVENT;
	else if(eCondition & HC_MOUSECLICK_EVENT)
		return HC_MOUSECLICK_EVENT;

	return HCEvent(0);
}

/**
* Takes control when the mouse leaves us
*
* @return void
**/
void CHiliteButton::OnCursorExited(void)
{
	// jump down
	BaseClass::OnCursorExited();

	// are we hiliting in this condition?
	if(m_iHiliteConditions & HILITE_CONDITION_MOUSEEXITED)
		Hilite(false, this, HILITE_CONDITION_MOUSEEXITED);
}

/**
* Takes control when the user mouses over us
*
* @return void
**/
void CHiliteButton::OnCursorEntered(void) 
{
	// jump down
	BaseClass::OnCursorEntered();

	// are we hiliting in this condition?
	if(m_iHiliteConditions & HILITE_CONDITION_MOUSEENTERED)
		Hilite(true, this, HILITE_CONDITION_MOUSEENTERED);
}

/**
* Takes control when the user presses the mouse
* 
* @return void
**/
void CHiliteButton::OnMousePressed(vgui::MouseCode sCode)
{
	// jump down
	BaseClass::OnMousePressed(sCode);

	// are we hiliting in this condition?
	if(m_iHiliteConditions & HILITE_CONDITION_MOUSEDOWN)
		Hilite(true, this, HILITE_CONDITION_MOUSEDOWN);
}

/**
* Applies the settings to the panel
*
* @return void
**/
void CHiliteButton::ApplySettings(KeyValues *pData)
{
	// jump down
	BaseClass::ApplySettings(pData);

	// set the background color
	SetBgColor(m_sHiliteBackgroundColor);
}

/***************************************************************************************/
/** CHiliteTextButton ******************************************************************/
/***************************************************************************************/

/**
* Constructor
*
* @param Panel *pParent The guy we report to
* @param const char *szName The name of the button
* @param const char *szText The text to use in the label
**/
CHiliteTextButton::CHiliteTextButton(Panel *pParent, const char *szName, const char *szText)
	: CHiliteButton(pParent, szName, szText)
{
	// no button border
	SetButtonBorderEnabled(false);
}

/**
* Applies the scheme settings
*
* @param IScheme *pScheme The scheme to apply
* @return void
**/
void CHiliteTextButton::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	vgui::IBorder *pBorder;

	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// are we using a border?
	if(!m_bUseBorder)
	{
		// get the blank border
		pBorder = pScheme->GetBorder("NoBorder");

		// no border
		SetButtonBorderEnabled(false);
		SetBorder(pBorder);
		SetDefaultBorder(pBorder);
		SetDepressedBorder(pBorder);
	}

	// are we drawing a background?
	if(m_sHiliteBackgroundColor != Color(0, 0, 0, 0))
	{
		// turn it on
		SetPaintBackgroundType(2);
		SetPaintBackgroundEnabled(true);
	}
}

/**
* Applies the settings to the panel
*
* @param KeyValues *pData The data to apply
* @return void
**/
void CHiliteTextButton::ApplySettings(KeyValues *pData)
{
	// jump down
	BaseClass::ApplySettings(pData);

	// are we drawing a background?
	if(m_sHiliteBackgroundColor != Color(0, 0, 0, 0))
	{
		// turn it on
		SetPaintBackgroundType(2);
		SetPaintBackgroundEnabled(true);
	}
}

/**
* Paints the button
*
* @return void
**/
void CHiliteTextButton::Paint(void)
{
	// what color should we be using?
	if(m_iHilite)
		SetFgColor(m_sHiliteTextColor);
	else
		SetFgColor(m_sLoliteTextColor);

	// paint the text
	BaseClass::Paint();
}

/**
* Returns the color to use for the background
*
* @return Color
**/
Color CHiliteTextButton::GetButtonBgColor(void)
{
	SetPaintBackgroundType(2);
	SetPaintBackgroundEnabled(true);
	return m_sHiliteBackgroundColor;
}

/***************************************************************************************/
/** CHiliteImageButton *****************************************************************/
/***************************************************************************************/

/**
* Constructor
*
* @param Panel *pParent The guy we report to
* @param const char *szName The name to use for the button
* @param const char *szTexture The name of the texture to use
* @param int iHiliteType The type of hilite we're doing
* @param const char *szImage The name fo teh texture to use for the button (*_hilite, *_lolite)
**/
CHiliteImageButton::CHiliteImageButton(Panel *pParent, const char *szName, const char *szTexture, int iHiliteType /* = 0 */)
	: BaseClass(pParent, szName, ""), m_pTextureHilite(NULL), m_pTextureLolite(NULL)
{
	// set the texture
	m_iHiliteType = iHiliteType;
	SetTexture(szTexture);

	// no border
	SetButtonBorderEnabled(false);

	// no overrides yet
	m_bOverridesInstalled = false;
}

/**
* Destructor
**/
CHiliteImageButton::~CHiliteImageButton()
{
	// destroy our textures
	if(m_pTextureHilite)
		delete(m_pTextureHilite);
	if(m_pTextureLolite)
		delete(m_pTextureLolite);
}

/**
* Applies the scheme settings
* 
* @param IScheme *pScheme The scheme we need to apply
* @return void
**/
void CHiliteImageButton::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	vgui::IBorder *pBorder;

	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// install the overrides
	if(!m_bOverridesInstalled)
		InstallOverrides();

	// get the blank border
	pBorder = pScheme->GetBorder("NoBorder");

	// no border
	SetButtonBorderEnabled(false);
	SetBorder(pBorder);
	SetDefaultBorder(pBorder);
	SetDepressedBorder(pBorder);

	// no hiliting
	SetArmedColor(m_sHiliteBackgroundColor, m_sHiliteBackgroundColor);
	SetDepressedColor(m_sHiliteBackgroundColor, m_sHiliteBackgroundColor);

	// if a texture was specified, load it
	if(strlen(m_szTexture))
		SetTexture(m_szTexture);

	// set all the size info
	if(m_iWidth == 0 && m_iHeight == 0)
		SetTextureSize(GetWide(), GetTall());
}

/**
* Installs the overrides for all of the stupid stuff that gets mashed somewhere
* and I can't figure it out
*
* @return void
**/
void CHiliteImageButton::InstallOverrides(void)
{
	// go through all of them
	if(m_iHiliteTypeOverride != -1)
		m_iHiliteType = m_iHiliteTypeOverride;
	if(m_fAlphaHiliteScaleXOverride != -1)
		m_fAlphaHiliteScaleX = m_fAlphaHiliteScaleXOverride;
	if(m_fAlphaHiliteScaleYOverride != -1)
		m_fAlphaHiliteScaleY = m_fAlphaHiliteScaleYOverride;
	if(m_iCropXOverride != -1)
		m_iCropX = m_iCropXOverride;
	if(m_iCropYOverride != -1)
		m_iCropY = m_iCropYOverride;
	if(m_iWidthOverride != -1)
		m_iWidth = m_iWidthOverride;
	if(m_iHeightOverride != -1)
		m_iHeight = m_iHeightOverride;

	// they're created
	m_bOverridesInstalled = true;
}

/**
* Sets the icon to use for a hilite
*
* @param int iTexture The texture to use
* @return void
**/
void CHiliteImageButton::SetHiliteTexture(int iTexture)
{
	// set it
	SetTexture(iTexture, m_pTextureHilite);
}

/**
* Sets the icon to use for lolite
* 
* @param int iTexture The texture to use
* @return void
**/
void CHiliteImageButton::SetLoliteTexture(int iTexture)
{
	// set it
	SetTexture(iTexture, m_pTextureLolite);
}

/**
* Sets the texture for the specified CHudTexture
*
* @param int iTexture The texture to use
* @param CHudTexture *&pTexture The CHudTexture to set
* @return void
**/
void CHiliteImageButton::SetTexture(int iTexture, CHudTexture *&pTexture)
{
	// do we have a texture already?
	if(!pTexture)
		pTexture = new CHudTexture();

	// make sure we're not using a font
	pTexture->bRenderUsingFont = false;

	// set the texture
	pTexture->textureId = iTexture;
}

/**
* Sets the texture to use
*
* @param const char *szTexture The name of the file to use
* @return void
**/
void CHiliteImageButton::SetTexture(const char *szTexture)
{
	// do we have an image?
	if(strlen(szTexture) > 0)
	{
		// are we using images or alpha?
		if(m_iHiliteType == HILITE_IMAGE_BUTTON_IMAGE)
		{
			char szStr[256];

			// do we have textures already?
			if(!m_pTextureHilite)
				m_pTextureHilite = new CHudTexture();
			if(!m_pTextureLolite)
				m_pTextureLolite = new CHudTexture();

			// make sure we're not using a font
			m_pTextureHilite->bRenderUsingFont = false;
			m_pTextureLolite->bRenderUsingFont = false;

			// load the hilite
			Q_snprintf(szStr, sizeof(szStr), "%s_hilite", szTexture);
			LoadTexture(szStr, m_pTextureHilite->textureId);

			// load the lolite
			Q_snprintf(szStr, sizeof(szStr), "%s_lolite", szTexture);
			LoadTexture(szStr, m_pTextureLolite->textureId);
		}
		else
		{
			// create the texture if we don't have one already
			if(!m_pTextureLolite)
				m_pTextureLolite = new CHudTexture();

			// make sure it doesn't use a font
			m_pTextureLolite->bRenderUsingFont = false;
			LoadTexture(szTexture, m_pTextureLolite->textureId);
		}
	}
}

/**
* Loads a texture after checking to see if already exists
*
* @param const char *szFile The name of the texture to load
* @param int &iID ID to fill in
* @return void
**/
void CHiliteImageButton::LoadTexture(const char *szFile, int &iID)
{
	iID = UTIL_LoadTexture(szFile);
}

/**
* Sets the crop values for painting the image
*
* @param int iX X Crop
* @param int iY Y Crop
* @return void
**/
void CHiliteImageButton::SetImageCrop(int iX, int iY)
{
	// just set them
	m_iCropX = iX;
	m_iCropY = iY;
}

/**
* Sets the size of the image
*
* @param int iWidth
* @param int iHeight
* @return void
**/
void CHiliteImageButton::SetTextureSize(int iWidth, int iHeight)
{
	// set them
	m_iWidth = iWidth;
	m_iHeight = iHeight;
}

/**
* Sets the size of the hilite
*
* @param float fX
* @param float fY
* @return void
**/
void CHiliteImageButton::SetHiliteScale(float fX, float fY)
{
	// set them
	m_fAlphaHiliteScaleX = fX;
	m_fAlphaHiliteScaleY = fY;
}

/**
* Determines the color to use for the hilite
*
* @return void
**/
Color CHiliteImageButton::GetButtonHiliteColor(void)
{
	// are we hiliting and enabled?
	if(m_iHilite && IsEnabled())
		return m_sHiliteTextColor;
	else
		return m_sLoliteTextColor;
}

/**
* Paints the image to the screen
*
* @return void
**/
void CHiliteImageButton::Paint(void)
{
	int iWidth, iHeight, iX, iY;
	Color sColor;

	// figure out what color to use
	sColor = GetButtonTextureColor();

	// pull the positioning info
	GetSize(iWidth, iHeight);
	GetPos(iX, iY);

	// make sure the bounds are correct along witht the texture coords
	if(m_pTextureHilite)
		m_pTextureHilite->SetBounds(0, XRES(m_iWidth), 0, YRES(m_iHeight));
	if(m_pTextureLolite)
		m_pTextureLolite->SetBounds(0, XRES(m_iWidth), 0, YRES(m_iHeight));

	// make sure nothing is stupid
	Assert(m_iWidth >= 0);
	Assert(m_iHeight >= 0);
	Assert(m_iCropX >= 0);
	Assert(m_iCropY >= 0);

	// draw the background alpha value if necessary
	if(m_iHiliteType == HILITE_IMAGE_BUTTON_ALPHA && ShouldDrawHilite())
	{
		// draw the rect
		vgui::surface()->DrawSetColor(GetButtonHiliteColor());
		vgui::surface()->DrawFilledRect(XRES(m_iCropX) * (1.0 - m_fAlphaHiliteScaleX), 
										YRES(m_iCropY) * (1.0 - m_fAlphaHiliteScaleY), 
										XRES(m_iCropX) * m_fAlphaHiliteScaleX, 
										YRES(m_iCropY) * m_fAlphaHiliteScaleY);
	}

	// are we hiliting with an image
	if(IsEnabled() && m_iHilite && m_pTextureHilite && m_iHiliteType == HILITE_IMAGE_BUTTON_IMAGE && ShouldDrawHilite())
		m_pTextureHilite->DrawSelfCropped(0, 
											0, 
											0, 
											0, 
											XRES(m_iCropX), 
											YRES(m_iCropY), 
											sColor);
	// if we're not hilited or we are hiliting the alpha draw the image or we're not enabled and we have the texture
	// draw the non-hilited image
	else if((!IsEnabled() || !m_iHilite || m_iHiliteType == HILITE_IMAGE_BUTTON_ALPHA) && m_pTextureLolite)
	{
		// draw the texture
		m_pTextureLolite->DrawSelfCropped(0, 
											0, 
											0, 
											0, 
											XRES(m_iCropX), 
											YRES(m_iCropY), 
											sColor);
	}
}