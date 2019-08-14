#include "cbase.h"
#include "buymenuoptions.h"
#include "hilitebutton.h"
#include "buyableitem.h"
#include "c_sdk_player.h"
#include "sdk_gamerules.h"
#include <vgui/ILocalize.h>
#include "igameuifuncs.h"
#include "imagepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
const char *CBuyMenuOptions::s_aLabelText[] = {	"#PURCHASE_PISTOLS",
												"#PURCHASE_SHOTGUNS",
												"#PURCHASE_SMGS",
												"#PURCHASE_RIFLES",
												"#PURCHASE_EXPLOSIVES" };

/**
* Constructor
**/
CBuyMenuOptions::CBuyMenuOptions(Panel *pParent, const char *szName)
	: BaseClass(pParent, szName)
{
	// load our settings
	LoadControlSettings("Resource/UI/BuyMenuOptions.res");

	// no purchase areas yet
	m_bPurchaseAreasCreated = false;
	m_pBuyTimeLabel = NULL;

	// no type selected
	m_eActiveType = BI_TYPE_NONE;
	m_iCategoryKey = -1;
}

/**
* Destructor
**/
CBuyMenuOptions::~CBuyMenuOptions()
{
	// kill our lists
	m_aAreas.Purge();
	m_aCategories.Purge();
}

/**
* Creates the area with all the buttons so you can buy stuff
* Best comment ever
*
* @return void
**/
void CBuyMenuOptions::CreatePurchaseAreas(void)
{
	CBuyMenuPurchaseArea *pArea;
	vgui::Panel *pParent;

	// find the parent
	pParent = FindChildByName("BuyMenuPurchaseArea");

	// bail if we didn't find it
	Assert(pParent);
	if(!pParent)
		return;
	
	// create the label
	m_pPurchaseLabel = new vgui::Label(pParent, "BuyMenuPurchaseAreaLabel", "");
	m_pPurchaseLabel->SetVisible(true);
	m_pPurchaseLabel->SetEnabled(true);
	m_pPurchaseLabel->SetSize(PURCHASE_LABEL_WIDTH, PURCHASE_LABEL_HEIGHT);

	// create a panel for each type
	for(int i = BI_TYPE_NONE + 1; i < BI_TYPE_COUNT; ++i)
	{
		// create the panel and add it to our list
		pArea = new CBuyMenuPurchaseArea(pParent, GetParent(), (BI_Type)i);
		m_aAreas.AddToTail(pArea);

		// turn it off
		pArea->SetVisible(false);
		pArea->SetEnabled(false);
	}

	// Find the buy time label... Why didn't we do this for PurchaseLabel ?
	m_pBuyTimeLabel = (vgui::Label*)FindChildByName("BuyTimeLabel");
}

/**
* Applies the scheme to the panel
*
* @param IScheme *pScheme The scheme to apply
* @return void
**/
void CBuyMenuOptions::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	CImagePanel *pPanel;

	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// show the background
	SetPaintBackgroundEnabled(true);
	SetBgColor(Color(0, 0, 0, 77));

	// create the purchase area
	if(!m_bPurchaseAreasCreated)
	{
		// create them
		CreatePurchaseAreas();
		m_bPurchaseAreasCreated = true;
	}

	// position our bonus icons
	for(int i = 0; i < m_aCategories.Count(); ++i)
	{
		// grab the button
		pPanel = (CImagePanel *)m_aCategories[i]->FindChildByName("SkillBonus");
		if(pPanel)
		{
			// size and position
			pPanel->SetSize(SKILL_BONUS_WIDTH, SKILL_BONUS_HEIGHT);
			pPanel->SetPos(SKILL_BONUS_OPTIONS_XOFFSET, SKILL_BONUS_OPTIONS_YOFFSET);
		}
	}

	// sort our buttons based on position leaving the cancel button at zero
	m_aCategories.Sort(CBuyMenuOptions::CategoryCompare);
}

/**
* Compares two hilite buttons for positioning
* 
* @param CHiliteImageButton *const *ppLeft The first element
* @param CHiliteImageButton *const *ppRight The second element
* @return int
**/
int CBuyMenuOptions::CategoryCompare(CHiliteImageButton * const *ppLeft, CHiliteImageButton * const *ppRight)
{
	int iLY, iRY, iJunk;

	// look for the cancel button
	if(strcmp((*ppLeft)->GetCommand(), "vguicancel") == 0)
		return -1;
	else if(strcmp((*ppLeft)->GetCommand(), "vguicancel") == 0)
		return 1;

	// get their positions
	(*ppLeft)->GetPos(iJunk, iLY);
	(*ppRight)->GetPos(iJunk, iRY);

	// compare positions
	if(iLY > iRY)
		return 1;
	else if(iLY < iRY)
		return -1;
	else
		return 0;
}

/**
* Creates a control of the given type
*
* @param const char *szControlName Name of the control to creat
* @return Panel * The control we created
**/
vgui::Panel *CBuyMenuOptions::CreateControlByName(const char *szControlName)
{
	// hilite button?
	if(!Q_strcmp(szControlName, "HiliteImageButton"))
	{
		CHiliteImageButton *pButton;
		CImagePanel *pPanel;

		// create the button and set the neighbor
		pButton = new CHiliteImageButton(this, "", "");
		pButton->SetNeighbor(this);

		// we want to handle this buttons commands
		pButton->AddActionSignalTarget(this);

		// hang on to the button
		m_aCategories.AddToTail(pButton);

		// add the bonus indicator
		pPanel = new CImagePanel(pButton, "SkillBonus");
		pPanel->SetTexture(SKILL_BONUS);
		pPanel->SetPos(SKILL_BONUS_OPTIONS_XOFFSET, SKILL_BONUS_OPTIONS_YOFFSET);
		pPanel->SetSize(SKILL_BONUS_WIDTH, SKILL_BONUS_HEIGHT);
		pPanel->SetVisible(false);
		pPanel->SetEnabled(false);

		return pButton;
	}
	else if(!Q_strcmp("ImagePanel", szControlName))
		return new CImagePanel(this, "");
	// hilite text button?
	if(!Q_strcmp(szControlName, "HiliteTextButton"))
		return new CHiliteTextButton(this, "", "");
	// anything else?
	else
		return BaseClass::CreateControlByName(szControlName);
}

/**
* Sets the active type
*
* @param BI_Type eActiveType The type of panel that is currently active
* @return void
**/
void CBuyMenuOptions::SetActiveType(BI_Type eActiveType)
{
	wchar_t *wszStr;
	bool bDeleteStr = false;
	char szStr[32];

	// set the category key
	if(eActiveType == BI_TYPE_NONE)
	{
		// no category
		m_iCategoryKey = -1;

		// turn off the hilted category
		if(m_aCategories.IsValidIndex(m_eActiveType))
			m_aCategories[m_eActiveType]->Hilite(false, this, HILITE_CONDITION_MOUSEUP);
	}
	else if(eActiveType > BI_TYPE_NONE && eActiveType < BI_TYPE_COUNT)
	{
		// figure out the slot and set it
		Q_snprintf(szStr, 32, "slot%d", eActiveType + 1);
		m_iCategoryKey = gameuifuncs->GetVGUI2KeyCodeForBind(szStr);
	}

	// did it change?
	if(eActiveType == m_eActiveType)
		return;

	// turn off the old one
	if(m_aAreas.IsValidIndex((int)m_eActiveType))
	{
		m_aAreas[(int)m_eActiveType]->SetVisible(false);
		m_aAreas[(int)m_eActiveType]->SetEnabled(false);
	}

	// switch categories
	m_eActiveType = eActiveType;

	// is it valid?
	if(!m_aAreas.IsValidIndex((int)eActiveType))
		return;

	// turn on the new one
	m_aAreas[(int)m_eActiveType]->SetVisible(true);
	m_aAreas[(int)m_eActiveType]->SetEnabled(true);

	// what will our label be?
	wszStr = vgui::localize()->Find(s_aLabelText[m_eActiveType]);

	// didn't get one, so convert whatever we have to unicode
	if(!wszStr)
	{
		// convert to unicode
		wszStr = new wchar_t[256];
		bDeleteStr = true;
		vgui::localize()->ConvertANSIToUnicode(s_aLabelText[m_eActiveType], wszStr, sizeof(wchar_t) * 256);
	}

	// set the text
	m_pPurchaseLabel->SetText(wszStr);

	// kill the message
	if(bDeleteStr)
		delete [] wszStr;
}

/**
* Determines what to do when we are hilited
*
* @param bool bState Hilite or unhilite?
* @param IHiliteable *pSender The guy who told us to hilite
* @param HiliteCondition eCondition The condition that caused us to hilite
* @return void
**/
void CBuyMenuOptions::Hilite(bool bState, IHiliteable *pSender, HiliteCondition eCondition)
{
	// if we weren't relayed a mouse down event we can ignore it
	if(!(eCondition & HILITE_CONDITION_MOUSEDOWN))
		return;

	// turn off all the buttons except the one that sent us the message
	for(int i = 0 ; i < m_aCategories.Count(); ++i)
	{
		// skip the one that sent us the message
		if(m_aCategories[i] != pSender)
			m_aCategories[i]->Hilite(false, this, HILITE_CONDITION_MOUSEUP);
	}
}

/**
* Updates the menu using the current skill class info
*
* @return void
**/
void CBuyMenuOptions::Update(void)
{
	CSDKPlayer *pPlayer;
	CImagePanel *pPanel;
	bool bProvidesBonus;

	// pull the player
	pPlayer = CSDKPlayer::GetLocalSDKPlayer();
	if(!pPlayer)
		return;

	// set the background color
	SetPaintBackgroundEnabled(true);
	if (pPlayer->GetTeamNumber() == TEAM_A)
		SetBgColor(Color(58, 45, 135, 77));
	else if (pPlayer->GetTeamNumber() == TEAM_B)
		SetBgColor(Color(100, 0, 0, 77));
	else
		SetBgColor(Color(0, 0, 0, 77));

	// go through our categories and update the buttons based on our class
	for(int i = 0; i < m_aCategories.Count(); ++i)
	{
		// check that we have a good index
		if(!m_aAreas.IsValidIndex(i) || !m_aAreas[i])
			continue;

		// get the panel
		pPanel = (CImagePanel *)m_aCategories[i]->FindChildByName("SkillBonus");

		// make sure we got it and update its state
		if(pPanel)
		{
			// does it provide a bonus?
			bProvidesBonus = m_aAreas[i]->AnyItemProvidesBonus();

			// turn it on/off
			pPanel->SetVisible(bProvidesBonus);
			pPanel->SetEnabled(bProvidesBonus);
		}
	}
}

/**
* Takes control when we receive a command
*
* @param const char *szCommand The command we need to handle
* @return void
**/
void CBuyMenuOptions::OnCommand(const char *szCommand)
{
	char *szSelection;

	// are we selecting?
	if(Q_strstr(szCommand, "select ") && Q_strlen(szCommand) > 7)
	{
		// bump to the correct spot
		szSelection = const_cast<char *>(szCommand) + 7;

		// what are we doing?
		if(!Q_strcmp(szSelection, "pistols"))
			SetActiveType(BI_TYPE_PISTOL);
		else if(!Q_strcmp(szSelection, "shotguns"))
			SetActiveType(BI_TYPE_SHOTGUN);
		else if(!Q_strcmp(szSelection, "smgs"))
			SetActiveType(BI_TYPE_SMG);
		else if(!Q_strcmp(szSelection, "rifles"))
			SetActiveType(BI_TYPE_RIFLE);
		else if(!Q_strcmp(szSelection, "explosives"))
			SetActiveType(BI_TYPE_EXPLOSIVES);
	}
	else if(!Q_strcmp(szCommand, "vguicancel") && gViewPortInterface)
		gViewPortInterface->ShowPanel(PANEL_BUY, false);

	// jump down
	BaseClass::OnCommand(szCommand);
}

/**
* Paints the options area of the menu
*
* @return void
**/
void CBuyMenuOptions::Paint()
{
	float fTime = ((CSDKGameRules*)g_pGameRules)->GetRemainingBuyTime();		

	if (gViewPortInterface && fTime < 0.0) {
		gViewPortInterface->ShowPanel(PANEL_BUY, false);
		return;
	}		

	// only change the text once per second
	int iTime = floor(fTime);
	if (m_pBuyTimeLabel && m_iBuySecondsLeft != iTime) {
		m_iBuySecondsLeft = iTime;

		char szTime[10];
		sprintf(szTime, "%d", iTime);
        m_pBuyTimeLabel->SetText(szTime);
	}

	BaseClass::Paint();
}

/**
* Handles keys being pressed
* 
* @return void
**/
void CBuyMenuOptions::OnKeyCodePressed(vgui::KeyCode code)
{
	char szStr[32];

	// are we reseting?
	// @TODO - fix the KEY_B thing!!!  GetVGUI...Bind returns KEY_PAD_2?!?!?
	// apparently valve will be releasing a fix for this in the next engine release
	if(code == gameuifuncs->GetVGUI2KeyCodeForBind("buymenu") || code == vgui::KEY_B)
	{
		// clear out the key
		SetActiveType(BI_TYPE_NONE);
		return;
	}

	// is it the zero key?
	if(code == gameuifuncs->GetVGUI2KeyCodeForBind("slot10") || code == gameuifuncs->GetVGUI2KeyCodeForBind("cancelselect"))
	{
		// abort
		SetActiveType(BI_TYPE_NONE);
		OnCommand("vguicancel");
		return;
	}

	// is this the first key we got?
	if(m_iCategoryKey == -1)
	{
		// set the category key
		SetCategoryKey(code);
		return;
	}

	// figure out which one we want
	for(int i = BI_TYPE_NONE + 1; i < BI_TYPE_COUNT; ++i)
	{
		// is this a real index?
		if(m_aAreas.IsValidIndex(i))
		{
			// is this the right one?
			Q_snprintf(szStr, 32, "slot%d", i + 1);
			if(m_iCategoryKey == gameuifuncs->GetVGUI2KeyCodeForBind(szStr))
			{
				// send the item key to the area
				m_aAreas[(int)i]->OnKeyCodePressed(code);

				// clear out the category key and go back to no active category
				SetActiveType(BI_TYPE_NONE);
			}
		}
	}
}

/**
* Sets the current category key after confirming it to be a real category
*
* @param KeyCode code The key that should become the category key
* @return void
**/
void CBuyMenuOptions::SetCategoryKey(vgui::KeyCode code)
{
	char szStr[32];

	// figure out which one we want
	for(int i = BI_TYPE_NONE + 1; i < BI_TYPE_COUNT; ++i)
	{
		// is this the right one?
		Q_snprintf(szStr, 32, "slot%d", i + 1);
		if(code == gameuifuncs->GetVGUI2KeyCodeForBind(szStr))
		{
			// tell the category we got a mouse down event
			if(m_aCategories.IsValidIndex(i))
				m_aCategories[i]->Hilite(true, NULL, HILITE_CONDITION_MOUSEDOWN);

			// store the category and set the active panel
			SetActiveType((BI_Type)i);
		}
	}
}