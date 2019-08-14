#ifndef _COBJECTIVEMENU_H_
#define _COBJECTIVEMENU_H_

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui/ILocalize.h>
#include <cl_dll/iviewport.h>
#include <UtlVector.h>
#include "c_objectivemanager.h"
#include "objectivemap.h"

#define OBJECTIVE_MENU_XOFFSET XRES(-270) + (ScreenWidth() / 2)
#define OBJECTIVE_MENU_YOFFSET YRES(50)
#define OBJECTIVE_MENU_WIDTH XRES(540)
#define OBJECTIVE_MENU_HEIGHT YRES(380)

#define OBJECTIVE_PANEL_WIDTH XRES(120)
#define OBJECTIVE_PANEL_HEIGHT YRES(120)
#define OBJECTIVE_PANEL_HSPACE YRES(10)
#define OBJECTIVE_PANEL_VOFFSET YRES(250)
#define OBJECTIVE_PANEL_HOFFSET XRES(15)

#define OBJECTIVE_BUTTON_WIDTH 110
#define OBJECTIVE_BUTTON_HEIGHT 70
#define OBJECTIVE_BUTTON_VOFFSET YRES(5)
#define OBJECTIVE_BUTTON_HOFFSET XRES(5)
#define OBJECTIVE_BUTTON_IMAGE_X_CROP 128
#define OBJECTIVE_BUTTON_IMAGE_Y_CROP 78
#define OBJECTIVE_BUTTON_SIZE 128

#define OBJECTIVE_LABEL_WIDTH XRES(100)
#define OBJECTIVE_LABEL_HEIGHT YRES(35)
#define OBJECTIVE_LABEL_VOFFSET YRES(80)
#define OBJECTIVE_LABEL_HOFFSET XRES(5)

#define OBJECTIVE_TIMER_ICON_XOFFSET XRES(520)
#define OBJECTIVE_TIMER_ICON_YOFFSET YRES(2)
#define OBJECTIVE_TIMER_ICON_WIDTH XRES(16)
#define OBJECTIVE_TIMER_ICON_HEIGHT YRES(16)

#define OBJECTIVE_TIMER_LABEL_XOFFSET XRES(475)
#define OBJECTIVE_TIMER_LABEL_YOFFSET YRES(-1)
#define OBJECTIVE_TIMER_LABEL_WIDTH XRES(40)
#define OBJECTIVE_TIMER_LABEL_HEIGHT YRES(24)

#define MAP_NAME_LENGTH 256
#define AUTO_CHOOSE_OBJECTIVE_TIME 1.0

using namespace vgui;

/**
* Class declaration for the objectives menu
**/
class CObjectiveMenu : public vgui::Frame, public IViewPortPanel, public IGameEventListener2
{
DECLARE_CLASS_SIMPLE( CObjectiveMenu, vgui::Frame );

public:
	// constructors and destructors
	CObjectiveMenu(IViewPort *pViewPort);
	~CObjectiveMenu(void);

	// inherited
	virtual void ShowPanel(bool bShow);
	void OnCommand(const char *szCommand);
	virtual void FireGameEvent(IGameEvent *pData);

	// inherited pure virtuals that i don't care about
	virtual const char *GetName( void ) { return PANEL_OBJECTIVES; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset();
	virtual void Update() {};
	virtual void SetTimer(float fTime) { m_fEndTimer = gpGlobals->curtime + fTime - AUTO_CHOOSE_OBJECTIVE_TIME; }
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
	virtual void Paint();
	vgui::Panel *CreateControlByName(const char *szControlName);
	void ApplySchemeSettings(vgui::IScheme *pScheme);

protected:

	// panel creation and fixing
	Panel *CreatePanel(const SObjective_t *pObj);
	void FixDirtyPanels(void);
	void SetButtonTexture(Panel *pPanel, SObjective_t *pObj);
	void ChangeMap(void);
	void UpdateChildButton(Panel *pPanel, SObjective_t *pObj);
	void UpdateObjectiveLabelText(vgui::Label *pLabel, const SObjective_t *pObj);

private:
	// state info
	char m_szMapName[MAP_NAME_LENGTH];
	bool m_bMapChanged;
	float m_fEndTimer;
	int m_iCurrentTimer;
	vgui::Label* m_pTimerLabel;

	// view info
	IViewPort	*m_pViewPort;
	CUtlVector<Panel *> m_aPanels;
	CObjectiveMap *m_pMapPanel;
};

#endif