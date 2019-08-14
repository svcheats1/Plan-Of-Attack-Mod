//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <stdio.h>

#include <cdll_client_int.h>
#include <cdll_util.h>
#include <globalvars_base.h>
#include <igameresources.h>

#include "clientscoreboarddialog.h"
#include "IGameUIFuncs.h" // for key bindings

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vstdlib/IKeyValuesSystem.h>

#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Label.h>
#include "MySectionedListPanel.h"

#include <cl_dll/iviewport.h>
#include <igameresources.h>

#include "skillclass.h"
#include "c_team.h"

#define SECTION_HEADER_ID 3

#include "voice_status.h"
#include "Friends/IFriendsUser.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// extern vars
extern IFriendsUser *g_pFriendsUser;
extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClientScoreBoardDialog::CClientScoreBoardDialog(IViewPort *pViewPort) : Frame( NULL, PANEL_SCOREBOARD )
{
	m_iPlayerIndexSymbol = KeyValuesSystem()->GetSymbolForString("playerIndex");

	memset(s_VoiceImage, 0x0, sizeof( s_VoiceImage ));
	TrackerImage = 0;
	m_pViewPort = pViewPort;

	// initialize dialog
	SetProportional(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(true);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );

	// set the scheme before any child control is created
	SetScheme("ClientScheme");

	m_pPlayerList = new MySectionedListPanel(this, "PlayerList");
	m_pPlayerList->SetVerticalScrollbar(false);
	m_pPlayerList->AddActionSignalTarget(this);

	LoadControlSettings("Resource/UI/ScoreBoard.res");
	m_iDesiredHeight = GetTall();
	m_pPlayerList->SetVisible( false ); // hide this until we load the images in applyschemesettings

	m_HLTVSpectators = 0;
	
	// update scoreboard instantly if on of these events occure
	gameeventmanager->AddListener(this, "hltv_status", false );
	gameeventmanager->AddListener(this, "server_spawn", false );
}

void CClientScoreBoardDialog::OnItemSelected(KeyValues *data)
{
	IGameResources *gr = GameResources();
	
	int iItemID = data->GetInt("itemID");
	KeyValues *playerData = m_pPlayerList->GetItemData(iItemID);
	int iPlayerIndex = playerData->GetInt("playerIndex");
	player_info_t pi;
	if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
		return;
	if (!gr->IsLocalPlayer(iPlayerIndex) && !FStrEq(pi.guid, "UNKNOWN") && !FStrEq(pi.guid, "STEAM_ID_LAN")) {
		TogglePlayerMute(iPlayerIndex);
		Update();
	}
}

void CClientScoreBoardDialog::TogglePlayerMute(int iPlayerIndex)
{
	CVoiceStatus* voicething = GetClientVoiceMgr();
	//voicething->StartSquelchMode();
	voicething->SetPlayerBlockedState(iPlayerIndex,!voicething->IsPlayerBlocked(iPlayerIndex));
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CClientScoreBoardDialog::~CClientScoreBoardDialog()
{
	gameeventmanager->RemoveListener(this);
}

//-----------------------------------------------------------------------------
// Purpose: clears everything in the scoreboard and all it's state
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::Reset()
{
	// clear
	m_pPlayerList->DeleteAllItems();
	m_pPlayerList->RemoveAllSections();

	m_iSectionId = 0;
	m_fNextUpdateTime = 0;
	// add all the sections
	InitScoreboardSections();
}

//-----------------------------------------------------------------------------
// Purpose: adds all the team sections to the scoreboard
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::InitScoreboardSections()
{
	AddHeader();					// 0
	
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer && pPlayer->GetTeamNumber() > 0) {
		AddSection(TYPE_TEAM, pPlayer->GetTeamNumber());	// display my team first
		for(int i = 1; i <= MAX_TEAMS; ++i) {
			if (i != pPlayer->GetTeamNumber())
				AddSection(TYPE_TEAM, i);
		}
	}
	else {
		for(int i = 1; i <= MAX_TEAMS; ++i)
			AddSection(TYPE_TEAM, i);
	}

	AddSection(TYPE_SPECTATORS, TEAM_SPECTATOR); // 3
}

//-----------------------------------------------------------------------------
// Purpose: sets up screen
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	ImageList *imageList = new ImageList(false);
	TrackerImage = imageList->AddImage(scheme()->GetImage("gfx/vgui/640_scoreboardtracker", true));//wth?

	m_pPlayerList->SetImageList(imageList, false);
	m_pPlayerList->SetVisible( true );

	// light up scoreboard a bit
	SetBgColor( Color( 0,0,0,100) );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Reset();
		Update();
		Activate();

		SetMouseInputEnabled( true );
	}
	else
	{
		BaseClass::SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}
}

void CClientScoreBoardDialog::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "hltv_status") == 0 )
	{
		// spectators = clients - proxies
		m_HLTVSpectators = event->GetInt( "clients" );
		m_HLTVSpectators -= event->GetInt( "proxies" );
	}

	else if ( Q_strcmp(type, "server_spawn") == 0 )
	{
		// We'll post the message ourselves instead of using SetControlString()
		// so we don't try to translate the hostname.
		const char *hostname = event->GetString( "hostname" );
		Panel *control = FindChildByName( "ServerName" );
		if ( control )
		{
			PostMessage( control, new KeyValues( "SetText", "text", hostname ) );
		}
		control->MoveToFront();
	}

	if( IsVisible() )
		Update();

}

bool CClientScoreBoardDialog::NeedsUpdate( void )
{
	return (m_fNextUpdateTime < gpGlobals->curtime);
		

}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::Update( void )
{
	// Set the title
	
	// Reset();
	m_pPlayerList->DeleteAllItems();
	
	FillScoreBoard();

	// grow the scoreboard to fit all the players
	int wide, tall;
	m_pPlayerList->GetContentSize(wide, tall);
	wide = GetWide();
	if (m_iDesiredHeight < tall)
	{
		SetSize(wide, tall);
		m_pPlayerList->SetSize(wide, tall);
	}
	else
	{
		SetSize(wide, m_iDesiredHeight);
		m_pPlayerList->SetSize(wide, m_iDesiredHeight);
	}

	MoveToCenterOfScreen();

	// update every second
	m_fNextUpdateTime = gpGlobals->curtime + 1.0f; 
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdateTeamInfo()
{
	wchar_t buf[64];
	wchar_t *pTeamName;
	int sectionID;
	C_Team *pTeam;

	int iPings[3] = { 0, 0, 0 };
	int iCounts[3] = { 0, 0, 0 };

	IGameResources *gr = GameResources();
	if (gr) {
		for(int i = 1; i <= gpGlobals->maxClients; ++i) {
			if (!gr->IsConnected(i) || gr->GetTeam(i) <= TEAM_SPECTATOR)
				continue;

			iPings[gr->GetTeam(i)] += gr->GetPing(i);
			iCounts[gr->GetTeam(i)]++;
		}
	}
	
	for(int i = TEAM_A; i <= TEAM_B; ++i)
	{
		pTeam = GetGlobalTeam(i);
		if(!pTeam)
			continue;
		sectionID = m_iTeamSectionMap[i];

		_snwprintf(buf, 64, L"Score: %d", pTeam->Get_Score());

		m_pPlayerList->ModifyColumn(sectionID, "dead", buf);

		pTeamName = vgui::localize()->Find(pTeam->Get_Name());

		_snwprintf(buf, 64, L"%s - %d player%s", 
			pTeamName,
			pTeam->Get_Number_Players(),
			pTeam->Get_Number_Players() == 1 ? L"" : L"s");

		m_pPlayerList->ModifyColumn(sectionID, "name", buf);

		if (iCounts[i] > 0)
			iPings[i] /= iCounts[i];

		_snwprintf(buf, 64, L"%d", iPings[i]);

		m_pPlayerList->ModifyColumn(sectionID, "ping", buf);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdatePlayerInfo()
{
	m_iSectionId = 0; // 0'th row is a header
	int selectedRow = -1;

	// walk all the players and make sure they're in the scoreboard
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		IGameResources *gr = GameResources();

		if ( gr && gr->IsConnected( i ) )
		{
			// add the player to the list
			KeyValues *playerData = new KeyValues("data");
			GetPlayerScoreInfo( i, playerData );

			const char *oldName = playerData->GetString("name","");
			int bufsize = strlen(oldName) * 2;
			char *newName = (char *)_alloca( bufsize );

			UTIL_MakeSafeName( oldName, newName, bufsize );

			playerData->SetString("name", newName);

			int sectionID;
			int itemID = FindItemIDForPlayerIndex( i );
			if (gr->GetTeam(i) >= 0)
  				sectionID = m_iTeamSectionMap[gr->GetTeam(i)];
			else
				sectionID = m_iTeamSectionMap[TEAM_SPECTATOR];
			
			if ( gr->IsLocalPlayer( i ) )
			{
				selectedRow = itemID;
			}
			if (itemID == -1)
			{
				// add a new row
				itemID = m_pPlayerList->AddItem( sectionID, playerData );
			}
			else
			{
				// modify the current row
				m_pPlayerList->ModifyItem( itemID, sectionID, playerData );
			}

			m_pPlayerList->SetItemFgColor(itemID, DetermineColor(gr->GetTeam(i)));
			
			playerData->deleteThis();
		}
		else
		{
			// remove the player
			int itemID = FindItemIDForPlayerIndex( i );
			if (itemID != -1)
			{
				m_pPlayerList->RemoveItem(itemID);
			}
		}
	}

	if ( selectedRow != -1 )
	{
		m_pPlayerList->SetSelectedItem(selectedRow);
	}
}

Color CClientScoreBoardDialog::DetermineColor(int iTeamID)
{
	IGameResources *gr = GameResources();
	return gr->GetTeamColor(iTeamID);

	/*
	// if this is my team, it's blue
	if (iTeamID == TEAM_A)
		return COLOR_BLUE;
	// enemies are red, but spectators aren't enemies
	else if (iTeamID == TEAM_B)
		return COLOR_RED;
	// spectators, use the GameResources data
	else {
		IGameResources *gr = GameResources();
		return gr->GetTeamColor(iTeamID);
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::AddHeader()
{
	// add the top header
	m_pPlayerList->AddSection(m_iSectionId, "");
	m_pPlayerList->SetSectionAlwaysVisible(m_iSectionId, true);
	m_pPlayerList->AddColumnToSection(m_iSectionId, "voice", "", 0, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "name", "", 0, scheme()->GetProportionalScaledValue(NAME_WIDTH) );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "dead", L"Dead", 0, scheme()->GetProportionalScaledValue(DEAD_WIDTH));
	m_pPlayerList->AddColumnToSection(m_iSectionId, "skill", L"Class", 0, scheme()->GetProportionalScaledValue(SKILL_WIDTH));
	m_pPlayerList->AddColumnToSection(m_iSectionId, "level", L"Level", 0, scheme()->GetProportionalScaledValue(XP_WIDTH));
	m_pPlayerList->AddColumnToSection(m_iSectionId, "caps", L"Caps", 0, scheme()->GetProportionalScaledValue(CAPS_WIDTH) );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "score", L"Kills", 0, scheme()->GetProportionalScaledValue(SCORE_WIDTH) );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "deaths", L"Deaths", 0, scheme()->GetProportionalScaledValue(DEATH_WIDTH) );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "ping", L"Ping", 0, scheme()->GetProportionalScaledValue(PING_WIDTH) );
	//m_pPlayerList->AddColumnToSection(m_iSectionId, "voice", L"Voice", MySectionedListPanel::COLUMN_IMAGE | MySectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "tracker", "#PlayerTracker", MySectionedListPanel::COLUMN_IMAGE, scheme()->GetProportionalScaledValue(FRIENDS_WIDTH) );
	m_pPlayerList->SetSectionFgColor( m_iSectionId, COLOR_YELLOW );
	m_iSectionId++;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the scoreboard (i.e the team header)
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::AddSection(int teamType, int teamNumber)
{
	IGameResources *gr = GameResources();

	if ( !gr )
		return;

	if ( teamType == TYPE_TEAM )
	{
		// setup the team name
		wchar_t *teamName = localize()->Find( gr->GetTeamName(teamNumber) );
		wchar_t name[64];
		wchar_t string1[1024];
		
		if (!teamName)
		{
			localize()->ConvertANSIToUnicode(gr->GetTeamName(teamNumber), name, sizeof(name));
			teamName = name;
		}

		// JD: not really sure what this does
		localize()->ConstructString( string1, sizeof( string1 ), localize()->Find("#Player"), 2, teamName );
		
		m_pPlayerList->AddSection(m_iSectionId, "", StaticPlayerSortFunc);
		m_pPlayerList->SetSectionAlwaysVisible(m_iSectionId, true);
		m_pPlayerList->AddColumnToSection(m_iSectionId, "voice", "", MySectionedListPanel::COLUMN_FONT, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );
		m_pPlayerList->SetColumnFont(m_iSectionId, "voice", "VoiceIcons");
		m_pPlayerList->AddColumnToSection(m_iSectionId, "name", teamName, 0, scheme()->GetProportionalScaledValue(NAME_WIDTH) );
		m_pPlayerList->AddColumnToSection(m_iSectionId, "dead", "", 0, scheme()->GetProportionalScaledValue(DEAD_WIDTH));
		m_pPlayerList->AddColumnToSection(m_iSectionId, "skill", "", 0, scheme()->GetProportionalScaledValue(SKILL_WIDTH));
		m_pPlayerList->AddColumnToSection(m_iSectionId, "level", "", 0, scheme()->GetProportionalScaledValue(XP_WIDTH));
		m_pPlayerList->AddColumnToSection(m_iSectionId, "caps", "", 0, scheme()->GetProportionalScaledValue(CAPS_WIDTH) );
		m_pPlayerList->AddColumnToSection(m_iSectionId, "frags", "", 0, scheme()->GetProportionalScaledValue(SCORE_WIDTH) );
		m_pPlayerList->AddColumnToSection(m_iSectionId, "deaths", "", 0, scheme()->GetProportionalScaledValue(DEATH_WIDTH) );
		m_pPlayerList->AddColumnToSection(m_iSectionId, "ping", "", 0, scheme()->GetProportionalScaledValue(PING_WIDTH) );
		//m_pPlayerList->AddColumnToSection(m_iSectionId, "voice", "", MySectionedListPanel::COLUMN_IMAGE | MySectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );

		m_pPlayerList->SetSectionFgColor(m_iSectionId, DetermineColor(teamNumber));

		m_iTeamSectionMap[teamNumber] = m_iSectionId++;
	}
	// JD: put everyone else in with spectators, including NOTEAMers
	else //if ( teamType == TYPE_SPECTATORS )
	{
		m_pPlayerList->AddSection(m_iSectionId, "");
		m_pPlayerList->AddColumnToSection(m_iSectionId, "name", "#Spectators", 0, scheme()->GetProportionalScaledValue(NAME_WIDTH));
		//m_pPlayerList->AddColumnToSection(m_iSectionId, "score", "", 0, scheme()->GetProportionalScaledValue(SCORE_WIDTH) );
		m_pPlayerList->SetSectionFgColor( m_iSectionId, DetermineColor(teamNumber) );

		// postfix increment on purpose
		m_iTeamSectionMap[teamNumber] = m_iSectionId++;
	}
	/*
	//mitch
	else if ( teamType == TYPE_NOTEAM )
	{
		IGameResources *gr = GameResources();

		if( !gr )
			return;

		wchar_t *teamName;
		wchar_t name[64];
		wchar_t string1[1024];

		teamName=name;

		localize()->ConstructString( string1, sizeof(string1), localize()->Find("#Player"),2,teamName);

		m_pPlayerList->AddSection(m_iSectionId, "", StaticPlayerSortFunc);

		m_pPlayerList->AddColumnToSection(m_iSectionId, "name", string1, 0, scheme()->GetProportionalScaledValue(NAME_WIDTH) );
		m_pPlayerList->AddColumnToSection(m_iSectionId, "frags", "", 0, scheme()->GetProportionalScaledValue(SCORE_WIDTH) );
		m_pPlayerList->AddColumnToSection(m_iSectionId, "deaths", "", 0, scheme()->GetProportionalScaledValue(DEATH_WIDTH) );
		m_pPlayerList->AddColumnToSection(m_iSectionId, "ping", "", 0, scheme()->GetProportionalScaledValue(PING_WIDTH) );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CClientScoreBoardDialog::StaticPlayerSortFunc(MySectionedListPanel *list, int itemID1, int itemID2)
{
	KeyValues *it1 = list->GetItemData(itemID1);
	KeyValues *it2 = list->GetItemData(itemID2);
	Assert(it1 && it2);

	// first compare caps
	int v1 = it1->GetInt("caps");
	int v2 = it2->GetInt("caps");
	if (v1 > v2)
		return true;
	else if (v1 < v2)
		return false;

	// then compare kills
	v1 = it1->GetInt("frags");
	v2 = it2->GetInt("frags");
	if (v1 > v2)
		return true;
	else if (v1 < v2)
		return false;

	// next compare deaths
	v1 = it1->GetInt("deaths");
	v2 = it2->GetInt("deaths");
	if (v1 > v2)
		return false;	// swap because deaths are bad
	else if (v1 < v2)
		return true;

	// the same, so compare itemID's (as a sentinel value to get deterministic sorts)
	return itemID1 < itemID2;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CClientScoreBoardDialog::GetPlayerScoreInfo(int playerIndex, KeyValues *kv)
{
	IGameResources *gr = GameResources();

	if (!gr )
		return false;

	if (CBasePlayer::GetLocalPlayer() && CBasePlayer::GetLocalPlayer()->GetTeamNumber() > TEAM_SPECTATOR &&
		CBasePlayer::GetLocalPlayer()->GetTeamNumber() == gr->GetTeam(playerIndex))
	{
		int iXP = gr->GetPlayerXP(playerIndex);
		// which skill level are we in?
		for(int i = 0; i < SKILL_LEVEL_COUNT; ++i)
		{
			// if we haven't passed this level, send the previous one back
			if(iXP < CSkillClass::s_iSkillLevels[i])
				break;
		}
		kv->SetInt("level", i + 1);
		kv->SetString("skill", gr->GetPlayerSkill(playerIndex));
	}
	// else ? we might need to do this to clear it out if you change teams

	kv->SetInt("deaths", gr->GetDeaths( playerIndex ) );
	kv->SetInt("frags", gr->GetFrags( playerIndex ) );
	kv->SetInt("caps", gr->GetCaps( playerIndex ) );
	kv->SetInt("ping", gr->GetPing( playerIndex ) ) ;
	kv->SetString("name", gr->GetPlayerName( playerIndex ) );
	//kv->SetInt("xp", gr->GetPlayerXP(playerIndex));
	kv->SetString("dead", (gr->IsAlive(playerIndex) ? "" : "DEAD"));
	kv->SetInt("playerIndex", playerIndex);
	//kv->SetInt("voice",	s_VoiceImage[GetClientVoice()->GetSpeakerStatus( playerIndex - 1) ]);	
	
	switch(GetClientVoiceMgr()->PlayerSpeakingState(playerIndex)) {
		case 0:
		default:
			kv->SetString("voice", "");
			break;
		case 1:
			kv->SetString("voice", "I");
			break;
		case 2:
			kv->SetString("voice", "G");
			break;
		case 3:
			kv->SetString("voice", "H");
			break;
	}

/*	// setup the tracker column
	if (g_pFriendsUser)
	{
		unsigned int trackerID = gEngfuncs.GetTrackerIDForPlayer(row);

		if (g_pFriendsUser->IsBuddy(trackerID) && trackerID != g_pFriendsUser->GetFriendsID())
		{
			kv->SetInt("tracker",TrackerImage);
		}
	}
*/
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: reload the player list on the scoreboard
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::FillScoreBoard()
{
	// update totals information
	UpdateTeamInfo();

	// update player info
	UpdatePlayerInfo();
} 

//-----------------------------------------------------------------------------
// Purpose: searches for the player in the scoreboard
//-----------------------------------------------------------------------------
int CClientScoreBoardDialog::FindItemIDForPlayerIndex(int playerIndex)
{
	for (int i = 0; i <= m_pPlayerList->GetHighestItemID(); i++)
	{
		if (m_pPlayerList->IsItemIDValid(i))
		{
			KeyValues *kv = m_pPlayerList->GetItemData(i);
			kv = kv->FindKey(m_iPlayerIndexSymbol);
			if (kv && kv->GetInt() == playerIndex)
				return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::MoveLabelToFront(const char *textEntryName)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->MoveToFront();
	}
}