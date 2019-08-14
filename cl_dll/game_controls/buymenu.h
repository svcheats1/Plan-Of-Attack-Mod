#ifndef _BUYMENU_H_
#define _BUYMENU_H_

#include <cl_dll/iviewport.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include "buyableitem.h"
#include "hud_element_helper.h"
#include "buymenuinventory.h"
#include "buymenuoptions.h"

//-----------------------------------------------------------------------------
// Purpose: Draws the class menu
//-----------------------------------------------------------------------------
class CBuyMenu : public vgui::Frame, public IViewPortPanel
{
public:
	DECLARE_CLASS_SIMPLE(CBuyMenu, vgui::Frame);

	// constructor / destructor
	CBuyMenu(IViewPort *pViewPort);
	~CBuyMenu();

	// inherited methods
	virtual const char *GetName( void ) { return PANEL_BUY; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update(void);
	virtual bool NeedsUpdate(void);
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnCommand(const char *szCommand);
	virtual void OnKeyCodePressed(KeyCode code);
	virtual void OnClose(void);

	// accessors
	static CUtlVector<IBuyableItem *> *GetPurchaseItems(BI_Type eType);

protected:
	// inherited methods
	virtual Panel *CreateControlByName(const char *szControlName);

	IViewPort	*m_pViewPort;

	// child panels
	CBuyMenuInventory *m_pInventory;
	CBuyMenuOptions *m_pOptions;

	// skill info
	SKILL_CLASS_INDEX m_eCurrentSkill;
};

/**
* Helper class for adding item to the buy menu
**/
class CBuyMenuHelper
{
public:
	friend class CBuyMenu;

	// constructor/destructor
	CBuyMenuHelper(IBuyableItem *pItem);
	~CBuyMenuHelper(void);

	// singleton
	static CBuyMenuHelper *GetInstance(void);
	static void Term(void);

protected:
	CUtlVector<IBuyableItem *> *GetItems(BI_Type eType);

private:
	// default constructor and destructor
	CBuyMenuHelper(void);

	void AddBuyableItem(IBuyableItem *pItem);
	void CreateCategorizedItems(void);

	static CBuyMenuHelper *s_pInstance;
	CUtlVector<IBuyableItem *> **m_pCategorizedItems;
	CUtlVector<IBuyableItem *> m_pItems;
};

// macro for adding an item to the buy menu
#define ADDBUYABLEITEM(className) static CBuyMenuHelper BuyMenuHelper_##className(new className())

#endif // BUYMENU_H
