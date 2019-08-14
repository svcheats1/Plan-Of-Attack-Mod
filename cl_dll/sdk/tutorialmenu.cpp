#include "cbase.h"
#include "tutorialmenu.h"
#include <cdll_client_int.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <FileSystem.h>
#include <KeyValues.h>
#include <convar.h>
#include <vgui_controls/ImageList.h>

#include <vgui_controls/Button.h>

#include <cl_dll/iviewport.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CON_COMMAND(showtutorial, "Shows the tutorial panel: [<command>]")
{
	KeyValues *pData;

	// make sure we have the viewport
	if(!gViewPortInterface)
		return;
	
	// make sure we at least have the command
	// don't know why it would get here otherwise
	if(engine->Cmd_Argc() < 1)
		return;
		
	// pull the tutorial panel
	IViewPortPanel *pPanel = gViewPortInterface->FindPanelByName(PANEL_TUTORIAL);
	if(pPanel)
	{
		// create the data to send to the panel
		pData = new KeyValues("data");
		if(engine->Cmd_Argc() == 2)
			pData->SetString("command", engine->Cmd_Argv(1));

		// send it to the panel and display it
		pPanel->SetData(pData);
		gViewPortInterface->ShowPanel(pPanel, true);

		// all set with the key values
		pData->deleteThis();
	}
	else
		Msg("Couldn't find tutorial panel.\n" );
}

/**
* Constructor
**/
CTutorialMenu::CTutorialMenu(IViewPort *pViewPort)
	: Frame(NULL, PANEL_TUTORIAL)
{
	// hang onto the viewport
	m_pViewPort = pViewPort;

	// set all the panel stuff
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetProportional(true);

	// hide the system buttons
	SetTitleBarVisible(false);

	// pull all our elements
	m_pHTMLMessagePrev = new HTML(this,"HTMLMessagePrev");
	m_pHTMLMessageCurr = new HTML(this,"HTMLMessageCurr");
	m_pHTMLMessageNext = new HTML(this,"HTMLMessageNext");
	m_pTitleLabel  = new Label(this, "MessageTitle", "Message Title");
	m_pNextButton = new CHiliteTextButton(this, "NextButton", ">");
	m_pPrevButton = new CHiliteTextButton(this, "PreviousButton", "<");
	
	// pull our settings
	LoadControlSettings("Resource/UI/Tutorial.res");
	
	// load the urls
	m_pPages = new KeyValues("DisplayMessages");
	m_pPages->LoadFromFile(::filesystem, TUTORIAL_URL_RES);

	// reset the panel
	Reset();
}

/**
* Destructor
**/
CTutorialMenu::~CTutorialMenu()
{
	// kill the urls
	m_pPages->deleteThis();
}

/**
* Sets the data from key values
*
* @param KeyValues *pData The data to use
* @return void
**/
void CTutorialMenu::SetData(KeyValues *pData)
{
	// pull the command string
	SetData(pData->GetString("command"));
}

/**
* Sets the data for the panel
*
* @param const char *szCommand The command to use when exiting
* @return void
**/
void CTutorialMenu::SetData(const char *szCommand)
{
	// if we have a command set our local command
	if(szCommand)
		Q_strncpy(m_szExitCommand, szCommand, sizeof(m_szExitCommand));
	else
		m_szExitCommand[0] = 0;

	// we can update
	Update();
}

/**
* Resets the menu
*
* @return void
**/
void CTutorialMenu::Reset(void)
{
	// no exit command yet
	m_szExitCommand[0] = 0;

	// jump right back to the start of the key values list
	m_pCurrentPage = m_pPages->GetFirstSubKey();

	// load all the pages
	LoadPrevHTMLMessage();
	ShowURL(m_pCurrentPage, m_pHTMLMessageCurr);
	LoadNextHTMLMessage();

	// update
	Update();
}

/**
* Updates the menu
*
* @return void
**/
void CTutorialMenu::Update(void)
{
	char szLang[256];
	KeyValues *pLangPage;

	// pull the current language
	engine->GetUILanguage(szLang, sizeof(szLang));

	// see if we can find the proper language
	pLangPage = m_pCurrentPage->FindKey(szLang);
	if(!pLangPage)
		pLangPage = m_pCurrentPage->FindKey("default");

	// display the title
	m_pTitleLabel->SetText(pLangPage->GetString("title"));

	// only the current page should be visible
	MakeMessageVisible(m_pHTMLMessagePrev, false);
	MakeMessageVisible(m_pHTMLMessageCurr, true);
	MakeMessageVisible(m_pHTMLMessageNext, false);

	// dim the next and previous
	if(m_pCurrentPage == m_pPages->GetFirstSubKey())
		m_pPrevButton->SetEnabled(false);
	else
		m_pPrevButton->SetEnabled(true);
	if(m_pCurrentPage->GetNextKey() == NULL)
		m_pNextButton->SetEnabled(false);
	else
		m_pNextButton->SetEnabled(true);
}

/**
* Sets the visibility of the message
*
* @param vgui::HTML *pMessage The message to set
* @param bool bVisible Determines whether we should display
* @return void
**/
void CTutorialMenu::MakeMessageVisible(vgui::HTML *pMessage, bool bVisible)
{
	// do we have it?
	if(pMessage)
	{
		// set the visibility and move it to front
		pMessage->SetVisible(bVisible);
		if(bVisible)
			pMessage->MoveToFront();
	}
}

/**
* Displays or hides our panel
*
* @param bool bShow True if we want to display the panel
* @return void
**/
void CTutorialMenu::ShowPanel(bool bShow)
{
	// don't do anything if we're in the proper state
	if(BaseClass::IsVisible() == bShow)
		return;

	// turn on/off the background
	m_pViewPort->ShowBackGround(bShow);

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
* Applies the scheme to the current panel
*
* @param vgui::IScheme *pScheme The scheme to apply
* @return void
**/
void CTutorialMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// pretty corners with black background
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(2);
}

/**
* Loads the url in the given key values page into the specified message
*
* @param KeyValues *pPage The key values page to load the url from
* @param vgui::HTML *pMessage The message to load the url into
* @return void
**/
void CTutorialMenu::ShowURL(KeyValues *pPage, vgui::HTML *pMessage)
{
	char szLang[256];
	KeyValues *pLangPage;

	// pull the current language
	engine->GetUILanguage(szLang, sizeof(szLang));

	// see if we can find the proper language
	pLangPage = pPage->FindKey(szLang);
	if(!pLangPage)
		pLangPage = pPage->FindKey("default");

	// display the proper url
	pMessage->OpenURL(pLangPage->GetString("url"));
}

/**
* Loads the next message to display
*
* @return void
**/
void CTutorialMenu::LoadNextHTMLMessage(void)
{
	KeyValues *pNextPage;

	// see if there's a next page
	pNextPage = m_pCurrentPage->GetNextKey();
	if(!pNextPage)
		return;

	// show the url for the page
	ShowURL(pNextPage, m_pHTMLMessageNext);
}

/**
* Loads the previous message to display
*
* @return void
**/
void CTutorialMenu::LoadPrevHTMLMessage(void)
{
	KeyValues *pPrevPage;

	// see if there's a prev page
	pPrevPage = FindPageBefore(m_pCurrentPage);
	if(!pPrevPage)
		return;

	// show the url for the page
	ShowURL(pPrevPage, m_pHTMLMessagePrev);
}

/**
* Handles a command sent from something in our panel
*
* @param const char *szCommand The command to handle
* @return void
**/
void CTutorialMenu::OnCommand(const char *szCommand)
{
	KeyValues *pPrev;
	vgui::HTML *pTemp;

	// if it's done we can bail
	if(!Q_strcmp(szCommand, "ok"))
    {
		// if we have something to do when we exit, let it happen
		if(m_szExitCommand[0])
			engine->ClientCmd(m_szExitCommand);
		
		// hide ourselves
		m_pViewPort->ShowPanel(this, false);
	}
	// refresh?
	else if(!Q_strcmp(szCommand, "refresh"))
	{
		// refresh it
		//m_pHTMLMessage->Refresh();
	}
	// next?
	else if(!Q_strcmp(szCommand, "next"))
	{
		// move on to the peer of the current
		if(m_pCurrentPage->GetNextKey())
		{
			// move on to the next page
			m_pCurrentPage = m_pCurrentPage->GetNextKey();

			// swap everything
			pTemp = m_pHTMLMessagePrev;
			m_pHTMLMessagePrev = m_pHTMLMessageCurr;
			m_pHTMLMessageCurr = m_pHTMLMessageNext;
			m_pHTMLMessageNext = pTemp;
			LoadNextHTMLMessage();
		}

		// update
		Update();
	}
	// previous?
	else if(!Q_strcmp(szCommand, "previous"))
	{
		// see if we can find the page before the current one
		pPrev = FindPageBefore(m_pCurrentPage);
		if(pPrev)
		{
			// set us to the previous
			m_pCurrentPage = pPrev;

			// swap everything
			pTemp = m_pHTMLMessageNext;
			m_pHTMLMessageNext = m_pHTMLMessageCurr;
			m_pHTMLMessageCurr = m_pHTMLMessagePrev;
			m_pHTMLMessagePrev = pTemp;
			LoadPrevHTMLMessage();
		}

		// update
		Update();
	}

	// jump down
	BaseClass::OnCommand(szCommand);
}

/**
* Finds the page before the specified one
*
* @param KeyValues *pPage The page to search for
* @return KeyValues *
**/
KeyValues *CTutorialMenu::FindPageBefore(KeyValues *pPage)
{
	KeyValues *pCurr;

	// we can bail if we're at the first page
	pCurr = m_pPages->GetFirstSubKey();
	if(pCurr == pPage)
		return NULL;

	// keep going until we get to the one before the current page
	while(pCurr && pCurr->GetNextKey() != pPage)
		pCurr = pCurr->GetNextKey();

	return pCurr;
}

/**
* Hook to create a new type of control
*
* @param const char *szControlName Name of the control to create
* @return Panel *
**/
Panel *CTutorialMenu::CreateControlByName(const char *szControlName)
{
	if(!Q_stricmp("HiliteTextButton", szControlName))
		return new CHiliteTextButton(this, "", "");
	else
		return BaseClass::CreateControlByName(szControlName);
}
