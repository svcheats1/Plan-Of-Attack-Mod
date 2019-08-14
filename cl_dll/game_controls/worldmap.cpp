#include "cbase.h"
#include "worldmap.h"
#include "c_baseplayer.h"
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Constructor
**/
CWorldMap::CWorldMap(IViewPort *pViewPort)
: BaseClass(NULL)
{
	// cache the viewport
	m_pViewPort = pViewPort;

	// change the panel bounds and move to the middle
	SetBounds(0, 0, WORLDMAP_PANEL_WIDTH, WORLDMAP_PANEL_HEIGHT);
	SetPos((ScreenWidth() - WORLDMAP_PANEL_WIDTH) / 2, (ScreenHeight() - WORLDMAP_PANEL_HEIGHT) / 2);

	// show everything
	m_bShowNames = true;
	m_bShowHealth = true;
	m_bFollowEntity = false;
	m_bShowPlayers = true;
	m_bShowRelativeAltitude = true;

	// set the zoom
	m_fZoom = 1.0;

	// set the size of the panel to use
	SetMapSize(WORLDMAP_MAP_WIDTH, WORLDMAP_MAP_HEIGHT);

	// initialize the legend
	InitLegend();
}

/**
* Destructor
**/
CWorldMap::~CWorldMap()
{
	// kill the label list
	delete [] m_aLegendLabels;
}

/**
* Initializes the legend
*
* @return void
**/
void CWorldMap::InitLegend(void)
{
	char szStr[128];

	// create the labels
	m_aLegendLabels = new vgui::Label*[3];
	for(int i = 0; i < 3; ++i)
	{
		// create the label
		Q_snprintf(szStr, sizeof(szStr), "LegendLabel%d", i);
		m_aLegendLabels[i] = new vgui::Label(this, szStr, "Label");
	}
}

/**
* Updates the legend and positioning info
*
* @return void
**/
void CWorldMap::UpdateLegend(void)
{
	int iXPos;
	char szStr[256];
	SObjective_t *pObj;

	// set all the text first so we can autosize
	m_iLabelsWidth = 0;
	for(int i = 0; i < 3; ++i)
	{
		// set the text
		pObj = GET_OBJ_MGR()->GetObjective(i);
		if(pObj->iCash > 0 && pObj->iXP > 0)
			Q_snprintf(szStr, sizeof(szStr), "$%d, %d Experience", pObj->iCash, pObj->iXP);
		else if(pObj->iCash > 0 && pObj->iXP <= 0)
			Q_snprintf(szStr, sizeof(szStr), "$%d", pObj->iCash);
		else if(pObj->iCash <= 0 && pObj->iXP > 0)
			Q_snprintf(szStr, sizeof(szStr), "%d Experience", pObj->iXP);
		else
			Q_snprintf(szStr, sizeof(szStr), "Fix your map!");
		m_aLegendLabels[i]->SetText(szStr);

		// autosize it but take back the height
		m_aLegendLabels[i]->SizeToContents();
		m_iLabelsWidth += m_aLegendLabels[i]->GetWide() + XRES(20);
		m_aLegendLabels[i]->SetTall(WORLDMAP_LEGEND_LABEL_HEIGHT);
	}
	m_iLabelsWidth -= XRES(20);

	// reposition everything
	iXPos = (GetWide() / 2) - ((MAP_ICON_SCALE * 3) + m_iLabelsWidth) / 2;
	iXPos += MAP_ICON_SCALE;
	for(int i = 0; i < 3; ++i)
	{
		// figure out the position
		m_aLegendLabels[i]->SetPos(iXPos, WORLDMAP_LEGEND_YPOS - (MAP_ICON_SCALE / 2));
		iXPos += MAP_ICON_SCALE + m_aLegendLabels[i]->GetWide() + XRES(20);

		// position in the cetner
		m_aLegendLabels[i]->SetContentAlignment(vgui::Label::a_west);
	}
}

/**
* Resets the map between rounds
*
* @return void
**/
void CWorldMap::ResetRound(void)
{
	// jump down
	BaseClass::ResetRound();

	// make sure the legend is up to date
	UpdateLegend();
}

/**
* Displays the panel
*
* @param bool bShow Should we be hiding or showing?
* @return void
**/
void CWorldMap::ShowPanel(bool bShow)
{
	// don't do anything if we're already all set
	if(BaseClass::IsVisible() == bShow)
		return;

	// turn us on
	SetVisible( bShow );
}

/**
* Sets the data coming from our resource file
* 
* @param KeyValues *pData The data we need to process
* @return void
**/
void CWorldMap::SetData(KeyValues *pData)
{
}

/**
* Applies the current scheme to the panel
*
* @param vgui::IScheme *pScheme The scheme to use
* @return void
**/
void CWorldMap::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// go down
	BaseClass::ApplySchemeSettings(pScheme);

	// give us some background with round corners
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(2);

	// change the panel bounds and move to the middle
	SetBounds(0, 0, WORLDMAP_PANEL_WIDTH, WORLDMAP_PANEL_HEIGHT);
	SetPos((ScreenWidth() - WORLDMAP_PANEL_WIDTH) / 2, (ScreenHeight() - WORLDMAP_PANEL_HEIGHT) / 2);

	// set the size of the panel to use
	SetMapSize(WORLDMAP_MAP_WIDTH, WORLDMAP_MAP_HEIGHT);
}

/**
* Resets the map info
*
* @return void
**/
void CWorldMap::MapReset(void)
{
	// jump down
	BaseClass::MapReset();

	// change the panel bounds and move to the middle
	//SetBounds(0, 0, WORLDMAP_PANEL_WIDTH, WORLDMAP_PANEL_HEIGHT);
	//SetPos((ScreenWidth() - WORLDMAP_PANEL_WIDTH) / 2, (ScreenHeight() - WORLDMAP_PANEL_HEIGHT) / 2);

	// set the size of the map
	SetMapSize(WORLDMAP_MAP_WIDTH, WORLDMAP_MAP_HEIGHT);

	// update the legend
	UpdateLegend();
}

/**
* Paints the map
*
* @return void
**/
void CWorldMap::Paint(void)
{
	// jump down
	BaseClass::Paint();

	// draw the legend
	DrawLegend();
}

/**
* Draws the legend for the objectives in the map
*
* @return void
**/
void CWorldMap::DrawLegend(void)
{
	int iIcon;
	SObjective_t *pObj;
	Vector2D vecPos;
	Vector2D vecOffset;
	int iXPos;

	// figure out the first x position
	iXPos = (GetWide() / 2) - ((MAP_ICON_SCALE * 3) + m_iLabelsWidth) / 2;

	// flip through our objectives
	for(pObj = GET_OBJ_MGR()->GetNextObjective(); pObj; pObj = GET_OBJ_MGR()->GetNextObjective(pObj))
	{
		// skip based
		if(pObj->bIsBase)
			continue;

		// figure out which icon to use
		iIcon = m_aObjectiveIcons[NEUTRAL_TEAM][ICON_STATE_DISABLED][pObj->iID];

		// figure out where it goes
		vecPos.x = iXPos;
		vecPos.y = WORLDMAP_LEGEND_YPOS;
		vecOffset.x = vecOffset.y = 0;

		// move the x pos along
		iXPos += m_aLegendLabels[pObj->iID]->GetWide() + XRES(20) + MAP_ICON_SCALE;

		// draw it at the proper location
		DrawTexturedSquare(vecPos, vecOffset, 0, MAP_ICON_SCALE, iIcon);
	}
}

/**
* Sets the size of the map
* Don't want to set the size of the panel!!!!!
*
* @param int wide
* @param int tall
* @return void
**/
void CWorldMap::SetMapSize(int wide, int tall)
{ 
	BaseClass::SetMapSize(wide, tall); 
	
	// make sure the panel size doesn't change!
	SetSize(WORLDMAP_PANEL_WIDTH, WORLDMAP_PANEL_HEIGHT);
}