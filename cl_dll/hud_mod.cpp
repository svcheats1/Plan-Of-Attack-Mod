#include "cbase.h"
#include "hud_mod.h"
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

DECLARE_HUDELEMENT(HudMOD);

HudMOD::HudMOD(const char *szName)
	: CHudElement(szName), Panel(NULL, "HudMOD")	
{
	// add myself as client-side listener for this event
	gameeventmanager->AddListener( this, "player_death", false );
	//gameeventmanager->AddListener( this, "player_levelup", false );

	// set our parent to the current viewport
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// not visible
	SetVisible( true );

	// show the background
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	m_pDeathChars = new KeyValues("DeathChars");
	
	m_pDeathChars->LoadFromFile(::filesystem, "scripts/weapon_death_chars.txt", "GAME");
}

HudMOD::~HudMOD()
{
	gameeventmanager->RemoveListener(this);
	m_pDeathChars->deleteThis();
}

void HudMOD::LevelInit(void)
{
	m_Messages.Purge();
}

bool HudMOD::ShouldDraw(void)
{
	return (m_Messages.Count() > 0);
}

int HudMOD::GetStringPixelWidth( wchar_t *pString, vgui::HFont hFont )
{
	int iLength = 0;
	for ( wchar_t *wch = pString; *wch != 0; wch++ )
	{
		iLength += surface()->GetCharacterWidth( hFont, *wch );
	}
	return iLength;
}

void HudMOD::Paint()
{
	wchar_t unicode1[256];
	wchar_t unicode2[256];
	wchar_t unicode3[256];

	const wchar_t *weaponname;

	int iStringWidth;

	IGameResources* gr = GameResources();
	if (!gr)
		return;

	wchar_t attacker[64];
	wchar_t user[64];	
	
	while ((m_Messages.Count() > 0 && m_Messages[0].starttime + 5 < gpGlobals->curtime) || m_Messages.Count() > 5)
	{
		m_Messages.Remove(0);
	}

	for(int i = 0; i < m_Messages.Count(); ++i)
	{
		if(!m_Messages[i].levelup)
		{
			localize()->ConvertANSIToUnicode(gr->GetPlayerName(gr->FindIndexByUserID(m_Messages[i].attackerid)),attacker,sizeof(attacker));
			localize()->ConvertANSIToUnicode(gr->GetPlayerName(gr->FindIndexByUserID(m_Messages[i].userid)),user,sizeof(user));

			weaponname = m_pDeathChars->GetWString(m_Messages[i].weapon, L"C");

			swprintf(unicode1, L"%s ", attacker);
			swprintf(unicode2, L" %s", user);
			swprintf(unicode3, L"%s", weaponname);

			iStringWidth = GetStringPixelWidth(unicode1, m_pScheme->GetFont("Default"));
		
			if(m_Messages[i].attackerid != m_Messages[i].userid)
				iStringWidth += GetStringPixelWidth(unicode2, m_pScheme->GetFont("Default"));

			iStringWidth += GetStringPixelWidth(unicode3, m_pScheme->GetFont("CSTypeDeath")); //change font to hl2 icons

			surface()->DrawSetTextPos(GetWide() - iStringWidth - 10, 25 * i + 3 );

			if(m_Messages[i].attackerid != m_Messages[i].userid)
			{
				surface()->DrawSetTextFont(m_pScheme->GetFont("Default"));

				SetFgColor(DetermineColor(m_Messages[i].attackerid));
				surface()->DrawSetTextColor(GetFgColor());

				PrintString(unicode1);
			}

			SetFgColor(m_pScheme->GetColor("HudMOD.FgColor", COLOR_RED));
			surface()->DrawSetTextColor(GetFgColor());

			surface()->DrawSetTextFont(m_pScheme->GetFont("CSTypeDeath")); //change font to hl2 icons

			wchar_t *ch = unicode3;
			surface()->DrawUnicodeChar(*ch);

			SetFgColor(m_Messages[i].color);
			surface()->DrawSetTextColor(GetFgColor());

			surface()->DrawSetTextFont(m_pScheme->GetFont("Default"));
		
			PrintString(unicode2);
		}
		else
		{
			localize()->ConvertANSIToUnicode(gr->GetPlayerName(gr->FindIndexByUserID(m_Messages[i].userid)),attacker,sizeof(user));
			SetFgColor(m_Messages[i].color);
			weaponname = L"p";
			swprintf(unicode2, L"%s", weaponname);
			swprintf(unicode1, L"%s", attacker);
			surface()->DrawSetTextPos(0, 25 * i + 3 );
			surface()->DrawSetTextFont(m_pScheme->GetFont("CSTypeDeath"));
			PrintString(unicode2);
			surface()->DrawSetTextFont(m_pScheme->GetFont("Default"));
			PrintString(unicode1);
		}
	}
}

void HudMOD::ApplySchemeSettings(IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// move down
	BaseClass::ApplySchemeSettings(pScheme);

	// set the background
	SetBgColor(m_pScheme->GetColor("Menu.BgColor", Color(255, 255, 255, 0)));
}

void HudMOD::FireGameEvent(IGameEvent *event)
{
	MODstruct message1;
	// check event type and print message
	
	message1.userid = event->GetInt("userid");
	message1.starttime = gpGlobals->curtime;

	if(strncmp(event->GetName(), "player_death", 12) == 0)
	{
		message1.attackerid = event->GetInt("attacker");
		Q_strncpy(message1.weapon, event->GetString("weapon","unknown"), sizeof(message1.weapon));
		message1.headshot = event->GetInt("headshot",false);
		message1.levelup = false;
	}
	else if(strncmp(event->GetName(), "player_levelup", 14) == 0)
	{
		return;
		//message1.levelup = true;
	}
	if(message1.attackerid == 0)
		message1.attackerid = message1.userid;

	// cache the color so we dont have to figure it out again and it doesn't change
	// suddenly if we change teams.
	message1.color = DetermineColor(message1.userid);

	//if (!(message1.levelup) || IsMyTeam(message1.userid))
	m_Messages.AddToTail(message1);

	PrintDeathInConsole(message1);
}

bool HudMOD::IsMyTeam(int iPlayerID)
{
	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	IGameResources* gr = GameResources();
	if (!pLocalPlayer || !gr)
		return false;
    
	int iPlayerIndex = gr->FindIndexByUserID(iPlayerID);
	if(gr->IsConnected(iPlayerIndex))
		return (pLocalPlayer->GetTeamNumber() == gr->GetTeam(iPlayerIndex));

	return false;
}

Color HudMOD::DetermineColor(int iPlayerID)
{
	IGameResources* gr = GameResources();
	if (gr)
		return gr->GetTeamColor(gr->GetTeam(gr->FindIndexByUserID(iPlayerID)));
	else
		return COLOR_GREY;
}

void HudMOD::PrintString( wchar_t *pString )
{
	for(wchar_t *ch = pString; *ch != 0; ch++)
	{
		surface()->DrawUnicodeChar(*ch);
	}
}

// this could make more sense to go in sdk_hud_chat, if it were to listen for events.
void HudMOD::PrintDeathInConsole(const MODstruct &info)
{
	IGameResources *gr = GameResources();

	if (!gr)
		return;

	char buf[256];
	// did we commit suicide
	if (info.attackerid == info.userid) {
		// did we commit suicide with a known weapon?
		const char *weaponname = m_pDeathChars->GetString(info.weapon, "C");
		// no, so say we committed suicide with 'world'
		if (strcmp(weaponname, "C") == 0) {
			Q_snprintf(buf, 255, "%s committed suicide with world\n", 
				gr->GetPlayerName(gr->FindIndexByUserID(info.userid)));
		}
		// yes, we used a weapon, print that
		else {
			Q_snprintf(buf, 255, "%s committed suicide with %s\n",
				gr->GetPlayerName(gr->FindIndexByUserID(info.userid)),
				info.weapon);
		}
	}
	// did we get a headshot?
	else if (info.headshot)
		Q_snprintf(buf, 255, "%s killed %s with %s (headshot)\n", 
			gr->GetPlayerName(gr->FindIndexByUserID(info.attackerid)),
			gr->GetPlayerName(gr->FindIndexByUserID(info.userid)),
			info.weapon);
	// just a normal kill
	else
		Q_snprintf(buf, 255, "%s killed %s with %s\n",
			gr->GetPlayerName(gr->FindIndexByUserID(info.attackerid)),
			gr->GetPlayerName(gr->FindIndexByUserID(info.userid)),
			info.weapon);

	// print it to console
	Msg(buf);
}