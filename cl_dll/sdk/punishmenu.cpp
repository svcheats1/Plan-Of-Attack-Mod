#include "cbase.h"
#include "punishmenu.h"
#include "iclientmode.h"
#include <cl_dll/iviewport.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Constructor
**/
CPunishMenu::CPunishMenu(IViewPort *pViewPort)
	: vgui::Frame(NULL, PANEL_PUNISH)
{
	// store the viewport
	m_pViewPort = pViewPort;

	// set all the panel stuff
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetProportional(true);

	// hide the system buttons
	SetTitleBarVisible(false);

	// grab the panel elements
	m_pNameLabel = new Label(this, "NameLabel", "Name");
	m_pStrikeButton = new CHiliteTextButton(this, "ForgiveButton", "#PunishForgive");
	m_pForgiveButton = new CHiliteTextButton(this, "StrikeButton", "#StrikeForgive");

	// pull our settings
	LoadControlSettings("Resource/UI/Punish.res");
}

/**
* Destructor
**/
CPunishMenu::~CPunishMenu()
{

}

/**
* Applies the scheme settings
*
* @param IScheme *pScheme The scheme to use
* @return void
**/
void CPunishMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// apply the scheme
	BaseClass::ApplySchemeSettings(pScheme);

	// we want rounded corners
	SetPaintBackgroundType(2);
}

/**
* Hook to create a new type of control
*
* @param const char *szControlName Name of the control to create
* @return Panel *
**/
Panel *CPunishMenu::CreateControlByName(const char *szControlName)
{
	if(!Q_stricmp("HiliteTextButton", szControlName))
		return new CHiliteTextButton(this, "", "");
	else
		return BaseClass::CreateControlByName(szControlName);
}

/**
* Sets the current player name
*
* @param const char *szName The name to use
* @return void
**/
void CPunishMenu::SetPlayerName(const char *szName)
{
	// set the label
	m_pNameLabel->SetText(szName);
}

/**
* Displays or hides our panel
*
* @param bool bShow True if we want to display the panel
* @return void
**/
void CPunishMenu::ShowPanel(bool bShow)
{
	// don't do anything if we're in the proper state
	if(BaseClass::IsVisible() == bShow)
		return;

	// turn us on
	if(bShow)
	{
		// get us going and let the mouse be used
		Activate();
		SetMouseInputEnabled(true);
	}
	else
	{
		// hide us, no mouse
		SetVisible(false);
		SetMouseInputEnabled(false);
	}
}

/**
* Updates our panel
*
* @return void
**/
void CPunishMenu::Update(void)
{
	// ?
}

/**
* Resets our panel
*
* @return void
**/
void CPunishMenu::Reset(void)
{
	// ?
}

/**
* Determines the action to take when we receive a new command
*
* @param const char *szCommand The command to handle
* @return void
**/
void CPunishMenu::OnCommand(const char *szCommand)
{
	char szStr[64];

	// forgive?
	if(FStrEq(szCommand, "forgive"))
	{
		// create the command and send it off
		Q_snprintf(szStr, sizeof(szStr), "forgive %d", m_iPlayer);
		engine->ServerCmd(szStr);
	}
	// strike?
	else if(FStrEq(szCommand, "strike"))
	{
		// create the command and send it off
		Q_snprintf(szStr, sizeof(szStr), "strike %d", m_iPlayer);
		engine->ServerCmd(szStr);
	}

	// hide ourselves
	m_pViewPort->ShowPanel(this, false);

	// jump down
	BaseClass::OnCommand(szCommand);
}