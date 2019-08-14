#ifndef _OBJECTIVE_MAP_H_
#define _OBJECTIVE_MAP_H_

#include "map.h"
#include <vgui_controls/Panel.h>
#include "utlvector.h"
#include "hilitebutton.h"

#define OBJECTIVE_MAP_XPOS XRES(175)
#define OBJECTIVE_MAP_YPOS YRES(30)
#define OBJECTIVE_MAP_WIDTH XRES(200)
#define OBJECTIVE_MAP_HEIGHT YRES(200)

#define OBJECTIVE_MAP_BUTTON_WIDTH 16
#define OBJECTIVE_MAP_BUTTON_HEIGHT 16

/**
* Class declaration for an objective map
* Allows objectives to be selected by clicking on them in the map
**/
class CObjectiveMap : public CMap
{
public:
	DECLARE_CLASS_SIMPLE(CObjectiveMap, CMap);

	// constructor / destructor
	CObjectiveMap(vgui::Panel *pParent);
	~CObjectiveMap(void);

	// inherited methods
	virtual void SetMap(const char *szLevelName);

	// accessors
	CHiliteButton *SetButtonNeighbor(CHiliteButton *pButton, SObjective_t *pObj);
	void UpdateButtons(void);

protected:

	// helpers
	void CreateButtons(Panel *pParent);
	virtual void MapReset(void);
	virtual void TeamChanged(void);

private:
	CUtlVector<CHiliteImageButton *> m_aButtons;
};

#endif