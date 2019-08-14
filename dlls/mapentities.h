//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MAPENTITIES_H
#define MAPENTITIES_H
#ifdef _WIN32
#pragma once
#endif

#include "mapentities_shared.h"

/**
* Simple class for tracking edict usage
**/
class CMapEntityRef
{
public:
	int m_iEdict;
	int m_iSerialNumber;
};

// global for tracking used edicts
extern CUtlLinkedList<CMapEntityRef, unsigned short> g_MapEntityRefs;

// This class provides hooks into the map-entity loading process that allows CS to do some tricks
// when restarting the round. The main trick it tries to do is recreate all 
class IMapEntityFilter
{
public:
	virtual bool ShouldCreateEntity( const char *pClassname ) = 0;
	virtual CBaseEntity* CreateNextEntity( const char *pClassname ) = 0;
};

/**
* Class declaration for a map entity filter that tracks the edicts
* that new entities are using
**/
class CLoadMapEntityFilter : public IMapEntityFilter
{
public:

	/**
	* Determines if we should create a specific type of entity
	* 
	* @param const char *szClassname The name of the entity we might be creating
	* @return bool
	**/
	virtual bool ShouldCreateEntity(const char *szClassname)
	{
		// create everything
		return true;
	}

	/**
	* Creates the next entity.  Here we hang onto a little info about
	* the dude we created
	*
	* @const char *szClassname The entity type to create
	* @return CBaseEntity *
	**/
	virtual CBaseEntity* CreateNextEntity(const char *szClassname)
	{
		CBaseEntity *pEnt;
		CMapEntityRef sRef;

		// create the entity
		pEnt = CreateEntityByName(szClassname);
		
		// setup the reference
		sRef.m_iEdict = -1;
		sRef.m_iSerialNumber = -1;

		// if we got the reference, add it to our list
		if(pEnt)
		{
			// add the edict
			sRef.m_iEdict = pEnt->entindex();

			// pull the serial number for the edict
			if(pEnt->edict())
				sRef.m_iSerialNumber = pEnt->edict()->m_NetworkSerialNumber;
		}

		// add the reference to our list
		g_MapEntityRefs.AddToTail(sRef);

		return pEnt;
	}
};

static const char *s_szPreserveList[] =	{
											"worldspawn",
											"sdk_gamerules",
											"sdk_gamerulessuppression",
											"scene_manager",
											"sdk_team_manager",
											"event_queue_saveload_proxy",
											"player_manager",
											"player",
											"sdk_player",
											//"weapon_"),
											"viewmodel",
											"predicted_viewmodel",
											"info_player_team1",
											"info_player_team2",
											"info_player_team1_suppression",
											"info_player_team2_suppression",
											"env_soundscape",
											"env_soundscape_proxy",
											"env_soundscape_triggerable",
											"env_sun",
											"env_wind",
											"env_fog_controller",
											"func_brush",
											"func_wall",
											"func_illusionary",
											"infodecal",
											"info_projecteddecal",
											"info_node",
											"info_target",
											"info_node_hint",
											"info_map_parameters",
											"keyframe_rope",
											"move_rope",
											"info_ladder",
											"point_viewcontrol",
											"shadow_control",
											"sky_camera",
											"soundent",
											"trigger_soundscape",
											"attached_model",
											""
										};

/**
* Class declaration representing a filter which determines what should be
* removed and then recreated when a restarting the game
**/
class CCleanMapEntityFilter : public IMapEntityFilter
{
public:
	/**
	* Determines if we should create a specific type of entity
	*
	* @param const char *szClassname The name of the entity we might be creating
	* @return bool
	**/
	virtual bool ShouldCreateEntity(const char *szClassname)
	{
		// if it's in our list we don't need to create it
		if(FindInList(s_szPreserveList, szClassname))
		{
			// move the reference list iterator along since we won't be incrementing
			// it in the create method
			if(m_iIterator != g_MapEntityRefs.InvalidIndex())
				m_iIterator = g_MapEntityRefs.Next(m_iIterator);

			return false;
		}

		return true;
	}

	/**
	* Creates the entity of the given class name
	*
	* @param const char *szClassname The name of the entity to create
	* @return CBaseEntity *
	**/
	virtual CBaseEntity *CreateNextEntity(const char *szClassname)
	{
		// if we're at the end of the list something went wrong.
		// it should be the same list (and therefore same references as when
		// we loaded using the load filter)
		if(m_iIterator == g_MapEntityRefs.InvalidIndex())
		{
			Assert(0);
			return NULL;
		}
		else
		{
			// pull the current entity to see if we can reuse it
			// also, move to the next item
			CMapEntityRef &sRef = g_MapEntityRefs[m_iIterator];
			m_iIterator = g_MapEntityRefs.Next(m_iIterator);

			// check that we have a valid index, if yes, see if there's anything
			// already in that slot
			if(sRef.m_iEdict == -1 || engine->PEntityOfEntIndex(sRef.m_iEdict))
			{
				// can't use the slot, just create a new ent
				return CreateEntityByName(szClassname);
			}
			else
			{
				// otherwise, create the entity in the same slot
				return CreateEntityByName(szClassname, sRef.m_iEdict);
			}
		}
	}

	int m_iIterator;
};

// Use the filter so you can prevent certain entities from being created out of the map.
// CSPort does this when restarting rounds. It wants to reload most entities from the map, but certain
// entities like the world entity need to be left intact.
void MapEntity_ParseAllEntities( const char *pMapData, IMapEntityFilter *pFilter=NULL, bool bActivateEntities=false );

const char *MapEntity_ParseEntity( CBaseEntity *&pEntity, const char *pEntData, IMapEntityFilter *pFilter );
void MapEntity_PrecacheEntity( const char *pEntData );


//-----------------------------------------------------------------------------
// Hierarchical spawn 
//-----------------------------------------------------------------------------
struct HierarchicalSpawn_t
{
	CBaseEntity *m_pEntity;
	int			m_nDepth;
};

void SpawnHierarchicalList( int nEntities, HierarchicalSpawn_t *pSpawnList, bool bActivateEntities );

#endif // MAPENTITIES_H
