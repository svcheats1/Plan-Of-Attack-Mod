//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           mdlviewer.h
// last modified:  Apr 28 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
// version:        1.2
//
// email:          mete@swissquake.ch
// web:            http://www.swissquake.ch/chumbalum-soft/
//
#ifndef INCLUDED_MDLVIEWER
#define INCLUDED_MDLVIEWER



#ifndef INCLUDED_MXWINDOW
#include "mxWindow.h"
#endif



#define IDC_FILE_LOADMODEL			1001
#define IDC_FILE_LOADBACKGROUNDTEX	1002
#define IDC_FILE_LOADGROUNDTEX		1003
#define IDC_FILE_UNLOADGROUNDTEX	1004
#define IDC_FILE_CLOSEPAKFILE		1007
#define IDC_FILE_RECENTMODELS1		1008
#define IDC_FILE_RECENTMODELS2		1009
#define IDC_FILE_RECENTMODELS3		1010
#define IDC_FILE_RECENTMODELS4		1011
#define IDC_FILE_RECENTMODELS5		1012
#define IDC_FILE_RECENTMODELS6		1013
#define IDC_FILE_RECENTMODELS7		1014
#define IDC_FILE_RECENTMODELS8		1015
#define IDC_FILE_EXIT				1016
#define IDC_FILE_REFRESH			1017
#define IDC_FILE_LOADMERGEDMODEL	1018
#define IDC_FILE_UNLOADMERGEDMODEL	1019
#define IDC_FILE_LOADMODEL_STEAM		1020
#define IDC_FILE_LOADMERGEDMODEL_STEAM	1021

#define IDC_OPTIONS_COLORBACKGROUND	1101
#define IDC_OPTIONS_COLORGROUND		1102
#define IDC_OPTIONS_COLORLIGHT		1103
#define IDC_OPTIONS_COLORAMBIENT	1104
#define IDC_OPTIONS_CENTERVIEW		1105
#define IDC_OPTIONS_MAKESCREENSHOT	1106
#define IDC_OPTIONS_DUMP			1107
#define IDC_OPTIONS_VIEWMODEL		1108

#define IDC_VIEW_FILEASSOCIATIONS	1201
#define IDC_VIEW_ACTIVITIES			1202
#define IDC_VIEW_HIDDEN				1203

#define IDC_HELP_GOTOHOMEPAGE		1301
#define IDC_HELP_ABOUT				1302



class mxMenuBar;
class MatSysWindow;
class ControlPanel;
class mxMenu;

enum { Action, Size, Timer, Idle, Show, Hide,
		MouseUp, MouseDown, MouseMove, MouseDrag,
		KeyUp, KeyDown
	};

class MDLViewer : public mxWindow
{
	mxMenuBar *mb;
	MatSysWindow *d_MatSysWindow;
	ControlPanel *d_cpl;
	mxMenu *menuView;

	void loadRecentFiles ();
	void saveRecentFiles ();
	void initRecentFiles ();

public:
	// CREATORS
	MDLViewer ();
	~MDLViewer ();

	// MANIPULATORS
	virtual int handleEvent (mxEvent *event);
	void redraw ();

	void Refresh( void );
	void LoadModelFile( const char *pszFile, int slot = -1 );
	void SaveScreenShot( const char *pszFile );
	void DumpText( const char *pszFile );

	// ACCESSORS
	mxMenuBar *getMenuBar () const { return mb; }
	MatSysWindow *getMatSysWindow () const { return d_MatSysWindow; }

	int GetCurrentHitboxSet( void );

private:
	const char* SteamGetOpenFilename();
};



extern MDLViewer *g_MDLViewer;
extern char g_appTitle[];



#endif // INCLUDED_MDLVIEWER