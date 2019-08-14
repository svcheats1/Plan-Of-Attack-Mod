#include "cbase.h"
#include "strategyeditormap.h"
#include <vgui/iinput.h>
#include "strategyarrow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Constructor
**/
CStrategyEditorMap::CStrategyEditorMap(vgui::Panel *pParent)
	: BaseClass(pParent), m_sPressCode(vgui::MOUSE_LAST)
{
	// ?
}

/**
* Initializes the map
*
* @return void
**/
void CStrategyEditorMap::Init(void)
{
	// just reset
	Reset();
}

/**
* Clears all the map info
*
* @param void
**/
void CStrategyEditorMap::ClearPoints(void)
{
	// delete all teh points
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < m_aPoints[i].Count(); ++j)
			m_aPoints[i][j]->MarkForDeletion();
		m_aPoints[i].Purge();
	}
}

/**
* Resets the map
*
* @return void
**/
void CStrategyEditorMap::Reset(void)
{
	CUtlVector<CStrategicArrow *> *pArrows;

	// clear out our key values
	SetKeyValues(NULL);

	// kill teh arrows
	pArrows = GetArrows();
	for(int i = 0; i < pArrows->Count(); ++i)
		(*pArrows)[i]->MarkForDeletion();
	pArrows->Purge();

	// clear out all the points
	ClearPoints();
		
	// create the arrows
	m_iCurrentArrow = 0;
	for(int i = 0; i < 3; ++i)
	{
		// add the arrow and set its size
		AddArrow(new CStrategicArrow(this));
		GetArrow(i)->SetSize(GetWide(), GetTall());
	}
	m_iCurrentArrow = 0;
	(*pArrows)[m_iCurrentArrow]->MoveToFront();
}

/**
* Handles mouse input.  Sets our mouse down to true. Prevents a mouse
* release that was initiated in another panel from creating a point
*
* @param vgui::MouseCode sCode The code for the mouse press
* @return void
**/
void CStrategyEditorMap::OnMousePressed(vgui::MouseCode sCode)
{
	// store the code
	m_sPressCode = sCode;

	BaseClass::OnMousePressed(sCode);
}

/**
* Handles mouse input.  We need to add a point at the position they clicked
*
* @param vgui::MouseCode sCode The code for the mouse press
* @return void
**/
void CStrategyEditorMap::OnMouseReleased(vgui::MouseCode sCode)
{
	int iXPos, iYPos;

	// check the code
	if(m_sPressCode != sCode)
	{
		BaseClass::OnMouseReleased(sCode);
		return;
	}
	else
		m_sPressCode = vgui::MOUSE_LAST;

	// don't care unless we left clicked
	if(sCode != vgui::MOUSE_LEFT)
	{
		BaseClass::OnMouseReleased(sCode);
		return;
	}

	// figure out where they were and create a new point
	vgui::input()->GetCursorPosition(iXPos, iYPos);
	ScreenToLocal(iXPos, iYPos);
	CreatePathPoint(iXPos, iYPos);

	// jump down
	BaseClass::OnMouseReleased(sCode);
}

/**
* Creates a new path node at the specified point
*
* @param int iXPos
* @param int iYPos
* @return void
**/
void CStrategyEditorMap::CreatePathPoint(int iXPos, int iYPos)
{
	// make sure we have the proper arrow
	if(!GetArrow(m_iCurrentArrow))
		return;

	// convert the panel coords to map coords
	GetArrow(m_iCurrentArrow)->PanelToMap(iXPos, iYPos);

	// add the arrow point
	AddArrowPoint(GetArrow(m_iCurrentArrow), iXPos, iYPos, GetArrow(m_iCurrentArrow)->GetArrowWidth());
}

/**
* Adds a a point to teh given arrow
*
* @param CStrategicArrow *pArrow The arrow to add the point to
* @param int iXPos The x coord
* @param int iYPos The y coord
* @param int iWidth The width
* @return void
**/
void CStrategyEditorMap::AddArrowPoint(CStrategicArrow *pArrow, int iXPos, int iYPos, int iWidth)
{
	CStrategyEditorPoint *pPoint;

	// create the point
	pPoint = new CStrategyEditorPoint(pArrow, this, iXPos, iYPos, iWidth, GetPointColor(m_iCurrentArrow));
	if(pPoint->IsValid())
		m_aPoints[m_iCurrentArrow].AddToTail(pPoint); // we're making the assumption that m_iCurrentArrow is correct
}

/**
* Defines the color to use for the points of a given arrow
*
* @param int iArrow The index of the arrow to get the color for
* @return Color
**/
Color CStrategyEditorMap::GetPointColor(int iArrow)
{
	// which one?
	switch(iArrow)
	{
	case 0:
		return Color(26, 174, 26, 128);
	case 1:
		return Color(219, 229, 65, 128);
	case 2:
		return Color(8, 206, 218, 128);
	default:
		return Color(255, 255, 255, 255);
	}
}

/**
* Adds an arrow
*
* @param CStrategicArrow *pArrow the arrow to add
* @return void
**/
void CStrategyEditorMap::AddArrow(CStrategicArrow *pArrow)
{
	// are we less than three?
	if(m_iCurrentArrow < 3)
	{
		// increment the count
		++m_iCurrentArrow;

		// add the arrow
		BaseClass::AddArrow(pArrow);
	}
}

/**
* Removes a point that was created by this map
*
* @param CStrategyEditorPoint *pPoint The point to remove
* @return void
**/
void CStrategyEditorMap::RemovePoint(CStrategyEditorPoint *pPoint)
{
	// see if we have it
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < m_aPoints[i].Count(); ++j)
		{
			// is this it?
			if(m_aPoints[i][j] == pPoint)
			{
				// kill it
				m_aPoints[i].Remove(j);
				return;
			}
		}
	}
}

/**
* Applies the scheme settings
* 
* @param vgui::IScheme *pScheme The scheme to apply
* @return void
**/
void CStrategyEditorMap::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// jump down
	BaseClass::ApplySchemeSettings(pScheme);

	// adjust the size of our arrow
	for(int i = 0; i < 3; ++i)
	{
		if(GetArrow(i))
			GetArrow(i)->SetSize(GetWide(), GetTall());
	}

	// black background
	SetBgColor(pScheme->GetColor("Black", Color(0, 0, 0, 255)));
}

/**
* Paints the panel
*
* @return void
**/
void CStrategyEditorMap::Paint(void)
{
	// jump down
	BaseClass::Paint();
}

/**
* Loads the arrows from key values
*
* @param KeyValues *pData The data to load from
* @return bool
**/
bool CStrategyEditorMap::LoadFromKeyValues(KeyValues *pData)
{
	bool bSuccess;
	CUtlVector<CStrategicArrow *> *pArrows;
	CStrategicArrow *pArrow;

	// jump down
	bSuccess = BaseClass::LoadFromKeyValues(pData);

	// make sure we have all three arrows
	pArrows = GetArrows();
	for(int i = pArrows->Count(); i < 3; ++i)
	{
		// create the new arrow and add it to the list
		pArrow = new CStrategicArrow(this);
		pArrow->SetSize(GetWide(), GetTall());
		pArrows->AddToTail(pArrow);
	}

	// now make sure we're at the first arrow
	m_iCurrentArrow = 0;
	if(GetArrow(m_iCurrentArrow))
		GetArrow(m_iCurrentArrow)->MoveToFront();

	return bSuccess;
}

/**
* Sets the current arrow
*
* @param int iArrow The index of the arrow to use
* @return void
**/
void CStrategyEditorMap::SetCurrentArrow(int iArrow)
{
	// is it valid?
	if(!GetArrow(iArrow))
		return;

	// set it and move the arrow to the front
	m_iCurrentArrow = iArrow;
	GetArrow(m_iCurrentArrow)->MoveToFront();
}

/**
* Sets the currently active team for map rotation
*
* @param int iTeam The team who is active
* @return void
**/
void CStrategyEditorMap::SetActiveTeam(int iTeam)
{
	// make sure we have key values
	if(!s_pMapKeyValues)
		return;

	// set the new team rotation
	if(iTeam == TEAM_A)
		m_iTeamRotate = s_pMapKeyValues->GetInt("team_a_rotate");
	else if(iTeam == TEAM_B)
		m_iTeamRotate = s_pMapKeyValues->GetInt("team_b_rotate");
	else
		Assert(0);
}

/**
* Determines the rotation to use based on the the selected team
*
* @return int
**/
int CStrategyEditorMap::GetTeamRotation(void)
{
	return m_iTeamRotate;
}