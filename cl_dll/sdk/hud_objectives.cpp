#include "cbase.h"
#include "hud_objectives.h"
#include "iclientmode.h"
#include "c_sdk_player.h"
#include "FileSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Declare me!
DECLARE_HUDELEMENT(HudObjectives);

/**
* Constructor
**/
HudObjectives::HudObjectives(const char *szName)
	: CHudElement(szName), BGPanel(NULL, "HudObjectives")
{
	// set our parent to the current viewport
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// SetHiddenBits( HIDEHUD_OBSERVING );

	// not visible
	SetVisible(false);

	// no background
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	// no radar, no timer
	m_pRadar = NULL;
	m_pTimer = NULL;
}

/**
* Destructor
**/
HudObjectives::~HudObjectives()
{
	// kill the list
	m_aIconSets.PurgeAndDeleteElements();
}

/**
* Determines if we should actually draw anything
*
* @return bool
**/
bool HudObjectives::ShouldDraw(void)
{
	// do we have any objectives?
	return (GET_OBJ_MGR()->GetObjectivesExist() && CHudElement::ShouldDraw());
}

/**
* Handles events from the objective manager
*
* @param SObjective_t *pObj The objective that changed
* @return void
**/
void HudObjectives::OnObjectiveEvent(SObjective_t *pObj)
{
	int i;

	// do we have this one yet?
	if((i = FindIconSet(pObj->iID)) == -1)
		CreateIconSet(pObj);
	else
		UpdateIconSet(i, pObj);

	// reposition the icon set
	// i realize it is annoying that this happens every time...
	PositionIconSets();
}

/**
* Find the icon set specified by the id
*
* @param int iID The id of the label
* @return int
**/
int HudObjectives::FindIconSet(int iID)
{
	// start iterating
	for(int i = 0; i < m_aIconSets.Count(); ++i)
	{
		// is this it?
		if(m_aIconSets[i]->GetID() == iID)
			return i;
	}

	return -1;
}

/**
* Updates the status of the icon set
*
* @param int i The index of the label
* @param SObjective_t *pObj The objective to update the label with
* @return void
**/
void HudObjectives::UpdateIconSet(int i, SObjective_t *pObj)
{
	// do we have this one?
	if(!m_aIconSets.IsValidIndex(i))
		return;

	// if we're a base use the larger icon
	if(pObj->bIsBase)
		m_aIconSets[i]->SetIconSize(OBJECTIVE_ICONSET_WIDTH_LARGE, OBJECTIVE_ICONSET_HEIGHT_LARGE);
	else
		m_aIconSets[i]->SetIconSize(OBJECTIVE_ICONSET_WIDTH_SMALL, OBJECTIVE_ICONSET_HEIGHT_SMALL);

	// set the icon
	m_aIconSets[i]->DetermineIcon(pObj->bActive, pObj->iOwner);

	// set the base info
	m_aIconSets[i]->SetOwner(pObj->iOwner);
	m_aIconSets[i]->SetIsBase(pObj->bIsBase);
}

/**
* Creates a new icon set using the objective information
*
* @param SObjective_t *pObj The objective information to use
* @return void
**/	
void HudObjectives::CreateIconSet(SObjective_t *pObj)
{
	ObjectiveIconSet *pSet;

	// create the set
	pSet = new ObjectiveIconSet(pObj);
	m_aIconSets.AddToTail(pSet);

	// if we're a base use the larger icon
	if(pObj->bIsBase)
		pSet->SetIconSize(OBJECTIVE_ICONSET_WIDTH_LARGE, OBJECTIVE_ICONSET_HEIGHT_LARGE);
	else
		pSet->SetIconSize(OBJECTIVE_ICONSET_WIDTH_SMALL, OBJECTIVE_ICONSET_HEIGHT_SMALL);

	// figure out the color to use
	pSet->DetermineIcon(pObj->bActive, pObj->iOwner);
}

/**
* Applies the new scheme settings
*
* @param IScheme *pScheme The new scheme
* @return void
**/
void HudObjectives::ApplySchemeSettings(IScheme *pScheme)
{	
	// cache the scheme
	m_pScheme = pScheme;

	// move down
	BaseClass::ApplySchemeSettings(pScheme);

	// no background
	SetPaintBackgroundEnabled(false);
}

/**
* Takes care of resizing when the panel changes
*
* @return void
**/
void HudObjectives::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	SObjective_t *pObj = NULL;
	int i;

	// go down
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);

	// reset all the objectives
	for(pObj = GET_OBJ_MGR()->GetNextObjective(pObj); pObj != NULL; pObj = GET_OBJ_MGR()->GetNextObjective(pObj))
	{
		// grab the index
		i = FindIconSet(pObj->iID);
		if(i == -1)
			continue;

		// update
		UpdateIconSet(i, pObj);
	}

	// reposition everything
	PositionIconSets();
}

/**
* Positions the sets based on their sorted order
*
* @return void
**/
void HudObjectives::PositionIconSets(void)
{
	// sort them based on id
	m_aIconSets.Sort(ObjectiveIconSetCompare);

	// iterate and set positions
	for(int i = 0; i < m_aIconSets.Count(); ++i)
	{
		switch(i)
		{
		case 0:
			m_aIconSets[i]->SetIconPos(XRES(68), YRES(1));
			break;
		case 1:
			m_aIconSets[i]->SetIconPos(XRES(83), YRES(56));
			break;
		case 2:
			m_aIconSets[i]->SetIconPos(XRES(99), YRES(82));
			break;
		case 3:
			m_aIconSets[i]->SetIconPos(XRES(124), YRES(96));
			break;
		case 4:
			// scale down as well as positioning
			m_aIconSets[i]->SetIconSize((float)OBJECTIVE_ICONSET_WIDTH_LARGE * .75,
								(float)OBJECTIVE_ICONSET_HEIGHT_LARGE * .75);
			m_aIconSets[i]->SetIconPos(XRES(157), YRES(92));
			break;
		}
	}
}

/**
* Compares two objective labels for positioning
* 
* @param ObjectiveIconSet *const *ppLeft The first element
* @param ObjectiveIconSet *const *ppRight The second element
* @return int
**/
int HudObjectives::ObjectiveIconSetCompare(ObjectiveIconSet * const *ppLeft, ObjectiveIconSet * const *ppRight)
{
	// my base goes on top
	if((*ppLeft)->IsBase() && C_BasePlayer::GetLocalPlayer() && (*ppLeft)->GetOwner() == GetLocalPlayerTeamNumber())
		return -1;
	else if((*ppRight)->IsBase() && C_BasePlayer::GetLocalPlayer() && (*ppRight)->GetOwner() == GetLocalPlayerTeamNumber())
		return 1;
	// their base goes on the bottom
	else if((*ppLeft)->IsBase())
		return 1;
	else if((*ppRight)->IsBase())
		return -1;
	// sort everything else by position
	else
		return (*ppLeft)->GetID() - (*ppRight)->GetID();
}

int HudObjectives::GetLocalPlayerTeamNumber(void)
{
	if (!CBasePlayer::GetLocalPlayer())
		return TEAM_A;

	return clamp(CBasePlayer::GetLocalPlayer()->GetTeamNumber(), TEAM_A, TEAM_B);
}

/**
* Futzes with the texture name to add a prefix based on the clients team
*
* @param const char *szName Name of the texture
* @param char *szStr The string to fill in
* @param int iSize The amount of buffer space we have
* @return void
**/
void HudObjectives::GetBackgroundTextureName(const char *szName, char *szStr, int iSize)
{
	// do we have a player yet?
	if(CSDKPlayer::GetLocalPlayer())
	{
		// red or blue?
		if(CSDKPlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_B)
			Q_snprintf(szStr, iSize, "vgui/red/%s", szName);
		else
			Q_snprintf(szStr, iSize, "vgui/blue/%s", szName);
	}
}

/**
* Paints all of our icons
*
* @return void
**/
void HudObjectives::Paint(void)
{
	// if we don't have the radar, go find it
	if(!m_pRadar)
		m_pRadar = (HudRadar *)GET_HUDELEMENT(HudRadar);

	// if we don't have the timer, go find it
	if(!m_pTimer)
		m_pTimer = (HudObjectiveTimer *)GET_HUDELEMENT(HudObjectiveTimer);

	// draw the map
	if(m_pRadar)
		m_pRadar->DrawMap();

	// draw each icon set
	for(int i = 0; i < m_aIconSets.Count(); ++i)
	{
		// make sure it's a valid index
		if(!m_aIconSets[i])
			continue;

		// draw it
		m_aIconSets[i]->DrawSelf();
	}

	// draw the background over top of the icons
	BaseClass::Paint();

	// draw the timer ring
	if(m_pTimer)
		m_pTimer->DrawTimerRing();

	// draw the marker over top of everything
	if(m_pRadar)
		m_pRadar->DrawObjectiveMarker();
}

/*************************************************************************************/
/** Start ObjectiveIconSet *************************************************************/
/*************************************************************************************/

/**
* Constructor
**/
ObjectiveIconSet::ObjectiveIconSet(SObjective_t *pObj)
{
	char szStr[256];

	// set the cached data
	m_iID = pObj->iID;
	m_bIsBase = pObj->bIsBase;
	m_iOwner = pObj->iOwner;

	// no overlay
	m_bDrawActiveOverlay = false;

	// create our textures
	for(int i = 0; i < OBJECTIVE_ICON_COUNT; ++i)
	{
		// don't add an icon for the neutral base objectives
		if(m_bIsBase && i == OBJECTIVE_ICON_NEUTRAL)
		{
			AddToSet((CHudTexture *)NULL);
			continue;
		}

		Assert(pObj->iID >= 0);

		// figure out the name of the texture
		memset((void *)szStr, 0, 256);
		if(!m_bIsBase)
			Q_snprintf(szStr, 256, "vgui/hud_objectives_obj%d_%d", pObj->iID + 1, i);
		else if(m_bIsBase && i != OBJECTIVE_ICON_NEUTRAL)
			Q_snprintf(szStr, 256, "vgui/hud_objectives_obj%sbase", (i == OBJECTIVE_ICON_RED ? "red" : "blue"));

		// add it to the set
		AddToSet(szStr);
	}

	// create the overlay texture
	m_pActiveOverlay = new CHudTexture();

	// load the texture
	m_pActiveOverlay->textureId = UTIL_LoadTexture(OBJECTIVE_ICON_ACTIVE);
	m_pActiveOverlay->bRenderUsingFont = false;
}

/**
* Destructor
**/
ObjectiveIconSet::~ObjectiveIconSet()
{
	// kill the overlay
	delete m_pActiveOverlay;
}

/**
* Determines the active icon
*
* @param bool bActive True if the label is active
* @param int iStatus -1 or the ID of the team that owns it
* @return void
**/
void ObjectiveIconSet::DetermineIcon(bool bActive, int iOwner)
{
	// are we active?
	if(bActive)
	{
		// are we switching on?
		if(!m_bDrawActiveOverlay)
		{
			m_fAlpha = 0;
			m_fLastUpdate = gpGlobals->curtime;
			m_bGrow = true;
		}

		// draw the overlay
		m_bDrawActiveOverlay = true;
	}
	else
		m_bDrawActiveOverlay = false;

	// neutral?
	if(iOwner == -1)
		m_iActiveIcon = OBJECTIVE_ICON_NEUTRAL;
	// owned?
	else
	{
		// blue team
		if(iOwner == TEAM_A)
			m_iActiveIcon = OBJECTIVE_ICON_BLUE;
		// red team
		else if(iOwner == TEAM_B)
			m_iActiveIcon = OBJECTIVE_ICON_RED;
		else
			m_iActiveIcon = OBJECTIVE_ICON_NEUTRAL;
	}
}

/**
* Sets the size of the icon set
*
* @param int iWidth Width of the icon
* @param int iHeight Height of the icon
* @return void
**/
void ObjectiveIconSet::SetIconSize(int iWidth, int iHeight)
{
	// jump down
	BaseClass::SetIconSize(iWidth, iHeight);

	// set the active icon's size
	m_pActiveOverlay->SetBounds(0, iWidth, 0, iHeight);
}

/**
* Draws the correct icon from the set based on the current state of the objective
* 
* @return void
**/
void ObjectiveIconSet::DrawSelf(void)
{
	// draw the baseclass
	BaseClass::DrawSelf();

	// draw the overlay if necessary
	if(m_bDrawActiveOverlay)
	{
		// figure out the alpha
		m_fAlpha += (m_bGrow ? 1 : -1) * (gpGlobals->curtime - m_fLastUpdate) / OBJECTIVE_ICON_ACTIVE_PULSE;
		m_fLastUpdate = gpGlobals->curtime;

		// make sure we're not too high or low
		if(m_fAlpha > 255)
		{
			m_fAlpha = 255;
			m_bGrow = !m_bGrow;
		}
		else if(m_fAlpha < 0)
		{
			m_fAlpha = 0;
			m_bGrow = !m_bGrow;
		}

		// draw the overlay
		m_pActiveOverlay->DrawSelf(m_iXPos, m_iYPos, Color(255, 255, 255, m_fAlpha));
	}
}