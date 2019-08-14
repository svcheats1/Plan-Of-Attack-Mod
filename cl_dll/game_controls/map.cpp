#include "cbase.h"
#include <vgui/isurface.h>
#include <vgui/ILocalize.h>
#include <filesystem.h>
#include <keyvalues.h>
#include <convar.h>
#include <mathlib.h>
#include <cl_dll/iviewport.h>
#include <igameresources.h>
#include "map.h"
#include "hud_macros.h"
#include "sdk_gamerules.h"
#include "c_sdk_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
IMap *IMap::s_pMap = NULL;
CMap::MapPlayer_t CMap::s_Players[MAX_PLAYERS];
bool CMap::s_bPlayersEstablished = false;
int CMap::s_iMapTextureID = -1;
KeyValues *CMap::s_pMapKeyValues = NULL;
int CMap::s_iFollowEntity = 0;

static ConVar cl_map_health("cl_map_health", "1", 0, "Show player's health in map overview.\n");
static ConVar cl_map_names ("cl_map_names",  "1", 0, "Show player's names in map overview.\n");
//static ConVar cl_map_tracks("cl_map_tracks", "1", 0, "Show player's tracks in map overview.\n");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

using namespace vgui;

/**
* Construtor
**/
CMap::CMap() 
	: BaseClass(NULL, PANEL_OVERVIEW)
{
	// no foreground yet
	m_pForeground = NULL;

	// setup the panel
	SetBgColor(Color(0,0,0,100));
	SetPaintBackgroundEnabled(true);
	SetVisible(false);

	// don't follow anyone
	m_bFollowEntity = false;

	// look whereever
	m_fZoom = 1.0f;
	m_vecMapCenter = Vector2D(512, 512);
	m_vecViewOrigin = Vector2D(512, 512);
	m_fViewAngle = 0;
	m_bRespectTeamRotations = true;
	m_iLastTeam = m_iTeamRotate = 0;

	// set the scheme before any child control is created
	SetScheme("ClientScheme");

	// reset the map
	Reset();
	
	// clear everything out for the players
	Q_memset(m_TeamColors, 0, sizeof(m_TeamColors));
	if(!s_bPlayersEstablished)
	{
		// clear out the space
		s_bPlayersEstablished = true;
		Q_memset(s_Players, 0, sizeof(s_Players));
	}

	// configure the team icons and the objective icons
	ConfigureTeamIcons();
	ConfigureObjectiveIcons();

	// no extras on the map except objectives
	m_bShowHealth = false;
	m_bShowNames = false;
	m_bShowObjectives = true;
	m_bShowPlayers = true;
	m_bShowRelativeAltitude = true;

    // make sure we're getting everything
	gameeventmanager->AddListener(this, "round_ended", false);
	gameeventmanager->AddListener(this, "game_newmap", false);
	gameeventmanager->AddListener(this, "player_disconnect", false);

	// no offset
	m_iXOffset = m_iYOffset = 0;

	// set the size of the map
	SetMapSize(MAP_PANEL_SIZE, MAP_PANEL_SIZE);
}

/**
* Destructor
**/
CMap::~CMap()
{
	// get rid of the listener
	gameeventmanager->RemoveListener(this);

	//TODO release Textures ? clear lists
}

/**
* Destroys our static members
*
* @return void
**/
void CMap::Term(void)
{
	// kill the key values
	if(s_pMapKeyValues)
		s_pMapKeyValues->deleteThis();
}

/**


/**
* Configures the team icons
*
* @return void
**/
void CMap::ConfigureTeamIcons(void)
{
	// clear the space
	Q_memset(m_TeamIcons, 0, sizeof(m_TeamIcons));

	// setup team red
	m_TeamColors[RED_ICON] = COLOR_RED;
	m_TeamIcons[RED_TEAM] = UTIL_LoadTexture("maps/red_arrow");

	// setup team blue
	m_TeamColors[BLUE_ICON] = COLOR_BLUE;
	m_TeamIcons[BLUE_TEAM] = UTIL_LoadTexture("maps/blue_arrow");

	// red dead, blue dead
	m_TeamIcons[RED_DEAD_ICON] = UTIL_LoadTexture("maps/red_dead_arrow");
	m_TeamIcons[BLUE_DEAD_ICON] = UTIL_LoadTexture("maps/blue_dead_arrow");

	// self
	m_TeamIcons[SELF_ICON] = UTIL_LoadTexture("maps/self_arrow");
	m_TeamIcons[SELF_DEAD_ICON] = UTIL_LoadTexture("maps/self_dead_arrow");
}

/**
* Configure the objective icons
*
* @return void
**/
void CMap::ConfigureObjectiveIcons(void)
{
	char szTexture[256];
	int i, j;

	// clear everything out
	Q_memset(m_aObjectiveIcons, 0, sizeof(m_aObjectiveIcons));

	// load all of them
	for(i = 0; i < TEAM_COUNT; ++i)
	{
		for(j = 0; j < ICON_STATE_COUNT; ++j)
		{
			for(int k = 0; k < MAX_OBJECTIVES; ++k)
			{
				// clear out the texture name
				Q_memset(szTexture, 0, sizeof(szTexture));

				// what color are we using?
				switch(i)
				{
					case RED_TEAM:
						Q_snprintf(szTexture, sizeof(szTexture), "maps/red-%d-%s", 
									k + 1, (j == ICON_STATE_ENABLED ? "enabled" : "disabled"));
						break;
					case BLUE_TEAM:
						Q_snprintf(szTexture, sizeof(szTexture), "maps/blue-%d-%s", 
									k + 1, (j == ICON_STATE_ENABLED ? "enabled" : "disabled"));
						break;
					case ACTIVE_TEAM:
						Q_snprintf(szTexture, sizeof(szTexture), "maps/green-%d-%s", 
									k + 1, (j == ICON_STATE_ENABLED ? "enabled" : "disabled"));
						break;
					case NEUTRAL_TEAM:
						Q_snprintf(szTexture, sizeof(szTexture), "maps/white-%d-%s", 
									k + 1, (j == ICON_STATE_ENABLED ? "enabled" : "disabled"));
						break;
				}

				// load the texture
				m_aObjectiveIcons[i][j][k] = UTIL_LoadTexture(szTexture);
			}
		}
	}

	// bases
	for(i = 0; i < ICON_STATE_COUNT; ++i)
	{
		for(j = 0; j < 2; ++j)
		{
			// active
			Q_memset(szTexture, 0, sizeof(szTexture));
			Q_snprintf(szTexture, sizeof(szTexture), "maps/green-%s-%s", 
						(j == 0 ? "a" : "b"), (i == ICON_STATE_ENABLED ? "enabled" : "disabled"));
			m_aActiveTeamBaseIcon[i][j] = surface()->CreateNewTextureID();
			surface()->DrawSetTextureFile(m_aActiveTeamBaseIcon[i][j], szTexture, true, false);

			// red
			Q_memset(szTexture, 0, sizeof(szTexture));
			Q_snprintf(szTexture, sizeof(szTexture), "maps/red-%s-%s", 
						(j == 0 ? "a" : "b"), (i == ICON_STATE_ENABLED ? "enabled" : "disabled"));
			m_aRedTeamBaseIcon[i][j] = surface()->CreateNewTextureID();
			surface()->DrawSetTextureFile(m_aRedTeamBaseIcon[i][j], szTexture, true, false);

			// blue
			Q_memset(szTexture, 0, sizeof(szTexture));
			Q_snprintf(szTexture, sizeof(szTexture), "maps/blue-%s-%s", 
						(j == 0 ? "a" : "b"), (i == ICON_STATE_ENABLED ? "enabled" : "disabled"));
			m_aBlueTeamBaseIcon[i][j] = surface()->CreateNewTextureID();
			surface()->DrawSetTextureFile(m_aBlueTeamBaseIcon[i][j], szTexture, true, false);
		}
	}
}

/**
* Handles the scheme information
* 
* @param vgui::IScheme *pScheme The scheme we need to use
* @return void
**/
void CMap::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBgColor(Color(0,0,0,100));
	SetPaintBackgroundEnabled(true);
	m_pScheme = pScheme;
}

/**
* Draws the map
*
* @return void
**/
void CMap::Paint(void)
{
	// check the resolution
	CheckResolution();

	// if we need an update go ahead and do one
	if(NeedsUpdate())
		Update();

	UpdateFollowingInfo();

	// draw the map texture
	DrawMapTexture();

	// draws the objectives
	DrawObjectives(GetFollowAngle());

	// draw the players
	DrawMapPlayers();

	BaseClass::Paint();
}

/**
* Updates all the info about the dude we're following
*
* @return void
**/
void CMap::UpdateFollowingInfo(void)
{
	Vector vecPos;

	// are we following anyone?
	m_qaFollowAngle = QAngle(0, 0, 0);
	if(s_iFollowEntity != 0)
	{
		// pull them
		C_BaseEntity *pEnt = ClientEntityList().GetEnt(s_iFollowEntity);
		
		// if we got them, center our view on them
		if(pEnt && pEnt->IsAlive())
		{
			// pull their positioning info
			vecPos = pEnt->EyePosition();

			// if we're following the local player we can use their eye angles directly
			// otherwise, we need to use the game resources information that isn't updated
			// as often
			if(CSDKPlayer::GetLocalPlayer() && CSDKPlayer::GetLocalPlayer()->entindex() == s_iFollowEntity && pEnt->entindex() == s_iFollowEntity)
				m_qaFollowAngle = pEnt->EyeAngles();
			else
			{
				// ask the player
				CSDKPlayer *pPlayer = ToSDKPlayer(pEnt);
				m_qaFollowAngle = pPlayer->GetNetworkedEyeAngles();
			}

			// set their current information
			if(s_iFollowEntity <= MAX_PLAYERS)
				SetPlayerPosition(s_iFollowEntity - 1, vecPos, m_qaFollowAngle);

			// center on them and turn when they turn
			SetCenter(WorldToMap(vecPos));
			SetAngle(m_qaFollowAngle[YAW]);
		}
		// stop trying to follow them
		else if(!pEnt)
			s_iFollowEntity = 0;
	}
}

/**
* Checks the resolution to make sure nothing has changed
*
* @return void
**/
void CMap::CheckResolution(void)
{
	// did the size of the map change?
	if(m_iLastScreenWidth != ScreenWidth() || m_iLastScreenHeight != ScreenHeight())
	{
		// set the dimensions
		m_iLastScreenWidth = ScreenWidth();
		m_iLastScreenHeight = ScreenHeight();

		// reset the map
		MapReset();
	}
}

/**
* Sets the position of one of the players
*
* @param int iIndex The index of the player we're setting for
* @param const Vector &vecPosition The players position
* @param const QAngle &qaAngle The angle they're looking?
* @return void
**/
void CMap::SetPlayerPosition(int iIndex, const Vector &vecPosition, const QAngle &qaAngle)
{
	MapPlayer_t *p = &s_Players[iIndex];

	p->angle = qaAngle;
	p->position = vecPosition;
}

/**
* Determines if we need to update the map
*
* @return bool
**/
bool CMap::NeedsUpdate(void)
{
	return m_fNextUpdateTime < gpGlobals->curtime;
}

/**
* Updates the info on the map
*
* @return void
**/
void CMap::Update(void)
{
	Vector vecPos;
	QAngle qaAngle;
	IGameResources *pGR;
	CSDKPlayer *pPlayer;

	// set the times
	m_fWorldTime = gpGlobals->curtime;
	m_fNextUpdateTime = gpGlobals->curtime + UPDATE_INTERVAL_TIME;

	// pull game resources
	pGR = GameResources();

	// flip through our players
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		// connected?
		if(pGR && pGR->IsConnected(i))
		{
			// get the player
			pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex(i);

			// make sure we got them
			if (!pPlayer)
				continue;

			// get their position and eye angles
			vecPos = pPlayer->GetNetworkedEyePosition();
			qaAngle = pPlayer->GetNetworkedEyeAngles();

			// skip if they're still at the origin
			if(vecPos.x == 0.0f && vecPos.y == 0.0f)
				continue;

			// they're alive, use actual health value
			if (pGR->IsAlive(i))
				s_Players[i - 1].health = pGR->GetHealth(i);
			// they're dead, use 0
			else
				s_Players[i - 1].health = 0;

			// store their index
			s_Players[i - 1].index = pPlayer->entindex();

			// set their color, health, icon, team
			s_Players[i - 1].color = (pGR->GetTeam(i) == TEAM_A
										? m_TeamColors[BLUE_TEAM]
										: m_TeamColors[RED_TEAM]);


			s_Players[i - 1].team = pGR->GetTeam(i);

			// did they die? set the time of death
			if(s_Players[i - 1].fTOD == 0 && s_Players[i - 1].health <= 0)
				s_Players[i - 1].fTOD = gpGlobals->curtime;
			else if((s_Players[i - 1].fTOD != 0 && s_Players[i - 1].health > 0) || 
				s_Players[i - 1].fTOD > gpGlobals->curtime)
			{
				s_Players[i - 1].fTOD = 0;
			}

			// figure out their icon
			if(pPlayer == CBasePlayer::GetLocalPlayer())
				s_Players[i - 1].icon = m_TeamIcons[(s_Players[i - 1].health <= 0 ? SELF_DEAD_ICON : SELF_ICON)];
			else if(pGR->GetTeam(i) == TEAM_A)
				s_Players[i - 1].icon = m_TeamIcons[(s_Players[i - 1].health <= 0 ? BLUE_DEAD_ICON : BLUE_ICON)];
			else
				s_Players[i - 1].icon = m_TeamIcons[(s_Players[i - 1].health <= 0 ? RED_DEAD_ICON : RED_ICON)];

			// set their name
			Q_snprintf(s_Players[i - 1].name, MAX_PLAYER_NAME_LENGTH, "%s", pGR->GetPlayerName(i));

			// pull their userid
			s_Players[i - 1].userid = pGR->GetUserID(i);

			// set their position only if they're alive
			if (pGR->IsAlive(i))
				SetPlayerPosition(i - 1, vecPos, qaAngle);
		}
		else if(pGR && !pGR->IsConnected(i))
		{
			// clear their trail
			memset((void *)&s_Players[i - 1], 0, sizeof(MapPlayer_t));
		}

	}
}

/**
* Resets the map information
*
* @return void
**/
void CMap::Reset(void)
{
	m_fNextUpdateTime = 0;
}

/**
* Finds the player in our player info list
* 
* @param int iID Id of the player to get
* @return CMap::MapPlayer_t *
**/
CMap::MapPlayer_t* CMap::GetPlayerByUserID(int iID)
{
	for (int i=0; i < MAX_PLAYERS; i++)
	{
		MapPlayer_t *player = &s_Players[i];

		if(player->userid == iID)
			return player;
	}

	return NULL;
}

/**
* Determines if the given coordinate is within the map boundaries
*
* @param Vector2D &vecPos The position to check
* @return bool
**/
bool CMap::IsInPanel(Vector2D &vecPos)
{
	int x,y,w,t;

	vgui::Panel::GetBounds(x,y,w,t);

	return (vecPos.x >= 0 && vecPos.x < w && vecPos.y >= 0 && vecPos.y < t);
}

/**
* Draws the objectives onto the map
*
* @param QAngle qaAngle The angle at which the map is rotated
* @return void
**/
void CMap::DrawObjectives(const QAngle &qaAngle)
{
	SObjective_t *pObj;
	Vector2D vecObjPos;
	float fViewAngle = qaAngle[YAW];
	CSDKPlayer *pPlayer = CSDKPlayer::GetLocalSDKPlayer();

	// are we drawing objectives?
	if(!m_bShowObjectives)
		return;

	// flatten if we're following someone
	fViewAngle = 180.0;

	// are we rotating?
	if(!m_bFollowEntity)
		fViewAngle = 180.0f;

	// start drawing them
	pObj = GET_OBJ_MGR()->GetNextObjective(NULL);
	for(; pObj != NULL; pObj = GET_OBJ_MGR()->GetNextObjective(pObj))
	{
		// convert to panel coords
		vecObjPos = WorldToPanel(pObj->vecPos);

		// skip it if we don't need to draw it
		if(!ShouldDrawItem(vecObjPos, MAP_ICON_SCALE))
			continue;

		// draw the square
		DrawTexturedSquare(vecObjPos, ComputeOffset(DRAW_TYPE_OBJECTIVE), fViewAngle, MAP_ICON_SCALE, GetObjectiveIcon(pObj, false));

		if (m_bShowRelativeAltitude && pPlayer) {
			vecObjPos.x -= XRES(12);
			float flDist = pObj->vecPos.z - pPlayer->EyePosition().z;
            Color sColor;
			if (pObj->bActive)
				sColor.SetColor(255, 176, 0, 255);
			else {
				switch(pObj->iOwner) {
					case TEAM_A:
						sColor.SetColor(0, 0, 255, 255);
						break;
					case TEAM_B:
						sColor.SetColor(255, 0, 0, 255);
						break;
					default:
					case -1:
						sColor.SetColor(255, 255, 255, 255);
						break;
				}
			}

			DrawAltitudeMarker(vecObjPos, flDist, sColor, 10);
		}
	}
}

/**
* Determines the icon to use for the specified objectives
*
* @param const SObjective_t *pObj The objective to find an icon for
* @param bool bEnabled True if we want the enabled version of the icon
* @return int ID of the icon for the objective
**/
int CMap::GetObjectiveIcon(const SObjective_t *pObj, bool bEnabled)
{
	int iIndex;

	// is it a base?
	if(pObj->bIsBase)
	{
		// is it active?
		if(pObj->bActive)
			return m_aActiveTeamBaseIcon[(bEnabled ? ICON_STATE_ENABLED : ICON_STATE_DISABLED)][pObj->iOwner - 1];
		else if(pObj->iOwner == TEAM_A)
			return m_aBlueTeamBaseIcon[(bEnabled ? ICON_STATE_ENABLED : ICON_STATE_DISABLED)][pObj->iOwner - 1];
		else
			return m_aRedTeamBaseIcon[(bEnabled ? ICON_STATE_ENABLED : ICON_STATE_DISABLED)][pObj->iOwner - 1];
	}
	else
	{
		// is it active?
		if(pObj->bActive)
			iIndex = ACTIVE_TEAM;
		// is there an owner?
		else if(pObj->iOwner == -1)
			iIndex = NEUTRAL_TEAM;
		// friend or foe?
		else
			iIndex = (pObj->iOwner == TEAM_A ? BLUE_TEAM : RED_TEAM);

		// send back the proper id
		return m_aObjectiveIcons[iIndex][(bEnabled ? ICON_STATE_ENABLED : ICON_STATE_DISABLED)][pObj->iID];
	}
}

/**
* Determines if we should draw something at the specified position
*
* @param const Vector2D &vecPosPanel The position in the panel that we want to draw the item
* @param int iSize The size we plan on drawing the item
* @return bool True if the item should be drawn
**/
bool CMap::ShouldDrawItem(const Vector2D &vecPosPanel, int iSize)
{
	// don't draw anything in the dead center
	if(vecPosPanel.x == 0 && vecPosPanel.y == 0)
		return false;

	return true;
}

/**
* Draws each of the players on top of the map
*
* @return void
**/
void CMap::DrawMapPlayers(void)
{
	MapPlayer_t *pPlayer;
	Vector2D vecPosPanel;
	Vector vecPos, vecDist;
	float fDeathAlpha;
	CSDKGameRules *pGameRules;

	// don't bother if we're not showing players
	if(!m_bShowPlayers)
		return;

	CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
	IGameResources *pGR = GameResources();

	if (!pLocalPlayer || !pGR)
		return;

	// i should show my enemies if...
	pGameRules = (CSDKGameRules *)g_pGameRules;
	bool bShouldShowEnemies = 
		((
			// i'm not alive and..
			!pLocalPlayer->IsAlive()
			// and it's not zero
			&& pGameRules->GetForceCam() == 0 
		)
		// OR if its the first round
		|| pGR->IsFirstRound())
		&& pGameRules->CanShowEnemiesOnMap(ToSDKPlayer(pLocalPlayer));

	// flip through our players
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		// make sure they're connected
		if ( !pGR->IsConnected(i + 1) )			
			continue;

		// pull the player
		pPlayer = &s_Players[i];
		
		// make sure they're not spectators
		if(pPlayer->team != TEAM_A && pPlayer->team != TEAM_B)
			continue;

		// if I shouldn't show my enemies, then don't show my enemies
		if (!bShouldShowEnemies && pPlayer->team != pLocalPlayer->GetTeamNumber())
			continue;

		// set the first trail spot to their current position
		pPlayer->trail.vecPos = pPlayer->position;
		pPlayer->trail.qaAngle = pPlayer->angle;

		// figure out where in the panel we need to draw them
		vecPosPanel = WorldToPanel(pPlayer->trail.vecPos);

		// make sure the player is on the panel
		if(!IsInPanel(vecPosPanel) || !ShouldDrawItem(vecPosPanel, MAP_ICON_SCALE))
			continue;

		// figure out the death alpha
		if(!CBasePlayer::GetLocalPlayer() || pPlayer->index == CBasePlayer::GetLocalPlayer()->entindex() || pPlayer->fTOD == 0)
			fDeathAlpha = 1;
		else
			fDeathAlpha = clamp(1.0 - ((gpGlobals->curtime - pPlayer->fTOD) / DEATH_FADE_TIME), 0, 1);

		// don't draw any trails that are too far away
		vecDist = pPlayer->trail.vecPos - pPlayer->trail.vecPos;
		if(vecDist.LengthSqr() < (128 * 128))
			// draw the players icon
			DrawPlayerIcon(pPlayer, pPlayer->trail, 255 * fDeathAlpha, pPlayer->index == s_iFollowEntity);

		// are we displaying player health?
		if(m_bShowHealth)
			DrawPlayerHealth(pPlayer, pPlayer->trail, fDeathAlpha * 255.0);

		// finally, draw their name
		if(m_bShowNames)
			DrawPlayerName(pPlayer, pPlayer->trail, fDeathAlpha * 255.0);

		if(m_bShowRelativeAltitude)
			DrawPlayerAltitude(pPlayer, pPlayer->trail, fDeathAlpha * 255.0);
	}
}

void CMap::DrawPlayerAltitude(MapPlayer_t *pPlayer, const FootStep_t &sStep, int iAlpha)
{
	Vector vecPos;
	float flDist;
	Color sColor;
	Vector2D vecPosPanel;

	CSDKPlayer *pLocalPlayer = CSDKPlayer::GetLocalSDKPlayer();

	if (!pPlayer || !pLocalPlayer)
		return;

	// get where we are
	vecPos = pLocalPlayer->EyePosition();

	// what's our altitude difference?
	flDist = (pPlayer->position.z - vecPos.z);

	// time to start doing the real drawing

	// get the panel pos
	vecPosPanel = WorldToPanel(sStep.vecPos);
	// move it left
	vecPosPanel.x -= XRES(7);

	// figure out the color
	sColor.SetColor(pPlayer->color.r(), pPlayer->color.g(), pPlayer->color.b(), iAlpha);

	DrawAltitudeMarker(vecPosPanel, flDist, sColor, 10);
}

void CMap::DrawAltitudeMarker(Vector2D vecPosPanel, float flDist, Color sColor, int iUpYPos)
{
	const char* szText;
	wchar_t wszText[2];
	int iTall, iWide;
	vgui::HFont hPlayerFont;

	// if its not big enough, bail
	if (abs(flDist) < MIN_ALTITUDE_DIST)
		return;

    // draw the "they're above me" icon
	if (flDist > 0) {
		vecPosPanel.y -= YRES(iUpYPos);
		szText = "-";
	}
	// draw the "they're below me" icon
	else {
		//vecPosPanel.y += YRES(0);
		szText = "+";
	}

	// convert to wchars
	localize()->ConvertANSIToUnicode(szText, wszText, sizeof(wszText));

	// pull the font
	hPlayerFont = m_pScheme->GetFont("MapIcons", true);

	// figure out how big to  draw
	surface()->GetTextSize(hPlayerFont, wszText, iWide, iTall);

	// don't draw if we're outside the map
	if(!ShouldDrawItem(vecPosPanel, iWide))
		return;

	// center it
	vecPosPanel.x -= (iWide / 2);

	// draw text
	DrawColoredText(vecPosPanel, sColor, hPlayerFont, wszText);
	/*surface()->DrawSetTextColor(sColor);
	surface()->DrawSetTextPos(vecPosPanel.x, vecPosPanel.y);
	surface()->DrawPrintText(wszText, wcslen(wszText));*/
}

/**
* Draws the name of the specified player
*
* @param MapPlayer_t *pPlayer The player whose name to draw
* @param const FootStep_t &sStep The footstep at which to draw the name
* @param int iAlpha The alpha value to use
* @return void
**/
void CMap::DrawPlayerName(MapPlayer_t *pPlayer, const FootStep_t &sStep, int iAlpha)
{
	wchar_t wszName[ 64 ];
	int iWide, iTall;
	vgui::HFont hPlayerFont;
	IScheme *pScheme;
	Vector2D vecPosPanel;
	float fViewAngle;
	Color sColor;

	// rotate back 90 degrees and ignore z
	fViewAngle = sStep.qaAngle[YAW] - 90.0;

	// are we turning the map?
	if(!m_bFollowEntity)
		fViewAngle = 0.0f;

	// move the name under the icon
	vecPosPanel = WorldToPanel(sStep.vecPos);
	vecPosPanel.y += YRES(10.0);

	// pull the font and set the color
	pScheme = vgui::scheme()->GetIScheme(GetScheme());
	hPlayerFont = pScheme->GetFont("Default");
	sColor = Color(0, 0, 0, iAlpha);

	// localize the name...bots? and then truncate
	localize()->ConvertANSIToUnicode(pPlayer->name, wszName, sizeof(wszName));
	wszName[MAX_MAP_NAME_LENGTH] = 0;

	// figure out how big to  draw
	surface()->GetTextSize(hPlayerFont, wszName, iWide, iTall);

	// don't draw if we're outside the map
	if(!ShouldDrawItem(vecPosPanel, iWide))
		return;

	// center it
	vecPosPanel.x -= (iWide / 2);

	// add the offset
	vecPosPanel += ComputeOffset(DRAW_TYPE_HEALTH);

	// draw shadow text
	DrawColoredText(vecPosPanel, sColor, hPlayerFont, wszName);
	vecPosPanel.x -= 1;
	/*surface()->DrawSetTextColor(0, 0, 0, iAlpha);
	surface()->DrawSetTextPos(vecPosPanel.x + 1, vecPosPanel.y);
	surface()->DrawPrintText(wszName, wcslen(wszName));*/

	// figure out the color
	if (CBasePlayer::GetLocalPlayer() && pPlayer->index == CBasePlayer::GetLocalPlayer()->entindex())
		sColor.SetColor(255, 176, 0, 255);
	else
		sColor.SetColor(pPlayer->color.r(), pPlayer->color.g(), pPlayer->color.b(), iAlpha);

	// draw text
	DrawColoredText(vecPosPanel, sColor, hPlayerFont, wszName);
	/*surface()->DrawSetTextColor(sColor);
	surface()->DrawSetTextPos(vecPosPanel.x, vecPosPanel.y);
	surface()->DrawPrintText(wszName, wcslen(wszName));*/
}

/**
* Draws the health of the specified player
*
* @param MapPlayer_t *pPlayer The player whose health to draw
* @param const FootStep_t &sStep The footstep at which to draw the bar
* @param int iAlpha The alpha value to use
* @return void
**/
void CMap::DrawPlayerHealth(MapPlayer_t *pPlayer, const FootStep_t &sStep, int iAlpha)
{
	Vector2D vecPos1, vecPos2;
	Color sColor;

	// upper left
	vecPos1 = WorldToPanel(sStep.vecPos);
	vecPos1.x += -XRES(3.0) + (MAP_ICON_SCALE / 2.0);
	vecPos1.y -= ((MAP_ICON_SCALE / 3.0) + YRES(3.0));

	// lower right
	vecPos2 = WorldToPanel(sStep.vecPos);
	vecPos2.x += XRES(3.0) + (MAP_ICON_SCALE / 2.0);
	vecPos2.y += (MAP_ICON_SCALE / 3.0) + YRES(3.0);

	// draw outer box
	/*surface()->DrawSetColor(Color(0, 0, 0, iAlpha * .8));
	surface()->DrawFilledRect(vecPos1.x, 
								vecPos1.y, 
								vecPos2.x, 
								vecPos2.y);*/
	sColor = Color(0, 0, 0, iAlpha * .8);
	DrawColoredRect(vecPos1, vecPos2, sColor);

	// upper left
	vecPos1 = WorldToPanel(sStep.vecPos);
	vecPos1.x += -XRES(1.0) + (MAP_ICON_SCALE / 2.0);
	vecPos1.y += (MAP_ICON_SCALE / 3.0) - (2.0 * (MAP_ICON_SCALE / 3.0) * ((float)pPlayer->health / 100.0));

	// lower right
	vecPos2 = WorldToPanel(sStep.vecPos);
	vecPos2.x += XRES(1.0) + (MAP_ICON_SCALE / 2.0);
	vecPos2.y += (MAP_ICON_SCALE / 3.0);

	// add the offset
	vecPos1 += ComputeOffset(DRAW_TYPE_HEALTH);
	vecPos2 += ComputeOffset(DRAW_TYPE_HEALTH);

	// draw inner box
	sColor.SetColor(pPlayer->color.r(), pPlayer->color.g(), pPlayer->color.b(), iAlpha);
	/*surface()->DrawSetColor(sColor);
	surface()->DrawFilledRect(vecPos1.x, 
								vecPos1.y,
								vecPos2.x,
								vecPos2.y);*/
	DrawColoredRect(vecPos1, vecPos2, sColor);
}

/**
* Draws the icon for the specified player
* 
* @param MapPlayer_t *pPlayer The player whose icon to draw
* @param const FootStep_t &sStep Position to draw the icon
* @param int iAlpha The alpha value to use when drawing
* @param bool bIsFollowEntity True if this player is the local one
* @return void
**/
void CMap::DrawPlayerIcon(MapPlayer_t *pPlayer, const FootStep_t &sStep, int iAlpha, bool bIsFollowedEntity)
{
	float fViewAngle;
	Vector2D vecPlayerPos;

	// convert to panel coords and figure out the angle
	vecPlayerPos = WorldToPanel(sStep.vecPos);
	fViewAngle = sStep.qaAngle[YAW];

	// following the player?
	if(m_bFollowEntity)
	{
		if(GetRotateMap())
			fViewAngle = 180.0 + (bIsFollowedEntity ? 0 : s_Players[s_iFollowEntity - 1].angle[YAW] - fViewAngle);
		else
			fViewAngle = 180.0 + (bIsFollowedEntity ? 0 : s_Players[s_iFollowEntity - 1].angle[YAW] - fViewAngle);
	}
	else
	{
		if(!GetRotateMap())
			fViewAngle = (fViewAngle + 90.0f) * -1;
		else
			fViewAngle *= -1;
	}

	// if we're respecting team rotations we need to take that into account
	if(m_bRespectTeamRotations)
		fViewAngle += GetTeamRotation();

	// draw the square
	DrawTexturedSquare(vecPlayerPos, ComputeOffset(DRAW_TYPE_PLAYER), fViewAngle, MAP_ICON_SCALE, pPlayer->icon, iAlpha);
}

/**
* Draws a textured, centered square over the given position using the 
* specified size and texture
*
* @param const Vector2D &vecPos The position in the panel at which to draw the square
* @param const Vector2D &vecPos The offset to add to the position after rotating the square
* @param float fAngle The angle at which to draw
* @param int iSize The length of one side of the square
* @param int iTexture The texture to use
* @param int iAlpha The alpha value to use
* @return void
**/
void CMap::DrawTexturedSquare(const Vector2D &vecPos, const Vector2D &vecOffset, float fAngle, int iSize, int iTexture, int iAlpha /* = 255 */)
{
	float fX, fY;
	Vertex_t aPoints[4];

	// figure out where each of the corners goes
	for(int i = 0; i < 4; ++i)
	{
		SinCos((fAngle / 180 * M_PI) + ((1 + (i * 2)) * M_PI / 4), &fY, &fX);

		// compute the new position
		aPoints[i].m_Position.x = vecPos.x + (fX * (float)(iSize / 2) * SQRT_TWO);
		aPoints[i].m_Position.y = vecPos.y + (fY * (float)(iSize / 2) * SQRT_TWO);
		aPoints[i].m_TexCoord.x = i > 0 && i < 3;
		aPoints[i].m_TexCoord.y = i > 1;

		// add the offset
		aPoints[i].m_Position += vecOffset;
	}

	// draw the icon
	surface()->DrawSetColor(0, 0, 0, iAlpha);
	surface()->DrawSetTexture(iTexture);
	surface()->DrawTexturedPolygon(4, aPoints);
}

/**
* Draws a colored rectangle
*
* @param const Vector2D &vecUpperLeft Upper left coordinate
* @param const Vector2D &vecLowerRight Lower right coordinate
* @param const Color &sColor The color to use
* @return void
**/
void CMap::DrawColoredRect(const Vector2D &vecUpperLeft, const Vector2D &vecLowerRight, const Color &sColor)
{
	// draw it
	surface()->DrawSetColor(sColor);
	surface()->DrawFilledRect(vecUpperLeft.x, 
								vecUpperLeft.y,
								vecLowerRight.x,
								vecLowerRight.y);
}

/**
* Draws a colored text element
*
* @param const Vector2D &vecPos The position at which to draw
* @param const Color &sColor The color to use
* @param const HFont &hPlayerFont The font to use
* @param const wchar_t *wszText The text to draw
* @return void
**/
void CMap::DrawColoredText(const Vector2D &vecPos, const Color &sColor, const HFont &hPlayerFont, const wchar_t *wszText)
{
	// draw text
	surface()->DrawSetTextFont(hPlayerFont);
	surface()->DrawSetTextColor(sColor);
	surface()->DrawSetTextPos(vecPos.x, vecPos.y);
	surface()->DrawPrintText(wszText, wcslen(wszText));
}

/**
* Draws the map texture to the screen
*
* @return void
**/
void CMap::DrawMapTexture(void)
{
	// make sure we have a texture
	if(s_iMapTextureID < 0)
		return;
	
	// figure out where to draw based on zoom etc
	Vertex_t points[4] =
	{
		Vertex_t(MapToPanel(Vector2D(0, 0)), Vector2D(0, 0)),
		Vertex_t(MapToPanel(Vector2D(MAP_SIZE - 1, 0)), Vector2D(1, 0)),
		Vertex_t(MapToPanel(Vector2D(MAP_SIZE - 1, MAP_SIZE - 1)), Vector2D(1, 1)),
		Vertex_t(MapToPanel(Vector2D(0, MAP_SIZE - 1)), Vector2D(0, 1))
	};

	// add the offset
	for(int i = 0; i < 4; ++i)
		points[i].m_Position += ComputeOffset(DRAW_TYPE_MAP);

	// draw the texture
	surface()->DrawSetColor(0, 0, 0, 255);
	surface()->DrawSetTexture(s_iMapTextureID);
	surface()->DrawTexturedPolygon(4, points);

	// @TEMP - this is useful for debugging...
	// draw red center point
	//surface()->DrawSetColor(255, 0, 0, 255);
	//Vector2D center = MapToPanel(m_vecViewOrigin);
	//surface()->DrawFilledRect(center.x - 2, center.y - 2, center.x + 2, center.y + 2);
}

/**
* Converts a position in the map to a position in the parent panel
*
* @param const Vector2D &vecMapPos Position to convert
* @return Vector2D
**/
Vector2D CMap::MapToPanel(const Vector2D &vecMapPos)
{
	Vector2D panelpos;
	float viewAngle;
	Vector offset;

	// figure out the view angle
	 viewAngle = m_fViewAngle - 90.0f;

	// determine the difference between map and world
	offset.x = vecMapPos.x - m_vecMapCenter.x;
	offset.y = vecMapPos.y - m_vecMapCenter.y;
	offset.z = 0;

	// are we turning the map?
	if(!m_bFollowEntity)
	{
		if(GetRotateMap())
			viewAngle = 90.0f;
		else
			viewAngle = 0.0f;
	}

	// if we're respecting team rotations we need to take that into account
	if(m_bRespectTeamRotations)
		viewAngle += GetTeamRotation();

	// rotate around z
	VectorYawRotate(offset, viewAngle, offset);

	// scale based on the zoom
	offset.x *= m_fZoom / MAP_SIZE;
	offset.y *= m_fZoom / MAP_SIZE;

	// offset the final position from the center
	panelpos.x = (m_iMapWidth * 0.5f) + (m_iMapWidth * offset.x);
	panelpos.y = (m_iMapHeight * 0.5f) + (m_iMapHeight * offset.y);

	return panelpos;
}

/**
* Sends back the current rotation for the team we are on
*
* @return int
**/
int CMap::GetTeamRotation(void)
{
	if (!CSDKPlayer::GetLocalSDKPlayer())
		return 0;

	// if we're not on a real team or we were on a different team
	// figure out the new rotation
	if((m_iLastTeam != TEAM_A && m_iLastTeam != TEAM_B) ||
		CSDKPlayer::GetLocalSDKPlayer()->GetTeamNumber() != m_iLastTeam)
	{
		// pull the amount
		if(CSDKPlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_A)
		{
			// set the info
			m_iTeamRotate = s_pMapKeyValues->GetInt("team_a_rotate");
			m_iLastTeam = TEAM_A;
		}
		else if(CSDKPlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_B)
		{
			// set the info
			m_iTeamRotate = s_pMapKeyValues->GetInt("team_b_rotate");
			m_iLastTeam = TEAM_B;
		}
		else
			m_iLastTeam = m_iTeamRotate = 0;

		// the team changed
		TeamChanged();
	}

	return m_iTeamRotate;
}

/**
* Sets the time so we know when to draw stuff
*
* @param float fTime The current time
* @return void
**/
void CMap::SetTime(float fTime)
{
	m_fWorldTime = fTime;
}

/**
* Sets the map.  Loads the appropriate textures and metadata
* 
* @param const char *szLevelName Name of the level we're loading
* @return void
**/
void CMap::SetMap(const char * szLevelName)
{
	char tempfile[MAX_PATH];

	// load new KeyValues
	if((s_pMapKeyValues && Q_strcmp(szLevelName, GetMapName()) == 0) || !Q_strlen(szLevelName))
		return;	// map didn't change

	// clear the values out if we've changed levels
	if(s_pMapKeyValues)
		s_pMapKeyValues->deleteThis();

	// create the new ones
	// note that the setname of the KeyValues seems to be temporary
	// and become the name of the file from which it loads its data
	// that is why it is necessary to set the "levelname" in the next few lines
	s_pMapKeyValues = new KeyValues(szLevelName);

	// set the level name
	s_pMapKeyValues->SetString("levelname", szLevelName);

	// where are we loading from?
	Q_snprintf(tempfile, sizeof(tempfile), "maps/meta/%s.txt", szLevelName);
	
	// pull the values
	if(!s_pMapKeyValues->LoadFromFile(vgui::filesystem(), tempfile, "GAME"))
	{
		DevMsg(1, "CMap::OnNewLevel: couldn't load file %s.\n", tempfile);
		s_iMapTextureID = -1;
		return;
	}

	// create the texture
	Q_snprintf(tempfile, sizeof(tempfile), "%s/%s", 
				s_pMapKeyValues->GetString("directory"), s_pMapKeyValues->GetString("material"));
	s_iMapTextureID = UTIL_LoadTexture(tempfile);

	// reset our team rotate information
	m_iLastTeam = m_iTeamRotate = 0;
}

/**
* Resets the map at the end of the round
*
* @return void
**/
void CMap::ResetRound(void)
{
	for (int i=0; i<MAX_PLAYERS; i++)
	{
		MapPlayer_t *p = &s_Players[i];
		Q_memset((void *)&p->trail, 0, sizeof(p->trail));
	}

	// reset the update time
	m_fNextUpdateTime = gpGlobals->curtime;
}

/**
* Handles game events
*
* @param KeyValues *pEvent The event we need to handle
* @return void
**/
void CMap::FireGameEvent(IGameEvent *pEvent)
{
	const char * type = pEvent->GetName();

	if(Q_strcmp(type, "game_newmap") == 0)
	{
		// set the map
		SetMap(pEvent->GetString("mapname"));
		ResetRound();
	}
	else if(Q_strcmp(type, "round_ended") == 0)
	{
		// reset for the start of a round
		ResetRound();
	}
	else if(Q_strcmp(type, "player_disconnect") == 0)
	{
		// clear their trail
		int userid = pEvent->GetInt("userid");
		IGameResources *gr = GameResources();
		if (gr) {
			int i = gr->FindIndexByUserID(userid);

			if (i > 0 && i <= MAX_PLAYERS)
				memset((void *)&s_Players[i - 1], 0, sizeof(MapPlayer_t));
		}
	}
}

/**
* Sets the center of the map texture for converting from world positions
*
* @param const Vector2D &vecMapPos Position to use as teh center
* @return void
**/
void CMap::SetCenter(Vector2D &vecMapPos)
{
	int width, height;

	// set the center info
	m_vecViewOrigin = vecMapPos;
	m_vecMapCenter = vecMapPos;

	// how much room do we have?
	width = MAP_SIZE / (m_fZoom * 2);
	height = MAP_SIZE / (m_fZoom * 2);

	// make sure we're within range
	/*if(m_vecMapCenter.x < width)
		m_vecMapCenter.x = width;
	if(m_vecMapCenter.x > (MAP_SIZE - width))
		m_vecMapCenter.x = (MAP_SIZE - width);
	if(m_vecMapCenter.y < height)
		m_vecMapCenter.y = height;
	if(m_vecMapCenter.y > (MAP_SIZE - height))
		m_vecMapCenter.y = (MAP_SIZE - height);*/

	// center if in full map mode
	if(!m_bFollowEntity)
	{
		m_vecMapCenter.x = MAP_SIZE / 2;
		m_vecMapCenter.y = MAP_SIZE / 2;
	}
}

/**
* Converts world coordinates to map coordinates
*
* @param const Vector &vecWorldPos The world position to convert
* @return Vector2D
**/
Vector2D CMap::WorldToMap(const Vector &vecWorldpos)
{
	Vector2D offset(vecWorldpos.x, vecWorldpos.y);

	// subtract off the map origin
	offset -= GetMapOrigin();

	offset.x /= GetMapScale();
	offset.y /= -GetMapScale();

	return offset;
}

/**
* Sets the zoom level to use when displaying the map
* 
* @param float fZoom The zoom level
* @return void
**/
void CMap::SetZoom(float fZoom)
{
	m_fZoom = fZoom;

	if(m_fZoom == -1.0f)
		m_fZoom = GetFullZoom();
	else if(m_fZoom < 0.5f)
		m_fZoom = 0.5f;
	else if(m_fZoom > 5.0f)
		m_fZoom = 5.0f;
}

/**
* Sets the size of the map in the panel
*
* @param int iWidth
* @param int iHeight
* @return void
**/
void CMap::SetMapSize(int iWidth, int iHeight)
{
	m_iMapWidth = iWidth;
	m_iMapHeight = iHeight;
}

/**
* Applies the offset to the vector
*
* @param DrawType eType The type of thing we want the offset for
* @return Vector2D The offset to use
**/
Vector2D CMap::ComputeOffset(DrawType eType)
{
	return Vector2D(m_iXOffset, m_iYOffset);
}

/**
* Resets the map so our sizing is all set
*
* @return void
**/
void CMap::MapReset(void)
{
	// set the map
	if(s_pMapKeyValues)
		SetMap(s_pMapKeyValues->GetString("levelname"));

	// set the size of the map
	SetMapSize(MAP_PANEL_SIZE, MAP_PANEL_SIZE);

	// reset our update time
	m_fNextUpdateTime = gpGlobals->curtime;
}

/**
* Returns the origin of the map
*
* @return Vector2D
**/
Vector2D CMap::GetMapOrigin(void)
{
	// do we have the key values?
	if(s_pMapKeyValues)
		return Vector2D(s_pMapKeyValues->GetInt("pos_x"), s_pMapKeyValues->GetInt("pos_y"));

	return Vector2D(0, 0);
}

/**
* Returns the scale of the map
*
* @return float
**/
float CMap::GetMapScale(void)
{
	// do we have the key values?
	if(s_pMapKeyValues)
		return s_pMapKeyValues->GetFloat("scale", 1.0);

	return 1.0;
}

/**
* Returns whether or not the map needs to be rotated
*
* @return bool
**/
bool CMap::GetRotateMap(void)
{
	// do we have the key values?
	if(s_pMapKeyValues)
		return s_pMapKeyValues->GetInt("rotate") != 0;

	return false;
}

/**
* Returns the full zoom level of the map
*
* @return float
**/
float CMap::GetFullZoom(void)
{
	// do we have the key values?
	if(s_pMapKeyValues)
		return s_pMapKeyValues->GetFloat("zoom", 1.0);

	return 1.0;
}

/**
* Sends back the name of the map
* 
* @return const char *
**/
const char *CMap::GetMapName(void)
{
	// do we have the key values?
	if(s_pMapKeyValues)
		return s_pMapKeyValues->GetString("levelname");

	return "";
}

/**
* Sends back the map directory
*
* @return const char *
**/
const char *CMap::GetMapDirectory(void)
{
	// do we have the key values/
	if(s_pMapKeyValues)
		return s_pMapKeyValues->GetString("directory");

	return "";
}

/**
* Determines wht to do if the size of the panel changes
*
* @param int newWide
* @param int newTall
* @return void
**/
/*
void CMap::OnSizeChanged(int newWide, int newTall)
{
	// resize the foreground
	GetForeground()->SetSize(newWide, newTall);

	// jump down
	BaseClass::OnSizeChanged(newWide, newTall);
}

/**
* Gets the current foreground
*
* @return CMapForeground *
**
CMapForeground *CMap::GetForeground(void)
{
	// if we don't have it we need to create it
	if(!m_pForeground)
		m_pForeground = CreateForeground();

	return m_pForeground;
}

/**
* Creates our foreground panel
*
* @return CMapForeground *
**
CMapForeground *CMap::CreateForeground(void)
{
	return new CMapForeground(this, "MapForeground");
}
*/