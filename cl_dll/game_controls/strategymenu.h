#ifndef _STRATEGYMENU_H_
#define _STRATEGYMENU_H_
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <cl_dll/iviewport.h>
#include "map.h"
#include "hilitebutton.h"
#include "strategymap.h"
#include <KeyValues.h>

#define AUTO_CHOOSE_STRATEGY_TIME 0.5

class CStrategyMenu : public vgui::Frame, public IViewPortPanel, public IGameEventListener2
{
	DECLARE_CLASS_SIMPLE(CStrategyMenu, vgui::Frame);

public:
	CStrategyMenu(IViewPort *pViewport);
	~CStrategyMenu();
	static void Term();

	void ShowPanel(bool bShow);

	// new functions
	void StrategyChosen(CStrategyMap* strategy);
	void Hilite(CStrategyMap* pStrategy, bool bEnable);

	// mutators
	void SetTimer(float fTime) { m_fCloseTime = gpGlobals->curtime + fTime - AUTO_CHOOSE_STRATEGY_TIME; }
	void SetOffensive(bool bOffensive) { m_bOffensive = bOffensive; }

	// overwritten functions
	virtual void FireGameEvent(IGameEvent *pEvent);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual Panel *CreateControlByName(const char *szControlName);
	virtual void OnCommand(const char *command);
	virtual void Paint(void);

	// functions we must implement, but we don't really want to -> inline
	virtual const char *GetName( void ) { return PANEL_STRATEGY; }
	virtual void SetData(KeyValues *data) {};
	virtual void Update() {};
	virtual void Reset() {};
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

private:
	bool MatchesGameState(KeyValues *keyValues);
	void PreparePanels(bool bShow);
	void PurgeVectors();
	void UpdateStrategies();
	void UpdateGameStateFilter();
	static int RandomCompare(CStrategyMap* const *ppLeft, CStrategyMap* const *ppRight);

	float m_fCloseTime;
	int m_iCurrentTimer, m_iPage;
	bool m_bOffensive;

	IViewPort *m_pViewPort;
	vgui::Label *m_pTimerLabel;
	CUtlVector<vgui::Label*> m_aLabels;
	CUtlVector<CStrategyMap*> m_aStrategies;
	CUtlVector<CStrategyMap*> m_aPassesFilter;

	// items we need to hold onto if we change resolutions
	static char *m_szMapName;
};

#endif