//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Just about as simple as a shader gets. Specify a vertex
//          and pixel shader, bind textures, and that's it.
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "convar.h"

// Note: you have to run buildshaders.bat to generate these files from the FXC code.
#include "sdk_lightmap_ps20.inc"
#include "sdk_lightmap_vs20.inc"


BEGIN_VS_SHADER( SDK_Lightmap, "Help for SDK_Lightmap" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			// Enable the texture for base texture and lightmap.
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0, 0 );

			sdk_lightmap_vs20_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "sdk_lightmap_vs20", vshIndex.GetIndex() );

			sdk_lightmap_ps20_Static_Index pshIndex;
			pShaderShadow->SetPixelShader( "sdk_lightmap_ps20", pshIndex.GetIndex() );

			DefaultFog();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
		}
		Draw();
	}
END_SHADER
