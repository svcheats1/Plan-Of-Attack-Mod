#ifndef _BUYMENUPURCHASEAREA_H_
#define _BUYMENUPURCHASEAREA_H_

#include <vgui_controls/Panel.h>
#include <vgui/KeyCode.h>
#include "buyableitem.h"
#include "hilitebutton.h"
#include "c_sdk_player.h"
#include <vgui/KeyCode.h>

#define PURCHASE_ITEM_TEXTURE_WIDTH 205
#define PURCHASE_ITEM_TEXTURE_HEIGHT 102
#define PURCHASE_ITEM_IMAGE_WIDTH 180
#define PURCHASE_ITEM_IMAGE_HEIGHT 85
#define PURCHASE_ITEM_IMAGE_ALPHA_X .99
#define PURCHASE_ITEM_IMAGE_ALPHA_Y .96
#define PURCHASE_ITEM_IMAGE_VSPACE 5
#define PURCHASE_ITEM_IMAGE_HSPACE 5

#define PURCHASE_AREA_XOFFSET 30
#define PURCHASE_AREA_YOFFSET 40

#define SKILL_BONUS "vgui/buymenu/skill_bonus"
#define SKILL_BONUS_ITEM_XOFFSET XRES(5)
#define SKILL_BONUS_ITEM_YOFFSET YRES(65)
#define SKILL_BONUS_WIDTH XRES(16)
#define SKILL_BONUS_HEIGHT YRES(16)

// forward declarations
class CBuyItemButton;

/**
* Class declaration for a buy menu purchase area in which the user may select the weapon
* they wish to purchase
**/
class CBuyMenuPurchaseArea : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(CBuyMenuPurchaseArea, vgui::Panel);

	// constructor / destructor
	CBuyMenuPurchaseArea(vgui::Panel *pParent, vgui::Panel *pRoot, BI_Type eType);
	~CBuyMenuPurchaseArea();

	// inherited methods
	virtual void Paint(void);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed(vgui::KeyCode code);

	// accessors
	bool AnyItemProvidesBonus(void);

protected:
	// helpers
	void CreatePurchaseArea(void);
	CBuyItemButton *CreateButton(IBuyableItem *pItem, Panel *pParent, int iCount);
	void PositionButton(CBuyItemButton *pButton, int iCount);
	void BuyItemByCode(vgui::KeyCode code, CUtlVector<CBuyItemButton *> *pButtons);
	void UpdateBonusIndicators(CUtlVector<CBuyItemButton *> *pButtons);
	void UpdateSkill(void);

private:
	BI_Type m_eType;

	CUtlVector<CBuyItemButton *> m_aTeamAButtons;
	CUtlVector<CBuyItemButton *> m_aTeamBButtons;

	int m_iCurrentTeam;
	SKILL_CLASS_INDEX m_eCurrentSkill;

	vgui::Panel *m_pRoot;
};

/**
* Class declaration for a button representing a buyable item
**/
class CBuyItemButton : public CHiliteImageButton
{
public:
	DECLARE_CLASS_SIMPLE(CBuyItemButton, CHiliteImageButton);

	/**
	* Constructor
	**/
	CBuyItemButton(vgui::Panel *pParent, IBuyableItem *pItem, const char *szTexture, int iHiliteType = 0)
		: BaseClass(pParent, "", szTexture, iHiliteType), m_pItem(pItem)
	{
		// load up our disabled overlay
		m_pDisabled = new CHudTexture();
		m_pDisabled->textureId = UTIL_LoadTexture("vgui/buymenu/weapons/weapon_disabled");
		m_pDisabled->bRenderUsingFont = false;
	}

	/**
	* Destructor
	**/
	~CBuyItemButton()
	{
		// kill the texture
		delete m_pDisabled;
	}

	/**
	* Accessor to the item this button represents
	*
	* @return IBuyableItem * The item this button represents
	**/
	IBuyableItem *GetItem(void)
	{
		return m_pItem;
	}

	/**
	* Paints the button
	*
	* @return void
	**/
	virtual void Paint(void)
	{
		CSDKPlayer *pPlayer;
		bool bCanBuyItem;

		// pull the player
		pPlayer = CSDKPlayer::GetLocalSDKPlayer();
		if(!pPlayer)
			return;

		// figure out if we can have the item
		bCanBuyItem = pPlayer->CanBuyItem(m_pItem);

		// if we don't have enough money for this item, disable it
		if(!bCanBuyItem)
			SetEnabled(false);
		// if we do have enough money, check that we are visible and then enable
		else if(IsVisible() && bCanBuyItem)
			SetEnabled(true);

		// jump down
		BaseClass::Paint();

		// draw the disabled overlay
		if(!IsEnabled())
		{
			// draw the disabled overlay
			m_pDisabled->SetBounds(0, XRES(205), 0, YRES(102));
			m_pDisabled->DrawSelfCropped(0, 0, 0, 0, XRES(180), YRES(86), Color(255, 255, 255, 255));
		}
	}
	
private:
	IBuyableItem *m_pItem;
	CHudTexture *m_pDisabled;
};

#endif