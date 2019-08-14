#include "cbase.h"
#include "strategyeditor.h"
#include "c_baseplayer.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Constructor
**/
CStrategyEditor::CStrategyEditor(IViewPort *pViewPort)
	: Frame(NULL, PANEL_STRATEGYEDITOR), m_pViewPort(pViewPort)
{
	// set the parent
	SetParent(enginevgui->GetPanel(PANEL_GAMEUIDLL));

	// set the scheme and title
	SetScheme("ClientScheme");
	SetTitle("#StrategyEditor_Menu", true);

	// not initially visible, but enabled
	SetVisible(false);
	SetEnabled(true);
	SetMoveable(true);
	SetSizeable(false);
	SetPaintBackgroundType(2);
	SetPaintBackgroundEnabled(true);
	SetMenuButtonVisible(false);
	SetProportional(true);

	// load the layout
	LoadControlSettings("resource/ui/StrategyEditor.res");

	// establish all our components
	EstablishComponents();

	// no scheme yet
	m_pScheme = NULL;

	// no team / no map
	memset(m_szLastTeam, 0, sizeof(m_szLastTeam));
	memset(m_szLastMap, 0, sizeof(m_szLastMap));
}

/**
* Destructor
**/
CStrategyEditor::~CStrategyEditor()
{
	// something here...
}

/**
* Activates our panel
*
* @return void
**/
void CStrategyEditor::Activate(void)
{
	// jump down
	BaseClass::Activate();

	// make sure we are using the ui panel as our parent so we can appear outside of the menu
	SetParent(enginevgui->GetPanel(PANEL_GAMEUIDLL));
}

/**
* Establishes all the components of the panel
*
* @return void
**/
void CStrategyEditor::EstablishComponents(void)
{
	KeyValues *pData;
	int iRow;

	// create our filesystem dialog
	m_pOpenSaveDialog = new CStrategyEditorSaveOpen(this, "Strategy Editor", false);

	// grab the map combo and add everything we have maps for
	m_pMapCombo = (vgui::ComboBox *)FindChildByName("MapCombo");
	LoadMaps();

	// grab each of the arrow buttons
	m_pArrow1Button = (CHiliteTextButton *)FindChildByName("Arrow1Button");
	m_pArrow2Button = (CHiliteTextButton *)FindChildByName("Arrow2Button");
	m_pArrow3Button = (CHiliteTextButton *)FindChildByName("Arrow3Button");

	// setup the hilite
	m_pArrow1Button->Hilite(true, this, HILITE_CONDITION_NEIGHBOR);
	m_pArrow2Button->Hilite(false, this, HILITE_CONDITION_NEIGHBOR);
	m_pArrow3Button->Hilite(false, this, HILITE_CONDITION_NEIGHBOR);

	// pull the rest of the elements
	m_pStrategyNameText = (vgui::TextEntry *)FindChildByName("StrategyName");
	m_pTeamCombo = (vgui::ComboBox *)FindChildByName("TeamCombo");
	m_pTypeCombo = (vgui::ComboBox *)FindChildByName("TypeCombo");
	m_pTargetCombo = (vgui::ComboBox *)FindChildByName("TargetCombo");
	m_pObjective1Combo = (vgui::ComboBox *)FindChildByName("Objective1Combo");
	m_pObjective2Combo = (vgui::ComboBox *)FindChildByName("Objective2Combo");
	m_pObjective3Combo = (vgui::ComboBox *)FindChildByName("Objective3Combo");

	// grab all the labels
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("StrategyMenuLabel"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("MapInfoLabel"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("MapLabel"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("StrategyInfoLabel"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("StrategyNameLabel"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("TeamLabel"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("TypeLabel"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("TargetLabel"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("ObjectiveControlLabel"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("Objective1Label"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("Objective2Label"));
	m_aLabels.AddToTail((vgui::Label *)FindChildByName("Objective3Label"));

	// set the default name
	m_pStrategyNameText->SetText("CustomStrategy");

	// set the team options
	pData = new KeyValues("team", "team", 1);
	iRow = m_pTeamCombo->AddItem("American Alliance", pData);
	if(CBasePlayer::GetLocalPlayer() && CBasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_A)
	{
		// activate the row and flip the map
		m_pTeamCombo->ActivateItemByRow(iRow);
		RotateMapForTeam(TEAM_A);
	}
	pData->deleteThis();
	pData = new KeyValues("team", "team", 2);
	iRow = m_pTeamCombo->AddItem("Coalition Forces", pData);
	if(CBasePlayer::GetLocalPlayer() && CBasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_B)
	{
		// activate the row and flip the map
		m_pTeamCombo->ActivateItemByRow(iRow);
		RotateMapForTeam(TEAM_B);
	}
	pData->deleteThis();
	if(!CBasePlayer::GetLocalPlayer()
			|| (CBasePlayer::GetLocalPlayer()->GetTeamNumber() != TEAM_A
				&& CBasePlayer::GetLocalPlayer()->GetTeamNumber() != TEAM_B))
	{
		// activate the row and flip the map
		m_pTeamCombo->ActivateItemByRow(0);
		RotateMapForTeam(TEAM_A);
	}

	m_pTeamCombo->SelectNoText();

	// set the type combo options
	pData = new KeyValues("type", "type", "attacking");
	m_pTypeCombo->AddItem("Attack", pData);
	pData->deleteThis();
	pData = new KeyValues("type", "type", "defending");
	m_pTypeCombo->AddItem("Defend", pData);
	pData->deleteThis();
	m_pTypeCombo->ActivateItemByRow(0);
	m_pTypeCombo->SelectNoText();

	// set the target combo options
	pData = new KeyValues("target", "objective", 1);
	m_pTargetCombo->AddItem("Objective 1", pData);
	pData->deleteThis();
	pData = new KeyValues("target", "objective", 2);
	m_pTargetCombo->AddItem("Objective 2", pData);
	pData->deleteThis();
	pData = new KeyValues("target", "objective", 3);
	m_pTargetCombo->AddItem("Objective 3", pData);
	pData->deleteThis();
	pData = new KeyValues("target", "objective", 0);
	m_pTargetCombo->AddItem("Base", pData);
	pData->deleteThis();
	m_pTargetCombo->ActivateItemByRow(0);
	m_pTargetCombo->SelectNoText();

	// set the objective combo options
	pData = new KeyValues("obj_status", "obj_status", "*");
	m_pObjective1Combo->AddItem("Any", pData);
	m_pObjective2Combo->AddItem("Any", pData);
	m_pObjective3Combo->AddItem("Any", pData);
	pData->deleteThis();
	pData = new KeyValues("obj_status", "obj_status", "1");
	m_pObjective1Combo->AddItem("American Alliance", pData);
	m_pObjective2Combo->AddItem("American Alliance", pData);
	m_pObjective3Combo->AddItem("American Alliance", pData);
	pData->deleteThis();
	pData = new KeyValues("obj_status", "obj_status", "2");
	m_pObjective1Combo->AddItem("Coalition Forces", pData);
	m_pObjective2Combo->AddItem("Coalition Forces", pData);
	m_pObjective3Combo->AddItem("Coalition Forces", pData);
	pData->deleteThis();
	pData = new KeyValues("obj_status", "obj_status", "0");
	m_pObjective1Combo->AddItem("Neutral", pData);
	m_pObjective2Combo->AddItem("Neutral", pData);
	m_pObjective3Combo->AddItem("Neutral", pData);
	pData->deleteThis();
	pData = new KeyValues("obj_status", "obj_status", "!");
	m_pObjective1Combo->AddItem("Not My Team", pData);
	m_pObjective2Combo->AddItem("Not My Team", pData);
	m_pObjective3Combo->AddItem("Not My Team", pData);
	pData->deleteThis();
	pData = new KeyValues("obj_status", "obj_status", "~");
	m_pObjective1Combo->AddItem("Not The Enemy Team", pData);
	m_pObjective2Combo->AddItem("Not The Enemy Team", pData);
	m_pObjective3Combo->AddItem("Not The Enemy Team", pData);
	pData->deleteThis();
	m_pObjective1Combo->ActivateItemByRow(0);
	m_pObjective2Combo->ActivateItemByRow(0);
	m_pObjective3Combo->ActivateItemByRow(0);
	m_pObjective1Combo->SelectNoText();
	m_pObjective2Combo->SelectNoText();
	m_pObjective3Combo->SelectNoText();
}

/**
* Loads all the map data for our combo box
*
* @return void
**/
void CStrategyEditor::LoadMaps(void)
{
	FileFindHandle_t sSearchHandle;
	const char *szFileName;
	char szMap[64];
	KeyValues *pValues;

	// find all the meta files
	szFileName = vgui::filesystem()->FindFirst("maps/meta/*.txt", &sSearchHandle); 
	while(szFileName != NULL)
	{
		// sort out the name of the map
		Q_strncpy(szMap, szFileName, sizeof(szMap));
		szMap[min(sizeof(szMap), strlen(szFileName)) - 4] = 0;

		// add the item
		pValues = new KeyValues("map", "map", szMap);
		m_pMapCombo->AddItem(szMap, pValues);

		// move along
		szFileName = vgui::filesystem()->FindNext(sSearchHandle);
	}
}

/**
* Resets the entire strategy
*
* @return void
**/
void CStrategyEditor::Reset(void)
{
	// just reset the map
	m_pMap->Reset();

	// setup the hilite
	m_pArrow1Button->Hilite(true, this, HILITE_CONDITION_NEIGHBOR);
	m_pArrow2Button->Hilite(false, this, HILITE_CONDITION_NEIGHBOR);
	m_pArrow3Button->Hilite(false, this, HILITE_CONDITION_NEIGHBOR);
}

/**
* Creates a control of the specified type
*
* @param const char *szControlName The name of the control to create
* @return Panel *
**/
vgui::Panel *CStrategyEditor::CreateControlByName(const char *szControlName)
{
	// hilites?
	if(FStrEq(szControlName, "HiliteTextButton"))
		return new CHiliteTextButton(this, "", "");
	// strategy map?
	else if(FStrEq(szControlName, "StrategyEditorMap"))
	{
		// create the map
		m_pMap = new CStrategyEditorMap(this);
		m_pMap->Init();
		m_pMap->SetCurrentArrow(0);

		return m_pMap;
	}

	return BaseClass::CreateControlByName(szControlName);
}

/**
* Paints the panel
*
* @return void
**/
void CStrategyEditor::Paint(void)
{
	// jump down
	BaseClass::Paint();
}

/**
* Applies the scheme to our panel
*
* @param vgui::IScheme *pScheme The scheme to use
* @return void
**/
void CStrategyEditor::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// make sure it's proportional
	SetProportional(true);

	// change the panel bounds and move to the middle
	SetBounds(0, 0, STRATEGY_EDITOR_WIDTH, STRATEGY_EDITOR_HEIGHT);
	SetPos((ScreenWidth() - STRATEGY_EDITOR_WIDTH) / 2, (ScreenHeight() - STRATEGY_EDITOR_HEIGHT) / 2);

	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// not initially visible, but enabled
	SetMoveable(true);
	SetSizeable(false);
	SetPaintBackgroundType(2);
	SetPaintBackgroundEnabled(true);
	SetMenuButtonVisible(false);
	SetBgColor(pScheme->GetColor("StrategyEditorBg", Color(0, 0, 0, 255)));
	SetProportional(true);

	// make everything ready
	m_pTeamCombo->GetMenu()->MakeReadyForUse();
	m_pTypeCombo->GetMenu()->MakeReadyForUse();
	m_pTargetCombo->GetMenu()->MakeReadyForUse();
	m_pObjective1Combo->GetMenu()->MakeReadyForUse();
	m_pObjective2Combo->GetMenu()->MakeReadyForUse();
	m_pObjective3Combo->GetMenu()->MakeReadyForUse();

	// set our label color
	for(int i = 0; i < m_aLabels.Count(); ++i)
		m_aLabels[i]->SetFgColor(pScheme->GetColor("StrategyEditorText", Color(255, 255, 255, 255)));
}

/**
* Displays the panel
*
* @param bool bShow True if we want to display the panel
* @return void
**/
void CStrategyEditor::ShowPanel(bool bShow)
{
	// make it visible and enable control
	if(bShow)
	{
		// activate ourselves
		Activate();

		// make sure the right team is selected
		if(CBasePlayer::GetLocalPlayer() && CBasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_A)
		{
			// activate the row and flip the map
			m_pTeamCombo->ActivateItemByRow(0);
			RotateMapForTeam(TEAM_A);
		}
		else if(CBasePlayer::GetLocalPlayer() && CBasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_B)
		{
			// activate the row and flip the map
			m_pTeamCombo->ActivateItemByRow(1);
			RotateMapForTeam(TEAM_A);
		}

		// show it
		SetVisible(true);
		SetMouseInputEnabled(true);
	}
	else
	{
		// set the correct map
		if(engine->GetLevelName() && m_pMap)
		{
			// set the map
			m_pMap->SetMap(engine->GetLevelName());
			if(m_pOpenSaveDialog)
				m_pOpenSaveDialog->SetMap(engine->GetLevelName());
		}

		// hide it and all that
		SetVisible(false);
		SetMouseInputEnabled(false);
	}
}

/**
* Allows us to think a bit
*
* @return void
**/
void CStrategyEditor::OnThink(void)
{	
	char szTemp[64];
	KeyValues *pValues;

	// this is incredibly stupid but the message stuff in vgui appears to be messed up
	m_pTeamCombo->GetText(szTemp, sizeof(szTemp));
	if(!FStrEq(m_szLastTeam, szTemp))
	{
		// pull the key values
		pValues = m_pTeamCombo->GetActiveItemUserData();

		// set the new one and rotate the map accordingly
		Q_strncpy(m_szLastTeam, szTemp, sizeof(m_szLastTeam));
		RotateMapForTeam(pValues->GetInt("team"));
	}

	// this is also incredibly stupid
	m_pMapCombo->GetText(szTemp, sizeof(szTemp));
	if(!FStrEq(m_szLastMap, szTemp))
	{
		// pull out the key values
		pValues = m_pMapCombo->GetActiveItemUserData();

		// set the new map
		if(m_pMap)
			m_pMap->SetMap(pValues->GetString("map"));
		if(m_pOpenSaveDialog)
			m_pOpenSaveDialog->SetMap(pValues->GetString("map"));

		// rotate the map for the proper team
		pValues = m_pTeamCombo->GetActiveItemUserData();
		RotateMapForTeam(pValues->GetInt("team"));
	}

	// if we're in defense mode we need to disable the objective selection
	m_pTypeCombo->GetText(szTemp, sizeof(szTemp));
	if(FStrEq(szTemp, "Defend"))
		m_pTargetCombo->SetEnabled(false);
	else
		m_pTargetCombo->SetEnabled(true);
}

/**
* Determines if we need an update
*
* @return bool
**/
bool CStrategyEditor::NeedsUpdate(void)
{
	return false;
}

/**
* Updates our panel
*
* @return void
**/
void CStrategyEditor::Update(void)
{
	// ?
}

/**
* Determines what to do when we receive certain commands
*
* @param const char *szCommand The command to handle
* @return void
**/
void CStrategyEditor::OnCommand(const char *szCommand)
{
	// save?
	if(FStrEq(szCommand, "save"))
		Save();
	else if(FStrEq(szCommand, "load"))
		Load();
	else if(FStrEq(szCommand, "arrow1"))
	{
		// hilite this one and turn the rest off
		m_pArrow1Button->Hilite(true, this, HILITE_CONDITION_NEIGHBOR);
		m_pArrow2Button->Hilite(false, this, HILITE_CONDITION_NEIGHBOR);
		m_pArrow3Button->Hilite(false, this, HILITE_CONDITION_NEIGHBOR);

		// set the current arrow
		m_pMap->SetCurrentArrow(0);
	}
	else if(FStrEq(szCommand, "arrow2"))
	{
		// hilite this one and turn the rest off
		m_pArrow2Button->Hilite(true, this, HILITE_CONDITION_NEIGHBOR);
		m_pArrow1Button->Hilite(false, this, HILITE_CONDITION_NEIGHBOR);
		m_pArrow3Button->Hilite(false, this, HILITE_CONDITION_NEIGHBOR);

		// set the current arrow
		m_pMap->SetCurrentArrow(1);
	}
	else if(FStrEq(szCommand, "arrow3"))
	{
		// hilite this one and turn the rest off
		m_pArrow3Button->Hilite(true, this, HILITE_CONDITION_NEIGHBOR);
		m_pArrow1Button->Hilite(false, this, HILITE_CONDITION_NEIGHBOR);
		m_pArrow2Button->Hilite(false, this, HILITE_CONDITION_NEIGHBOR);

		// set the current arrow
		m_pMap->SetCurrentArrow(2);
	}
	else if(FStrEq(szCommand, "reset"))
		Reset();
	else if(FStrEq(szCommand, "vguicancel"))
	{
		// reset the panel
		Reset();
		ShowPanel(false);
	}
}

/**
* Saves the current arrow set
*
* @return void
**/
void CStrategyEditor::Save(void)
{
	// make the save dialog visible
	m_pOpenSaveDialog->SetType(SO_SAVE);
	m_pOpenSaveDialog->SetVisible(false);
	m_pOpenSaveDialog->DoModal(false);

	// move the panel around a bit so it's not over the arrows
	// has to happen after domodal
	m_pOpenSaveDialog->SetPos(XRES(390), (ScreenHeight() / 2) - (m_pOpenSaveDialog->GetTall() / 2));
}

/**
* Performs the actual save to the given file
*
* @param const char *szFile The file to save to
* @param const char *szDir The directory to save in
* @return void
**/
void CStrategyEditor::SaveToFile(const char *szFile, const char *szDir)
{
	char szStr[512];
	KeyValues *pValues, *pDefs;

	// make sure we have a reasonable file name
	if(Q_strlen(szFile) <= 0)
		return;

	// grab the key values
	pValues = m_pMap->GetKeyValues(true);
	pDefs = pValues->FindKey("definition");
	if(!pDefs)
	{
		Assert(0);
		return;
	}

	// set the name
	m_pStrategyNameText->GetText(szStr, sizeof(szStr));
	pDefs->SetString("name", szStr);

	// add everything else
	AddDataFromCombos(pValues);
		
	// write them them to our file and clear the key values
	if(Q_strlen(szFile) < 5 || !FStrEq(&(szFile[Q_strlen(szFile) - 4]), ".txt"))
		Q_snprintf(szStr, sizeof(szStr), "%s%s.txt", szDir, szFile);
	else
		Q_snprintf(szStr, sizeof(szStr), "%s%s", szDir, szFile);
	pValues->SaveToFile(filesystem, szStr);
}

/**
* Adds all the data from the combos
*
* @param KeyValues *pValues The key values to add the combo data to
* @return void
**/
void CStrategyEditor::AddDataFromCombos(KeyValues *pValues)
{
	KeyValues *pData, *pType;
	char szStr[4];

	// it's custom so we're transmitting
	pValues->SetInt("transmit", 1);

	// set the team
	pData = m_pTeamCombo->GetActiveItemUserData();
	pValues->SetInt("team", pData->GetInt("team"));

	// remove an existing type sections
	pType = pValues->FindKey("attacking");
	if(pType)
	{
		// pull it out and kill it
		pValues->RemoveSubKey(pType);
		pType->deleteThis();
	}
	pType = pValues->FindKey("defending");
	if(pType)
	{
		// pull it out and kill it
		pValues->RemoveSubKey(pType);
		pType->deleteThis();
	}

	// create the type section
	pData = m_pTypeCombo->GetActiveItemUserData();
	pType = pValues->FindKey(pData->GetString("type"), true);
	
	// set the objective if we're attacking
	if(FStrEq(pType->GetName(), "attacking"))
	{
		pData = m_pTargetCombo->GetActiveItemUserData();
		pType->SetInt("objective", pData->GetInt("objective"));
	}

	// set the obj_status string
	memset(szStr, 0, sizeof(szStr));
	pData = m_pObjective1Combo->GetActiveItemUserData();
	Q_snprintf(szStr, 1, "%s", pData->GetString("obj_status"));
	pData = m_pObjective2Combo->GetActiveItemUserData();
	Q_snprintf(szStr + 1, 1, "%s", pData->GetString("obj_status"));
	pData = m_pObjective3Combo->GetActiveItemUserData();
	Q_snprintf(szStr + 2, 1, "%s", pData->GetString("obj_status"));
	pType->SetString("obj_status", szStr);
}

/**
* Loads a new arrow set
*
* @return void
**/
void CStrategyEditor::Load(void)
{
	// make the save dialog visible
	m_pOpenSaveDialog->SetType(SO_OPEN);
	m_pOpenSaveDialog->SetVisible(false);
	m_pOpenSaveDialog->DoModal(false);
}

/**
* Loads the arrow set from key values
*
* @param const char *szFile The name of the file to load from
* @param const char *szDir The directory the file is in
* @return void
**/
void CStrategyEditor::LoadFromFile(const char *szFile,  const char *szDir)
{
	char szStr[512];
	KeyValues *pValues, *pDefs, *pType;
		
	// clear everything in the map
	m_pMap->ClearPoints();

	// figure out where the file is
	Q_snprintf(szStr, sizeof(szStr), "%s%s", szDir, szFile);

	// create the key values and load them from our file
	pValues = new KeyValues("strategy");
	pValues->LoadFromFile(filesystem, szStr);

	// pull out the definitions section and kill all the rest of the info
	pDefs = pValues->FindKey("definition");
	if(!pDefs)
	{
		pValues->deleteThis();
		return;
	}

	// set the key values and load from them
	m_pMap->SetKeyValues(pValues);
	m_pMap->LoadFromKeyValues(pDefs);

	// setup the name
	m_pStrategyNameText->SetText(pDefs->GetString("name"));

	// set the team drop down
	m_pTeamCombo->ActivateItemByRow(pValues->GetInt("team") - 1);
	RotateMapForTeam(pValues->GetInt("team"));

	// pull the type info
	pType = pValues->FindKey("attacking");
	if(!pType)
	{
		// try defending
		pType = pValues->FindKey("defending");
		if(!pType)
			return;
	}

	// set the type
	m_pTypeCombo->ActivateItemByRow(FStrEq(pType->GetName(), "attacking") ? 0 : 1);

	// set the objective
	m_pTargetCombo->ActivateItemByRow(pType->GetInt("objective") == 0 ? 3 : pType->GetInt("objective") - 1);

	// set the status info
	Q_strncpy(szStr, pType->GetString("obj_status"), sizeof(szStr));
	ActivateObjectiveComboRow(m_pObjective1Combo, szStr[0]);
	ActivateObjectiveComboRow(m_pObjective2Combo, szStr[1]);
	ActivateObjectiveComboRow(m_pObjective3Combo, szStr[2]);
}

/**
* Activates the row in the combo that corresponds to the item
*
* @param vgui::ComboBox *pBox The combo box whose item to activate
* @param char cItem The data for the row to activate
* @return void
**/
void CStrategyEditor::ActivateObjectiveComboRow(vgui::ComboBox *pBox, char cItem)
{
	// check the item
	switch(cItem)
	{
	case '*':
		pBox->ActivateItemByRow(0);
		break;
	case '1':
		pBox->ActivateItemByRow(1);
		break;
	case '2':
		pBox->ActivateItemByRow(2);
		break;
	case '0':
		pBox->ActivateItemByRow(3);
		break;
	case '!':
		pBox->ActivateItemByRow(4);
		break;
	case '~':
		pBox->ActivateItemByRow(5);
		break;
	default:
		Assert(0);
		pBox->ActivateItemByRow(0);
	}
}

/**
* Rotates the map to the orientation for the specified team
*
* @param int iTeam The team to rotate for
* @return void
**/
void CStrategyEditor::RotateMapForTeam(int iTeam)
{
	// set the active team
	if(m_pMap)
		m_pMap->SetActiveTeam(iTeam);
}