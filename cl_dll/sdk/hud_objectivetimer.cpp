#include "cbase.h"
#include "hud_objectivetimer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT(HudObjectiveTimer);

HudObjectiveTimer::HudObjectiveTimer(const char *szName)
	: CHudSelfTimer(szName)
{
	// hide when this stuff happens
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	gameeventmanager->AddListener(this, "objective_notification", false);
	m_wszObjName = NULL;
	m_bDeleteObjName = false;

	// no seconds
	m_fSecondsRemaining = -1;

	// load our texture
	m_iTexture = UTIL_LoadTexture(OBJECTIVE_TIMER_TEXTURE);
}

HudObjectiveTimer::~HudObjectiveTimer()
{
	gameeventmanager->RemoveListener(this);
}

void HudObjectiveTimer::Init()
{
	CHudElement::Reset();
}

void HudObjectiveTimer::LevelInit()
{
	m_iState = TIMER_STATE_DEACTIVATE;
}

bool HudObjectiveTimer::ShouldDraw()
{
	// @TRJ - the tooltip system is taking care of this now
	return false;
	//return (m_iState != TIMER_STATE_DEACTIVATE && BaseClass::ShouldDraw());
}

void HudObjectiveTimer::FireGameEvent(IGameEvent *event)
{
	if (!FStrEq(event->GetName(), "objective_notification")) {
		BaseClass::FireGameEvent(event);
		return;
	}

	m_iState = (OBJECTIVE_TIMER_STATE) event->GetInt("state");
	m_iOffensiveTeam = event->GetInt("offteam");

	// objective ID handling
	m_iObjID = event->GetInt("id");
	SObjective_t *psObj = GET_OBJ_MGR()->GetObjective(m_iObjID);

	// No objective, so disable the timer (if necessary) 
	if (!psObj) {
		m_iState = TIMER_STATE_DEACTIVATE;
		m_iObjID = -1;
        return;
	}

	// name handling
	if (m_wszObjName && m_bDeleteObjName)
		delete [] m_wszObjName;

	m_wszObjName = localize()->Find(psObj->szName);
	m_bDeleteObjName = !m_wszObjName;
	if (!m_wszObjName) {
		m_wszObjName = new wchar_t[64];
		localize()->ConvertANSIToUnicode(psObj->szName, m_wszObjName, sizeof(wchar_t) * 64);
	}
    
	// get the duration
	SetTimerDuration(event->GetFloat("duration"));

	float fTime = -1;
	if (m_iState == TIMER_STATE_FROZEN/* && GetLocalTeamNumber() == m_iOffensiveTeam*/)
	{
		fTime = event->GetFloat("leewaytimer");

		// store the time remaining when we paused
		m_fPauseTime = m_fEndTime - gpGlobals->curtime;
	}

	// if we're counting down pull out the time left
	if(m_iState == TIMER_STATE_COUNTDOWN)
		m_fSecondsRemaining = fTime = event->GetFloat("timer");

	// kill the timer if we deactivated
	if(m_iState == TIMER_STATE_DEACTIVATE)
		m_fSecondsRemaining = -1;
	
	if (fTime >= 0)
		SetEndTime(fTime + gpGlobals->curtime);

	BaseClass::FireGameEvent(event);
}

int HudObjectiveTimer::GetStringPixelWidth( wchar_t *pString, vgui::HFont hFont )
{
	int iLength = 0;
	for ( wchar_t *wch = pString; *wch != 0; wch++ )
	{
		iLength += surface()->GetCharacterWidth( hFont, *wch );
	}
	return iLength;
}

void HudObjectiveTimer::OnThink()
{
	if (m_iState == TIMER_STATE_FROZEN && GetLocalTeamNumber() != m_iOffensiveTeam)
		m_fEndTime += gpGlobals->frametime;

	BaseClass::OnThink();
}

int HudObjectiveTimer::Round(float fl)
{
	return ceil(fl);
}

/**
* Draws the timer ring
*
* @return void
**/
void HudObjectiveTimer::DrawTimerRing(void)
{
	float fPercentRemaining = 1.0;
	float fX, fY, fXTemp;
	Vertex_t *pVerts;
	int iCurrPoint;

	// decrement the number of seconds
	if(m_fSecondsRemaining > 0 && m_iState == TIMER_STATE_COUNTDOWN)
		m_fSecondsRemaining -=	gpGlobals->frametime * (0.9f + (clamp((float)GET_OBJ_MGR()->GetPlayersInZone(), 1.0f, 3.0f) * 0.1f));

	// figure out how many points to draw
	/*if(m_iState == TIMER_STATE_COUNTDOWN)
		fPercentRemaining = (m_fEndTime - gpGlobals->curtime) / m_fDuration;
	else if(m_iState == TIMER_STATE_FROZEN)
		fPercentRemaining = m_fPauseTime / m_fDuration;*/
	// avoid div by zero
	if(m_fDuration == 0)
		fPercentRemaining = 1;
	else
		fPercentRemaining = (m_fSecondsRemaining < 0 ? m_fDuration : m_fSecondsRemaining) / m_fDuration;
	fPercentRemaining = clamp(fPercentRemaining, 0, 1);

	// get the angle info
	SinCos((fPercentRemaining * M_PI * 2.0) - (M_PI / 2.0), &fY, &fX);

	// figure out the coords at the edge of the square
	// basically we're moving back down the line we just drew
	fXTemp = fX;
	fX /= max(abs(fXTemp), abs(fY));
	fY /= max(abs(fXTemp), abs(fY)); 

	// draw the left half
	iCurrPoint = 0;
	if(fPercentRemaining > 0.5)
	{
		// create some space and setup the first position
		pVerts = new Vertex_t[5];
		pVerts[iCurrPoint].m_Position.x = (fX * OBJECTIVE_TIMER_RADIUS) + OBJECTIVE_TIMER_XOFFSET;
		pVerts[iCurrPoint].m_Position.y = (fY * OBJECTIVE_TIMER_RADIUS) + OBJECTIVE_TIMER_YOFFSET;
		pVerts[iCurrPoint].m_TexCoord.x = (fX + 1.0) / 2.0;
		pVerts[iCurrPoint].m_TexCoord.y = (fY + 1.0) / 2.0;

		// draw the center point
		++iCurrPoint;
		pVerts[iCurrPoint].m_Position.x = OBJECTIVE_TIMER_XOFFSET;
		pVerts[iCurrPoint].m_Position.y = OBJECTIVE_TIMER_YOFFSET;
		pVerts[iCurrPoint].m_TexCoord.x = .5;
		pVerts[iCurrPoint].m_TexCoord.y = .5;

		// draw the lower right
		++iCurrPoint;
		pVerts[iCurrPoint].m_Position.x = OBJECTIVE_TIMER_XOFFSET;
		pVerts[iCurrPoint].m_Position.y = OBJECTIVE_TIMER_RADIUS + OBJECTIVE_TIMER_YOFFSET;
		pVerts[iCurrPoint].m_TexCoord.x = .5;
		pVerts[iCurrPoint].m_TexCoord.y = 1;

		// do we need the lower left?
		if(fPercentRemaining > .625)
		{
			// set the lower left
			++iCurrPoint;
			pVerts[iCurrPoint].m_Position.x = -OBJECTIVE_TIMER_RADIUS + OBJECTIVE_TIMER_XOFFSET;
			pVerts[iCurrPoint].m_Position.y = OBJECTIVE_TIMER_RADIUS + OBJECTIVE_TIMER_YOFFSET;
			pVerts[iCurrPoint].m_TexCoord.x = 0;
			pVerts[iCurrPoint].m_TexCoord.y = 1;
		}
		
		// do we need the upper left?
		if(fPercentRemaining > .875)
		{
			// draw the upper left
			++iCurrPoint;
			pVerts[iCurrPoint].m_Position.x = -OBJECTIVE_TIMER_RADIUS + OBJECTIVE_TIMER_XOFFSET;
			pVerts[iCurrPoint].m_Position.y = -OBJECTIVE_TIMER_RADIUS + OBJECTIVE_TIMER_YOFFSET;
			pVerts[iCurrPoint].m_TexCoord.x = 0;
			pVerts[iCurrPoint].m_TexCoord.y = 0;
		}

		// draw the space
		surface()->DrawSetColor(255, 0, 0, 255);
		surface()->DrawSetTexture(m_iTexture);
		surface()->DrawTexturedPolygon(++iCurrPoint, pVerts);

		// clear it out
		delete [] pVerts;
	}

	/**************************************************/

	// create some space
	iCurrPoint = 0;
	pVerts = new Vertex_t[5];

	// are we actually in the correct half?
	if(fPercentRemaining <= 0.5)
	{
		// figure out the real position
		pVerts[iCurrPoint].m_Position.x = (fX * OBJECTIVE_TIMER_RADIUS) + OBJECTIVE_TIMER_XOFFSET;
		pVerts[iCurrPoint].m_Position.y = (fY * OBJECTIVE_TIMER_RADIUS) + OBJECTIVE_TIMER_YOFFSET;
		pVerts[iCurrPoint].m_TexCoord.x = (fX + 1.0) / 2.0;
		pVerts[iCurrPoint].m_TexCoord.y = (fY + 1.0) / 2.0;
	}
	else
	{
		// just go to the lower left
		pVerts[iCurrPoint].m_Position.x = OBJECTIVE_TIMER_XOFFSET;
		pVerts[iCurrPoint].m_Position.y = OBJECTIVE_TIMER_RADIUS + OBJECTIVE_TIMER_YOFFSET;
		pVerts[iCurrPoint].m_TexCoord.x = .5;
		pVerts[iCurrPoint].m_TexCoord.y = 1;
	}

	// draw the center point
	++iCurrPoint;
	pVerts[iCurrPoint].m_Position.x = OBJECTIVE_TIMER_XOFFSET;
	pVerts[iCurrPoint].m_Position.y = OBJECTIVE_TIMER_YOFFSET;
	pVerts[iCurrPoint].m_TexCoord.x = .5;
	pVerts[iCurrPoint].m_TexCoord.y = .5;

	// draw the upper left
	++iCurrPoint;
	pVerts[iCurrPoint].m_Position.x = OBJECTIVE_TIMER_XOFFSET;
	pVerts[iCurrPoint].m_Position.y = OBJECTIVE_TIMER_YOFFSET - OBJECTIVE_TIMER_RADIUS;
	pVerts[iCurrPoint].m_TexCoord.x = .5;
	pVerts[iCurrPoint].m_TexCoord.y = 0;

	// do we need the upper right?
	if(fPercentRemaining > .125)
	{
		// set the upper right left
		++iCurrPoint;
		pVerts[iCurrPoint].m_Position.x = OBJECTIVE_TIMER_RADIUS + OBJECTIVE_TIMER_XOFFSET;
		pVerts[iCurrPoint].m_Position.y = OBJECTIVE_TIMER_YOFFSET - OBJECTIVE_TIMER_RADIUS;
		pVerts[iCurrPoint].m_TexCoord.x = 1;
		pVerts[iCurrPoint].m_TexCoord.y = 0;
	}
	
	// do we need the lower right?
	if(fPercentRemaining > .375)
	{
		// draw the lower right
		++iCurrPoint;
		pVerts[iCurrPoint].m_Position.x = OBJECTIVE_TIMER_RADIUS + OBJECTIVE_TIMER_XOFFSET;
		pVerts[iCurrPoint].m_Position.y = OBJECTIVE_TIMER_RADIUS + OBJECTIVE_TIMER_YOFFSET;
		pVerts[iCurrPoint].m_TexCoord.x = 1;
		pVerts[iCurrPoint].m_TexCoord.y = 1;
	}

	// draw the space
	surface()->DrawSetColor(255, 0, 0, 255);
	surface()->DrawSetTexture(m_iTexture);
	surface()->DrawTexturedPolygon(++iCurrPoint, pVerts);

	// clear it out
	delete [] pVerts;

}

/**
* Draws the text for the timer
*
* @return void
**/
void HudObjectiveTimer::DrawTimerText(void)
{
	int iWidth, iTall, iStringWidth;
	wchar_t *unicode;
	wchar_t wszNumber[16];

	// figure out the number to use
	switch(m_iObjID)
	{
		case 0:
			swprintf(wszNumber, L" (I)");
			break;
		case 1:
			swprintf(wszNumber, L" (II)");
			break;
		case 2:
			swprintf(wszNumber, L" (III)");
			break;
		default:
			wszNumber[0] = 0;
	}

	if (m_iState == TIMER_STATE_COUNTDOWN) {
		SetFgColor(m_pScheme->GetColor("ObjectiveTimer.CountdownColor", COLOR_YELLOW));
		unicode = GetCountdownString(wszNumber);
	}
	else if (m_iState == TIMER_STATE_FROZEN) {
		SetFgColor(m_pScheme->GetColor("ObjectiveTimer.FrozenColor", COLOR_RED));
		unicode = GetFrozenString(wszNumber);
	}
	else
		return;

	surface()->DrawSetTextColor(GetFgColor());
	
	iStringWidth = GetStringPixelWidth(unicode, m_hNumberFont); 
	surface()->GetScreenSize(iWidth, iTall);

    surface()->DrawSetTextFont(m_hNumberFont);
	surface()->DrawSetTextPos(digit_xpos + (iWidth / 2) - (iStringWidth / 2), digit_ypos);
	for (wchar_t *ch = unicode; *ch != 0; ch++)
	{
		surface()->DrawUnicodeChar(*ch);
	}

	delete [] unicode;
}

void HudObjectiveTimer::Paint()
{
	// draw the text if we need to 
	if(m_iState != TIMER_STATE_DEACTIVATE)
		DrawTimerText();
}

wchar_t* HudObjectiveTimer::GetCountdownString(const wchar_t *wszNumber)
{
	wchar_t *unicode = new wchar_t[256];
	int iSec = m_iMinutes * 60 + m_iSeconds;

	// if we're frozen use the pause time
	if(m_iState == TIMER_STATE_FROZEN)
		iSec = m_fPauseTime;

	if (iSec == 1)
		swprintf(unicode, L"%s%s will be captured in 1 second", m_wszObjName, wszNumber);
	else
		swprintf(unicode, L"%s%s will be captured in %d seconds", m_wszObjName, wszNumber, iSec);

	return unicode;
}

wchar_t* HudObjectiveTimer::GetFrozenString(const wchar_t *wszNumber)
{
	int iSec = m_iMinutes * 60 + m_iSeconds;

	if (GetLocalTeamNumber() == m_iOffensiveTeam) {
		wchar_t *unicode = new wchar_t[256];
		if (iSec > 1)
			swprintf(unicode, L"Your team has left %s%s! The timer will reset in %d seconds!", m_wszObjName, wszNumber, iSec);
		else
			swprintf(unicode, L"Your team has left %s%s! The timer will reset in 1 second!", m_wszObjName, wszNumber);
		return unicode;
	}
	else
		return GetCountdownString(wszNumber);
}

int HudObjectiveTimer::GetLocalTeamNumber() 
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer)
		return pPlayer->GetTeamNumber();

	return -1;
}

void HudObjectiveTimer::ApplySchemeSettings(IScheme *pScheme)
{
	m_pScheme = pScheme;

	BaseClass::ApplySchemeSettings(pScheme);
}