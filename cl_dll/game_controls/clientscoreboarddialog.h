//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CLIENTSCOREBOARDDIALOG_H
#define CLIENTSCOREBOARDDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <cl_dll/iviewport.h>
#include <igameevents.h>
#include <vgui/MouseCode.h>
#include <vgui/MessageMap.h>

#include "MySectionedListPanel.h"

#define TYPE_NOTEAM			0	// NOTEAM must be zero :)
#define TYPE_TEAM			1	// a section for a single team	
#define TYPE_SPECTATORS		2	// a section for a spectator group
#define TYPE_BLANK			3

#define VOICE_ICON_SIZE		12

//-----------------------------------------------------------------------------
// Purpose: Game ScoreBoard
//-----------------------------------------------------------------------------
class CClientScoreBoardDialog : public vgui::Frame, public IViewPortPanel, public IGameEventListener2
{
private:
	DECLARE_CLASS_SIMPLE( CClientScoreBoardDialog, vgui::Frame );

protected:
// column widths at 640
	enum { NAME_WIDTH = 160, 
			SKILL_WIDTH = 60,
			CAPS_WIDTH = 40,
			XP_WIDTH = 40,
			SCORE_WIDTH = 40,
			DEATH_WIDTH = 40,
			PING_WIDTH = 80, 
			DEAD_WIDTH = 60,
			VOICE_WIDTH = 25,
			FRIENDS_WIDTH = 0 };
	// total = 340 (<-- not correct. it's really 572. someone sucks at adding.)

public:
	CClientScoreBoardDialog( IViewPort *pViewPort );
	virtual ~CClientScoreBoardDialog();

	virtual const char *GetName( void ) { return PANEL_SCOREBOARD; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void );
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
  	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
 	
	// IGameEventListener interface:
	virtual void FireGameEvent( IGameEvent *event);
			

protected:
	// functions to override
	virtual bool GetPlayerScoreInfo(int playerIndex, KeyValues *outPlayerInfo);
	virtual void InitScoreboardSections();
	virtual void UpdateTeamInfo();
	virtual void UpdatePlayerInfo();
	virtual Color DetermineColor(int iTeamID);
	
	virtual void AddHeader(); // add the start header of the scoreboard
	virtual void AddSection(int teamType, int teamNumber); // add a new section header for a team

	MESSAGE_FUNC_PARAMS( OnItemSelected, "ItemSelected", data );
	virtual void TogglePlayerMute(int iPlayerIndex);//const char *szName);

	// sorts players within a section
	static bool StaticPlayerSortFunc(MySectionedListPanel *list, int itemID1, int itemID2);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	// finds the player in the scoreboard
	int FindItemIDForPlayerIndex(int playerIndex);

	int m_iNumTeams;

	MySectionedListPanel *m_pPlayerList;
	int				m_iSectionId; // the current section we are entering into
	int				m_iTeamSectionMap[MAX_TEAMS+1];

	int s_VoiceImage[7];
	int TrackerImage;
	int	m_HLTVSpectators;

	void MoveLabelToFront(const char *textEntryName);

private:
	int			m_iPlayerIndexSymbol;
	int			m_iDesiredHeight;
	IViewPort	*m_pViewPort;
	float		m_fNextUpdateTime;

	// methods
	void FillScoreBoard();
};


#endif // CLIENTSCOREBOARDDIALOG_H
