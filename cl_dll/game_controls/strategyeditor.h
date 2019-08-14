#ifndef _STRATEGYEDITOR_H_
#define _STRATEGYEDITOR_H_

#include <vgui_controls/Frame.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>
#include "strategyeditormap.h"
#include "strategyeditorsaveopen.h"

#define STRATEGY_EDITOR_WIDTH XRES(620)
#define STRATEGY_EDITOR_HEIGHT YRES(460)

/**
* Class declaration for a strategy editor
**/
class CStrategyEditor : public IViewPortPanel, public vgui::Frame, public IHiliteable
{
public:
	DECLARE_CLASS_SIMPLE(CStrategyEditor, vgui::Frame);

	// constructor / destructor
	CStrategyEditor(IViewPort *pViewPort);
	~CStrategyEditor();

	// inherited methods
	const char *GetName(void) { return PANEL_STRATEGYEDITOR; }
	virtual void Paint(void);
	virtual Panel *CreateControlByName(const char *szControlName);
	virtual void SetData(KeyValues *data) {};
	virtual void Reset(void);
	virtual void Update(void);
	virtual bool NeedsUpdate(void);
	virtual bool HasInputElements(void) { return true; }
	virtual void ShowPanel(bool bShow);
	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible(void) { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnCommand(const char *szCommand);
	virtual void Hilite(bool bState, IHiliteable *pSender, HiliteCondition eCondition) {};
	virtual bool ShouldHilite(void) { return false; }
	virtual void OnThink(void);
	virtual void Activate(void);

	// accessors
	void Save(void);
	void SaveToFile(const char *szFile, const char *szDir);
	void Load(void);
	void LoadFromFile(const char *szFile, const char *szDir);

protected:
	// helpers
	void EstablishComponents(void);
	void RotateMapForTeam(int iTeam);
	void AddDataFromCombos(KeyValues *pValues);
	void ActivateObjectiveComboRow(vgui::ComboBox *pBox, char cItem);
	void LoadMaps(void);

private:

	char m_szLastTeam[64];
	char m_szLastMap[64];
	IViewPort *m_pViewPort;
	vgui::IScheme *m_pScheme;
	CStrategyEditorSaveOpen *m_pOpenSaveDialog;
	CStrategyEditorMap *m_pMap;
	vgui::ComboBox *m_pMapCombo;
	CHiliteTextButton *m_pArrow1Button;
	CHiliteTextButton *m_pArrow2Button;
	CHiliteTextButton *m_pArrow3Button;
	vgui::TextEntry *m_pStrategyNameText;
	vgui::ComboBox *m_pTeamCombo;
	vgui::ComboBox *m_pTypeCombo;
	vgui::ComboBox *m_pTargetCombo;
	vgui::ComboBox *m_pObjective1Combo;
	vgui::ComboBox *m_pObjective2Combo;
	vgui::ComboBox *m_pObjective3Combo;
	KeyValues *m_pLastFileKeyValues;
	CUtlVector<vgui::Label *> m_aLabels;
};

#endif