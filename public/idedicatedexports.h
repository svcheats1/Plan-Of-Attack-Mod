//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IDEDICATEDEXPORTS_H
#define IDEDICATEDEXPORTS_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class IDedicatedExports
{
public:
	virtual void Sys_Printf( char *text ) = 0;
};

#define VENGINE_DEDICATEDEXPORTS_API_VERSION "VENGINE_DEDICATEDEXPORTS_API_VERSION001"

#endif // IDEDICATEDEXPORTS_H
