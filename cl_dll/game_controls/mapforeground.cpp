#include "cbase.h"
#include "mapforeground.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

/**
* Constructor
**/
CMapForeground::CMapForeground(vgui::Panel *pParent, const char *szName)
	: BaseClass(pParent, szName)
{
	// set the parent
	m_pParent = (CMap *)pParent;
}

/**
* Paints the background for the map
*
* @return void
**/
void CMapForeground::Paint(void)
{
	// update our following info
	m_pParent->UpdateFollowingInfo();

	// draw everything
	Draw();

	// jump down
	BaseClass::Paint();
}

/**
* Draws items in the foreground
*
* @return void
**/
void CMapForeground::Draw(void)
{
	// draws the objectives
	m_pParent->DrawObjectives(m_pParent->GetFollowAngle());

	// draw the players
	m_pParent->DrawMapPlayers();
}