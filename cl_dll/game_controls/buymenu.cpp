//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "buymenu.h"
#include "igameuifuncs.h"

using namespace vgui;

#include "mouseoverpanelbutton.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
CBuyMenuHelper *CBuyMenuHelper::s_pInstance = NULL;

/**
* Constructor
**/
CBuyMenu::CBuyMenu(IViewPort *pViewPort) 
	: Frame(NULL, PANEL_BUY)
{
	// set the scheme and title
	SetScheme("ClientScheme");
	SetTitle("#Buy_Menu", true);

	// setup our panel
	SetMoveable(false);
	SetSizeable(false);
	SetProportional(true);
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(1);

	// hide the system buttons
	SetTitleBarVisible(false);
	
	// never delete us
	SetDeleteSelfOnClose(false);

	// load our settings
	LoadControlSettings("Resource/UI/BuyMenu.res");

	// store the viewport
	m_pViewPort = pViewPort;
	
	// no children yet
	m_pOptions = NULL;
	m_pInventory = NULL;

	// no skill
	m_eCurrentSkill = NONE_CLASS_INDEX;
}


/**
* Destructor
**/
CBuyMenu::~CBuyMenu()
{
	// ?
}

/**
* Applies the current scheme
*
* @param vgui::IScheme *pScheme The scheme we need to use
* @return void
**/
void CBuyMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// set us to the size of the screen
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	SetSize(wide, tall);

	// find our panels
	m_pInventory = (CBuyMenuInventory *)FindChildByName("BuyMenuInventory");
	m_pOptions = (CBuyMenuOptions *)FindChildByName("BuyMenuOptions");

	// update them
	if (m_pOptions)
		m_pOptions->Update();
}

/**
* Hides or displays the panel
*
* @param bool bShow Hiding or displaying?
* @return void
**/
void CBuyMenu::ShowPanel(bool bShow)
{
	// don't bother if we're already displayed
	if(BaseClass::IsVisible() == bShow)
		return;

	// hiding or showing?
	if(bShow)
	{	
		// update
		Update();

		// show us
		SetVisible(true);
		SetMouseInputEnabled(true);
		SetKeyBoardInputEnabled(true);

		if (m_pOptions)
			m_pOptions->SetActiveType(BI_TYPE_NONE);
	}
	else
	{	
		// hide us
		SetVisible(false);
		SetMouseInputEnabled(false);
		SetKeyBoardInputEnabled(false);
	}
}

/**
* Returns the items to be added dynamically to the submenus
*
* @return CUtlVector<IBuyableItem *>
**/
CUtlVector<IBuyableItem *> *CBuyMenu::GetPurchaseItems(BI_Type eType)
{
	return CBuyMenuHelper::GetInstance()->GetItems(eType);
}

/**
* Hook to create a new type of control
*
* @param const char *szControlName Name of the control to create
* @return Panel *
**/
Panel *CBuyMenu::CreateControlByName(const char *szControlName)
{
	// this is stupid.  i tried caching the panels i'm creating here into
	// my member vars but the get nulled out by the time i try to make them
	// visible
	// WHAT THE F#^K!?!?!?

	// inventory?
	if(!Q_stricmp("BuyMenuInventory", szControlName))
		return new CBuyMenuInventory(this, "BuyMenuInventory");
	// options ?
	else if(!Q_stricmp("BuyMenuOptions", szControlName))
		return new CBuyMenuOptions(this, "BuyMenuOptions");
	// anything else?
	else
		return BaseClass::CreateControlByName(szControlName);
}

/**
* Determines if we need an update
*
* @return bool
**/
bool CBuyMenu::NeedsUpdate(void)
{
	CSDKPlayer *pPlayer;

	// bail if we don't have a player
	if(!CBasePlayer::GetLocalPlayer())
		return false;

	// if we don't have a skill or our skill is out of date we need to update
	pPlayer = ToSDKPlayer(CBasePlayer::GetLocalPlayer());
	if(pPlayer && pPlayer->GetSkillClass() && 
		(m_eCurrentSkill == NONE_CLASS_INDEX || pPlayer->GetSkillClass()->GetClassIndex() != m_eCurrentSkill))
	{
		// switch the skill
		m_eCurrentSkill = pPlayer->GetSkillClass()->GetClassIndex();
		return true;
	}

	return false;
}

/**
* Updates our panel
*
* @return void
**/
void CBuyMenu::Update(void)
{
	// update the inventory
	if (m_pOptions)
		m_pOptions->Update();
}

/**
* Handles command from child buttons
*
* @param const char *szCommand The command to handle
* @return void
**/
void CBuyMenu::OnCommand(const char *szCommand)
{
	// if we're not canceling send stuff to the server
	if(Q_stricmp(szCommand, "vguicancel") != 0)
		engine->ServerCmd(szCommand);
	// otherwise, kill the panel
	else
		gViewPortInterface->ShowPanel(PANEL_BUY, false);

	// go down
	BaseClass::OnCommand(szCommand);
}

/**
* Handles key down events
*
* @param KeyCode code The key that was pressed
* @return void
**/
void CBuyMenu::OnKeyCodePressed(KeyCode code)
{
	// let the options menu take care of it
	if(m_pOptions)
		m_pOptions->OnKeyCodePressed(code);

	// jump down
	BaseClass::OnKeyCodePressed(code);
}

/**
* Takes care of us when we get closed
*
* @return void
**/
void CBuyMenu::OnClose(void)
{
	// pretend we hit the 'b' key so we reset our categories
	OnKeyCodePressed(gameuifuncs->GetVGUI2KeyCodeForBind("buymenu"));

	// jump down
	BaseClass::OnClose();
}

/************************************************************************************/
/** Start CBuyMenuHelper ************************************************************/
/************************************************************************************/

/**
* Default constructor
**/
CBuyMenuHelper::CBuyMenuHelper(void)
{
	m_pCategorizedItems = NULL;
}

/**
* Categorizes all the items in the menu
*
* @return void
**/
void CBuyMenuHelper::CreateCategorizedItems(void)
{
	// first level
	m_pCategorizedItems = new CUtlVector<IBuyableItem *>*[BI_TYPE_COUNT];

	// second level
	for(int i = 0; i < BI_TYPE_COUNT; ++i)
		m_pCategorizedItems[i] = new CUtlVector<IBuyableItem *>;

	// put the items we have into the categorized array
	for(int i = 0; i < m_pItems.Size(); ++i) {
		// init the item as late as possible...
		m_pItems[i]->BI_Init();

		// throw it into the type bucket
		int iType = m_pItems[i]->GetType();
		if (iType >= 0)
			m_pCategorizedItems[iType]->AddToTail(m_pItems[i]);
	}

	// might as well get rid of these
	m_pItems.Purge();
}

/**
* Finds all the items of a given type
*
* @param BI_Type eType The type of item to return
* @return CUtlVector<IBuyableItem *> * The items of that type
**/
CUtlVector<IBuyableItem *> *CBuyMenuHelper::GetItems(BI_Type eType)
{
	// make sure the categories exist
	if (!m_pCategorizedItems)
		CreateCategorizedItems();

	// send back the category
	return m_pCategorizedItems[(int)eType];
}

/**
* Destructor
**/
CBuyMenuHelper::~CBuyMenuHelper(void)
{
	// did we create the items?
	if(m_pCategorizedItems)
	{
		// kill each set
		for(int i = 0; i < BI_TYPE_COUNT; ++i)
		{
			// kill this one
			if(m_pCategorizedItems[i])
				delete(m_pCategorizedItems[i]);
		}

		// kill the items
		delete [] m_pCategorizedItems;
	}
}

/**
* Constructor for the CBuyMenuHelper class.  Calls the singleton and adds a new item
*
* @param IBuyableItem *pItem The item to add
**/
CBuyMenuHelper::CBuyMenuHelper(IBuyableItem *pItem)
{
	// add the item to the singleton
	GetInstance()->AddBuyableItem(pItem);
}

/**
* Manages the singleton
*
* @return CBuyMenuHelper
**/
CBuyMenuHelper *CBuyMenuHelper::GetInstance(void)
{
	// do we have it?
	if(!s_pInstance)
		s_pInstance = new CBuyMenuHelper();

	return s_pInstance;
}

/**
* Terminates the singleton
*
* @return void
**/
void CBuyMenuHelper::Term(void)
{
	// kill it
	if(s_pInstance)
		delete(s_pInstance);
}

/**
* Adds a buyable item to the global buy menu instance
*
* @param IBuyableItem *pItem The item to add
* @return void
**/
void CBuyMenuHelper::AddBuyableItem(IBuyableItem *pItem)
{
	// store the item
	m_pItems.AddToTail(pItem);
}
