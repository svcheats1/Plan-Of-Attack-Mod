#include "cbase.h"
#include "mathlib.h"
#include "strategymenu.h"
#include "strategymap.h"
#include <FileSystem.h>
#include <vgui/ISurface.h>
#include "c_team.h"
#include "imagepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define DEBUG_STRATEGIES
#define STRATEGY_MAP_SIZE XRES(150)
#define STRATEGY_TIMER_ICON_XOFFSET XRES(480)
#define STRATEGY_TIMER_ICON_YOFFSET YRES(7)
#define STRATEGY_TIMER_ICON_WIDTH	XRES(16)
#define STRATEGY_TIMER_ICON_HEIGHT	YRES(16)

using namespace vgui;

char* CStrategyMenu::m_szMapName = NULL;

CStrategyMenu::CStrategyMenu(IViewPort *pViewPort)
: Frame(NULL, PANEL_STRATEGY)
{
	// set the scheme and set no resize
	SetScheme("ClientScheme");
	SetDeleteSelfOnClose(false);
	SetSizeable(false);
	SetProportional(true);

	// load the layout
	LoadControlSettings("resource/ui/StrategyMenu.res");

	// not initially visible, but enabled
	SetVisible(false);
	SetEnabled(true);
	SetMoveable(false);
	SetPaintBackgroundType(2);
	SetPaintBackgroundEnabled(true);

	m_iCurrentTimer = 0;
	m_pViewPort = pViewPort;
	m_pTimerLabel = NULL;
	m_iPage = 0;

	gameeventmanager->AddListener(this, "game_newmap", false);

	// find our labels
	m_aLabels.Purge();
	m_aLabels.AddToTail((Label*)FindChildByName("Strategy1"));
	m_aLabels.AddToTail((Label*)FindChildByName("Strategy2"));
	m_aLabels.AddToTail((Label*)FindChildByName("Strategy3"));

	m_pTimerLabel = (Label*)FindChildByName("Timer");

	UpdateStrategies();
}

CStrategyMenu::~CStrategyMenu() 
{
	PurgeVectors();

	gameeventmanager->RemoveListener(this);
}

void CStrategyMenu::Term()
{
	if (m_szMapName)
		delete [] m_szMapName;
}

void CStrategyMenu::PurgeVectors()
{
	// delete them how they want to be deleted
	for(int i = 0; i < m_aStrategies.Count(); ++i) {
		m_aStrategies[i]->MarkForDeletion();
	}

	// purge the pointers
	m_aStrategies.Purge();
}

void CStrategyMenu::FireGameEvent(IGameEvent *pEvent)
{
	if(FStrEq(pEvent->GetName(), "game_newmap"))
	{
		// if the map didn't change, then bail
		if (m_szMapName && FStrEq(pEvent->GetString("mapname"), m_szMapName))
			return;

		// set the map
		if (m_szMapName)
			delete [] m_szMapName;

		m_szMapName = new char[strlen(pEvent->GetString("mapname"))];
		Q_strcpy(m_szMapName, pEvent->GetString("mapname"));

		// update our panels
		UpdateStrategies();
	}
}

void CStrategyMenu::ShowPanel(bool bShow)
{
	// are we already doing this?
	// do we have buttons?
	if(Frame::IsVisible() == bShow)
		return;

	// what are we doing?
	if(bShow)
	{
		m_iPage = 0;

		// we can see it and use our mouse
		UpdateGameStateFilter();

		if (m_aPassesFilter.Count() > 0) {
			PreparePanels(true);
			SetVisible(true);
			SetMouseInputEnabled(true);
		}
		else
			return;
	}
	else
	{
		// can't see it and no mouse
		SetVisible(false);
		SetMouseInputEnabled(false);
	}

	// do the same for the background
	m_pViewPort->ShowBackGround(bShow);
}

void CStrategyMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// find our labels
	m_aLabels.Purge();
	m_aLabels.AddToTail((Label*)FindChildByName("Strategy1"));
	m_aLabels.AddToTail((Label*)FindChildByName("Strategy2"));
	m_aLabels.AddToTail((Label*)FindChildByName("Strategy3"));

	m_pTimerLabel = (Label*)FindChildByName("StrategyTimer");

	CImagePanel *pImagePanel = NULL;

	// find the timer and reposition...
	// not sure why this doesn't happen on its own
	pImagePanel = (CImagePanel *)FindChildByName("StrategyTimerIcon");
	if(pImagePanel)
	{
		pImagePanel->SetPos(STRATEGY_TIMER_ICON_XOFFSET, STRATEGY_TIMER_ICON_YOFFSET);
		pImagePanel->SetSize(STRATEGY_TIMER_ICON_WIDTH, STRATEGY_TIMER_ICON_HEIGHT);
	}

	SetPaintBackgroundType(2);
	SetPaintBackgroundEnabled(true);

	PreparePanels(IsVisible());
}

/**
* Creates a control of the specified type
*
* @param const char *szControlName Name of the control to create
* @return Panel * The new control or NULL
**/
Panel *CStrategyMenu::CreateControlByName(const char *szControlName)
{
	// hilite text button?
	if(!Q_strcmp(szControlName, "HiliteTextButton"))
		return new CHiliteTextButton(this, "", "");
	
	// let someelse do it
	return BaseClass::CreateControlByName(szControlName);
}

void CStrategyMenu::UpdateGameStateFilter()
{
	m_aPassesFilter.Purge();

	for(int i = 0; i < m_aStrategies.Count(); ++i) {
		if (MatchesGameState(m_aStrategies[i]->GetKeyValues())) {
			m_aPassesFilter.AddToTail(m_aStrategies[i]);
		}
	}

	m_aPassesFilter.Sort(CStrategyMenu::RandomCompare);
}

int CStrategyMenu::RandomCompare(CStrategyMap* const *ppLeft, CStrategyMap* const *ppRight)
{
	return (random->RandomInt(0, 1) == 0 ? -1 : 1);
}

void CStrategyMenu::PreparePanels(bool bShow)
{
	for(int i = 0; i < m_aStrategies.Count(); ++i)
		m_aStrategies[i]->SetVisible(false);

	for(int i = 0; i < m_aLabels.Count(); ++i)
		m_aLabels[i]->SetVisible(false);

	if (bShow) {
		// go through our strategies and pick 3
		for(int i = m_iPage * 3, iVisible = 0; 
			i < m_aPassesFilter.Count() && iVisible < 3; 
			++i, ++iVisible)
		{
			m_aPassesFilter[i]->SetPos(XRES(32) + iVisible * (STRATEGY_MAP_SIZE + XRES(10)), YRES(50));
			m_aPassesFilter[i]->SetMapSize(STRATEGY_MAP_SIZE, STRATEGY_MAP_SIZE);
			m_aPassesFilter[i]->InvalidateLayout();
			m_aPassesFilter[i]->SetVisible(true);

			Hilite(m_aPassesFilter[i], false);

			m_aLabels[iVisible]->SetText(m_aPassesFilter[i]->GetName());
			m_aLabels[iVisible]->SetVisible(true);
		}

		// are there even more items for the next page?
		FindChildByName("NextButton")->SetVisible(m_aPassesFilter.Count() > (m_iPage + 1) * 3);

		// are there more pages?
		FindChildByName("PreviousButton")->SetVisible(m_iPage > 0);
	}
}

void CStrategyMenu::UpdateStrategies()
{
	if (!m_szMapName)
		return;

	FileFindHandle_t searchHandle;
	char searchPath[64];
	char filePath[128];

	PurgeVectors();

	// where the strategies are
	sprintf(searchPath, "strategies/%s/*.txt", m_szMapName);

	// find them and destroy ^W load them
	const char *filename;
	for(filename = vgui::filesystem()->FindFirst(searchPath, &searchHandle);
		filename != NULL;
		filename = vgui::filesystem()->FindNext(searchHandle)) 
	{
		// get the file path
		sprintf(filePath, "strategies/%s/%s", m_szMapName, filename);

		// try to load it from file
		KeyValues *keyValues = new KeyValues("strategy");
		if (keyValues->LoadFromFile(vgui::filesystem(), filePath, "GAME")) {
			// get the definition key
			KeyValues *definition = keyValues->FindKey("definition");
			// found it?
			if (definition) {
				// create the map
				CStrategyMap *map = new CStrategyMap(this); // the map will take care of cleanup
				if (map->LoadFromKeyValues(definition)) {
					// set the misc info
					map->SetFilename(filename);
					map->SetKeyValues(keyValues);
					map->SetShouldTransmit(keyValues->GetInt("transmit", 1));

					// add it to our list
					m_aStrategies.AddToTail(map);
				}
				else {
					delete map;
					keyValues->deleteThis();
				}
			}
			else
				// bah, smush it
				keyValues->deleteThis();
		}
	}
}

bool CStrategyMenu::MatchesGameState(KeyValues *keyValues)
{
	// make sure we have a local player
	if (!CBasePlayer::GetLocalPlayer())
		return false;

	// see if our teams match
	int iMyTeam = CBasePlayer::GetLocalPlayer()->GetTeamNumber();
	if (keyValues->GetInt("team") != iMyTeam) {
		//DevMsg("Team didn't match");
		return false;
	}

	// offense!
	if (m_bOffensive) {
		KeyValues *pData = keyValues->FindKey("attacking");
		// if this strategy is not for use while attacking, then bail
		if (!pData) {
			//DevMsg("Strategy not offensive");
			return false;
		}

		// get our active objective
		SObjective_t *pObj = C_ObjectiveManager::GetInstance()->GetActiveObjective();
		if (!pObj) {
			//DevMsg("No active objective");
			return false;
		}

		// move objectives >3 to 0
		int id = pObj->iID + 1;
		if (id > 3)
			id = 0;

		bool bMatchedObjective = false, bFoundObjective = false, 
			bMatchedCriteria = false, bFoundCriteria = false;

		// iterate over all our data
		for(KeyValues *pair = pData->GetFirstSubKey(); pair != NULL; pair = pair->GetNextKey()) {
			// look for objective
			if (FStrEq(pair->GetName(), "objective")) {
				// we did this already
				if (bMatchedObjective)
					continue;
				
				// we found one!
				bFoundObjective = true;

				// did it match?
				if (pair->GetInt() == id)
					bMatchedObjective = true;
			}
			// look for objective status
			else if (FStrEq(pair->GetName(), "obj_status")) {
				// we did this already
				if (bMatchedCriteria)
					continue;

				// get the value
				const char *szStatus = pair->GetString();

				// must be length 3
				if(strlen(szStatus) != 3)
					continue;

				// found 1
				bFoundCriteria = true;

				// this is sloppy. consider refactoring..... 
				// look at the objectives and compare statuses
				for(int i = 0; i < 3; ++i) {
					// * = any
					if (szStatus[i] == '*') {
						// last time through, true!
						if (i == 2)
							bMatchedCriteria = true;
						// go again
						continue;
					}

					// get the objective in question
					SObjective_t *pObj = C_ObjectiveManager::GetInstance()->GetObjective(i);
					if (!pObj)
						break;

					// ! = not owned by my team
					if(szStatus[i] == '!')
					{
						// if i own it, bail
						if(pObj->iOwner == iMyTeam)
							break;
						// otherwise, if this is the last item we were successful
						else if(i == 2)
							bMatchedCriteria = true;

						// go again
						continue;
					}
					// ~ = not owned by the enemy team
					else if(szStatus[i] == '~')
					{
						// if the other team owns it, bail
						if(pObj->iOwner == (iMyTeam == TEAM_A ? TEAM_B : TEAM_A))
							break;
						// otherwise, if this is the last item we were successful
						else if(i == 2)
							bMatchedCriteria = true;

						// go again
						continue;
					}

					int iOwner = pObj->iOwner;
					// neutral objectives should be 0, not -1...
					if (iOwner < 0)
						iOwner = 0;

					// + 48 is the ascii value for 0...
					// do the owners match?
					if (iOwner + 48 == szStatus[i]) {
						// last time through, true!
						if (i == 2)
							bMatchedCriteria = true;
						// go again
						continue;
					}
					// didn't match so bail
					else
						break;
				}
			}
		}

		if (bFoundObjective && !bMatchedObjective) {
			//DevMsg("Didn't match active objective");
			return false;
		}
		if (bFoundCriteria && !bMatchedCriteria) {
			//DevMsg("Didn't match objective status");
			return false;
		}
	}
	// defense!
	else {
		KeyValues *pData = keyValues->FindKey("defending");
		// if this strategy is not for use while defending, then bail
		if (!pData) {
			//DevMsg("Strategy not defensive");
			return false;
		}
		bool bMatchedCriteria = false, bFoundCriteria = false;

		// iterate over all our data
		for(KeyValues *pair = pData->GetFirstSubKey(); pair != NULL; pair = pair->GetNextKey()) {
			// look for objective status
			if (FStrEq(pair->GetName(), "obj_status")) {
				// we did this already and won
				if (bMatchedCriteria)
					continue;

				// get the value
				const char *szStatus = pair->GetString();

				// must be length 3
				if(strlen(szStatus) != 3)
					continue;

				// found 1
				bFoundCriteria = true;

				// this is sloppy. consider refactoring..... 
				// look at the objectives and compare statuses
				for(int i = 0; i < 3; ++i) {
					// * = any
					if (szStatus[i] == '*') {
						// last time through, true!
						if (i == 2)
							bMatchedCriteria = true;
						// go again
						continue;
					}

					// get the objective in question
					SObjective_t *pObj = C_ObjectiveManager::GetInstance()->GetObjective(i);
					if (!pObj)
						break;

					// ! = not owned by my team
					if(szStatus[i] == '!')
					{
						// if i own it, bail
						if(pObj->iOwner == iMyTeam)
							break;
						// otherwise, if this is the last item we were successful
						else if(i == 2)
							bMatchedCriteria = true;

						// go again
						continue;
					}
					// ~ = not owned by the enemy team
					else if(szStatus[i] == '~')
					{
						// if the other team owns it, bail
						if(pObj->iOwner == (iMyTeam == TEAM_A ? TEAM_B : TEAM_A))
							break;
						// otherwise, if this is the last item we were successful
						else if(i == 2)
							bMatchedCriteria = true;

						// go again
						continue;
					}

					int iOwner = pObj->iOwner;
					// neutral objectives should be 0, not -1...
					if (iOwner < 0)
						iOwner = 0;

					// + 48 is the ascii value for 0...
					// do the owners match?
					if (iOwner + 48 == szStatus[i]) {
						// last time through, true!
						if (i == 2)
							bMatchedCriteria = true;
						// go again
						continue;
					}
					// didn't match so bail
					else
						break;
				}
			}
		}

		if (bFoundCriteria && !bMatchedCriteria) {
			//DevMsg("Didn't match objective status");
			return false;
		}
	}

	// passed all our tests, true!
	return true;
}

void CStrategyMenu::OnCommand(const char *command)
{
	if (FStrEq(command, "vguicancel")) {
		Close();
		gViewPortInterface->ShowBackGround(false);
	}
	else if (FStrEq(command, "next")) {
		// would there be items to display on the next page?
		if ((m_aPassesFilter.Count() > (m_iPage + 1) * 3)) {
			m_iPage++;
			PreparePanels(true);
		}
	}
	else if (FStrEq(command, "previous")) {
		if (m_iPage > 0) {
			m_iPage--;
			PreparePanels(true);
		}
	}
	else 
		BaseClass::OnCommand(command);
}

void CStrategyMenu::StrategyChosen(CStrategyMap *strategy)
{
	char szCommand[256];

	// will we be transmitting?
	if(!strategy->ShouldTransmit())
	{
		// get the file name
		const char* szFilename = strategy->GetFilename();

		// Notify the server.
		memset(szCommand, 0, sizeof(szCommand));
		Q_snprintf(szCommand, sizeof(szCommand), "choosestrategy 0 %s", szFilename);
		engine->ServerCmd(szCommand);
	}
	else
	{
		CUtlBuffer sBuffer;
		int iNumCommands;
		const char *szFormat = "choosestrategy 1 %d %s";
		const char *szBlockFormat = "choosestrategyblock 1 %d %s";
		KeyValues *pDefs;
		char szBlock[245];
		int iBytesToCopy, i;

		// pull out the definitions
		pDefs = (strategy->GetKeyValues() ? strategy->GetKeyValues()->FindKey("definition") : NULL);
		if(!pDefs)
		{
			Assert(0);
			Close();
			gViewPortInterface->ShowBackGround(false);
			return;
		}
			
		// write to the buffer and figure out how many commands we're going to need
		// first message always sent.  see how many more are needs for the trailing messages.
		pDefs->RecursiveSaveToFile(sBuffer, 0);
		if(sBuffer.TellPut() < ((int)sizeof(szBlock) - (Q_strlen(szFormat) - 5)))
			iNumCommands = 1;
		else
			iNumCommands = 1 + ceil((sBuffer.TellPut() - (sizeof(szBlock) - (Q_strlen(szFormat) - 5))) / 
									(float)(sizeof(szBlock) - (Q_strlen(szBlockFormat) - 5)));

		// how many bytes are we going to need to copy
		iBytesToCopy = min((int)sizeof(szBlock) - (Q_strlen(szFormat) - 5), sBuffer.TellPut() - sBuffer.TellGet());

		// setup the command and send it off
		// protocol: "choosestrategy 1 <number of commands> <data set>
		//			number of commands includes this command (1+)
		i = 0;
		do
		{
			// clear out the buffer and copy the new info in
			memset(szBlock, 0, sizeof(szBlock));
			memset(szCommand, 0, sizeof(szCommand));
			sBuffer.Get(szBlock, iBytesToCopy);
			Q_snprintf(szCommand, sizeof(szCommand), szFormat, iNumCommands - (++i), szBlock);
	
			// send off the command
			engine->ServerCmd(szCommand);

			// how many to copy this time?
			szFormat = szBlockFormat;
			iBytesToCopy = min((int)sizeof(szBlock) - (Q_strlen(szFormat) - 5), sBuffer.TellPut() - sBuffer.TellGet());

		} while(iBytesToCopy > 0);
	}

	Close();
	gViewPortInterface->ShowBackGround(false);
}

void CStrategyMenu::Hilite(CStrategyMap* pStrategy, bool bEnable)
{
	// figure out which strategy this is
	int i, iVisible;
	for(i = 0, iVisible = 0; i < m_aPassesFilter.Count() && iVisible < 3; ++i) {
		if (m_aPassesFilter[i] == pStrategy)
			break;

		if (m_aPassesFilter[i]->IsVisible())
			++iVisible;
	}

	if (iVisible < 3)
		m_aLabels[iVisible]->SetFgColor(bEnable ? Color(255, 176, 0, 255) : Color(255, 255, 255, 255));
}

void CStrategyMenu::Paint()
{
	if (m_pTimerLabel) {
		float fTime = (m_fCloseTime - gpGlobals->curtime);
		int iTime = floor(fTime);
		if (iTime != m_iCurrentTimer) {
			char szTime[10];
			Q_snprintf(szTime, 10, "%d", iTime);
			m_pTimerLabel->SetText(szTime);
			m_iCurrentTimer = iTime;
		}
	}

	if (gpGlobals->curtime > m_fCloseTime) {
		StrategyChosen(m_aPassesFilter.Element(random->RandomInt(0, m_aPassesFilter.Count() - 1)));
		Close();
		gViewPortInterface->ShowBackGround(false);
	}

	BaseClass::Paint();
}