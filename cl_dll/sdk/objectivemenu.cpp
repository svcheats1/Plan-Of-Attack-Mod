#include "cbase.h"
#include "objectivemenu.h"
#include "gamestringpool.h"
#include "c_objectivemanager.h"
#include "hilitebutton.h"
#include "imagepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Default constructor
**/
CObjectiveMenu::CObjectiveMenu(IViewPort *pViewPort)
	: Frame(NULL, "ObjectiveMenu", 0), m_pViewPort(pViewPort)
{
	// set up all of our panels
	m_aPanels.SetSize(MAX_OBJECTIVES);

	// set all our panels to null
	for(int i = 0; i < m_aPanels.Count(); ++i)
		m_aPanels[i] = NULL;

	// set the scheme and set no resize
	SetScheme("ClientScheme");
	SetDeleteSelfOnClose(false);
	SetSizeable(false);
	SetProportional(true);

	// load our layout
	LoadControlSettings("Resource/UI/ObjectivesMenu.res");

	// reset the map
	Reset();

	// make sure we know when the level changes
	gameeventmanager->AddListener(this, "game_newmap", false);

	// map hasn't changed
	m_bMapChanged = true;
	m_pMapPanel = NULL;
	m_pTimerLabel = NULL;
	m_iCurrentTimer = 0;

	// clear out our map name
	memset((void *)m_szMapName, 0, 256);

	// not initially visible, but enabled
	SetVisible(false);
	SetEnabled(true);
	SetPaintBackgroundType(2);
	SetPaintBackgroundEnabled(true);
}

/**
* Destructor
**/
CObjectiveMenu::~CObjectiveMenu(void)
{
	// kill the map
	if(m_pMapPanel)
		m_pMapPanel->MarkForDeletion();

	// destroy our panel list
	m_aPanels.Purge();

	// get rid of our listener
	gameeventmanager->RemoveListener(this);
}

/**
* Handles commands that are sent our way
*
* @param const char *szCommand The command we received
* @return void
**/
void CObjectiveMenu::OnCommand(const char *szCommand)
{
	// jump down
	Frame::OnCommand(szCommand);

	// send the command to the server
	engine->ServerCmd(szCommand);

	// kill the panel
	Frame::OnCommand("Close");
	gViewPortInterface->ShowBackGround(false);
}

/**
* Creates panels that don't exist but should
* @NOTE - We may want this to load stuff from a file at some point... 
* @SEE - SObjective_t declaration
*
* @param const SObjective_t *pObj The objective to create a button for
* @return Panel *
**/
Panel *CObjectiveMenu::CreatePanel(const SObjective_t *pObj)
{
	CHiliteImageButton *pButton; 
	Panel *pPanel;
	Label *pLabel;
	char szStr[256];

	// create the panel to frame the button
	pPanel = new Panel(this);
	pPanel->SetSize(OBJECTIVE_PANEL_WIDTH, OBJECTIVE_PANEL_HEIGHT);
	pPanel->SetPaintBackgroundEnabled(true);
	pPanel->SetPaintBackgroundType(2);
	pPanel->SetVisible(true);
	pPanel->SetEnabled(true);

	// draw the label
	Q_snprintf(szStr, sizeof(szStr), "chooseobjective_label_%d", pObj->iID);
	pLabel = new vgui::Label(pPanel, szStr, "");
	pLabel->SetSize(OBJECTIVE_LABEL_WIDTH, OBJECTIVE_LABEL_HEIGHT);
	pLabel->SetPos(OBJECTIVE_LABEL_HOFFSET, OBJECTIVE_LABEL_VOFFSET);
	pLabel->SetContentAlignment(vgui::Label::a_northwest);
	pLabel->SetVisible(true);
	pLabel->SetEnabled(true);

	// set the objective text
	UpdateObjectiveLabelText(pLabel, pObj);

	// create the button
	Q_snprintf(szStr, sizeof(szStr), "chooseobjective_button_%d", pObj->iID);
	pButton = new CHiliteImageButton(pPanel, szStr, "");
	pButton->SetSize(XRES(OBJECTIVE_BUTTON_WIDTH), YRES(OBJECTIVE_BUTTON_HEIGHT));
	pButton->SetImageCrop(OBJECTIVE_BUTTON_IMAGE_X_CROP, OBJECTIVE_BUTTON_IMAGE_Y_CROP);
	pButton->SetTextureSize(OBJECTIVE_BUTTON_SIZE, OBJECTIVE_BUTTON_SIZE);
	pButton->SetVisible(true);
	pButton->SetEnabled(true);

	// set the command
	Q_snprintf(szStr, sizeof(szStr), "chooseobjective %d", pObj->iID);
	pButton->SetCommand(szStr);
	pButton->AddActionSignalTarget(this);

	// set the text
	pButton->SetText(pObj->szName);

	m_pTimerLabel = (vgui::Label*)FindChildByName("Timer");

	return pPanel;
}

/**
* Updates the text of the specified label
*
* @param vgui::Label The label to update
* @param const SObjective_t The objective information to use in the update
* @return void
**/
void CObjectiveMenu::UpdateObjectiveLabelText(vgui::Label *pLabel, const SObjective_t *pObj)
{
	wchar_t *wszLocalized;
	wchar_t wszStr[256], wszStrNew[256];
	int iWide, a, b, c, i, j, k;
	HFont sFont;
	IScheme *pScheme;

	// internationalizizzle
	wszLocalized = localize()->Find(pObj->szName);
	if (wszLocalized)
		swprintf(wszStr, L"%s\n", wszLocalized);
	else
	{
		localize()->ConvertANSIToUnicode(pObj->szName, wszStr, sizeof(wszStr));
		swprintf(wszStr, L"%s\n", wszStr);
	}

	// add on the cash and xp info
	if(pObj->iCash > 0)
		swprintf(wszStr, L"%s  $%d\n", wszStr, pObj->iCash);
	if(pObj->iXP > 0)
		swprintf(wszStr, L"%s  %d Experience\n", wszStr, pObj->iXP);

	// if we're the base, add the bankrupt message
	if(pObj->bIsBase)
	{
		// setup the message
		wszLocalized = localize()->Find("#BankruptEnemy");
		if (wszLocalized)
			swprintf(wszStr, L"%s\n", wszLocalized);
		else
		{
			localize()->ConvertANSIToUnicode("#BankruptEnemy", wszStr, sizeof(wszStr));
			swprintf(wszStr, L"%s\n", wszStr);
		}
	}

	// figure out the font we'll be using
	sFont = INVALID_FONT;
	if(pLabel->GetFont() == INVALID_FONT)
	{
		// pull the scheme and get the default font
		pScheme = scheme()->GetIScheme(GetScheme());
		if(pScheme)
			sFont = pScheme->GetFont("Default", pLabel->IsProportional());
	}
	else
		sFont = pLabel->GetFont();

	// clear the new string
	memset(wszStrNew, 0, sizeof(wchar_t) * 256);

	// add new lines when we get to fat
	// start iterating through our characters
	for(i = j = iWide = 0; i < (int)wcslen(wszStr); ++i)
	{
		// is it a newline? clear out the width
		if(wszStr[i] == '\n')
			iWide = 0;
		// add on the width of the character
		else
		{
			// figure out the width of the character
			surface()->GetCharABCwide(sFont, wszStr[i], a, b, c);
			iWide += a + b + c;

			// are we over?
			if(iWide > pLabel->GetWide())
			{
				// backtrack to the last space in the new str
				for(k = 0; k < j; ++k)
				{
					// is it a space?
					if(wszStrNew[j - k] == ' ')
					{
						wszStrNew[j - k] = '\n';
						break;
					}
				}

				// couldn't find it?
				// stick the newline in right here
				if(k == j)
					wszStrNew[j++] = '\n';

				// start over
				iWide = 0;
			}
		}

		// set the character
		wszStrNew[j++] = wszStr[i];
	}
	wszStrNew[j] = 0;

	// se thte text
	pLabel->SetText(wszStrNew);
}

/**
* Fixes panels that are dirty
*
* @return void
**/
void CObjectiveMenu::FixDirtyPanels(void)
{
	int iButtons = 0;

	// find any that are dirty
	for(int i = 0; i < MAX_OBJECTIVES; ++i)
	{
		// grabs the objective
		SObjective_t *pObj = GET_OBJ_MGR()->GetObjective(i);

		// did we get it?
		if(!pObj || !pObj->bDirty)
			continue;

		// does it exist?
		if(!m_aPanels[pObj->iID])
			// create the panel
			m_aPanels[pObj->iID] = CreatePanel(pObj);

		// did the map change?
		SetButtonTexture(m_aPanels[pObj->iID], pObj);

		// position / reposition if necessary
		m_aPanels[pObj->iID]->SetPos((iButtons * (OBJECTIVE_PANEL_WIDTH + OBJECTIVE_PANEL_HSPACE)) + OBJECTIVE_PANEL_HOFFSET, OBJECTIVE_PANEL_VOFFSET);
		m_aPanels[pObj->iID]->SetSize(OBJECTIVE_PANEL_WIDTH, OBJECTIVE_PANEL_HEIGHT);

		// turn off and/or hide if we can't select
		//m_aPanels[pObj->iID]->SetEnabled(pObj->iOwner != C_BasePlayer::GetLocalPlayer()->GetTeamNumber());
		//m_aPanels[pObj->iID]->SetVisible(!pObj->bIsBase || pObj->iOwner != C_BasePlayer::GetLocalPlayer()->GetTeamNumber());

		// update our button's state
		UpdateChildButton(m_aPanels[pObj->iID], pObj);

		// only increment if we drew the button
		if(!pObj->bIsBase || pObj->iOwner != C_BasePlayer::GetLocalPlayer()->GetTeamNumber())
			++iButtons;
	}

	// did the map change?
	if(m_bMapChanged)
	{
		// change the map
		ChangeMap();

		m_bMapChanged = false;
	}

	// make sure we have a map, then update the buttons
	if(m_pMapPanel)
		m_pMapPanel->UpdateButtons();
}

/**
* Updates the child of the specified button
* @NOTE - this seems like it should happen recursively when the parent panel's state gets set...
*
* @param Panel *pPanel The panel whose button needs updating
* @param SObjective_t *pObj The objective to use during the update
* @return void
**/
void CObjectiveMenu::UpdateChildButton(Panel *pPanel, SObjective_t *pObj)
{
	CHiliteImageButton *pButton;
	char szStr[256];

	// make sure we got the panel, objective, and player
	if(!pPanel || !pObj || !C_BasePlayer::GetLocalPlayer())
		return;

	// find the child
	Q_snprintf(szStr, sizeof(szStr), "chooseobjective_button_%d", pObj->iID);
	pButton = (CHiliteImageButton *)pPanel->FindChildByName(szStr);

	// make sure we got it
	if(!pButton)
		return;

	// turn off and/or hide if we can't select
	pButton->SetEnabled(pObj->iOwner != C_BasePlayer::GetLocalPlayer()->GetTeamNumber());
	pButton->SetVisible(!pObj->bIsBase || pObj->iOwner != C_BasePlayer::GetLocalPlayer()->GetTeamNumber());

	// set the size in case our resolution changed
	pButton->SetSize(XRES(OBJECTIVE_BUTTON_WIDTH), YRES(OBJECTIVE_BUTTON_HEIGHT));
	pButton->SetTextureSize(OBJECTIVE_BUTTON_SIZE, OBJECTIVE_BUTTON_SIZE);
}

/**
* Changes the current map texture
*
* @return void
**/
void CObjectiveMenu::ChangeMap(void)
{
	char szStr[256];
	CHiliteButton *pButton;
	Label *pLabel;

	// make sure we have one
	if(!m_pMapPanel)
		m_pMapPanel = new CObjectiveMap(this);

	// if we don't have a real map name get one
	// this happens in the case that the resolution is changed
	// and this panel gets deleted
	if(!Q_strlen(m_szMapName))
	{
		// copy the name
		Q_snprintf(m_szMapName, MAP_NAME_LENGTH, "%s", m_pMapPanel->GetMapName());

		// do we still not have it?
		if(!Q_strlen(m_szMapName))
			return;
	}

	// set the map
	m_pMapPanel->SetMap(m_szMapName);

	// change neighbors for all the hilites
	for(int i = 0; i < MAX_OBJECTIVES; ++i)
	{
		// grabs the objective
		SObjective_t *pObj = GET_OBJ_MGR()->GetObjective(i);

		// did we get it?
		if(!pObj)
			continue;

		// make sure we have a real index
		if(!m_aPanels.IsValidIndex(pObj->iID) || !m_aPanels[pObj->iID])
			continue;

		// find the child
		Q_snprintf(szStr, sizeof(szStr), "chooseobjective_button_%d", pObj->iID);
		pButton = (CHiliteButton *)m_aPanels[pObj->iID]->FindChildByName(szStr);

		// did we get it?
		if(pButton)
			// set the neighbors
			pButton->SetNeighbor(m_pMapPanel->SetButtonNeighbor(pButton, pObj));

		// find the label
		Q_snprintf(szStr, sizeof(szStr), "chooseobjective_label_%d", pObj->iID);
		pLabel = (vgui::Label *)m_aPanels[pObj->iID]->FindChildByName(szStr);

		// did we get it?
		if(pLabel)
			// update the text
			UpdateObjectiveLabelText(pLabel, pObj);
	}
}

/**
* Sets the button texture for a given panel
*
* @param Panel *pPanel The panel whose button texture we need to set
* @param SObjective_t *pObj The objective to use when setting the texture
* @return void
**/
void CObjectiveMenu::SetButtonTexture(Panel *pPanel, SObjective_t *pObj)
{
	char szStr[256];
	CHiliteImageButton *pButton;

	// figure out what the button should be called
	Q_snprintf(szStr, sizeof(szStr), "chooseobjective_button_%d", pObj->iID);
	pButton = (CHiliteImageButton *)pPanel->FindChildByName(szStr);

	// did we find it?
	if(pButton)
	{
		// set the texture
		Q_snprintf(szStr, sizeof(szStr), "%s/objective_%d", m_pMapPanel->GetMapDirectory(), pObj->iID);
		pButton->SetTexture(szStr);
	}
}

/**
* Displays or hides this panel
*
* @param bool bShow Specifies whether we are hiding or showing
* @return void
**/
void CObjectiveMenu::ShowPanel(bool bShow)
{
	// are we already doing this?
	// do we have buttons?
	if(Frame::IsVisible() == bShow || !GET_OBJ_MGR()->GetObjectivesExist())
		return;

	// what are we doing?
	if(bShow)
	{
		// if we don't have a map, change to one
		if(!m_pMapPanel)
			ChangeMap();

		// make sure we have the map name
		if(!Q_strlen(m_szMapName) && m_pMapPanel)
			Q_snprintf(m_szMapName, MAP_NAME_LENGTH, "%s", m_pMapPanel->GetMapName());

		// fix all the panels
		FixDirtyPanels();

		// hide the buy menu because it is particuarily fidgity
		gViewPortInterface->ShowPanel(PANEL_BUY, false);

		// we can see it and use our fish(mouse)
		SetVisible(true);
		SetMouseInputEnabled(true);
	}
	else
	{
		// can't see it and no mouse(fish)
		SetVisible(false);
		SetMouseInputEnabled(false);
	}

	// do the same for the background
	m_pViewPort->ShowBackGround(bShow);
}

/**
* Handles events that were passed from the game event manager
*
* @param KeyValues *pData The data we need to handle
* @return void
**/
void CObjectiveMenu::FireGameEvent(IGameEvent *pData)
{
	// map change?
	if(Q_strcmp(pData->GetName(), "game_newmap") == 0)
	{
		// the map changed
		m_bMapChanged = true;

		// store the map name
		Q_snprintf(m_szMapName, sizeof(m_szMapName), "%s", pData->GetString("mapname"));
	}
}

/**
* Applies the scheme settings to the panel
*
* @param vgui::IScheme *pScheme The scheme to use
* @return void
**/
void CObjectiveMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	CImagePanel *pPanel = NULL;
	vgui::Label *pLabel;

	BaseClass::ApplySchemeSettings(pScheme);

	// find the timer and reposition...
	// not sure why this doesn't happen on its own
	pPanel = (CImagePanel *)FindChildByName("TimerIcon");
	if(pPanel)
	{
		pPanel->SetPos(OBJECTIVE_TIMER_ICON_XOFFSET, OBJECTIVE_TIMER_ICON_YOFFSET);
		pPanel->SetSize(OBJECTIVE_TIMER_ICON_WIDTH, OBJECTIVE_TIMER_ICON_HEIGHT);
	}

	// find the label and reposition
	pLabel = (Label *)FindChildByName("Timer");
	if(pLabel)
	{
		pLabel->SetPos(OBJECTIVE_TIMER_LABEL_XOFFSET, OBJECTIVE_TIMER_LABEL_YOFFSET);
		pLabel->SetSize(OBJECTIVE_TIMER_LABEL_WIDTH, OBJECTIVE_TIMER_LABEL_HEIGHT);
	}

	SetPaintBackgroundType(2);
	SetPaintBackgroundEnabled(true);
}

/**
* Resets the menu
*
* @return void
**/
void CObjectiveMenu::Reset(void)
{
	//int iButtons = 0;

	// set the bounds and position
	SetSize(OBJECTIVE_MENU_WIDTH, OBJECTIVE_MENU_HEIGHT);	
	SetPos(OBJECTIVE_MENU_XOFFSET, OBJECTIVE_MENU_YOFFSET);

	// find any that are dirty
	/*for(int i = 0; i < MAX_OBJECTIVES; ++i)
	{
		// grab the objective
		SObjective_t *pObj = GET_OBJ_MGR()->GetObjective(i);

		// does it exist?
		if(!pObj || !m_aPanels.IsValidIndex(pObj->iID) || !m_aPanels[pObj->iID])
			continue;

		// position / reposition if necessary
		m_aPanels[pObj->iID]->SetPos((iButtons * (OBJECTIVE_PANEL_WIDTH + OBJECTIVE_PANEL_HSPACE)) + OBJECTIVE_PANEL_HOFFSET, OBJECTIVE_PANEL_VOFFSET);
		m_aPanels[pObj->iID]->SetSize(OBJECTIVE_PANEL_WIDTH, OBJECTIVE_PANEL_HEIGHT);

		// only increment if we drew the button
		if(!pObj->bIsBase || pObj->iOwner != C_BasePlayer::GetLocalPlayer()->GetTeamNumber())
			++iButtons;
	}*/
}

/**
* Creates a control of the given type
*
* @param const char *szControlName Name of the control to creat
* @return Panel * The control we created
**/
vgui::Panel *CObjectiveMenu::CreateControlByName(const char *szControlName)
{
	// image panel?
	if(!Q_strcmp("ImagePanel", szControlName))
		return new CImagePanel(this, "");
	// anything else?
	else
		return BaseClass::CreateControlByName(szControlName);
}

void CObjectiveMenu::Paint()
{
	float fTime = (m_fEndTimer - gpGlobals->curtime);

	// set the timer label to the remaining time
	if (m_pTimerLabel) {
		int iTime = floor(fTime);
		if (iTime != m_iCurrentTimer) {
			char szTime[10];
			Q_snprintf(szTime, 10, "%d", iTime);
			m_pTimerLabel->SetText(szTime);
			m_iCurrentTimer = iTime;
		}
	}

	// if we're out of time, choose an objective
	if (fTime < 0.0) {
		CUtlVector<int> objIDs;

		// go through the objectives and figure out what we got available
		for(int i = 0; i < GET_OBJ_MGR()->GetObjectiveCount(); ++i) {
			SObjective_t* pObj = GET_OBJ_MGR()->GetObjective(i);
			if (!pObj->bIsBase && pObj->iOwner != C_BasePlayer::GetLocalPlayer()->GetTeamNumber()) {
				objIDs.AddToTail(pObj->iID);
			}
		}

		// if we got any, choose one
		if (objIDs.Count() > 0) {
			int iObj = objIDs.Element(random->RandomInt(0, objIDs.Count() - 1));
			char buf[32];
			Q_snprintf(buf, sizeof(buf), "chooseobjective %d", iObj);
			OnCommand(buf);
		}
	}

	BaseClass::Paint();
}