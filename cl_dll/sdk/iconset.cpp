#include "cbase.h"
#include "iconset.h"
#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>

/**
* Constructor
**/
IconSet::IconSet()
{
	// no width/height
	m_iWidth = m_iHeight = 0;
	
	// start at zero, zero
	m_iXPos = m_iYPos = 0;

	// no active icon
	m_iActiveIcon = -1;
}

/**
* Destructor
**/
IconSet::~IconSet()
{
	// kill our texture list
	m_aIcons.PurgeAndDeleteElements();
}

/**
* Sets the size of all the icons in the set
*
* @param int iWidth
* @param int iHeight
* @return void
**/
void IconSet::SetIconSize(int iWidth, int iHeight)
{
	CHudTexture *pTexture;

	// set the size
	m_iWidth = iWidth;
	m_iHeight = iHeight;

	// set the bounds for each of our icons
	for(int i = 0; i < m_aIcons.Count(); ++i)
	{
		// pull the texture
		pTexture = GetTexture(i);
		if(!pTexture)
			continue;

		// set the bounds
		m_aIcons[i]->SetBounds(0, iWidth, 0, iHeight);
	}
}

/**
* Sets the position of the icon set
*
* @param int iXPos
* @param int iYPos
* @return void
**/
void IconSet::SetIconPos(int iXPos, int iYPos)
{
	// set them
	m_iXPos = iXPos;
	m_iYPos = iYPos;
}

/**
* Finds the texture by index
* 
* @param int iIndex The index of the element we want
* @return CHudTexture * or NULL
**/
CHudTexture *IconSet::GetTexture(int iIndex)
{
	// check the index
	if(!m_aIcons.IsValidIndex(iIndex))
		return NULL;

	return m_aIcons[iIndex];
}

/**
* Draws the correct icon from the set based on the current state of the objective
* 
* @return void
**/
void IconSet::DrawSelf(void)
{
	CHudTexture *pTexture;

	// pull the texture
	pTexture = GetTexture(m_iActiveIcon);
	if(!pTexture)
		return;

	// draw the texture
	pTexture->DrawSelf(m_iXPos, m_iYPos, Color(255, 255, 255, 255));
}

/**
* Adds the texture of the specified name to the set
*
* @param const char *szName Name of the texture to add
* @return void
**/
void IconSet::AddToSet(const char *szName)
{
	CHudTexture *pTexture;

	// create the new texture
	pTexture = new CHudTexture();

	// create the id and set the texture
	pTexture->textureId = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile(pTexture->textureId, szName, true, false);

	// add it
	AddToSet(pTexture);
}

/**
* Adds the specified texture to the set
*
* @param CHudTexture *pTexture The texture to add
* @return void
**/
void IconSet::AddToSet(CHudTexture *pTexture)
{
	// we can add null textures but we won't do anything to them
	if(pTexture)
	{
		// set the size
		pTexture->SetBounds(0, m_iWidth, 0, m_iHeight);

		// don't use a font
		pTexture->bRenderUsingFont = false;
	}

	// add it to our list
	m_aIcons.AddToTail(pTexture);

}