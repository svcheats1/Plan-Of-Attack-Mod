#include "cbase.h"
#include "hud_identify.h"
#include "hud_macros.h"
#include "FileSystem.h"
#include <vgui/isurface.h>
#include <utlvector.h>
#include <c_playerresource.h>
#include <igameresources.h>

#include "cbase.h"
#include "iclientmode.h"
#include "c_sdk_player.h"

#include "vgui/ILocalize.h"

using namespace vgui;

DECLARE_HUDELEMENT(HudIdentify);

HudIdentify::HudIdentify(const char *szName)
	: CHudElement(szName), Label(NULL, "HudIdentify", "")	
{

	// set our parent to the current viewport
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// not visible
	SetVisible( true );

	// show the background
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	// no user id and we end now
	m_iLastUserID  = -1;
	m_fEndDisplay = 0;
}

HudIdentify::~HudIdentify()
{
}

void HudIdentify::LevelInit(void)
{
	m_fEndDisplay = 0;
	m_iLastUserID = -1;
}

bool HudIdentify::ShouldDraw(void)
{
	return true;
}

void HudIdentify::Paint()
{
	Vector vecMuzzlePos, vecEndPos, vecForward, vecLaserPos;
	CSDKPlayer *pPlayer = CSDKPlayer::GetLocalSDKPlayer();
	char text[32];
	Color sColor;

	if(!pPlayer || !pPlayer->IsAlive())
	{
		SetText("");
		return;
	}

	Vector vecSrc = pPlayer->Weapon_ShootPosition(); //shooting position
	QAngle shootAngles = pPlayer->EyeAngles() + pPlayer->GetPunchAngle() + pPlayer->GetSniperDrift(); //shooting angles
	Vector vecDirShooting, vecRight, vecUp;
	AngleVectors( shootAngles, &vecDirShooting, &vecRight, &vecUp );

	Vector vecDir = vecDirShooting;// + x * vecSpread * vecRight +	y * vecSpread * vecUp;
	VectorNormalize( vecDir );
	
	Vector vecEnd = vecSrc + vecDir * MAX_TRACE_LENGTH;

	trace_t	tr;
	//UTIL_TraceLine(vecMuzzlePos, vecEndPos, (MASK_SHOT & ~CONTENTS_WINDOW), pPlayer->GetLocalPlayer(), COLLISION_GROUP_NONE, &tr);
	UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID|CONTENTS_DEBRIS|CONTENTS_HITBOX, pPlayer, COLLISION_GROUP_NONE, &tr );

	// check that we actually found an entity and they're a player
	if(tr.m_pEnt && tr.m_pEnt->IsPlayer())
	{
		// only change our text if they're on our team
		CBasePlayer *pTarget = dynamic_cast<CBasePlayer *>(tr.m_pEnt);
		if(pTarget->GetTeam() == pPlayer->GetTeam())
		{
			// reset the end time and store their user id
			m_fEndDisplay = gpGlobals->curtime + HUD_IDENTIFY_DISPLAY_TIME;
			m_iLastUserID = pTarget->GetUserID();

			// draw their name
			SetFgColor(DetermineColor(m_iLastUserID));
			SetContentAlignment(Label::a_center);
			sprintf(text, "%s (%d)", pTarget->GetPlayerName(), pTarget->GetHealth() );
			SetText(text);
		}
	}
	// start fading
	else if(m_iLastUserID != -1)
	{
		// are we past the display time?
		if(m_fEndDisplay <= gpGlobals->curtime)
		{
			m_iLastUserID = -1;
			m_fEndDisplay = 0;

		}
		else
		{
			// figure out the new color
			sColor = DetermineColor(m_iLastUserID);
			sColor.SetColor(sColor.r(), 
							sColor.g(), 
							sColor.b(), 
							sColor.a() * ((m_fEndDisplay - gpGlobals->curtime) / HUD_IDENTIFY_DISPLAY_TIME));
			SetFgColor(sColor);
		}
	}
	else
		SetText("");

	BaseClass::Paint();
}

void HudIdentify::ApplySchemeSettings(IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// move down
	BaseClass::ApplySchemeSettings(pScheme);

	// set the background
	SetBgColor(m_pScheme->GetColor("Menu.BgColor", Color(255, 255, 255, 0)));
	
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	// we are as wide as the screen
	SetWide(wide);
	// we're 10 pixels down from center
	SetPos(0, tall / 2 + XRES(10));
}

Color HudIdentify::DetermineColor(int iPlayerID)
{
	IGameResources* gr = GameResources();
	if (gr)
		return gr->GetTeamColor(gr->GetTeam(gr->FindIndexByUserID(iPlayerID)));
	else
		return COLOR_GREY;
}