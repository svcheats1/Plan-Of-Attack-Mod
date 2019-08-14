#ifndef _STRATEGYEDITORSAVEOPEN_H_
#define _STRATEGYEDITORSAVEOPEN_H_

#include <vgui_controls/FileOpenDialog.h>
#include "IGameEvents.h"

#define STRATEGY_DIR "strategies"

/**
* Enum for dialog types
**/
enum SOType
{
	SO_SAVE = 0,
	SO_OPEN,
};

/**
* Class declaraiton for our save dialog for the strategy editor
**/
class CStrategyEditorSaveOpen : public vgui::FileOpenDialog, public IGameEventListener2
{
public:
	DECLARE_CLASS_SIMPLE(CStrategyEditorSaveOpen, vgui::FileOpenDialog);

	// constructor
	CStrategyEditorSaveOpen(Panel *parent, const char *title, bool bOpenOnly);
	~CStrategyEditorSaveOpen();

	// inherited methods
	virtual void OnOpen(void);
	virtual void FireGameEvent(IGameEvent *pEvent);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	
	// accessors
	void SetType(SOType eType);
	void SetMap(const char *szMap);

private:
	char *m_szMapName;
	SOType m_eType;
};

#endif