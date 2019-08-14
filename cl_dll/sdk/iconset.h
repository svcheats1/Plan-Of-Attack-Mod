#ifndef _ICONSET_H_
#define _ICONSET_H_

#include "hud.h"

/**
* Class declaration for an icon set
* Stores a set of CHudTextures
**/
class IconSet
{
public:
	// constructor / destructor
	IconSet();
	~IconSet();

	// accessors
	virtual void DrawSelf(void);
	virtual void AddToSet(const char *szName);
	virtual void AddToSet(CHudTexture *pTexture);

	// positioning/sizing
	virtual void SetIconSize(int iWidth, int iHeight);
	virtual void SetIconPos(int iXPos, int iYPos);

protected:
	// helpers
	CHudTexture *GetTexture(int iIndex);

	// state
	int m_iActiveIcon;

	// icon positioning and sizing
	int m_iXPos;
	int m_iYPos;
	int m_iWidth;
	int m_iHeight;

private:
	// icons
	CUtlVector<CHudTexture *> m_aIcons;
};

#endif