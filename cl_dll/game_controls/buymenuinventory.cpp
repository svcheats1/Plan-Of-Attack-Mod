#include "cbase.h"
#include "buymenuinventory.h"
#include "c_sdk_player.h"
#include <vgui_controls/Label.h>
#include "hilitebutton.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define WEAPON_TEXTURE_LOCATION "vgui/buymenu/weapons/"
#define EMPTY_TEXTURE_NAME "empty"

/**
* Constructor
**/
CBuyMenuInventory::CBuyMenuInventory(Panel *pParent, const char *szName)
	: BaseClass(pParent, szName)
{
	// load our settings
	LoadControlSettings("Resource/UI/BuyMenuInventory.res");

	// no panels
	m_pPrimary = m_pSecondary = m_pAccessory = NULL;
	m_pCashLabel = NULL;
	m_pPrimaryWeapon = m_pSecondaryWeapon = m_pAccessoryWeapon = NULL;
	m_iTeamNumber = m_iCash = 0;
}

/**
* Destructor
**/
CBuyMenuInventory::~CBuyMenuInventory()
{
	// ?
}

/**
* Applies the scheme to the panel
*
* @param IScheme *pScheme The scheme to apply
* @return void
**/
void CBuyMenuInventory::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// show the background
	SetPaintBackgroundEnabled(true);
}

/**
* Creates controls by name
*
* @param const char *szControlName Name of the control to create
* @return Panel * The new control
**/
vgui::Panel *CBuyMenuInventory::CreateControlByName(const char *szControlName)
{
	// image label?
	if(!Q_strcmp("ImagePanel", szControlName))
		return new CImagePanel(this, "");
	// anything else
	else
		return BaseClass::CreateControlByName(szControlName);
}

/**
* Updates the discard button for the given label
*
* @param CImagePanel *pPanel The panel whose discard button to update
* @param bool bEnable True if we should turn this guy on and make it visible
* @return void
**/
void CBuyMenuInventory::UpdateDiscardButton(CImagePanel *pPanel, bool bEnable)
{
	CHiliteImageButton *pDiscardButton;
	char szStr[256];

	// check the panel
	if(!pPanel)
		return;

	// see if we can find one already
	pDiscardButton = (CHiliteImageButton *)pPanel->FindChildByName("DiscardButton");

	// if we can't find it, add it
	if(!pDiscardButton)
	{
		// create the discard button
		pDiscardButton = new CHiliteImageButton(pPanel, "DiscardButton", "vgui/buymenu/discard", HILITE_IMAGE_BUTTON_IMAGE);

		// not enabled, can't see it
		pDiscardButton->SetVisible(false);
		pDiscardButton->SetEnabled(false);

		// send the signal back to our parent
		pDiscardButton->AddActionSignalTarget(GetParent());

		// set the command
		Q_snprintf(szStr, sizeof(szStr), "drop %s", pPanel->GetName());
		pDiscardButton->SetCommand(szStr);
	}

	// make sure we got it
	if(pDiscardButton)
	{
		// sizing / positioning
		pDiscardButton->SetSize(XRES(DISCARD_BUTTON_WIDTH), YRES(DISCARD_BUTTON_HEIGHT));
		pDiscardButton->SetPos(XRES(DISCARD_BUTTON_XOFFSET), YRES(DISCARD_BUTTON_YOFFSET));
		pDiscardButton->SetImageCrop(DISCARD_BUTTON_IMAGE_WIDTH, DISCARD_BUTTON_IMAGE_HEIGHT);
		pDiscardButton->SetTextureSize(DISCARD_BUTTON_WIDTH, DISCARD_BUTTON_HEIGHT);

		// turn it on or off
		pDiscardButton->SetVisible(bEnable);
		pDiscardButton->SetEnabled(bEnable);
	}
}

/**
* Finds all of our children
* 
* @return void
**/
void CBuyMenuInventory::EstablishChildren(void)
{
	// check each of them
	if(!m_pPrimary)
		m_pPrimary = (CImagePanel *)FindChildByName("PrimaryWeapon");
	if(!m_pSecondary)
		m_pSecondary = (CImagePanel *)FindChildByName("SecondaryWeapon");
	if(!m_pAccessory)
		m_pAccessory = (CImagePanel *)FindChildByName("AccessoryWeapon");
	if(!m_pCashLabel)
		m_pCashLabel = (vgui::Label *)FindChildByName("CashLabel");
}

/**
* Updates the inventory to display the player's current weapons
*
* @return void
**/
void CBuyMenuInventory::Update(void)
{
	char szStr[256];

	// setup the background color
	SetPaintBackgroundEnabled(true);
	if (m_iTeamNumber == TEAM_A)
		SetBgColor(Color(58, 45, 135, 77));
	else if (m_iTeamNumber == TEAM_B)
		SetBgColor(Color(100, 0, 0, 77));
	else
		SetBgColor(Color(0, 0, 0, 77));

	// establish the children
	EstablishChildren();

	// update our buttons
	UpdateDiscardButton(m_pPrimary, m_pPrimaryWeapon != NULL);
	UpdateDiscardButton(m_pSecondary, m_pSecondaryWeapon != NULL);
	UpdateDiscardButton(m_pAccessory, m_pAccessoryWeapon != NULL);

	// update our cash text
	if(m_pCashLabel)
	{
		// update our cash
		Q_snprintf(szStr, sizeof(szStr), "$%d", m_iCash);
		m_pCashLabel->SetText(szStr);
	}

	// update the weapon textures
	UpdateWeaponTextures();
}

bool CBuyMenuInventory::NeedsUpdate()
{
	CSDKPlayer *pPlayer;

	// pull the player
	CBasePlayer *p = CBasePlayer::GetLocalPlayer();
	if (!p)
		return false;

	pPlayer = ToSDKPlayer(p);
	if (!pPlayer)
		return false;

	if (pPlayer->GetTeamNumber() == m_iTeamNumber &&
		pPlayer->GetPrimaryWeapon() == m_pPrimaryWeapon && 
		pPlayer->GetSecondaryWeapon() == m_pSecondaryWeapon &&
		pPlayer->GetWeaponBySlot(3) == m_pAccessoryWeapon &&
		pPlayer->GetCash() == m_iCash)
	{
		return false;
	}
	else {
		m_iTeamNumber = pPlayer->GetTeamNumber();
		m_pPrimaryWeapon = pPlayer->GetPrimaryWeapon();
		m_pSecondaryWeapon = pPlayer->GetSecondaryWeapon();
		// this is a 0-based index, so slot 3 is accessible by pressing '4'
		m_pAccessoryWeapon = pPlayer->GetWeaponBySlot(3);
		m_iCash = pPlayer->GetCash();
		return true;
	}
}

void CBuyMenuInventory::UpdateWeaponTextures()
{
	char texFile[128];
	
	Q_strcpy(texFile, WEAPON_TEXTURE_LOCATION);
	if (m_pPrimaryWeapon)
        Q_strcat(texFile, m_pPrimaryWeapon->GetClassname());
	else
		Q_strcat(texFile, EMPTY_TEXTURE_NAME);
	m_pPrimary->SetTexture(texFile);

	Q_strcpy(texFile, WEAPON_TEXTURE_LOCATION);
	if (m_pSecondaryWeapon)
		Q_strcat(texFile, m_pSecondaryWeapon->GetClassname());
	else
		Q_strcat(texFile, EMPTY_TEXTURE_NAME);
	m_pSecondary->SetTexture(texFile);

	Q_strcpy(texFile, WEAPON_TEXTURE_LOCATION);
	if (m_pAccessoryWeapon)
		Q_strcat(texFile, m_pAccessoryWeapon->GetClassname());
	else
		Q_strcat(texFile, EMPTY_TEXTURE_NAME);
	m_pAccessory->SetTexture(texFile);
}

void CBuyMenuInventory::Paint()
{
	// update if necessary
	if(NeedsUpdate())
		Update();

	BaseClass::Paint();
}