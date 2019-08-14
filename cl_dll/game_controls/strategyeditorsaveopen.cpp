#include "cbase.h"
#include "strategyeditorsaveopen.h"
#include "strategyeditor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Constructor
**/
CStrategyEditorSaveOpen::CStrategyEditorSaveOpen(vgui::Panel *pParent, const char *szTitle, bool bOpenOnly)
	: BaseClass(pParent, szTitle, bOpenOnly)
{
	// we only want .txt files
	AddFilter("*.txt", "Text File (.txt)", true);

	// we want to know when the map changes
	gameeventmanager->AddListener(this, "game_newmap", false);
	m_szMapName = NULL;
}

/**
* Destructor
**/
CStrategyEditorSaveOpen::~CStrategyEditorSaveOpen()
{
	// kill the map name
	if(m_szMapName)
		delete [] m_szMapName;
}

/**
* Determines what to do when someone selects a file
*
* @return void
**/
void CStrategyEditorSaveOpen::OnOpen(void)
{
	char szFile[128];
	char szDir[512];

	// jump down
	BaseClass::OnOpen();

	// tell our parent which file
	GetSelectedFileName(szFile, sizeof(szFile));
	GetCurrentDirectory(szDir, sizeof(szDir));
	if(Q_strlen(szFile) > 0 && !filesystem->IsDirectory(szFile))
	{
		// saving or loading?
		if(m_eType == SO_SAVE)
			((CStrategyEditor *)GetParent())->SaveToFile(szFile, szDir);
		else if(m_eType == SO_OPEN)
			((CStrategyEditor *)GetParent())->LoadFromFile(szFile, szDir);
	}
}

/**
* Handles any events we need to listen for
*
* @param IGameEvent *pEvent The event that occurred
* @return void
**/
void CStrategyEditorSaveOpen::FireGameEvent(IGameEvent *pEvent)
{
	// make sure this the right event
	if(FStrEq(pEvent->GetName(), "game_newmap"))
	{
		// set the map
		SetMap(pEvent->GetString("mapname"));
	}
}

/**
* Sets the map to use for the open dialog
*
* @param const char *szMap The map to use
* @return void
**/
void CStrategyEditorSaveOpen::SetMap(const char *szMap)
{
	char szPath[256], szFullPath[512];

	// if the map didn't change, then bail
	if(m_szMapName && FStrEq(szMap, m_szMapName))
		return;

	// kill the old map
	if(m_szMapName)
		delete [] m_szMapName;

	// copy the name
	m_szMapName = new char[strlen(szMap) + 1];
	Q_strcpy(m_szMapName, szMap);

	// set the current directory
	Q_snprintf(szPath, sizeof(szPath), "%s/%s", STRATEGY_DIR, m_szMapName);
	filesystem->GetLocalPath(szPath, szFullPath, sizeof(szFullPath));
	if(filesystem->IsDirectory(szPath))
		SetStartDirectory(szFullPath);
	else
	{
		filesystem->GetLocalPath(STRATEGY_DIR, szFullPath, sizeof(szFullPath));
		SetStartDirectory(szFullPath);
	}
}

/**
* Sets the type of the dialog
*
* @param SOType eType The type to use
* @return void
**/
void CStrategyEditorSaveOpen::SetType(SOType eType)
{
	vgui::Button *pButton;

	// store the type
	m_eType = eType;

	// change the text of the button
	pButton = dynamic_cast<vgui::Button *>(FindChildByName("OpenButton"));
	if(pButton)
	{
		// check the type
		if(m_eType == SO_SAVE)
			pButton->SetText("Save");
		else if(m_eType == SO_OPEN)
			pButton->SetText("Open");
	}
}

/**
* Applies the scheme to the panel
*
* @param vgui::IScheme *pScheme The scheme to apply
* @return void
**/
void CStrategyEditorSaveOpen::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// not proportional
	SetProportional(false);
}