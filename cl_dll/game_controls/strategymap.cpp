#include "cbase.h"
#include "strategymap.h"
#include "strategymenu.h"
#include "strategyarrow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define MAX_ARROWS 3

CStrategyMap::CStrategyMap(vgui::Panel *pParent)
{
	// set our parent
	SetParent(pParent);

	// reset the map
	MapReset();

	// don't show most stuff
	m_bShowNames = false;
	m_bShowHealth = false;
	m_bFollowEntity = false;
	m_bShowPlayers = false;

	// show objectives
	m_bShowObjectives = true;

	// set the zoom
	m_fZoom = 1.0;
	m_szName[0] = NULL;

	// set our drawing state
	SetEnabled(true);
	SetVisible(false);

	// no key values yet
	m_pKeyValues = NULL;
}

CStrategyMap::~CStrategyMap()
{
	m_aArrows.PurgeAndDeleteElements();

	// kill the key values
	if(m_pKeyValues)
		m_pKeyValues->deleteThis();
}

/**
* Sets the key values
*
* @param KeyValues *pData The key values to hang on to
* @return void
**/
void CStrategyMap::SetKeyValues(KeyValues *pData)
{
	// delete any old data
	if(m_pKeyValues)
		m_pKeyValues->deleteThis();

	// set it
	m_pKeyValues = pData;
}

/**
* Sends back the key values that represent this map
* If we don't have the info we can create it.  The awkwardness here is that
* key values wasn't set up to add multiple values with same identifying key...
*
* @param bool bCreate If true and we don't have the key values we will create them
* @return KeyValues *
**/
KeyValues *CStrategyMap::GetKeyValues(bool bCreate/* = false*/)
{
	KeyValues *pDefs, *pKey, *pMisc, *pSub, *pDefsSub;
	CUtlVector<CStrategicArrow::SStrategicArrowPoint *> aPoints;
	char szStr[64], szName[128];
	int iLastWidth;

	// if we're not creating send back whatever we have
	if(!bCreate)
		return m_pKeyValues;
	else
	{
		// if we had something already we need to pull out the definition section
		if(m_pKeyValues)
		{
			// see if we can find it
			pDefs = m_pKeyValues->FindKey("definition");
			if(pDefs)
			{
				// see if we can find a name
				if(pDefs->FindKey("name"))
					Q_strncpy(szName, pDefs->GetString("name"), sizeof(szName));
				else
					szName[0] = 0;

				// pull i tout and kill it
				m_pKeyValues->RemoveSubKey(pDefs);
				pDefs->deleteThis();
			}
		}

		// create them
		if(!m_pKeyValues)
			m_pKeyValues = new KeyValues("strategy");
		pDefs = m_pKeyValues->FindKey("definition", true);
		pDefsSub = NULL;

		// add the name if we have one
		if(szName[0])
		{
			// add the string and move the sub pointer along
			pDefs->SetString("name", szName);
			pDefsSub = pDefs->FindKey("name");
		}

		// run through all the arrows
		for(int i = 0; i < m_aArrows.Count(); ++i)
		{
			// make sure we have some points
			if(!m_aArrows[i]->IsComplete())
				continue;

			// create a new key and add it as a sub key
			pKey = new KeyValues("arrow");
			if(pDefsSub)
				pDefsSub->SetNextKey(pKey);
			else
				pDefs->SetSubKey(pKey);
			pDefsSub = pKey;

			// add the type
			pKey->SetString("type", "push");
			pSub = pKey->FindKey("type");

			// run through all the points
			aPoints = m_aArrows[i]->GetPoints();
			iLastWidth = -1;
			for(int i = 0; i < aPoints.Count(); ++i)
			{
				// do we have a different width
				if(aPoints[i]->m_iWidth != iLastWidth)
				{
					// write the width
					pMisc = new KeyValues("width");
					pMisc->SetInt(aPoints[i]->m_iWidth);
					pSub->SetNextKey(pMisc);
					pSub = pMisc;

					// record the width so we don't have to do this again
					iLastWidth = aPoints[i]->m_iWidth;
				}

				// and the point
				pMisc = new KeyValues("point");
				Q_snprintf(szStr, sizeof(szStr), "%d %d", (int)aPoints[i]->m_vecPos.x, (int)aPoints[i]->m_vecPos.y);
				pMisc->SetString(szStr);
				pSub->SetNextKey(pMisc);
				pSub = pMisc;
			}
		}
	}

	return m_pKeyValues;
}

bool CStrategyMap::LoadFromKeyValues(KeyValues *data)
{
	m_aArrows.PurgeAndDeleteElements();

	if (!data)
		return false;

	Q_strncpy(m_szName, data->GetString("name"), sizeof(m_szName));

	// iterate over the key values
	for(KeyValues *kvArrow = data->GetFirstSubKey(); kvArrow != NULL; kvArrow = kvArrow->GetNextKey()) {
		// look for 'arrow'
		if (FStrEq(kvArrow->GetName(), "arrow")) {
			if (m_aArrows.Count() >= MAX_ARROWS)
				continue;

			// initialize data
			int iWidth = 10;
			int iPointCount = 0;
			CStrategicArrow *arrow = new CStrategicArrow(this);
			arrow->SetSize(GetMapWide(), GetMapTall());
			
			// iterate over the 'arrow' subkey
			for (KeyValues *kvArrowData = kvArrow->GetFirstSubKey(); kvArrowData != NULL; kvArrowData = kvArrowData->GetNextKey()) {
				// found type!
				if (FStrEq(kvArrowData->GetName(), "type")) {
					arrow->SetArrowType(AliasToArrowType(kvArrowData->GetString()));
				}
				// found [new] width!
				else if (FStrEq(kvArrowData->GetName(), "width")) {
					iWidth = kvArrowData->GetInt();
                }
				// found new control point!
				else if (FStrEq(kvArrowData->GetName(), "point")) {
					if (iPointCount >= MAX_POINTS)
						continue;

					int iX, iY;
					// parse the x and y
					if (sscanf(kvArrowData->GetString(), "%d %d", &iX, &iY) == 2) {
						// if it worked, increment our point count and add the point
						++iPointCount;
						AddArrowPoint(arrow, iX, iY, iWidth);
					}
				}
			}

			// only add the arrow to our array if it had the enough points
			if (iPointCount >= 4) {
				AddArrow(arrow);
			}
			// otherwise delete it
			else
				delete arrow;
		}
	}

	return (m_aArrows.Count() > 0);
}

/**
* Adds a point to the given arrow
*
* @param CStrategicArrow *pArrow The arrow to add a point to
* @param int iX The x coord
* @param int iY The y coord
* @param int iWidth The width of the arrow
* @return void
**/
void CStrategyMap::AddArrowPoint(CStrategicArrow *pArrow, int iX, int iY, int iWidth)
{
	// add the point
	pArrow->AddPoint(iX, iY, iWidth);
}

void CStrategyMap::MapReset(void)
{
	// set the map
	if(s_pMapKeyValues)
		SetMap(s_pMapKeyValues->GetString("levelname"));

	// set the size of the map
	int wide, tall;
	GetSize(wide, tall);
	SetMapSize(wide, tall);

	// reset our update time
	m_fNextUpdateTime = gpGlobals->curtime;
}

void CStrategyMap::Paint()
{
	BaseClass::Paint();

	// we must manually paint the arrows because they are not panels and
	// hence to not follow the parent/child tree structure done by VGUI.
	for(int i = 0; i < m_aArrows.Count(); ++i) {
		m_aArrows[i]->Paint();
	}
}

void CStrategyMap::SetZ(float z)
{
	for(int i = 0; i < m_aArrows.Count(); ++i)
		m_aArrows[i]->SetZ(z);
}

void CStrategyMap::SetMapSize(int wide, int tall)
{ 
	BaseClass::SetMapSize(wide, tall); 
	BaseClass::SetSize(wide, tall); 
	
	for(int i = 0; i < m_aArrows.Count(); ++i)
		m_aArrows[i]->SetSize(wide, tall);
}

void CStrategyMap::OnMouseReleased(MouseCode code)
{
	// left clicks only
	if (code != MOUSE_LEFT)
		return;

	CStrategyMenu *menu = dynamic_cast<CStrategyMenu*>(GetParent());
	if (menu)
        menu->StrategyChosen(this);
}

void CStrategyMap::OnCursorEntered()
{
	CStrategyMenu *menu = dynamic_cast<CStrategyMenu*>(GetParent());
	if (menu)
        menu->Hilite(this, true);
}

void CStrategyMap::OnCursorExited()
{
	CStrategyMenu *menu = dynamic_cast<CStrategyMenu*>(GetParent());
	if (menu)
        menu->Hilite(this, false);
}