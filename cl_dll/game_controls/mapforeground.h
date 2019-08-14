#ifndef _MAPBACKGROUND_H_
#define _MAPBACKGROUND_H_

#include <vgui_controls/Panel.h>
#include "map.h"
#include <vgui/isurface.h>

// forward declarations
class CMap;

/**
* Class declaration for a map background
**/
class CMapForeground : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(CMapForeground, vgui::Panel);

	// constructor
	CMapForeground(vgui::Panel *pParent, const char *szName);

	// inherited methods
	virtual void Paint(void);

	// accessors
	virtual void Draw(void);

protected:

	CMap *m_pParent;
};

#endif