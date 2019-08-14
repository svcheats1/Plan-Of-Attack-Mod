#ifndef _IMAGEPANEL_H_
#define _IMAGEPANEL_H_

#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
using namespace vgui;

/**
* Class declaration for a PANEL that simply displays an image
**/
class CImagePanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(CImagePanel, vgui::Panel);

	// constructor / destructor
	CImagePanel(Panel *pParent, const char *szName);
	~CImagePanel();

	// inherited methods
	virtual void Paint(void);
	virtual void ApplySchemeSettings(IScheme *pScheme);

	// accessors
	int SetTexture(const char *szTexture);
	int GetTextureID(void);
	const char *GetTextureName(void);

protected:
	CPanelAnimationVarAliasType(int, m_iTextureID, "Texture", "", "textureid");
	CPanelAnimationVar(int, m_iImageWidth, "imageWidth", "-1");
	CPanelAnimationVar(int, m_iImageHeight, "imageHeight", "-1");
	CPanelAnimationVar(int, m_iTextureWidth, "textureWidth", "200");
	CPanelAnimationVar(int, m_iTextureHeight, "textureHeight", "150");
	CPanelAnimationVar(float, m_fBackgroundScaleX, "imageBackgroundScaleX", "0.99");
	CPanelAnimationVar(float, m_fBackgroundScaleY, "imageBackgroundScaleY", "0.95");
	CPanelAnimationVar(Color, m_BackgroundColor, "backgroundColor", "255 255 255 255");

	bool m_bDisplayCropped;
	char m_szTextureName[256];
	CHudTexture *m_pHudTexture;
};

#endif