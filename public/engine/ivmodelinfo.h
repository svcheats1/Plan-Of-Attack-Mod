//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IVMODELINFO_H
#define IVMODELINFO_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class KeyValues;
struct vcollide_t;
struct model_t;
class Vector;
class QAngle;
class CGameTrace;
typedef CGameTrace trace_t;
struct studiohdr_t;
struct virtualmodel_t;
typedef unsigned char byte;


//-----------------------------------------------------------------------------
// Model info interface
//-----------------------------------------------------------------------------

// change this when the new version is incompatable with the old
#define VMODELINFO_CLIENT_INTERFACE_VERSION	"VModelInfoClient003"
#define VMODELINFO_SERVER_INTERFACE_VERSION	"VModelInfoServer001"

class IVModelInfo
{
public:
	virtual							~IVModelInfo( void ) { }

	virtual const model_t			*GetModel( int modelindex ) const = 0;
	// Returns index of model by name
	virtual int						GetModelIndex( const char *name ) const = 0;

	// Returns name of model
	virtual const char				*GetModelName( const model_t *model ) const = 0;
	virtual vcollide_t				*GetVCollide( const model_t *model ) const = 0;
	virtual vcollide_t				*GetVCollide( int modelindex ) const = 0;
	virtual void					GetModelBounds( const model_t *model, Vector& mins, Vector& maxs ) const = 0;
	virtual	void					GetModelRenderBounds( const model_t *model, Vector& mins, Vector& maxs ) const = 0;
	virtual int						GetModelFrameCount( const model_t *model ) const = 0;
	virtual int						GetModelType( const model_t *model ) const = 0;
	virtual void					*GetModelExtraData( const model_t *model ) = 0;
	virtual bool					ModelHasMaterialProxy( const model_t *model ) const = 0;
	virtual bool					IsTranslucent( model_t const* model ) const = 0;
	virtual bool					IsTranslucentTwoPass( const model_t *model ) const = 0;
	virtual void					RecomputeTranslucency( const model_t *model ) = 0;
	virtual int						GetModelMaterialCount( const model_t* model ) const = 0;
	virtual void					GetModelMaterials( const model_t *model, int count, IMaterial** ppMaterial ) = 0;
	virtual bool					IsModelVertexLit( const model_t *model ) const = 0;
	virtual const char				*GetModelKeyValueText( const model_t *model ) = 0;
	virtual float					GetModelRadius( const model_t *model ) = 0;

	virtual const studiohdr_t		*FindModel( const studiohdr_t *pStudioHdr, void **cache, const char *modelname ) const = 0;
	virtual const studiohdr_t		*FindModel( void *cache ) const = 0;
	virtual	virtualmodel_t			*GetVirtualModel( const studiohdr_t *pStudioHdr ) const = 0;
	virtual byte					*GetAnimBlock( const studiohdr_t *pStudioHdr, int iBlock ) const = 0;

	// Available on client only!!!
	virtual void					GetModelMaterialColorAndLighting( const model_t *model, Vector const& origin,
										QAngle const& angles, trace_t* pTrace, 
										Vector& lighting, Vector& matColor ) = 0;
	virtual void					GetIlluminationPoint( const model_t *model, Vector const& origin, 
										QAngle const& angles, Vector* pLightingCenter ) = 0;

	virtual int						GetModelContents( int modelIndex ) const = 0;
	virtual studiohdr_t				*GetStudiomodel( const model_t *mod ) = 0;
	virtual int						GetModelSpriteWidth( const model_t *model ) const = 0;
	virtual int						GetModelSpriteHeight( const model_t *model ) const = 0;
};


class IVModelInfoClient : public IVModelInfo
{
public:
	// Sets/gets a map-specified fade range
	virtual void					SetLevelScreenFadeRange( float flMinSize, float flMaxSize ) = 0;
	virtual void					GetLevelScreenFadeRange( float *pMinArea, float *pMaxArea ) const = 0;

	// Sets/gets a map-specified per-view fade range
	virtual void					SetViewScreenFadeRange( float flMinSize, float flMaxSize ) = 0;

	// Computes fade alpha based on distance fade + screen fade
	virtual unsigned char			ComputeLevelScreenFade( const Vector &vecAbsOrigin, float flRadius, float flFadeScale ) const = 0;
	virtual unsigned char			ComputeViewScreenFade( const Vector &vecAbsOrigin, float flRadius, float flFadeScale ) const = 0;
};


#endif // IVMODELINFO_H