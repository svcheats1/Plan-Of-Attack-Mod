#include "cbase.h"
#include "imagepanel.h"

/**
* Constructor
**/
CImagePanel::CImagePanel(Panel *pParent, const char *szName)
	: BaseClass(pParent, szName, NULL)
{
	// no texture
	m_iTextureID = -1;
	m_pHudTexture = new CHudTexture();
	m_pHudTexture->bRenderUsingFont = false;
}

/**
* Destructor
**/
CImagePanel::~CImagePanel()
{
	delete m_pHudTexture;
}

/**
* Gets the current texture
*
* @return int
**/
int CImagePanel::GetTextureID(void)
{
	return m_iTextureID;
}

/**
* Sends back the texture name
*
* @return const char *
**/
const char *CImagePanel::GetTextureName(void)
{
	return m_szTextureName;
}

/**
* Sets the texture to use
*
* @param const char *szTexture
* @return int The new texture id
**/
int CImagePanel::SetTexture(const char *szTexture)
{
	// did we already load that one?
	if(!Q_strcmp(szTexture, m_szTextureName))
		return m_iTextureID;

	return (m_pHudTexture->textureId = m_iTextureID = UTIL_LoadTexture(szTexture));
}

void CImagePanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_bDisplayCropped = true;
	if (m_iImageWidth == -1 && m_iImageHeight == -1)
		m_bDisplayCropped = false;
	else {
		m_iImageWidth = (m_iImageWidth == -1) ? GetWide() : m_iImageWidth;
		m_iImageHeight = (m_iImageHeight == -1) ? GetTall() : m_iImageHeight;
	}

	// do we already have a texture id?
	if(m_pHudTexture->textureId != -1)
		m_iTextureID = m_pHudTexture->textureId;
}

/**
* Paints the panel
*
* @return void
**/
void CImagePanel::Paint(void)
{
	// draw the label?
	BaseClass::Paint();

	vgui::surface()->DrawSetColor(m_BackgroundColor);
	vgui::surface()->DrawFilledRect(XRES(m_iImageWidth) * (1.0 - m_fBackgroundScaleX), 
									YRES(m_iImageHeight) * (1.0 - m_fBackgroundScaleY), 
									XRES(m_iImageWidth) * m_fBackgroundScaleX, 
									YRES(m_iImageHeight) * m_fBackgroundScaleY);

    // draw the texture
	m_pHudTexture->textureId = m_iTextureID;
	if (m_bDisplayCropped) {
		m_pHudTexture->SetBounds(0, XRES(m_iTextureWidth), 0, YRES(m_iTextureHeight));
		m_pHudTexture->DrawSelfCropped(0, 0, 0, 0, XRES(m_iImageWidth), YRES(m_iImageHeight), Color(0, 0, 0, 255));
	}
	else {
		m_pHudTexture->SetBounds(0, GetWide(), 0, GetTall());
		m_pHudTexture->DrawSelf(0, 0, GetWide(), GetTall(), Color(0, 0, 0, 255));
	}
}