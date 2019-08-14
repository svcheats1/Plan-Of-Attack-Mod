#ifndef _BUYMENUOPTIONS_H_
#define _BUYMENUOPTIONS_H_

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui/IScheme.h>
#include <cl_dll/iviewport.h>
#include "utlvector.h"
#include "buymenupurchasearea.h"
#include <vgui/KeyCode.h>

#define PURCHASE_LABEL_WIDTH XRES(200)
#define PURCHASE_LABEL_HEIGHT YRES(30)

#define SKILL_BONUS_OPTIONS_XOFFSET XRES(110)
#define SKILL_BONUS_OPTIONS_YOFFSET YRES(0)

/**
* Class declaration for the buy menu options panel
**/
class CBuyMenuOptions : public vgui::EditablePanel, public IHiliteable
{
public:
	DECLARE_CLASS_SIMPLE(CBuyMenuOptions, vgui::EditablePanel);

	// constructor / destructor
	CBuyMenuOptions(Panel *pParent, const char *szName);
	~CBuyMenuOptions();

	// inherited methods
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnCommand(const char *szCommand);
	virtual void Update(void);
	virtual void Paint(void);
	virtual void OnKeyCodePressed(vgui::KeyCode code);

	// accessors
	void SetActiveType(BI_Type eActiveType);

protected:
	// inherited methods
	virtual Panel *CreateControlByName(const char *szControlName);
	virtual void Hilite(bool bState, IHiliteable *pSender, HiliteCondition eCondition);
	virtual bool ShouldHilite(void) { return true; }

	// helpers
	void CreatePurchaseAreas(void);
	void SetCategoryKey(vgui::KeyCode code);
	static int CategoryCompare(CHiliteImageButton * const *ppLeft, CHiliteImageButton * const *ppRight);

private:
	CUtlVector<CBuyMenuPurchaseArea *> m_aAreas;
	CUtlVector<CHiliteImageButton *> m_aCategories;
	vgui::Label *m_pPurchaseLabel;
	vgui::Label *m_pBuyTimeLabel;
	bool m_bPurchaseAreasCreated;
	int m_iBuySecondsLeft;
	static const char *s_aLabelText[];

	BI_Type m_eActiveType;
	int m_iCategoryKey;
};

extern IViewPort *gViewPortInterface;

#endif