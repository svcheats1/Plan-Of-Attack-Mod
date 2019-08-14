#ifndef MAP_H
#define MAP_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vector.h>
#include <igameevents.h>
#include <shareddefs.h>
#include <const.h>
#include "mapforeground.h"

#include "imap.h"
#include "c_objectivemanager.h"

#define MAP_SIZE	1024 // all maps must be 1024 x 1024
#define MAX_MAP_NAME_LENGTH 16
#define MAP_PANEL_SIZE XRES(256)
#define MAP_ICON_SCALE XRES(16)
#define DEATH_FADE_TIME 10
#define UPDATE_INTERVAL_TIME 0.1
#define MIN_ALTITUDE_DIST 144

// teams
enum
{
	RED_TEAM = 0,
	BLUE_TEAM,
	NEUTRAL_TEAM,
	ACTIVE_TEAM,
	TEAM_COUNT,
};

// icons
enum
{
	RED_ICON = 0,
	BLUE_ICON,
	RED_DEAD_ICON,
	BLUE_DEAD_ICON,
	SELF_ICON,
	SELF_DEAD_ICON,
};

// icon states
enum
{
	ICON_STATE_ENABLED = 0,
	ICON_STATE_DISABLED,
	ICON_STATE_COUNT,
};

// types of things we might be drawing
enum DrawType
{
	DRAW_TYPE_MAP = 0,
	DRAW_TYPE_PLAYER,
	DRAW_TYPE_OBJECTIVE,
	DRAW_TYPE_NAME,
	DRAW_TYPE_HEALTH,
	DRAW_TYPE_MARKER,
};

/**
* Class declaration for an in-game map implemenation
*
* Keep in mind that this is still abstract.  Basically we should
* be inheriting from this and either a hud element or viewport to
* create the overview map and radar
**/
class CMap : public vgui::Panel, public IMap
{
	DECLARE_CLASS_SIMPLE(CMap, vgui::Panel);

public:	

	// constructor/destructor
	CMap();
	virtual ~CMap();
	static void Term(void);

	// events
	virtual void FireGameEvent(IGameEvent * event);
	virtual bool IsServerSide() { return false; };

	// inherited methods
	virtual void Paint(void);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Reset(void);
	virtual void Update(void);
	virtual bool NeedsUpdate(void);
	virtual bool HasInputElements(void) { return false; }
	//virtual void OnSizeChanged(int newWide, int newTall);

	// accessors
	virtual float GetZoom(void) { return m_fZoom; }
	virtual void SetMap(const char *szLevelName);
	virtual void SetTime(float fTime);
	virtual void SetZoom(float fZoom);
	virtual void SetFollowEntity(bool bState) { m_bFollowEntity = bState; }
	virtual void SetFollowEntity(int iEntIndex) { s_iFollowEntity = iEntIndex; }
	virtual void SetCenter(Vector2D &vecMapPos);
	virtual void SetAngle(float fAngle) { m_fViewAngle = fAngle; }
	virtual void SetPlayerPosition(int iIndex, const Vector &vecPos, const QAngle &qaAngle);
	void SetMapSize(int iWidth, int iHeight);
	int GetMapWide(void) { return m_iMapWidth; }
	int GetMapTall(void) { return m_iMapHeight; }
	const char *GetMapName(void);
	const char *GetMapDirectory(void);
	QAngle GetFollowAngle(void) { return m_qaFollowAngle; }
	void UpdateFollowingInfo(void);
	
	// helpers
	virtual Vector2D WorldToMap(const Vector &vecWorldpos);

	// our child panels are our friends
	friend class CMapForeground;

protected:

	/**
	* Representation of a footprint
	**/
	typedef struct FootStep_s
	{
		Vector vecPos;
		QAngle qaAngle;
	} FootStep_t;

	/**
	* Player representation in the map
	**/
	typedef struct MapPlayer_s
	{
		int	index;		// player's entity index
		int	userid;		// user ID on server
		int	icon;		// players texture icon ID
		Color color;		// players team color
		char name[MAX_PLAYER_NAME_LENGTH];
		int	team;		// N,T,CT
		int	health;		// 0..100, 7 bit
		float fTOD;		// time of death
		Vector position;	// current x pos
		QAngle angle;		// view origin 0..360
		FootStep_t trail;
	} MapPlayer_t;

	// drawing helpers
	virtual void DrawMapTexture(void);
	void DrawMapPlayers(void);
	virtual void DrawPlayerIcon(MapPlayer_t *pPlayer, const FootStep_t &sStep, int iAlpha, bool bIsFollowEntity);
	void DrawPlayerHealth(MapPlayer_t *pPlayer, const FootStep_t &sStep, int iAlpha);
	void DrawPlayerName(MapPlayer_t *pPlayer, const FootStep_t &sStep, int iAlpha);
	void DrawPlayerAltitude(MapPlayer_t *pPlayer, const FootStep_t &sStep, int iAlpha);
	void DrawAltitudeMarker(Vector2D vecPosPanel, float flDist, Color sColor, int iUpYPos);
	virtual void DrawObjectives(const QAngle &qaAngle);
	virtual bool ShouldDrawItem(const Vector2D &vecPosPanel, int iSize);
	virtual int GetMaxShouldDrawSize(void) { return min(GetWide(), GetTall()); }
	virtual void DrawTexturedSquare(const Vector2D &vecPos, const Vector2D &vecOffset, float fAngle, int iSize, int iTexture, int iAlpha = 255);
	virtual void DrawColoredRect(const Vector2D &vecUpperLeft, const Vector2D &vecLowerRight, const Color &sColor);
	virtual void DrawColoredText(const Vector2D &vecPos, const Color &sColor, const vgui::HFont &hPlayerFont, const wchar_t *wszText);
	virtual Vector2D ComputeOffset(DrawType eType);
	virtual void CheckResolution(void);
	virtual int GetTeamRotation(void);
	virtual void TeamChanged(void) { }
	//virtual CMapForeground *GetForeground(void);
	//virtual CMapForeground *CreateForeground(void);

	// icons
	int GetObjectiveIcon(const SObjective_t *pObj, bool bEnabled);
	void ConfigureTeamIcons(void);
	void ConfigureObjectiveIcons(void);

	// state info
	virtual void ResetRound(void);
	virtual void MapReset(void);

	// generic helpers
	bool IsInPanel(Vector2D &vecPos);
	MapPlayer_t* GetPlayerByUserID(int iID);
	Vector2D MapToPanel(const Vector2D &vecMapPos);
	Vector2D WorldToPanel(const Vector &vecWorldPos) { return MapToPanel(WorldToMap(vecWorldPos)); }

	// player and team information	
	static MapPlayer_t s_Players[MAX_PLAYERS];
	static bool s_bPlayersEstablished;
	Color m_TeamColors[MAX_TEAMS];
	int	m_TeamIcons[SELF_DEAD_ICON + 1];
	int m_aObjectiveIcons[TEAM_COUNT][ICON_STATE_COUNT][MAX_OBJECTIVES];
	int m_aRedTeamBaseIcon[ICON_STATE_COUNT][2]; // a = 0
	int m_aBlueTeamBaseIcon[ICON_STATE_COUNT][2]; // a = 0
	int m_aActiveTeamBaseIcon[ICON_STATE_COUNT][2]; // a = 0
	
	// map info accessors
	Vector2D GetMapOrigin(void);
	float GetMapScale(void);
	bool GetRotateMap(void);
	float GetFullZoom(void);
	
	// meta-data
	static KeyValues *s_pMapKeyValues; // keyvalues describing overview parameters
	static int s_iMapTextureID;		// texture id for current overview image

	// positioning and size
	Vector2D m_vecViewOrigin;	// map coordinates that are in the center of the pverview panel
	Vector2D m_vecMapCenter;	// map coordinates that are in the center of the pverview panel
	int m_iMapWidth;			// size of the map in the panel
	int m_iMapHeight;			// size of the map in the panel
	int m_iLastScreenWidth;
	int m_iLastScreenHeight;

	// zoom
	float m_fZoom;		// current zoom n = overview panel shows 1/n^2 of whole map 

	// view information
	static int	s_iFollowEntity; // entity number to follow, 0 = off
	float m_fViewAngle;	// rotation of overview map
	bool m_bFollowEntity;	// if true, map rotates with view angle
	bool m_bRespectTeamRotations; // if true, the map will rotate based on the local player's team
	int m_iLastTeam;
	int m_iTeamRotate;
	QAngle m_qaFollowAngle;

	// updating
	float m_fNextUpdateTime;
	float m_fWorldTime;	// current world time

	// state info
	bool m_bShowNames;
	bool m_bShowHealth;
	bool m_bShowObjectives;
	bool m_bShowPlayers;
	bool m_bShowRelativeAltitude;

	// offset in panel coords for everything
	int m_iXOffset;
	int m_iYOffset;

	vgui::IScheme *m_pScheme;

private:
	// foreground
	CMapForeground *m_pForeground;
};

#endif //
