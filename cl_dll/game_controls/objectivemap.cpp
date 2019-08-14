#include "cbase.h"
#include "objectivemap.h"
#include "objectivemenu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Constructor
**/
CObjectiveMap::CObjectiveMap(vgui::Panel *pParent)
{
	// set our parent
	SetParent(pParent);

	// reset the map
	MapReset();

	// don't show anything
	m_bShowNames = false;
	m_bShowHealth = false;
	m_bShowObjectives = false;
	m_bFollowEntity = false;
	m_bShowPlayers = false;

	// set the zoom
	m_fZoom = 1.0;

	// set our drawing state
	SetEnabled(true);
	SetVisible(true);

	// create our buttons
	CreateButtons(pParent);
}

/**
* Destructor
**/
CObjectiveMap::~CObjectiveMap(void)
{
	// destroy our button list
	m_aButtons.Purge();
}

/**
* Resets the map
*
* @return void
**/
void CObjectiveMap::MapReset(void)
{
	// jump down
	BaseClass::MapReset();

	// reset our parent
	if(GetParent())
		((CObjectiveMenu *)GetParent())->Reset();

	// setup positioning
	SetBounds(OBJECTIVE_MAP_XPOS, OBJECTIVE_MAP_YPOS, OBJECTIVE_MAP_WIDTH, OBJECTIVE_MAP_HEIGHT);
	SetPos(OBJECTIVE_MAP_XPOS, OBJECTIVE_MAP_YPOS);
	SetSize(OBJECTIVE_MAP_WIDTH, OBJECTIVE_MAP_HEIGHT);
	SetMapSize(OBJECTIVE_MAP_WIDTH, OBJECTIVE_MAP_HEIGHT);
}

/**
* Sets the map to use
*
* @param const char *szLevelName The name of the level to load a map for
* @return void
**/
void CObjectiveMap::SetMap(const char *szLevelName)
{
	SObjective_t *pObj;
	Vector2D vecPos;

	// first, let the base class take care of it
	BaseClass::SetMap(szLevelName);

	// turn all of them off
	for(int i = 0; i < MAX_OBJECTIVES; ++i)
	{
		if(m_aButtons.IsValidIndex(i) && m_aButtons[i])
		{
			m_aButtons[i]->SetVisible(false);
			m_aButtons[i]->SetEnabled(false);
		}
	}

	// change the info for each of the buttons
	pObj = GET_OBJ_MGR()->GetNextObjective(NULL);
	for(; pObj != NULL; pObj = GET_OBJ_MGR()->GetNextObjective(pObj))
	{
		// make sure we have a valid id
		if(!m_aButtons.IsValidIndex(pObj->iID) || !m_aButtons[pObj->iID])
			continue;

		// where are we?
		vecPos = WorldToPanel(pObj->vecPos);

		// position over the objective
		m_aButtons[pObj->iID]->SetSize(XRES(OBJECTIVE_MAP_BUTTON_WIDTH), YRES(OBJECTIVE_MAP_BUTTON_HEIGHT));
		m_aButtons[pObj->iID]->SetPos(vecPos.x - (XRES(OBJECTIVE_MAP_BUTTON_WIDTH) / 2), 
										vecPos.y - (YRES(OBJECTIVE_MAP_BUTTON_HEIGHT) / 2));
		m_aButtons[pObj->iID]->SetTextureSize(OBJECTIVE_MAP_BUTTON_WIDTH, OBJECTIVE_MAP_BUTTON_HEIGHT);
		m_aButtons[pObj->iID]->SetImageCrop(OBJECTIVE_MAP_BUTTON_WIDTH, OBJECTIVE_MAP_BUTTON_HEIGHT);

		// set the icons
		m_aButtons[pObj->iID]->SetHiliteTexture(GetObjectiveIcon(pObj, true));
		m_aButtons[pObj->iID]->SetLoliteTexture(GetObjectiveIcon(pObj, false));

		// turn off and/or hide if we can't select
		m_aButtons[pObj->iID]->SetVisible(true);
		m_aButtons[pObj->iID]->SetEnabled(C_BasePlayer::GetLocalPlayer() ? pObj->iOwner != C_BasePlayer::GetLocalPlayer()->GetTeamNumber() : false);
	}
}

/**
* If our team changes we need to update the position of all of our buttons
*
* @return void
**/
void CObjectiveMap::TeamChanged(void)
{
	SObjective_t *pObj;
	Vector2D vecPos;

	// change the info for each of the buttons
	pObj = GET_OBJ_MGR()->GetNextObjective(NULL);
	for(; pObj != NULL; pObj = GET_OBJ_MGR()->GetNextObjective(pObj))
	{
		// make sure we have a valid id
		if(!m_aButtons.IsValidIndex(pObj->iID) || !m_aButtons[pObj->iID])
			continue;

		// where are we?
		vecPos = WorldToPanel(pObj->vecPos);

		// position over the objective
		m_aButtons[pObj->iID]->SetPos(vecPos.x - (XRES(OBJECTIVE_MAP_BUTTON_WIDTH) / 2), 
										vecPos.y - (YRES(OBJECTIVE_MAP_BUTTON_HEIGHT) / 2));
	}
}

/**
* Updates the buttons so the textures and status are correct
*
* @return
**/
void CObjectiveMap::UpdateButtons(void)
{
	SObjective_t *pObj;

	// change the info for each of the buttons
	pObj = GET_OBJ_MGR()->GetNextObjective(NULL);
	for(; pObj != NULL; pObj = GET_OBJ_MGR()->GetNextObjective(pObj))
	{
		// set the icons
		m_aButtons[pObj->iID]->SetHiliteTexture(GetObjectiveIcon(pObj, true));
		m_aButtons[pObj->iID]->SetLoliteTexture(GetObjectiveIcon(pObj, false));

		// turn off and/or hide if we can't select
		m_aButtons[pObj->iID]->SetVisible(true);
		m_aButtons[pObj->iID]->SetEnabled(pObj->iOwner != C_BasePlayer::GetLocalPlayer()->GetTeamNumber());
	}
}

/**
* Creates the buttons we are going to use on top of the map
*
* @param Panel *pParent Action signals are sent to this panel
* @return void
**/
void CObjectiveMap::CreateButtons(Panel *pParent)
{
	char szStr[256];

	// set the size
	m_aButtons.SetSize(MAX_OBJECTIVES);

	// create all of them
	for(int i = 0; i < MAX_OBJECTIVES; ++i)
	{
		// create the button and make sure its not visible yet
		Q_snprintf(szStr, sizeof(szStr), "chooseobjective_map_button_%d", i);
		m_aButtons[i] = new CHiliteImageButton(this, szStr, "");
		m_aButtons[i]->SetEnabled(false);
		m_aButtons[i]->SetVisible(false);

		// set the command
		Q_snprintf(szStr, sizeof(szStr), "chooseobjective %d", i);
		m_aButtons[i]->SetCommand(szStr);
		m_aButtons[i]->AddActionSignalTarget(pParent);
	}
}

/**
* Sets the neighbor button for the specified objective
*
* @param CHiliteButton *pButton The button we need to find a neighbor for
* @param SObjective_t *pObj The objective associated with the button
* @return CHiliteButton * The button that was assigned as a neighbor to pButton
**/
CHiliteButton *CObjectiveMap::SetButtonNeighbor(CHiliteButton *pButton, SObjective_t *pObj)
{
	// make sure we got a button
	if(!pButton || !pObj)
		return NULL;

	// see if we have that button
	if(!m_aButtons.IsValidIndex(pObj->iID) || !m_aButtons[pObj->iID])
		return NULL;

	// set the neighbor
	m_aButtons[pObj->iID]->SetNeighbor(pButton);

	return m_aButtons[pObj->iID];
}