//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: CHud handles the message, calculation, and drawing the HUD
//
// $NoKeywords: $
//=============================================================================//
#ifndef HUD_H
#define HUD_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlVector.h"
#include "UtlDict.h"
#include "ConVar.h"
#include <vgui/vgui.h>
#include <Color.h>
#include <bitbuf.h>

namespace vgui
{
	class IScheme;
}

// basic rectangle struct used for drawing
typedef struct wrect_s
{
	int	left;
	int right;
	int top;
	int bottom;
} wrect_t;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudTexture
{
public:
	CHudTexture()
	{
		Q_memset( szShortName, 0, sizeof( szShortName ) );
		Q_memset( szTextureFile, 0, sizeof( szTextureFile ) );
		Q_memset( texCoords, 0, sizeof( texCoords ) );
		Q_memset( &rc, 0, sizeof( rc ) );
		textureId = -1;
		bRenderUsingFont = -1;
		cCharacterInFont = 0;
		hFont = NULL;

		// set up the coords
		texCoords[0] = 0;
		texCoords[1] = 0;
		texCoords[2] = 1;
		texCoords[3] = 1;
	}

	CHudTexture& operator =( const CHudTexture& src )
	{
		if ( this == &src )
			return *this;

		Q_strncpy( szShortName, src.szShortName, sizeof( szShortName ) );
		Q_strncpy( szTextureFile, src.szTextureFile, sizeof( szTextureFile ) );
		Q_memcpy( texCoords, src.texCoords, sizeof( texCoords ) );
		textureId = src.textureId;
		rc = src.rc;
		bRenderUsingFont = src.bRenderUsingFont;
		cCharacterInFont = src.cCharacterInFont;
		hFont = src.hFont;

		return *this;
	}

	int Width() const
	{
		return rc.right - rc.left;
	}

	int Height() const
	{
		return rc.bottom - rc.top;
	}

	void SetBounds(int iLeft, int iRight, int iTop, int iBottom)
	{
		rc.left = iLeft;
		rc.right = iRight;
		rc.top = iTop;
		rc.bottom = iBottom;
	}

	void DrawSelf( int x, int y, Color& clr ) const;
	virtual void DrawSelf( int x, int y, int w, int h, Color& clr ) const;
	void DrawSelfCropped( int x, int y, int cropx, int cropy, int cropw, int croph, Color& clr ) const;

	char		szShortName[ 64 ];
	char		szTextureFile[ 64 ];

	bool		bRenderUsingFont;
	char		cCharacterInFont;
	vgui::HFont hFont;

	// vgui texture Id assigned to this item
	int			textureId;
	// s0, t0, s1, t1
	float		texCoords[ 4 ];

	// Original bounds
	wrect_t		rc;
};

#include "hudtexturehandle.h"

class CHudElement;

//-----------------------------------------------------------------------------
// Purpose: Main hud manager
//-----------------------------------------------------------------------------
class CHud 
{
public:
	//For progress bar orientations
	static const int			HUDPB_HORIZONTAL;
	static const int			HUDPB_VERTICAL;
	static const int			HUDPB_HORIZONTAL_INV;

public:
								CHud();
								~CHud();

	// Init's called when the HUD's created at DLL load
	void						Init( void );
	// VidInit's called when the video mode's changed
	void						VidInit( void );
	// Shutdown's called when the engine's shutting down
	void						Shutdown( void );
	// LevelInit's called whenever a new level's starting
	void						LevelInit( void );
	// LevelShutdown's called whenever a level's finishing
	void						LevelShutdown( void );
	
	void						ResetHUD( void );

	// A saved game has just been loaded
	void						OnRestore();

	void						Think();

	void						ProcessInput( bool bActive );
	void						UpdateHud( bool bActive );

	void						InitColors( vgui::IScheme *pScheme );

	// Hud element registration
	void						AddHudElement( CHudElement *pHudElement );
	void						RemoveHudElement( CHudElement *pHudElement );
	// Search list for "name" and return the hud element if it exists
	CHudElement					*FindElement( const char *pName );
	
	bool						IsHidden( int iHudFlags );

	float						GetSensitivity();

	void						DrawProgressBar( int x, int y, int width, int height, float percentage, Color& clr, unsigned char type );
	void						DrawIconProgressBar( int x, int y, CHudTexture *icon, CHudTexture *icon2, float percentage, Color& clr, int type );

	CHudTexture					*GetIcon( const char *szIcon );

	// loads a new icon into the list, without duplicates
	CHudTexture					*AddUnsearchableHudIconToList( CHudTexture& texture );
	CHudTexture					*AddSearchableHudIconToList( CHudTexture& texture );

	void						RefreshHudTextures();

	// User messages
	void						MsgFunc_WorldMap(bf_read &msg);
	void						MsgFunc_ChangeSkill(bf_read &msg);
	void						MsgFunc_ChooseObjective(bf_read &msg);
	void						MsgFunc_BuyMenu(bf_read &msg);
	void						MsgFunc_AllowPunish(bf_read &msg);
	void						MsgFunc_ChangeTeam(bf_read &msg);
	void						MsgFunc_SyncRoundTimer(bf_read &msg);
	void						MsgFunc_ResetHUD(bf_read &msg);
	void 						MsgFunc_SendAudio(bf_read &msg);

public:

	int							m_iKeyBits;
	float						m_flMouseSensitivity;
	float						m_flMouseSensitivityFactor;

	Color					m_clrNormal;
	Color					m_clrCaution;
	Color					m_clrYellowish;

	CUtlVector< CHudElement * >		m_HudList;

private:
	void						InitFonts();

	void						SetupNewHudTexture( CHudTexture *t );

	bool						m_bHudTexturesLoaded;

	// Global list of known icons
	CUtlDict< CHudTexture *, int >		m_Icons;
};

extern CHud gHUD;

//-----------------------------------------------------------------------------
// Global fonts used in the client DLL
//-----------------------------------------------------------------------------
extern vgui::HFont g_hFontTrebuchet24;

void LoadHudTextures( CUtlDict< CHudTexture *, int >& list, char *szFilenameWithoutExtension, const unsigned char *pICEKey );


#endif // HUD_H
