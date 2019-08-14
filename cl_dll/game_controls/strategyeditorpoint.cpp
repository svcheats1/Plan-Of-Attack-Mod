#include "cbase.h"
#include "strategyeditorpoint.h"
#include "strategyarrow.h"
#include <vgui/iinput.h>
#include "vgui/KeyCode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Constructor
**/
CStrategyEditorPoint::CStrategyEditorPoint(CStrategicArrow *pParentArrow, CStrategyEditorMap *pMap, int iXPos, int iYPos, int iWidth, Color sColor)
	: BaseClass(pParentArrow), m_pArrow(pParentArrow), m_pPoint(NULL), m_pMap(pMap), m_sColor(sColor), m_bFinalized(false)
{
	// set our position and size
	// this must happen first
	MapToPanel(iXPos, iYPos);
	SetPos(iXPos - (SEPOINT_WIDE / 2), iYPos - (SEPOINT_TALL / 2));
	SetSize(SEPOINT_WIDE, SEPOINT_TALL);

	// initialize it
	InitPoint(iWidth);

	// not dragging or resizing
	m_bIsResizingArrow = false;
	m_bIsResizingPoint = false;
	m_bIsDragging = false;

	// visible and enabled
	SetVisible(true);
	SetEnabled(true);
}

/**
* Initializes the point
*
* @param int iWidth The width to use for the arrow
* @return void
**/
void CStrategyEditorPoint::InitPoint(int iWidth)
{
	int iX, iY;

	// add ourselves to the arrow as a point
	GetPos(iX, iY);
	iX += GetWide() / 2;
	iY += GetTall() / 2;
	m_pArrow->PanelToMap(iX, iY);
	m_pPoint = m_pArrow->AddPoint(iX, iY, iWidth);

	if (!m_pPoint) {
		// kill the panel
		MarkForDeletion();
		m_bIsValid = false;
	}
	else
		m_bIsValid = true;
	
	// we can resize whenever
	m_fNextResizeTime = 0;
}

/**
* Converts map coordinates to panel coordinates
*
* @param int &iXPos
* @param int &iYPos
* @return void
**/
void CStrategyEditorPoint::MapToPanel(int &iXPos, int &iYPos)
{
	// map is always 1024 so scale down by the fraction of our panel size
	iXPos *= ((float)m_pArrow->GetWide() / 1024.0f);
	iYPos *= ((float)m_pArrow->GetTall() / 1024.0f);
}

/**
* Applies the scheme
*
* @param vgui::scheme *pScheme The scheme to apply
* @return void
**/
void CStrategyEditorPoint::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// no background
	//SetBgColor(m_sColor);
	SetPaintBackgroundEnabled(false);
	//SetPaintBackgroundType(2);
}

/**
* Paints the panel
*
* @return void
**/
void CStrategyEditorPoint::Paint(void)
{
	int iXPos, iYPos;

	// dragging? update the position
	if(m_bIsDragging)
	{
		// figure out where the mouse is
		vgui::input()->GetCursorPosition(iXPos, iYPos);
		m_pArrow->ScreenToLocal(iXPos, iYPos);

		// make sure we're not off the panel
		if(iXPos > m_pArrow->GetWide())
			iXPos = m_pArrow->GetWide();
		else if(iXPos < 0)
			iXPos = 0;
		if(iYPos > m_pArrow->GetTall())
			iYPos = m_pArrow->GetTall();
		else if(iYPos < 0)
			iYPos = 0;

		// reset to the cursor position
		iXPos -= GetWide() / 2;
		iYPos -= GetTall() / 2;
		SetPos(iXPos, iYPos);

		// now reposition the arrow
		iXPos += GetWide() / 2;
		iYPos += GetTall() / 2;
		m_pArrow->PanelToMap(iXPos, iYPos);
		m_pPoint->m_vecPos = Vector(iXPos, iYPos, 0);

		// unfinalize it
		m_pArrow->SetFinalized(false);
		m_bFinalized = false;
	}
	else if((m_bIsResizingPoint || m_bIsResizingArrow) && gpGlobals->curtime >= m_fNextResizeTime)
	{
		// if we're resizing this point change our size
		if(m_bIsResizingPoint)
		{
			// change the width
			m_pPoint->m_iWidth = clamp(m_pPoint->m_iWidth + (m_bGrow ? 1 : -1), MIN_WIDTH, MAX_WIDTH);

			// and force a recalculation
			m_pArrow->SetFinalized(false);
		}
		else
			m_pArrow->ModifyWidth(m_bGrow ? 1 : -1);

		// move the next time along
		m_fNextResizeTime = gpGlobals->curtime + RESIZE_PERIOD;
		m_bFinalized = false;
	}

	// if we're not finalized, finalize
	if(!m_bFinalized)
		Finalize();

	// draw the point to the screen
	DrawPoint();
}

/**
* Finalizes the points for our circle
*
* @return void
**/
void CStrategyEditorPoint::Finalize(void)
{
	int iRadius, iX, iY, iTri;
	float fX, fY;
	Vector vecCenter;

	// make sure we have a real number of points
	if(STRATEGY_EDITOR_POINT_POINTS % 3 != 0)
	{
		Assert(0);
		return;
	}

	// some circle basics
	GetPos(iX, iY);
	iRadius = GetWide() / 2;
	vecCenter.x = iX + iRadius;
	vecCenter.y = iY + iRadius;

	// create the rest of the points
	iTri = STRATEGY_EDITOR_POINT_POINTS / 3;
	for(int i = 0; i < STRATEGY_EDITOR_POINT_POINTS; ++i)
	{
		// start in the center
		m_vecPoints[i] = vecCenter;
		m_vecPoints[i].z = 1.0;
		ArrowPointToScreen(m_vecPoints[i]);
		++i;

		// calculate the next point
		SinCos((float)iTri / (float)(STRATEGY_EDITOR_POINT_POINTS / 3) * M_PI * 2.0f, &fX, &fY);
		m_vecPoints[i].x = fX * (float)iRadius;
		m_vecPoints[i].y = fY * (float)iRadius;
		m_vecPoints[i] += vecCenter;
		m_vecPoints[i].z = 1.0;
		ArrowPointToScreen(m_vecPoints[i]);
		++i;
		--iTri;

		// calculate the next point
		SinCos((float)iTri / (float)(STRATEGY_EDITOR_POINT_POINTS / 3) * M_PI * 2.0f, &fX, &fY);
		m_vecPoints[i].x = fX * (float)iRadius;
		m_vecPoints[i].y = fY * (float)iRadius;
		m_vecPoints[i] += vecCenter;
		m_vecPoints[i].z = 1.0;
		ArrowPointToScreen(m_vecPoints[i]);
	}

	// we're done
	m_bFinalized = true;
}

/**
* Draws the point to the screen
*
* @return void
**/
void CStrategyEditorPoint::DrawPoint(void)
{		
	CMeshBuilder sMeshBuilder;
	IMaterial *pMaterial;
	IMesh *pMesh;
	int iColor;
	unsigned char *ucColor;

	// grab the blank material and the mesh
	pMaterial = materials->FindMaterial( "vgui/white", 0);
	pMesh = materials->GetDynamicMesh(true, NULL, NULL, pMaterial);

	// figure out the color info
	iColor = m_sColor.GetRawColor();
	ucColor = (unsigned char *)&iColor;

	// flip through all the points
	for(int i = 0; i < STRATEGY_EDITOR_POINT_POINTS; ++i)
	{
		// start the mesh
		sMeshBuilder.Begin(pMesh, MATERIAL_TRIANGLES, 1);

		// set the color, texture coord and positin
		// then advance the vertex
		sMeshBuilder.Color4ubv(ucColor);
		sMeshBuilder.TexCoord2f(0, i % 2, (i / 2) % 2);
		sMeshBuilder.Position3fv(m_vecPoints[i].Base());
		sMeshBuilder.AdvanceVertex();

		// set the color, texture coord and positin
		// then advance the vertex
		++i;
		sMeshBuilder.Color4ubv(ucColor);
		sMeshBuilder.TexCoord2f(0, i % 2, (i / 2) % 2);
		sMeshBuilder.Position3fv(m_vecPoints[i].Base());
		sMeshBuilder.AdvanceVertex();

		// set the color, texture coord and positin
		// then advance the vertex
		++i;
		sMeshBuilder.Color4ubv(ucColor);
		sMeshBuilder.TexCoord2f(0, i % 2, (i / 2) % 2);
		sMeshBuilder.Position3fv(m_vecPoints[i].Base());
		sMeshBuilder.AdvanceVertex();

		// finish the mesh
		sMeshBuilder.End();
		pMesh->Draw();
	}
}

/**
* Converts arrow point coordinates to screen coords
*
* @param Vector &pPoint The point to move
* @return void
**/
void CStrategyEditorPoint::ArrowPointToScreen(Vector &vecPoint)
{
	int iX, iY;

	// add on the position of my parents
	for(Panel* pParent = GetParent(); pParent != NULL; pParent = pParent->GetParent())
	{
		// get their position and add it on
		pParent->GetPos(iX, iY);
		vecPoint.x += iX;
		vecPoint.y += iY;
	}
}

/**
* Determines the action to take when we press the mouse over a point
*
* @param MouseCode sCode The code for the press
* @return void
**/
void CStrategyEditorPoint::OnMousePressed(vgui::MouseCode sCode)
{
	// don't care about anything but mouse left or right
	if(sCode != vgui::MOUSE_LEFT && sCode != vgui::MOUSE_RIGHT)
		return;

	// if there are key modifiers then we might be resizing
	if(vgui::input()->IsKeyDown(vgui::KEY_LSHIFT) || vgui::input()->IsKeyDown(vgui::KEY_RSHIFT))
		m_bIsResizingArrow = true;
	else if(vgui::input()->IsKeyDown(vgui::KEY_LALT) || vgui::input()->IsKeyDown(vgui::KEY_RALT))
	{
		m_bIsResizingPoint = true;
	}
	// otherwise, we're dragging if using the left mouse
	else if(sCode == vgui::MOUSE_LEFT)
		m_bIsDragging = true;

	// if we're resizing we neeed to know if we should grow or shrink
	if(m_bIsResizingArrow || m_bIsResizingPoint)
	{
		// grow with left mouse
		if(sCode == vgui::MOUSE_LEFT)
			m_bGrow = true;
		else
			m_bGrow = false;
	}
}

/**
* Determines the action to take when we release the mouse over a point
*
* @param MouseCode sCode The code for the release
* @return void
**/
void CStrategyEditorPoint::OnMouseReleased(vgui::MouseCode sCode)
{
	// don't care about anything but mouse left or right
	if(sCode != vgui::MOUSE_LEFT && sCode != vgui::MOUSE_RIGHT)
		return;

	// right mouse? delete if we're not resizing
	if(sCode == vgui::MOUSE_RIGHT && !m_bIsResizingArrow && !m_bIsResizingPoint && m_pPoint)
	{
		// tell the arrow and the map to get rid of this point
		m_pMap->RemovePoint(this);
		m_pArrow->RemovePoint(m_pPoint);
		m_pPoint = NULL;

		// kill the panel
		MarkForDeletion();
	}

	// no resizing or dragging
	m_bIsResizingArrow  = false;
	m_bIsResizingPoint = false;
	m_bIsDragging = false;
}