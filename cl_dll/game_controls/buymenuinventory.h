#ifndef _BUYMENUINVENTORY_H_
#define _BUYMENUINVENTORY_H_

#include <vgui_controls/EditablePanel.h>
#include <vgui/IScheme.h>
#include "imagepanel.h"
#include "c_sdk_player.h"

#define DISCARD_BUTTON_XOFFSET 5
#define DISCARD_BUTTON_YOFFSET 65
#define DISCARD_BUTTON_WIDTH 50
#define DISCARD_BUTTON_HEIGHT 16
#define DISCARD_BUTTON_IMAGE_WIDTH 100
#define DISCARD_BUTTON_IMAGE_HEIGHT 32

/**
* Class declaration for a panel representing the player's current inventory
**/
class CBuyMenuInventory : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE(CBuyMenuInventory, vgui::EditablePanel);

	// constructor / destructor
	CBuyMenuInventory(Panel *pParent, const char *szName);
	~CBuyMenuInventory();

	// helpers
	void Update(void);

	// inherited methods
	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void Paint(void);

protected:
	// inherited methods
	virtual Panel *CreateControlByName(const char *szControlName);

	// helpers
	void EstablishChildren(void);
	void UpdateWeaponTextures();
	bool NeedsUpdate(void);
	void UpdateDiscardButton(CImagePanel *pPanel, bool bEnable);

private:
	// cached stuff so we know when it changes
	CBaseCombatWeapon *m_pPrimaryWeapon;
	CBaseCombatWeapon *m_pSecondaryWeapon;
	CBaseCombatWeapon *m_pAccessoryWeapon;
	int m_iTeamNumber;
	int m_iCash;

	// our panels
	CImagePanel *m_pPrimary;
	CImagePanel *m_pSecondary;
	CImagePanel *m_pAccessory;
	vgui::Label *m_pCashLabel;
};

#endif