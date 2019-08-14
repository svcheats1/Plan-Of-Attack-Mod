#include "cbase.h"
#include "buymenupurchasearea.h"
#include "buymenu.h"
#include "c_sdk_player.h"
#include <vgui/IPanel.h>
#include <vgui/mousecode.h>
#include "igameuifuncs.h"
#include "imagepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Constructor
**/
CBuyMenuPurchaseArea::CBuyMenuPurchaseArea(vgui::Panel *pParent, vgui::Panel *pRoot, BI_Type eType)
	: BaseClass(pParent)
{
	// store the type
	m_eType = eType;

	// store the root
	m_pRoot = pRoot;

	// no team, no skill
	m_iCurrentTeam = -1;
	m_eCurrentSkill = NONE_CLASS_INDEX;

	// create our panels
	// @NOTE - we should do this last in the constructor
	CreatePurchaseArea();
}

/**
* Destructor
**/
CBuyMenuPurchaseArea::~CBuyMenuPurchaseArea()
{
	// kill our lists
	m_aTeamAButtons.Purge();
	m_aTeamBButtons.Purge();
}

/**
* Creates the purchase area including the buttons
*
* @return void
**/
void CBuyMenuPurchaseArea::CreatePurchaseArea(void)
{
	CUtlVector<IBuyableItem *> *pItems;

	// make sure we have a valid type
	if(m_eType <= BI_TYPE_NONE || m_eType >= BI_TYPE_COUNT)
	{
		Assert(0);
		return;
	}

	// get the items we need to add
	pItems = CBuyMenu::GetPurchaseItems(m_eType);

	// sort based on position
	pItems->Sort(IBuyableItem::Compare);

	// add each item
	for(int i = 0; i < pItems->Count(); ++i)
	{
		IBuyableItem *pItem = (*pItems)[i];
		if(pItem->GetBuyableTeam() == BI_TEAM_BOTH || pItem->GetBuyableTeam() == BI_TEAM_A)
			m_aTeamAButtons.AddToTail(CreateButton(pItem, this, m_aTeamAButtons.Count()));
		if(pItem->GetBuyableTeam() == BI_TEAM_BOTH || pItem->GetBuyableTeam() == BI_TEAM_B)
			m_aTeamBButtons.AddToTail(CreateButton(pItem, this, m_aTeamBButtons.Count()));
	}
}

/**
* Creates a new hilite button for a given item
*
* @param IBuyableItem *pItem The item to create the button for
* @param Panel *pParent The parent to use for the new button
* @param int iCount The number of buttons created so far
* @return CBuyItemButton * The new button
**/
CBuyItemButton *CBuyMenuPurchaseArea::CreateButton(IBuyableItem *pItem, Panel *pParent, int iCount)
{
	CBuyItemButton *pButton;
	char szTexture[256], szStr[256];
	CImagePanel *pImage;

	// create the name of the texture
	sprintf(szTexture, "vgui/buymenu/weapons/%s", pItem->GetClassType());

	// create the new button
	pButton = new CBuyItemButton(pParent, pItem, szTexture, HILITE_IMAGE_BUTTON_ALPHA);

	// set the hilite info
	pButton->SetHiliteScale(PURCHASE_ITEM_IMAGE_ALPHA_X, PURCHASE_ITEM_IMAGE_ALPHA_Y);

	// right now they are off
	pButton->SetVisible(false);
	pButton->SetEnabled(false);

	// set the command
	Q_snprintf(szStr, sizeof(szStr), "buy %s", pItem->GetClassType());
	pButton->SetCommand(szStr);

	// add the handler as the buy menu
	pButton->AddActionSignalTarget(m_pRoot);

	// add the bonus indicator
	pImage = new CImagePanel(pButton, "SkillBonus");
	pImage->SetTexture(SKILL_BONUS);
	pImage->SetPos(SKILL_BONUS_ITEM_XOFFSET, SKILL_BONUS_ITEM_YOFFSET);
	pImage->SetSize(SKILL_BONUS_WIDTH, SKILL_BONUS_HEIGHT);
	pImage->SetEnabled(false);
	pImage->SetVisible(false);

	return pButton;
}

/**
* Applies the scheme to the panel
*
* @param vgui::IScheme *pScheme The scheme to use
* @return void
**/
void CBuyMenuPurchaseArea::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	int i;

	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// make sure we have a parent
	Assert(GetParent());
	if(!GetParent())
		return;

	// set our size
	SetSize(GetParent()->GetWide(), GetParent()->GetTall());
	SetPos(XRES(PURCHASE_AREA_XOFFSET), YRES(PURCHASE_AREA_YOFFSET));

	// set all the button positions
	for(i = 0; i < m_aTeamAButtons.Count(); ++i)
		PositionButton(m_aTeamAButtons[i], i);
	for(i = 0; i < m_aTeamBButtons.Count(); ++i)
		PositionButton(m_aTeamBButtons[i], i);
}

/**
* Positions one of the buttons
*
* @param CBuyItemButton *pButton The button to position
* @param int iCount The number of buttons positioned
* @return void
**/
void CBuyMenuPurchaseArea::PositionButton(CBuyItemButton *pButton, int iCount)
{
	CImagePanel *pPanel;

	// position it
	if(iCount % 2 == 0)
		pButton->SetPos(0, (iCount / 2) * (YRES(PURCHASE_ITEM_IMAGE_HEIGHT) + YRES(PURCHASE_ITEM_IMAGE_VSPACE)));
	else
		pButton->SetPos(XRES(PURCHASE_ITEM_IMAGE_WIDTH) + XRES(PURCHASE_ITEM_IMAGE_HSPACE), 
						(iCount / 2) * (YRES(PURCHASE_ITEM_IMAGE_HEIGHT) + YRES(PURCHASE_ITEM_IMAGE_VSPACE)));

	// set the size
	pButton->SetSize(XRES(PURCHASE_ITEM_IMAGE_WIDTH), YRES(PURCHASE_ITEM_IMAGE_HEIGHT));
	pButton->SetTextureSize(PURCHASE_ITEM_TEXTURE_WIDTH, PURCHASE_ITEM_TEXTURE_HEIGHT);
	pButton->SetImageCrop(PURCHASE_ITEM_IMAGE_WIDTH, PURCHASE_ITEM_IMAGE_HEIGHT);

	// find the bonus button and set its positioning stuff
	pPanel = (CImagePanel *)pButton->FindChildByName("SkillBonus");
	if(pPanel)
	{
		// size and position
		pPanel->SetPos(SKILL_BONUS_ITEM_XOFFSET, SKILL_BONUS_ITEM_YOFFSET);
		pPanel->SetSize(SKILL_BONUS_WIDTH, SKILL_BONUS_HEIGHT);
	}
}

/**
* Paints the purchase area
* 
* @return void
**/
void CBuyMenuPurchaseArea::Paint(void)
{
	bool bIsTeamA;

	// make sure we have a player
	if(!CBasePlayer::GetLocalPlayer())
		return;

	// check to see if we switched teams
	if(m_iCurrentTeam != CBasePlayer::GetLocalPlayer()->GetTeamNumber())
	{
		// what team are we on?
		bIsTeamA = CBasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_A;

		// switch the state of the buttons based on team and current cash
		for(int i = 0; i < m_aTeamAButtons.Count(); ++i)
		{
			// turn it on
			m_aTeamAButtons[i]->SetVisible(bIsTeamA);
			m_aTeamAButtons[i]->SetEnabled(bIsTeamA);
		}
		for(int i = 0; i < m_aTeamBButtons.Count(); ++i)
		{
			// turn it on
			m_aTeamBButtons[i]->SetVisible(!bIsTeamA);
			m_aTeamBButtons[i]->SetEnabled(!bIsTeamA);
		}
		
		// store the team
		m_iCurrentTeam = CBasePlayer::GetLocalPlayer()->GetTeamNumber();
	}

	// jump down
	BaseClass::Paint();
}

/**
* Updates our information regarding the player's skill
*
* @return void
**/
void CBuyMenuPurchaseArea::UpdateSkill(void)
{
	CSDKPlayer *pPlayer;

	// check that we have a player
	if(!CBasePlayer::GetLocalPlayer())
		return;

	// if we don't know the player's skill or it isn't the same as their old one
	pPlayer = ToSDKPlayer(CBasePlayer::GetLocalPlayer());
	if(pPlayer && pPlayer->GetSkillClass() && 
		(m_eCurrentSkill == NONE_CLASS_INDEX || pPlayer->GetSkillClass()->GetClassIndex() != m_eCurrentSkill))
	{
		// switch the skill
		m_eCurrentSkill = pPlayer->GetSkillClass()->GetClassIndex();

		// change the bonus indicators
		UpdateBonusIndicators(&m_aTeamAButtons);
		UpdateBonusIndicators(&m_aTeamBButtons);
	}
}

/**
* Updates the bonus indicators for a given set of buttons
*
* @param CUtlVector<CBuyItemButton *> *pButtons The buttons whose bonus indicators to update
* @return void
**/
void CBuyMenuPurchaseArea::UpdateBonusIndicators(CUtlVector<CBuyItemButton *> *pButtons)
{
	CImagePanel *pImage;
	bool bProvidesBonus;
	const CSkillClass *pSkill;

	// go through all the buttons
	for(int i = 0; i < pButtons->Count(); ++i)
	{
		// pull the child
		pImage = (CImagePanel *)((*pButtons)[i]->FindChildByName("SkillBonus"));

		// did we get it?
		if(pImage)
		{
			// do we get a bonus for this item?
			pSkill = CSkillClass::GetSkillClassModel(m_eCurrentSkill);
			if(!pSkill)
				continue;

			bProvidesBonus = pSkill->ProvidesBonus((*pButtons)[i]->GetItem());
				
			// switch their state
			pImage->SetEnabled(bProvidesBonus);
			pImage->SetVisible(bProvidesBonus);
		}
	}
}

/**
* Determines if any of the items in this area provide a bonus
* @NOTE - this assumes the team's weapons are "balanced"
*
* @return bool
**/
bool CBuyMenuPurchaseArea::AnyItemProvidesBonus(void)
{	
	const CSkillClass *pSkill;

	// update our skill info
	UpdateSkill();

	// flip through all the buttons and check
	pSkill = CSkillClass::GetSkillClassModel(m_eCurrentSkill);
	if(pSkill)
	{
		for(int i = 0; i < m_aTeamAButtons.Count(); ++i)
		{
			if(pSkill->ProvidesBonus(m_aTeamAButtons[i]->GetItem()))
				return true;
		}
		for(int i = 0; i < m_aTeamBButtons.Count(); ++i)
		{
			if(pSkill->ProvidesBonus(m_aTeamBButtons[i]->GetItem()))
				return true;
		}
	}

	return false;
}

/**
* Handles a key being pressed
*
* @param KeyCode code The key that was pressed
* @return void
**/
void CBuyMenuPurchaseArea::OnKeyCodePressed(KeyCode code)
{
	// check that we have a player, then buy the item
	if(CBasePlayer::GetLocalPlayer())
		BuyItemByCode(code, (CBasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_A
								? &m_aTeamAButtons
								: &m_aTeamBButtons));
}

/**
* Buys the given item corresponding to the key code
*
* @param vgui::KeyCode code The code specifying the item to buy
* @param CUtlVector<CBuyItemButton *> *pButtons The buttons to select the item from
* @return void
**/
void CBuyMenuPurchaseArea::BuyItemByCode(vgui::KeyCode code, CUtlVector<CBuyItemButton *> *pButtons)
{
	char szStr[256];

	// go through the buttons
	for(int i = 0; i < pButtons->Count(); ++i)
	{
		// create the string
		Q_snprintf(szStr, sizeof(szStr), "slot%d", i + 1);

		// is this the key code? fake the mouse click
		if(code == gameuifuncs->GetVGUI2KeyCodeForBind(szStr) && pButtons->IsValidIndex(i) && (*pButtons)[i])
			((*pButtons)[i])->FireActionSignal();
	}
}