#include "cbase.h"
#include "strategyarrow.h"
#include "igameresources.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static const char * s_ArrowTypeInfo[] = 
{
	"push",		// ARROW_PUSH,
	"hold",		// ARROW_HOLD,
	"none",		// ARROW_NONE,
	NULL
};

eArrowType AliasToArrowType( const char *alias )
{
	if (alias)
	{
		for( int i=0; s_ArrowTypeInfo[i] != NULL; ++i )
			if (!Q_stricmp( s_ArrowTypeInfo[i], alias ))
				return (eArrowType)i;
	}

	return ARROW_NONE;
}

CStrategicArrow::CStrategicArrow(vgui::Panel *pParent)
{
	SetParent(pParent);

	m_bFinalized = false;
	m_eArrowType = ARROW_PUSH;

	SetSize(ScreenWidth(), ScreenHeight());
	SetPos(0, 0);
	
	SetColor(Color(255, 0, 0));
	m_color[3] = 50;
	SetZ(DEFAULT_ARROW_Z_VERTEX);

	// set the intial arrow size
	m_iInitialWidth = INITIAL_ARROW_WIDTH;
}

CStrategicArrow::~CStrategicArrow()
{
	// kill teh list of points
	m_aPoints.PurgeAndDeleteElements();
}

void CStrategicArrow::SetPos(int x, int y) 
{ 
	m_bFinalized = false; 
	BaseClass::SetPos(x, y);
}

void CStrategicArrow::SetSize(int wide, int tall)
{ 
	m_bFinalized = false; 
	BaseClass::SetSize(wide, tall);
}

/**
* Converts a point in the panel space to the map space
*
* @param int &iXPos The x coordinate to convert
* @param int &iYPos The y coordinate to convert
* @return void
**/
void CStrategicArrow::PanelToMap(int &iXPos, int &iYPos)
{
	// map is always 1024 so scale up by the fraction of our panel size
	iXPos /= ((float)GetWide() / 1024.0f);
	iYPos /= ((float)GetTall() / 1024.0f);
}

void CStrategicArrow::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);
}	

CStrategicArrow::SStrategicArrowPoint *CStrategicArrow::AddPoint(float x, float y, int width/* = 10*/)
{
	if (m_aPoints.Count() >= MAX_POINTS)
		return NULL;

	SStrategicArrowPoint *pPoint;

	width = clamp(width, MIN_WIDTH, MAX_WIDTH);

	m_bFinalized = false;

	Vector v(x, y, 0.0);
	pPoint = new SStrategicArrowPoint();
	pPoint->m_iWidth = width;
	pPoint->m_vecPos = v;
	m_aPoints.AddToTail(pPoint);

	return pPoint;
}

CStrategicArrow::SStrategicArrowPoint *CStrategicArrow::AddPoint(Vector &point, int width/* = 10*/)
{
	if (m_aPoints.Count() >= MAX_POINTS)
		return NULL;

	SStrategicArrowPoint *pPoint;

	width = clamp(width, MIN_WIDTH, MAX_WIDTH);

	m_bFinalized = false;

	pPoint = new SStrategicArrowPoint();
	pPoint->m_vecPos = point;
	pPoint->m_iWidth = width;
	m_aPoints.AddToTail(pPoint);

	return pPoint;
}

/**
* Removes a point from the arrow
*
* @param CStrategicArrow::SStrategicArrowPoint *pPoint The point to remove
* @return void
**/
void CStrategicArrow::RemovePoint(const CStrategicArrow::SStrategicArrowPoint *pPoint)
{
	// see if we can find it
	for(int i = 0; i < m_aPoints.Count(); ++i)
	{
		// is this it?
		if(m_aPoints[i] == pPoint)
		{
			// kill teh point
			delete m_aPoints[i];
			m_aPoints.Remove(i);

			// need to figure out points again
			m_bFinalized = false;

			break;
		}
	}
}

void CStrategicArrow::Finalize()
{
	m_bFinalized = true;
	CalculateTriPoints();
}

void CStrategicArrow::Paint()
{
	if (CBasePlayer::GetLocalPlayer()) {
		switch(CBasePlayer::GetLocalPlayer()->GetTeamNumber()) {
			case 1:
				SetColor(Color(0, 0, 255, 50));
				break;
			case 2:
			default:
				SetColor(Color(255, 0, 0, 50));
				break;
		}
	}

	if (!m_bFinalized)
		Finalize();

	if (m_aTriPoints.Count() < 3)
		return;

	DrawTriPoints();
	DrawArrowHead();
}

void CStrategicArrow::DrawTriPoints()
{
	IMaterial *pMaterial = materials->FindMaterial( "vgui/white", 0);
	IMesh *pMesh = materials->GetDynamicMesh(true, NULL, NULL, pMaterial);

	m_MeshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, m_aTriPoints.Count() - 2 );

	int color = m_color.GetRawColor();
	unsigned char *ucColor = (unsigned char *) &color;
	for(int i = 0; i < m_aTriPoints.Count(); ++i) {
		m_MeshBuilder.Color4ubv( ucColor );
		m_MeshBuilder.TexCoord2f(0, i % 2, (i / 2) % 2);	// [0, 0], [1, 0], [0, 1], [1, 1], ...
		m_MeshBuilder.Position3fv( m_aTriPoints[i].Base() );
		m_MeshBuilder.AdvanceVertex();
	}

	m_MeshBuilder.End();
	pMesh->Draw();

#ifdef DEBUG_STRATEGIES
	for(int i = 1; i < m_aTriPoints.Count() - 1; ++i) {
		vgui::ISurface *s = vgui::surface();
		Vector v = m_aTriPoints[i];
		MapToScreen(v, false);
		s->DrawSetColor(0, 255, 0, 255);
		s->DrawOutlinedCircle(v.x, v.y, 4, 6);
	}
#endif
}

void CStrategicArrow::DrawArrowHead()
{
	Vector points[3];
	int width = m_aPoints[m_aPoints.Count() - 2]->m_iWidth;

	// perpendicular point 1
	points[0].x = m_vEndPoint.x + (m_vEndTangent.y * width);
	points[0].y = m_vEndPoint.y - (m_vEndTangent.x * width);
	
	// end of the arrow head
	points[1].x = m_vEndPoint.x + (m_vEndTangent.x * width);
	points[1].y = m_vEndPoint.y + (m_vEndTangent.y * width);

	// perpendicular point 2
	points[2].x = m_vEndPoint.x - (m_vEndTangent.y * width);
	points[2].y = m_vEndPoint.y + (m_vEndTangent.x * width);

#ifdef DEBUG_STRATEGIES
	vgui::ISurface *s = vgui::surface();
	s->DrawSetColor(0, 0, 255, 255);
	for(int i = 0; i < 3; ++i) {
		Vector v = points[i];
		MapToScreen(v, false);
		s->DrawOutlinedCircle(v.x, v.y, 4, 6);
	}
#endif

	// translate the points to screen coords
	for(int i = 0; i < 3; ++i) {
		MapToScreen(points[i]);
		points[i].z = m_Z;
	}

	IMaterial *pMaterial = materials->FindMaterial( "vgui/white", 0);
	IMesh *pMesh = materials->GetDynamicMesh(true, NULL, NULL, pMaterial);

	m_MeshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 1 );

	int color = m_color.GetRawColor();
	unsigned char *ucColor = (unsigned char *) &color;
	for(int i = 0; i < 3; ++i) {
		m_MeshBuilder.Color4ubv( ucColor );
		m_MeshBuilder.TexCoord2f(0, i % 2, (i / 2) % 2);
		m_MeshBuilder.Position3fv( points[i].Base() );
		m_MeshBuilder.AdvanceVertex();
	}

	m_MeshBuilder.End();
	pMesh->Draw();	
}

void CStrategicArrow::CalculateTriPoints()
{
	m_aTriPoints.Purge();

    Vector linePoint, tangentVector, triPoint1, triPoint2;

	for(int i = 0; i < m_aPoints.Count() - 3; ++i) {
		//float tStepSize = PIXEL_STEP_SIZE / (aControlPoints[i+2] - aControlPoints[i+1]).Length();
		float tStepSize = 0.1;
		for(float t = 0; t < 1.0; t += tStepSize) {
			// the trinary operator here should save us a few cycles on 
			// lines that do not change width.
            int width = (m_aPoints[i+2]->m_iWidth == m_aPoints[i+1]->m_iWidth ? m_aPoints[i+1]->m_iWidth : ceil(m_aPoints[i+2]->m_iWidth * t + m_aPoints[i+1]->m_iWidth * (1.0 - t)));
			
			// where on the line are we?
			Catmull_Rom_Spline(
				m_aPoints[i]->m_vecPos, 
				m_aPoints[i+1]->m_vecPos, 
				m_aPoints[i+2]->m_vecPos, 
				m_aPoints[i+3]->m_vecPos,
				t, 
				linePoint);

			// what is our tangent at this point?
			Catmull_Rom_Spline_Tangent(
				m_aPoints[i]->m_vecPos, 
				m_aPoints[i+1]->m_vecPos, 
				m_aPoints[i+2]->m_vecPos, 
				m_aPoints[i+3]->m_vecPos, 
				t, 
				tangentVector);

			// normalize tangent
			VectorNormalize(tangentVector);

			// set the two perpendicular points
			triPoint1.Init();
			triPoint2.Init();
			triPoint1.x = linePoint.x - (tangentVector.y * width / 2.0);
			triPoint1.y = linePoint.y + (tangentVector.x * width / 2.0);
			triPoint1.z = m_Z;
			
			triPoint2.x = linePoint.x + (tangentVector.y * width / 2.0);
			triPoint2.y = linePoint.y - (tangentVector.x * width / 2.0);
			triPoint2.z = m_Z;

			//DevMsg("| %3f %3f | %3f %3f ", triPoint1.x, triPoint1.y, triPoint2.x, triPoint2.y);

			MapToScreen(triPoint1);
			MapToScreen(triPoint2);

			//DevMsg("| %3f %3f | %3f %3f |\n", triPoint1.x, triPoint1.y, triPoint2.x, triPoint2.y);

			m_aTriPoints.AddToTail(triPoint1);
			m_aTriPoints.AddToTail(triPoint2);
		}
	}

	m_vEndPoint = linePoint;
	m_vEndTangent = tangentVector;
}

void CStrategicArrow::MapToScreen(Vector &point, bool bDoParent /*= true*/)
{
	int iX, iY, iWide, iTall;
	GetBounds(iX, iY, iWide, iTall);

	point.x = clamp(point.x, 0, 1024);
	point.y = clamp(point.y, 0, 1024);

	point.x = iX + ceil((float)point.x / 1024.0 * iWide);
	point.y = iY + ceil((float)point.y / 1024.0 * iTall);

	if (bDoParent) {
		for(Panel* pParent = GetParent(); pParent != NULL; pParent = pParent->GetParent()) {
			pParent->GetPos(iX, iY);
			point.x += iX;
			point.y += iY;
		}
	}
}

void CStrategicArrow::OnMousePressed(MouseCode code)
{
	GetParent()->OnMousePressed(code);
}

void CStrategicArrow::OnMouseReleased(MouseCode code)
{
	GetParent()->OnMouseReleased(code);
}

void CStrategicArrow::OnCursorEntered()
{
	GetParent()->OnCursorEntered();
}

void CStrategicArrow::OnCursorExited()
{
	GetParent()->OnCursorExited();
}

/**
* Modifies the width of every point on teh arrow
*
* @param int iIncrement The amount to increment by
* @return void
**/
void CStrategicArrow::ModifyWidth(int iIncrement)
{
	// run through all the points
	for(int i = 0; i < m_aPoints.Count(); ++i)
	{
		// adjust the point
		m_aPoints[i]->m_iWidth = clamp(m_aPoints[i]->m_iWidth + iIncrement, MIN_WIDTH, MAX_WIDTH);
	}

	// increment our initial width
	m_iInitialWidth = clamp(m_iInitialWidth + iIncrement, MIN_WIDTH, MAX_WIDTH);

	// not finalized
	m_bFinalized = false;
}