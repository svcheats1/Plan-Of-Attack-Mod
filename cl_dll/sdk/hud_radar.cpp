#include "cbase.h"
#include "hud_radar.h"
#include "iclientmode.h"
#include "c_sdk_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Declare me!
DECLARE_HUDELEMENT(HudRadar);

/**
* Constructor
**/
HudRadar::HudRadar(const char *szName)
	: CHudElement(szName)
{
	// set our parent to the current viewport
	Panel *pParent = g_pClientMode->GetViewport();
	vgui::Panel::SetParent(pParent);

	// hide when this stuff happens
	// SetHiddenBits( HIDEHUD_OBSERVING );

	// set the name
	SetName("HudRadar");

	// not visible
	SetVisible( false );

	// show the background
	SetPaintBackgroundEnabled(true);
	SetPaintBorderEnabled(false);

	m_fZoom = 1.0;

	// don't rotate for teams
	m_bRespectTeamRotations = false;

	// no names so that they don't hang outside the map
	m_bShowNames = false;

	m_bShowRelativeAltitude = false;

	// load the marker texture
	m_iMarkerTexture = UTIL_LoadTexture(RADAR_MARKER_TEXTURE);

	m_iDeadTexture = UTIL_LoadTexture(RADAR_DEAD_TEXTURE);

	// set hte size of the map
	SetMapSize(RADAR_RADIUS * 2, RADAR_RADIUS * 2);
}

/**
* Destructor
**/
HudRadar::~HudRadar()
{
	// ?
}

/**
* Determines if we should actually draw anything
*
* @return bool
**/
bool HudRadar::ShouldDraw(void)
{
	// do we have any objectives?
	return (GET_OBJ_MGR()->GetObjectivesExist() 
		&& CHudElement::ShouldDraw());
}

/**
* Applies the new scheme settings
*
* @param IScheme *pScheme The new scheme
* @return void
**/
void HudRadar::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// move down
	BaseClass::ApplySchemeSettings(pScheme);

	// set the background
	SetBgColor(m_pScheme->GetColor("Menu.ClearColor", Color(255, 255, 255, 0)));

	// radius is the width of the element
	m_iRadius = RADAR_RADIUS - (RADAR_MARKER_SCALE / 2);
	m_iXOffset = RADAR_XOFFSET;
	m_iYOffset = RADAR_YOFFSET;
}

/**
* Resets the map so everything resizes
*
* @return void
**/
void HudRadar::MapReset(void)
{
	// jump down
	BaseClass::MapReset();

	// reset all of our size info
	m_iRadius = RADAR_RADIUS - (RADAR_MARKER_SCALE / 2);
	m_iXOffset = RADAR_XOFFSET;
	m_iYOffset = RADAR_YOFFSET;

	// set the size of the map
	SetMapSize(RADAR_RADIUS * 2, RADAR_RADIUS * 2);

	// reposition ourselves to the right side of the screen
	SetPos(XRES(640) - GetWide(), 0);
}

/**
* Figures out the vertices and texture coordinates to use for the radar circle
* 
* @param const FootStep_t &sStep Player's current position
* @return void
**/
void HudRadar::CalcVertices(const FootStep_t &sStep)
{
	float fX1, fY1, fX2, fY2, fViewAngle;
	Vector vecTPos, vecPos;

	/*Vertex_t points[4] =
	{
		Vertex_t(MapToPanel(Vector2D(0, 0)), Vector2D(0, 0)),
		Vertex_t(MapToPanel(Vector2D(MAP_SIZE - 1, 0)), Vector2D(1, 0)),
		Vertex_t(MapToPanel(Vector2D(MAP_SIZE - 1, MAP_SIZE - 1)), Vector2D(1, 1)),
		Vertex_t(MapToPanel(Vector2D(0, MAP_SIZE - 1)), Vector2D(0, 1))
	};*/

	// figure out the view angle
	fViewAngle = m_fViewAngle - 90.0;

	// are we turning the map?
	if(!m_bFollowEntity)
	{
		if(GetRotateMap())
			fViewAngle = 90.0f;
		else
			fViewAngle = 0.0f;
	}

	// convert to map
	Vector2D vecTemp = WorldToMap(sStep.vecPos);
	vecPos.x = vecTemp.x;
	vecPos.y = vecTemp.y;
	vecPos.z = 0;

	vecPos = (vecPos / (float)MAP_SIZE * 2.0);	// [0, 2]
	vecPos.x -= 1.0;	// [-1, 1]
	vecPos.y -= 1.0;

	float fViewAngleRadians = M_PI * fViewAngle / 180;

	// iterate over all the points
	for(int i = RADAR_CIRCLE_POINTS - 1; i >= 0; i--)
	{
		// figure out the x and y position
		SinCos((2.0 * M_PI * (float)i) / (float)RADAR_CIRCLE_POINTS, &fY1, &fX1);
		m_aVertices[i].m_Position.x = (fX1 * (float)m_iRadius) + (float)m_iRadius;
		m_aVertices[i].m_Position.y = (fY1 * (float)m_iRadius) + (float)m_iRadius;

		// add the offset
		m_aVertices[i].m_Position += ComputeOffset(DRAW_TYPE_MAP);

		// rotate now
		SinCos((2.0 * M_PI * (float)i / (float)RADAR_CIRCLE_POINTS) - fViewAngleRadians, &fY2, &fX2);

		// @WORKING
		//	 fX1, fY2 [-1,1]
		//   add 1 = [0,2]
		//   divide by 2 = [0, 1]
		//   divide by zoom = [0, 1/zoom], (makes the circle smaller)
		// ADD
		//	 offset [-1, 1]
		//   add 1 = [0, 2]
		//   divide by 2 [0, 1]
		// RESULT
		//	 [0, 2]

		// the last divide by 2 doesnt make any sense.
		m_aVertices[i].m_TexCoord.x = (((fX2 / m_fZoom * 2.0 + 1.0) / 2.0) + (vecPos.x + 1.0 / 2.0)) / 2.0;
		m_aVertices[i].m_TexCoord.y = (((fY2 / m_fZoom * 2.0 + 1.0) / 2.0) + (vecPos.y + 1.0 / 2.0)) / 2.0;

		/*
		// figure out what part of the texture we are displaying
		vecTPos.x = fX * ((float)MAP_SIZE / 2.0);
		vecTPos.y = fY * ((float)MAP_SIZE / 2.0);
		//vecTPos.z = 0;

		vecTPos.x = fX;
		vecTPos.y = fY;

		// rotate
		// VectorYawRotate(vecTPos, fViewAngle, vecTPos);

		/*
		vecTPos.x = (vecTPos.x + 1.0) / 2.0 * (float)MAP_SIZE / 2.0;
		vecTPos.y = (vecTPos.y + 1.0) / 2.0 * (float)MAP_SIZE / 2.0;
		//vecTPos.y += (float)MAP_SIZE / 2.0;
		

		// set the texture coords
		m_aVertices[i].m_TexCoord.x = (vecPos.x + vecTPos.x) / ((float)MAP_SIZE);
		m_aVertices[i].m_TexCoord.y = (vecPos.y + vecTPos.y) / ((float)MAP_SIZE);
		*/
	}
}

/**
* Draws the map
* 
* @return void
**/
void HudRadar::DrawMap(void)
{
	// call the base class
	if (!ShouldDraw())
		return;

	BaseClass::Paint();

	// draw the foreground
	// GetForeground()->Draw();

	// draw the gray overlay if they're dead
	if (!CBasePlayer::GetLocalPlayer()->IsAlive()) {
		surface()->DrawSetColor(0, 0, 0, 255);
		surface()->DrawSetTexture(m_iDeadTexture);
		surface()->DrawTexturedPolygon(RADAR_CIRCLE_POINTS, m_aVertices);
	}
}

/**
* Paints the radar to the hud
*
* @return void
**/
void HudRadar::Paint(void)
{
	// this intentionally does nothing but set the player to follow
	// the objectives hud element draws us using calls to DrawMap and DrawObjectiveMarker
	// has to do with layering
	m_fZoom = 0.33 * GetMapScale(); // @TRJ - zoomed out a bit - original -> 0.66 * GetMapScale();

	// do we have a local player?
	if(CSDKPlayer::GetLocalPlayer())
	{
		// start following
		SetFollowEntity(CSDKPlayer::GetLocalPlayer()->entindex());
		SetFollowEntity(true);
	}
}

/**
* Draws the map texture
*
* @return void
**/
void HudRadar::DrawMapTexture(void)
{
	// make sure we have a texture
	if(s_iMapTextureID < 0 || s_iFollowEntity <= 0 || s_iFollowEntity > MAX_PLAYERS)
		return;

	// figure out the points to draw at
	CalcVertices(s_Players[s_iFollowEntity - 1].trail);

	// @TEMP - debugging
	/*int aX[RADAR_CIRCLE_POINTS];
	int aY[RADAR_CIRCLE_POINTS];
	for(int i = 0; i < RADAR_CIRCLE_POINTS; ++i)
	{
		aX[i] = m_aVertices[i].m_Position.x;
		aY[i] = m_aVertices[i].m_Position.y;
	}
	surface()->DrawSetColor(255, 0, 0, 255);
	surface()->DrawPolyLine(aX, aY, RADAR_CIRCLE_POINTS);*/

	// draw the texture
	surface()->DrawSetColor(0, 255, 0, 255);
	surface()->DrawSetTexture(s_iMapTextureID);
	surface()->DrawTexturedPolygon(RADAR_CIRCLE_POINTS, m_aVertices);
}

/**
* Draws the objective marker along the edge of the radar
*
* @return void
**/
void HudRadar::DrawObjectiveMarker(void)
{
	SObjective_t *pObj;
	FootStep_t *pStep;
	Vector2D vecObjPos, vecPlayerPos, vecDist, vecPos1, vecPos2;

	// see if we have an active objective
	pObj = GET_OBJ_MGR()->GetActiveObjective();

	// make sure we got it
	if(!pObj)
		return;

	// pull the step
	pStep = &(s_Players[s_iFollowEntity - 1].trail);

	// convert to panel coords
	vecObjPos = WorldToPanel(pObj->vecPos);
	vecPlayerPos = WorldToPanel(pStep->vecPos);

	// are we within our radius?
	vecDist = vecObjPos - vecPlayerPos;
	if(vecDist.LengthSqr() > (m_iRadius * m_iRadius))
	{
		// normalize and move it out to the edge
		vecDist.NormalizeInPlace();
		vecDist *= m_iRadius;
	}

	// add the radius
	vecDist.x += m_iRadius;
	vecDist.y += m_iRadius;

	// draw the square
	DrawTexturedSquare(vecDist, ComputeOffset(DRAW_TYPE_MARKER), 180, RADAR_MARKER_SCALE, m_iMarkerTexture);
}

/**
* Determines if we should draw something at the given position
*
* @param const Vector2D &vecPanelPos The position to check
* @param int iSize The size of the item we're drawing
* @return bool True if we should draw the objective
**/
bool HudRadar::ShouldDrawItem(const Vector2D &vecPanelPos, int iSize)
{
	FootStep_t *pStep = &(s_Players[s_iFollowEntity - 1].trail);
	Vector2D vecPlayerPos = WorldToPanel(pStep->vecPos);

	// this is fudged until it looks right
	int radius = m_iRadius - iSize / 3;

	if ((vecPanelPos - vecPlayerPos).LengthSqr() > (radius * radius))
		return false; 

	return BaseClass::ShouldDrawItem(vecPanelPos, iSize);
}

/**
* Computes the offset to use when drawing various map items
*
* @param DrawType eType The type of element we want the offset for
* @return Vector2D The offset
**/
Vector2D HudRadar::ComputeOffset(DrawType eType)
{
	// if drawing the map or the marker, offset by the width of the icon
	if(eType == DRAW_TYPE_MARKER || eType == DRAW_TYPE_MAP)
		return Vector2D(m_iXOffset + (RADAR_MARKER_SCALE / 2), m_iYOffset + (RADAR_MARKER_SCALE / 2));

	return BaseClass::ComputeOffset(eType);
}

/**
* Creates our foreground
*
* @return CMapForeground *
**/
CMapForeground *HudRadar::CreateForeground(void)
{
	return new CRadarForeground(this, "MapForeground");
}