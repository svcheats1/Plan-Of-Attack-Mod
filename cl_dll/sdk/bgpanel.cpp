#include "cbase.h"
#include "bgpanel.h"

#include <vgui/ISurface.h>

/**
* Constructor
**/
BGPanel::BGPanel(Panel *pParent, const char *szName)
	: BaseClass(pParent, szName), m_pTexture(NULL)
{
	m_bReloadTexture = false;
}

/**
* Destructor
**/
BGPanel::~BGPanel()
{
	// kill our texture
	if(m_pTexture)
		delete(m_pTexture);

	if(Q_strlen(m_szReloadOnEvent))
		gameeventmanager->RemoveListener(this);
}

/**
* Applies the settings to the panel
*
* @return void
**/
void BGPanel::ApplySettings(KeyValues *pData)
{
	// go down
	BaseClass::ApplySettings(pData);

	// add the listener
	if(Q_strlen(m_szReloadOnEvent))
		gameeventmanager->AddListener(this, m_szReloadOnEvent, false);

	// try loading our texture
	LoadBackgroundTexture(m_szBgTexture);

	// set the background color
	SetBgColor(m_sBackgroundColor);
	if(m_sBackgroundColor != Color(0, 0, 0, 0))
	{
		SetPaintBackgroundEnabled(true);
		SetPaintBackgroundType(2);
	}
}

/**
* Applies the scheme settings
*
* @return void
**/
void BGPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// set the background color
	SetBgColor(m_sBackgroundColor);
	if(m_sBackgroundColor != Color(0, 0, 0, 0))
	{
		SetPaintBackgroundEnabled(true);
		SetPaintBackgroundType(2);
	}
}

/**
* Paints the background and then hands everything to the base class
*
* @return void
**/
void BGPanel::Paint(void)
{
	int iWide, iTall;

	// do we need to reload the texture?
	if(m_bReloadTexture && LoadBackgroundTexture(m_szBgTexture))
		m_bReloadTexture = false;

	// draw the texture
	if(m_pTexture)
	{
		// pull the texture size
		vgui::surface()->DrawGetTextureSize(m_pTexture->textureId, iWide, iTall);

		// are we scaling to the min?
		if(m_bScaleToMin)
			m_pTexture->DrawSelf(m_iBgXOffset, m_iBgYOffset, 
								min(GetWide() - (m_iBgXOffset * 2), GetTall() - (m_iBgYOffset * 2)) * iWide / iTall, 
								min(GetWide() - (m_iBgXOffset * 2), GetTall() - (m_iBgYOffset * 2)) * iTall / iWide, 
								Color(255, 255, 255, 255));
		else
			// draw the scaled texture
			m_pTexture->DrawSelf(m_iBgXOffset, m_iBgYOffset, 
								GetWide() - (m_iBgXOffset * 2), GetTall() - (m_iBgYOffset * 2),
								Color(255, 255, 255, 255));
	}

	// jump down
	BaseClass::Paint();
}

/**
* Loads the background texture
* 
* @param const char *szName The name of the texture to load
* @return bool True if we loaded the texture
**/
bool BGPanel::LoadBackgroundTexture(const char *szName)
{
	int iWidth, iHeight;
	char szStr[256];

	// make sure we have a string
	if(!Q_strlen(szName))
		return false;

	// figure out the name to use
	memset((void *)szStr, 0, 256);
	GetBackgroundTextureName(szName, szStr, 256);

	// do we have a texture name?
	if(!Q_strlen(szStr))
		return false;

	// create the new texture
	if(!m_pTexture)
		m_pTexture = new CHudTexture();

	// create an id if we don't already have one
	if(m_pTexture->textureId == -1)
		m_pTexture->textureId = vgui::surface()->CreateNewTextureID();

	// set up for drawing
	m_pTexture->bRenderUsingFont = false;
	vgui::surface()->DrawSetTextureFile(m_pTexture->textureId, szStr, true, false);

	// figure out how big it is and set the bounds
	vgui::surface()->DrawGetTextureSize(m_pTexture->textureId, iWidth, iHeight);
	m_pTexture->SetBounds(0, iWidth, 0, iHeight);

	return true;
}

/**
* Figures out the texture name to use
* This is designed as a hook to add prefixes, etc
*
* @param const char *szName Name of the file
* @param char *szStr The string to stick the name into
* @param int iSize The amount of space we have
* @return void
**/
void BGPanel::GetBackgroundTextureName(const char *szName, char *szStr, int iSize)
{
	// just copy the name
	Q_snprintf(szStr, iSize, "%s", szName);
}

/**
* Handles events sent our way
*
* @param KeyValues *pEvent The event we need to handle
* @return void
**/
void BGPanel::FireGameEvent(IGameEvent *pEvent)
{
	// are we listening for anything?
	if(Q_strlen(m_szReloadOnEvent) && FStrEq(m_szReloadOnEvent, pEvent->GetName()))
		m_bReloadTexture = true; // wait a tick before we do anything
}