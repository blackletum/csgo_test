﻿//========= Copyright ¿½ 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "studiorender.h"
#include "studio.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imorph.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterial.h"
#include "optimize.h"
#include "mathlib/mathlib.h"
#include "mathlib/vector.h"
#include "mathlib/vmatrix.h"
#include "studiorendercontext.h"
#include "tier2/tier2.h"
#include "tier0/vprof.h"
#include "filesystem.h"
//#include "tier0/miniprofiler.h"

//#define PROFILE_STUDIO VPROF
#define PROFILE_STUDIO

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

typedef void (*SoftwareProcessMeshFunc_t)( const mstudio_meshvertexdata_t *, matrix3x4_t *pPoseToWorld,
	CCachedRenderData &vertexCache, CMeshBuilder& meshBuilder, int numVertices, unsigned short* pGroupToMesh, unsigned int nAlphaMask,
											  IMaterial *pMaterial);

#define VERTEX_FORMAT_STANDARD MATERIAL_VERTEX_FORMAT_MODEL
#define VERTEX_FORMAT_SUBDQUAD ( VERTEX_POSITION | VERTEX_NORMAL | VERTEX_USERDATA_SIZE( 4 ) | VERTEX_FORMAT_USE_EXACT_FORMAT | VERTEX_FORMAT_PAD_POS_NORM )


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class IClientEntity;


static int boxpnt[6][4] = 
{
	{ 0, 4, 6, 2 }, // +X
	{ 0, 1, 5, 4 }, // +Y
	{ 0, 2, 3, 1 }, // +Z
	{ 7, 5, 1, 3 }, // -X
	{ 7, 3, 2, 6 }, // -Y
	{ 7, 6, 4, 5 }, // -Z
};	

static TableVector	hullcolor[8] = 
{
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 0.5, 0.5 },
	{ 0.5, 1.0, 0.5 },
	{ 1.0, 1.0, 0.5 },
	{ 0.5, 0.5, 1.0 },
	{ 1.0, 0.5, 1.0 },
	{ 0.5, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 }
};


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
static unsigned int s_nTranslucentModelHullCache = 0;
static unsigned int s_nSolidModelHullCache = 0;
void CStudioRender::R_StudioDrawHulls( int hitboxset, bool translucent )
{
	int			i, j;
//	float		lv;
	Vector		tmp;
	Vector		p[8];
	mstudiobbox_t		*pbbox;
	IMaterialVar *colorVar;

	mstudiohitboxset_t *s = m_pStudioHdr->pHitboxSet( hitboxset );
	if ( !s )
		return;

	pbbox		= s->pHitbox( 0 );
	if ( !pbbox )
		return;

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	if( translucent )
	{
		pRenderContext->Bind( m_pMaterialTranslucentModelHulls );
		colorVar = m_pMaterialTranslucentModelHulls->FindVarFast( "$color", &s_nTranslucentModelHullCache );
	}
	else
	{
		pRenderContext->Bind( m_pMaterialSolidModelHulls );
		colorVar = m_pMaterialSolidModelHulls->FindVarFast( "$color", &s_nSolidModelHullCache );
	}


	for (i = 0; i < s->numhitboxes; i++)
	{
		for (j = 0; j < 8; j++)
		{
			tmp[0] = (j & 1) ? pbbox[i].bbmin[0] : pbbox[i].bbmax[0];
			tmp[1] = (j & 2) ? pbbox[i].bbmin[1] : pbbox[i].bbmax[1];
			tmp[2] = (j & 4) ? pbbox[i].bbmin[2] : pbbox[i].bbmax[2];

			VectorTransform( tmp, m_pBoneToWorld[pbbox[i].bone], p[j] );
		}

		j = (pbbox[i].group % 8);
		g_pMaterialSystem->Flush();
		if( colorVar )
		{
			if( translucent )
			{
				colorVar->SetVecValue( 0.2f * hullcolor[j].x, 0.2f * hullcolor[j].y, 0.2f * hullcolor[j].z );
			}
			else
			{
				colorVar->SetVecValue( hullcolor[j].x, hullcolor[j].y, hullcolor[j].z );
			}
		}
		for (j = 0; j < 6; j++)
		{
#if 0
			tmp[0] = tmp[1] = tmp[2] = 0;
			tmp[j % 3] = (j < 3) ? 1.0 : -1.0;
			// R_StudioLighting( &lv, pbbox[i].bone, 0, tmp ); // BUG: not updated
#endif

			IMesh* pMesh = pRenderContext->GetDynamicMesh();
			CMeshBuilder meshBuilder;
			meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

			for (int k = 0; k < 4; ++k)
			{
				meshBuilder.Position3fv( p[boxpnt[j][k]].Base() );
				meshBuilder.AdvanceVertex();
			}
			
			meshBuilder.End();
			pMesh->Draw();
		}
	}
}


void CStudioRender::R_StudioDrawBones (void)
{
	int			i, j, k;
//	float		lv;
	Vector		tmp;
	Vector		p[8];
	Vector		up, right, forward;
	Vector		a1;
	const mstudiobone_t		*pbones;
	Vector		positionArray[4];

	pbones		= m_pStudioHdr->pBone( 0 );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	for (i = 0; i < m_pStudioHdr->numbones; i++)
	{
		if (pbones[i].parent == -1)
			continue;

		k = pbones[i].parent;

		a1[0] = a1[1] = a1[2] = 1.0;
		up[0] = m_pBoneToWorld[i][0][3] - m_pBoneToWorld[k][0][3];
		up[1] = m_pBoneToWorld[i][1][3] - m_pBoneToWorld[k][1][3];
		up[2] = m_pBoneToWorld[i][2][3] - m_pBoneToWorld[k][2][3];
		if (up[0] > up[1])
			if (up[0] > up[2])
				a1[0] = 0.0;
			else
				a1[2] = 0.0;
		else
			if (up[1] > up[2])
				a1[1] = 0.0;
			else
				a1[2] = 0.0;
		CrossProduct( up, a1, right );
		VectorNormalize( right );
		CrossProduct( up, right, forward );
		VectorNormalize( forward );
		VectorScale( right, 2.0, right );
		VectorScale( forward, 2.0, forward );

		for (j = 0; j < 8; j++)
		{
			p[j][0] = m_pBoneToWorld[k][0][3];
			p[j][1] = m_pBoneToWorld[k][1][3];
			p[j][2] = m_pBoneToWorld[k][2][3];

			if (j & 1)
			{
				VectorSubtract( p[j], right, p[j] );
			}
			else
			{
				VectorAdd( p[j], right, p[j] );
			}

			if (j & 2)
			{
				VectorSubtract( p[j], forward, p[j] );
			}
			else
			{
				VectorAdd( p[j], forward, p[j] );
			}

			if (j & 4)
			{ 
			}
			else
			{
				VectorAdd( p[j], up, p[j] );
			}
		}

		VectorNormalize( up );
		VectorNormalize( right );
		VectorNormalize( forward );

		pRenderContext->Bind( m_pMaterialModelBones );
		
		for (j = 0; j < 6; j++)
		{
			switch( j)
			{
			case 0:	VectorCopy( right, tmp ); break;
			case 1:	VectorCopy( forward, tmp ); break;
			case 2:	VectorCopy( up, tmp ); break;
			case 3:	VectorScale( right, -1, tmp ); break;
			case 4:	VectorScale( forward, -1, tmp ); break;
			case 5:	VectorScale( up, -1, tmp ); break;
			}
			// R_StudioLighting( &lv, -1, 0, tmp );  // BUG: not updated

			IMesh* pMesh = pRenderContext->GetDynamicMesh();
			CMeshBuilder meshBuilder;
			meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

			for (int k = 0; k < 4; ++k)
			{
				meshBuilder.Position3fv( p[boxpnt[j][k]].Base() );
				meshBuilder.AdvanceVertex();
			}
			
			meshBuilder.End();
			pMesh->Draw();
		}
	}
}


int CStudioRender::R_StudioRenderModel( IMatRenderContext *pRenderContext, int skin, 
	int body, int hitboxset, void /*IClientEntity*/ *pEntity,
	IMaterial **ppMaterials, int *pMaterialFlags, int flags, int boneMask, int lod, ColorMeshInfo_t *pColorMeshes )
{
	VPROF("CStudioRender::R_StudioRenderModel");

	int nDrawGroup = flags & STUDIORENDER_DRAW_GROUP_MASK;

	if ( m_pRC->m_Config.drawEntities == 2 )
	{
		if ( nDrawGroup != STUDIORENDER_DRAW_TRANSLUCENT_ONLY )
		{
			R_StudioDrawBones( );
		}
		return 0;
	}

	if ( m_pRC->m_Config.drawEntities == 3 )
	{
		if ( nDrawGroup != STUDIORENDER_DRAW_TRANSLUCENT_ONLY )
		{
			R_StudioDrawHulls( hitboxset, false );
		}
		return 0;
	}

	// BUG: This method is crap, though less crap than before.  It should just sort 
	// the materials though it'll need to sort at render time as "skin" 
	// can change what materials a given mesh may use
	int numFacesRendered = 0, numPasses = 0;

	// Build list of submodels
	BodyPartInfo_t *pBodyPartInfo = (BodyPartInfo_t*)stackalloc( m_pStudioHdr->numbodyparts * sizeof(BodyPartInfo_t) );
	for ( int i=0 ; i < m_pStudioHdr->numbodyparts; ++i ) 
	{
		pBodyPartInfo[i].m_nSubModelIndex = R_StudioSetupModel( i, body, &pBodyPartInfo[i].m_pSubModel, m_pStudioHdr );
	}

	// mark possible translucent meshes
	if ( nDrawGroup != STUDIORENDER_DRAW_TRANSLUCENT_ONLY )
	{
		// we're going to render the opaque meshes, so these will get counted in that pass
		m_bSkippedMeshes = false;
		m_bDrawTranslucentSubModels = false;
		numFacesRendered += R_StudioRenderFinal( pRenderContext, skin, m_pStudioHdr->numbodyparts, pBodyPartInfo, 
			pEntity, ppMaterials, pMaterialFlags, boneMask, lod, pColorMeshes );
		numPasses++;
	}
	else
	{
		m_bSkippedMeshes = true;
	}

	if ( m_bSkippedMeshes && nDrawGroup != STUDIORENDER_DRAW_OPAQUE_ONLY )
	{
		m_bDrawTranslucentSubModels = true;
		numFacesRendered += R_StudioRenderFinal( pRenderContext, skin, m_pStudioHdr->numbodyparts, pBodyPartInfo, 
			pEntity, ppMaterials, pMaterialFlags, boneMask, lod, pColorMeshes );
		numPasses++;
	}

#ifndef _CERT
	static ConVarRef mat_rendered_faces_count( "mat_rendered_faces_count" );
	static ConVarRef mat_print_top_model_vert_counts( "mat_print_top_model_vert_counts" );
	if ( numPasses && ( mat_rendered_faces_count.GetBool() || mat_print_top_model_vert_counts.GetBool() ) )
	{
		// Each model counts how many rendered faces it accounts for each frame:
		m_pStudioHWData->UpdateFacesRenderedCount( m_pStudioHdr, m_ModelFaceCountHash, lod, 1, numFacesRendered );
	}
#endif // !_CERT

	return numFacesRendered;
}


//-----------------------------------------------------------------------------
// Generate morph accumulator
//-----------------------------------------------------------------------------
void CStudioRender::GenerateMorphAccumulator( mstudiomodel_t *pSubModel )
{
	// Deal with all flexes
	// FIXME: HW Morphing doesn't work with translucent models yet
	if ( !m_pRC->m_Config.m_bEnableHWMorph || !m_pRC->m_Config.bFlex || m_bDrawTranslucentSubModels || 
		 !g_pMaterialSystemHardwareConfig->HasFastVertexTextures() )
		return;

	int nActiveMeshCount = 0;
	mstudiomesh_t *ppMeshes[512];

	// First, build the list of meshes that need morphing
	for ( int i = 0; i < pSubModel->nummeshes; ++i )
	{
		mstudiomesh_t *pMesh = pSubModel->pMesh(i);
		studiomeshdata_t *pMeshData = &m_pStudioMeshes[pMesh->meshid];
		Assert( pMeshData );

		int nFlexCount = pMesh->numflexes;
		if ( !nFlexCount )
			continue;

		for ( int j = 0; j < pMeshData->m_NumGroup; ++j )
		{
			studiomeshgroup_t* pGroup = &pMeshData->m_pMeshGroup[j];
			bool bIsDeltaFlexed = (pGroup->m_Flags & MESHGROUP_IS_DELTA_FLEXED) != 0;
			if ( !bIsDeltaFlexed )
				continue;

			ppMeshes[nActiveMeshCount++] = pMesh;
			Assert( nActiveMeshCount < 512 );
			break;
		}
	}

	if ( nActiveMeshCount == 0 )
		return;

	// HACK - Just turn off scissor for this model if it is doing morph accumulation
//	DisableScissor();

	// Next, accumulate morphs for appropriate meshes
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->BeginMorphAccumulation();
	for ( int i = 0; i < nActiveMeshCount; ++i )
	{
		mstudiomesh_t *pMesh = ppMeshes[i];
		studiomeshdata_t *pMeshData = &m_pStudioMeshes[pMesh->meshid];

		int nFlexCount = pMesh->numflexes;
		MorphWeight_t *pWeights = (MorphWeight_t*)stackalloc( nFlexCount * sizeof(MorphWeight_t) );
		ComputeFlexWeights( nFlexCount, pMesh->pFlex(0), pWeights );

		for ( int j = 0; j < pMeshData->m_NumGroup; ++j )
		{
			studiomeshgroup_t* pGroup = &pMeshData->m_pMeshGroup[j];
			if ( !pGroup->m_pMorph )
				continue;

			pRenderContext->AccumulateMorph( pGroup->m_pMorph, nFlexCount, pWeights );
		}
	}
	pRenderContext->EndMorphAccumulation();
}


//-----------------------------------------------------------------------------
// Computes eyeball state
//-----------------------------------------------------------------------------
void CStudioRender::ComputeEyelidStateFACS( mstudiomodel_t *pSubModel )
{
	for ( int j = 0; j < pSubModel->numeyeballs; j++ )
	{
		// FIXME: This might not be necessary... 
		R_StudioEyeballPosition( pSubModel->pEyeball( j ), &m_pEyeballState[ j ] );
		R_StudioEyelidFACS( pSubModel->pEyeball(j), &m_pEyeballState[j] );
	}
}


/*
================
R_StudioRenderFinal
inputs:
outputs: returns the number of triangles rendered.
================
*/
int CStudioRender::R_StudioRenderFinal( IMatRenderContext *pRenderContext, 
	int skin, int nBodyPartCount, BodyPartInfo_t *pBodyPartInfo, void /*IClientEntity*/ *pClientEntity,
	IMaterial **ppMaterials, int *pMaterialFlags, int boneMask, int lod, ColorMeshInfo_t *pColorMeshes )
{
	VPROF("CStudioRender::R_StudioRenderFinal");

	int numFacesRendered = 0;

	for ( int i=0 ; i < nBodyPartCount; i++ ) 
	{
		m_pSubModel = pBodyPartInfo[i].m_pSubModel;

		// NOTE: This has to run here because it effects flex targets,
		// so therefore it must happen prior to GenerateMorphAccumulator.
		ComputeEyelidStateFACS( m_pSubModel );
		GenerateMorphAccumulator( m_pSubModel );

		// Set up SW flex
		m_VertexCache.SetBodyPart( i );
		m_VertexCache.SetModel( pBodyPartInfo[i].m_nSubModelIndex );

		numFacesRendered += R_StudioDrawPoints( pRenderContext, skin, pClientEntity, 
			ppMaterials, pMaterialFlags, boneMask, lod, pColorMeshes );
	}
	return numFacesRendered;
}

ConVar r_flashlightscissor( "r_flashlightscissor", "0", FCVAR_MATERIAL_SYSTEM_THREAD );

void CStudioRender::PushScissor( FlashlightState_t *state )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	// Only scissor into the backbuffer
	if ( r_flashlightscissor.GetBool() && state->DoScissor() && ( pRenderContext->GetRenderTarget() == NULL ) )
	{
		pRenderContext->PushScissorRect( state->GetLeft(), state->GetTop(), state->GetRight(), state->GetBottom() );
	}
}

void CStudioRender::PopScissor( FlashlightState_t *state )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	// Only scissor into the backbuffer
	if ( r_flashlightscissor.GetBool() && state->DoScissor() && ( pRenderContext->GetRenderTarget() == NULL ) )
	{
		pRenderContext->PopScissorRect();
	}
}


//-----------------------------------------------------------------------------
// Draw shadows
//-----------------------------------------------------------------------------
void CStudioRender::DrawShadows( const DrawModelInfo_t& info, int flags, int boneMask )
{
	if ( !m_ShadowState.Count() )
		return;

	VPROF("CStudioRender::DrawShadows");

	IMaterial* pForcedMat = m_pRC->m_pForcedMaterial[ 0 ];
	OverrideType_t nForcedType = m_pRC->m_nForcedMaterialType;

	// Here, we have to redraw the model one time for each flashlight
	// Having a material of NULL means that we are a light source.
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	// Bail if we're using single-pass flashlight
	if ( IsGameConsole() || pRenderContext->SinglePassFlashlightModeEnabled() )
		return;

	pRenderContext->SetFlashlightMode( true );
	int i;
	for (i = 0; i < m_ShadowState.Count(); ++i )
	{
		if( !m_ShadowState[i].m_pMaterial )
		{
			Assert( m_ShadowState[i].m_pFlashlightState && m_ShadowState[i].m_pWorldToTexture );
			if ( ( m_ShadowState[i].m_pFlashlightState && m_ShadowState[i].m_pWorldToTexture ) )
			{
				pRenderContext->SetFlashlightStateEx( *m_ShadowState[i].m_pFlashlightState, *m_ShadowState[i].m_pWorldToTexture, m_ShadowState[i].m_pFlashlightDepthTexture );

				m_pCurrentFlashlight = m_ShadowState[i].m_pFlashlightState;

				PushScissor( m_ShadowState[i].m_pFlashlightState );

				R_StudioRenderModel( pRenderContext, info.m_Skin, info.m_Body, info.m_HitboxSet, info.m_pClientEntity,
					info.m_pHardwareData->m_pLODs[info.m_Lod].ppMaterials, 
					info.m_pHardwareData->m_pLODs[info.m_Lod].pMaterialFlags, flags, boneMask, info.m_Lod, info.m_pColorMeshes );

				PopScissor( m_ShadowState[i].m_pFlashlightState );

				m_pCurrentFlashlight = NULL;
			}
		}
	}
	pRenderContext->SetFlashlightMode( false );

	// Here, we have to redraw the model one time for each shadow
	for (int i = 0; i < m_ShadowState.Count(); ++i )
	{
		if( m_ShadowState[i].m_pMaterial )
		{
			m_pRC->m_pForcedMaterial[ 0 ] = m_ShadowState[i].m_pMaterial;
			m_pRC->m_nForcedMaterialType = OVERRIDE_NORMAL;
			R_StudioRenderModel( pRenderContext, 0, info.m_Body, 0, m_ShadowState[i].m_pProxyData,
				NULL, NULL, flags, boneMask, info.m_Lod, NULL );
		}
	}

	// Restore the previous forced material
	m_pRC->m_pForcedMaterial[ 0 ] = pForcedMat;
	m_pRC->m_nForcedMaterialType = nForcedType;
}

void CStudioRender::DrawStaticPropShadows( const DrawModelInfo_t &info, const StudioRenderContext_t &rc, const matrix3x4_t& rootToWorld, int flags )
{
	memcpy( &m_StaticPropRootToWorld, &rootToWorld, sizeof(matrix3x4_t) );
	memcpy( &m_PoseToWorld[0], &rootToWorld, sizeof(matrix3x4_t) );

	m_pRC = const_cast< StudioRenderContext_t* >( &rc );
	m_pBoneToWorld = &m_StaticPropRootToWorld;
	m_pStudioHdr = info.m_pStudioHdr;
	m_pStudioMeshes = info.m_pHardwareData->m_pLODs[info.m_Lod].m_pMeshData;
	m_pStudioHWData = info.m_pHardwareData;
	DrawShadows( info, flags, BONE_USED_BY_ANYTHING );
	m_pRC = NULL;
	m_pBoneToWorld = NULL;
	m_pStudioHdr = NULL;
	m_pStudioMeshes = NULL;
	m_pStudioHWData = NULL;
}

// Draw flashlight lighting on decals.
void CStudioRender::DrawFlashlightDecals( const DrawModelInfo_t& info, int lod )
{
	if ( !m_ShadowState.Count() || IsGameConsole() ) // game console implies single pass flashlight
		return;

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->SetFlashlightMode( true );
	int i;
	for (i = 0; i < m_ShadowState.Count(); ++i )
	{
		// This isn't clear.  This means that this is a flashlight if the material is NULL.  FLASHLIGHTFIXME
		if( !m_ShadowState[i].m_pMaterial )
		{
			Assert( m_ShadowState[i].m_pFlashlightState && m_ShadowState[i].m_pWorldToTexture );
			if ( m_ShadowState[i].m_pFlashlightState && m_ShadowState[i].m_pWorldToTexture )
			{
				pRenderContext->SetFlashlightStateEx( *m_ShadowState[i].m_pFlashlightState, *m_ShadowState[i].m_pWorldToTexture, m_ShadowState[i].m_pFlashlightDepthTexture );

				PushScissor( m_ShadowState[i].m_pFlashlightState );

				DrawDecal( info, lod, info.m_Body );

				PopScissor( m_ShadowState[i].m_pFlashlightState );
			}
		}
	}
	pRenderContext->SetFlashlightMode( false );
}


matrix3x4_t *ComputeSkinMatrix( mstudioboneweight_t &boneweights, matrix3x4_t *pPoseToWorld, matrix3x4_t &scratchMatrix )
{
	float flWeight0, flWeight1, flWeight2;

	switch( boneweights.numbones )
	{
	default:
	case 1:
		return &pPoseToWorld[boneweights.bone[0]];

	case 2:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			flWeight0 = boneweights.weight[0];
			flWeight1 = boneweights.weight[1];

			// NOTE: Inlining here seems to make a fair amount of difference
			scratchMatrix[0][0] = boneMat0[0][0] * flWeight0 + boneMat1[0][0] * flWeight1;
			scratchMatrix[0][1] = boneMat0[0][1] * flWeight0 + boneMat1[0][1] * flWeight1;
			scratchMatrix[0][2] = boneMat0[0][2] * flWeight0 + boneMat1[0][2] * flWeight1;
			scratchMatrix[0][3] = boneMat0[0][3] * flWeight0 + boneMat1[0][3] * flWeight1;
			scratchMatrix[1][0] = boneMat0[1][0] * flWeight0 + boneMat1[1][0] * flWeight1;
			scratchMatrix[1][1] = boneMat0[1][1] * flWeight0 + boneMat1[1][1] * flWeight1;
			scratchMatrix[1][2] = boneMat0[1][2] * flWeight0 + boneMat1[1][2] * flWeight1;
			scratchMatrix[1][3] = boneMat0[1][3] * flWeight0 + boneMat1[1][3] * flWeight1;
			scratchMatrix[2][0] = boneMat0[2][0] * flWeight0 + boneMat1[2][0] * flWeight1;
			scratchMatrix[2][1] = boneMat0[2][1] * flWeight0 + boneMat1[2][1] * flWeight1;
			scratchMatrix[2][2] = boneMat0[2][2] * flWeight0 + boneMat1[2][2] * flWeight1;
			scratchMatrix[2][3] = boneMat0[2][3] * flWeight0 + boneMat1[2][3] * flWeight1;
		}
		return &scratchMatrix;

	case 3:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			matrix3x4_t &boneMat2 = pPoseToWorld[boneweights.bone[2]];
			flWeight0 = boneweights.weight[0];
			flWeight1 = boneweights.weight[1];
			flWeight2 = boneweights.weight[2];

			scratchMatrix[0][0] = boneMat0[0][0] * flWeight0 + boneMat1[0][0] * flWeight1 + boneMat2[0][0] * flWeight2;
			scratchMatrix[0][1] = boneMat0[0][1] * flWeight0 + boneMat1[0][1] * flWeight1 + boneMat2[0][1] * flWeight2;
			scratchMatrix[0][2] = boneMat0[0][2] * flWeight0 + boneMat1[0][2] * flWeight1 + boneMat2[0][2] * flWeight2;
			scratchMatrix[0][3] = boneMat0[0][3] * flWeight0 + boneMat1[0][3] * flWeight1 + boneMat2[0][3] * flWeight2;
			scratchMatrix[1][0] = boneMat0[1][0] * flWeight0 + boneMat1[1][0] * flWeight1 + boneMat2[1][0] * flWeight2;
			scratchMatrix[1][1] = boneMat0[1][1] * flWeight0 + boneMat1[1][1] * flWeight1 + boneMat2[1][1] * flWeight2;
			scratchMatrix[1][2] = boneMat0[1][2] * flWeight0 + boneMat1[1][2] * flWeight1 + boneMat2[1][2] * flWeight2;
			scratchMatrix[1][3] = boneMat0[1][3] * flWeight0 + boneMat1[1][3] * flWeight1 + boneMat2[1][3] * flWeight2;
			scratchMatrix[2][0] = boneMat0[2][0] * flWeight0 + boneMat1[2][0] * flWeight1 + boneMat2[2][0] * flWeight2;
			scratchMatrix[2][1] = boneMat0[2][1] * flWeight0 + boneMat1[2][1] * flWeight1 + boneMat2[2][1] * flWeight2;
			scratchMatrix[2][2] = boneMat0[2][2] * flWeight0 + boneMat1[2][2] * flWeight1 + boneMat2[2][2] * flWeight2;
			scratchMatrix[2][3] = boneMat0[2][3] * flWeight0 + boneMat1[2][3] * flWeight1 + boneMat2[2][3] * flWeight2;
		}
		return &scratchMatrix;

	case 4:
		{
			Assert( 0 ); // results undefined for numbones == 4, as MAX_NUM_BONES_PER_VERT is 3
		}
		return &scratchMatrix;
	}

	Assert(0);
	return NULL;
}

static void ComputeSkinMatrixToMemory( mstudioboneweight_t &boneweights, matrix3x4_t *pPoseToWorld, matrix3x4_t &result )
{
	float flWeight0, flWeight1, flWeight2;

	switch( boneweights.numbones )
	{
	default:
	case 1:
		memcpy( &result, &pPoseToWorld[boneweights.bone[0]], sizeof(matrix3x4_t) );
		return;

	case 2:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			flWeight0 = boneweights.weight[0];
			flWeight1 = boneweights.weight[1];

			// NOTE: Inlining here seems to make a fair amount of difference
			result[0][0] = boneMat0[0][0] * flWeight0 + boneMat1[0][0] * flWeight1;
			result[0][1] = boneMat0[0][1] * flWeight0 + boneMat1[0][1] * flWeight1;
			result[0][2] = boneMat0[0][2] * flWeight0 + boneMat1[0][2] * flWeight1;
			result[0][3] = boneMat0[0][3] * flWeight0 + boneMat1[0][3] * flWeight1;
			result[1][0] = boneMat0[1][0] * flWeight0 + boneMat1[1][0] * flWeight1;
			result[1][1] = boneMat0[1][1] * flWeight0 + boneMat1[1][1] * flWeight1;
			result[1][2] = boneMat0[1][2] * flWeight0 + boneMat1[1][2] * flWeight1;
			result[1][3] = boneMat0[1][3] * flWeight0 + boneMat1[1][3] * flWeight1;
			result[2][0] = boneMat0[2][0] * flWeight0 + boneMat1[2][0] * flWeight1;
			result[2][1] = boneMat0[2][1] * flWeight0 + boneMat1[2][1] * flWeight1;
			result[2][2] = boneMat0[2][2] * flWeight0 + boneMat1[2][2] * flWeight1;
			result[2][3] = boneMat0[2][3] * flWeight0 + boneMat1[2][3] * flWeight1;
		}
		return;

	case 3:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			matrix3x4_t &boneMat2 = pPoseToWorld[boneweights.bone[2]];
			flWeight0 = boneweights.weight[0];
			flWeight1 = boneweights.weight[1];
			flWeight2 = boneweights.weight[2];

			result[0][0] = boneMat0[0][0] * flWeight0 + boneMat1[0][0] * flWeight1 + boneMat2[0][0] * flWeight2;
			result[0][1] = boneMat0[0][1] * flWeight0 + boneMat1[0][1] * flWeight1 + boneMat2[0][1] * flWeight2;
			result[0][2] = boneMat0[0][2] * flWeight0 + boneMat1[0][2] * flWeight1 + boneMat2[0][2] * flWeight2;
			result[0][3] = boneMat0[0][3] * flWeight0 + boneMat1[0][3] * flWeight1 + boneMat2[0][3] * flWeight2;
			result[1][0] = boneMat0[1][0] * flWeight0 + boneMat1[1][0] * flWeight1 + boneMat2[1][0] * flWeight2;
			result[1][1] = boneMat0[1][1] * flWeight0 + boneMat1[1][1] * flWeight1 + boneMat2[1][1] * flWeight2;
			result[1][2] = boneMat0[1][2] * flWeight0 + boneMat1[1][2] * flWeight1 + boneMat2[1][2] * flWeight2;
			result[1][3] = boneMat0[1][3] * flWeight0 + boneMat1[1][3] * flWeight1 + boneMat2[1][3] * flWeight2;
			result[2][0] = boneMat0[2][0] * flWeight0 + boneMat1[2][0] * flWeight1 + boneMat2[2][0] * flWeight2;
			result[2][1] = boneMat0[2][1] * flWeight0 + boneMat1[2][1] * flWeight1 + boneMat2[2][1] * flWeight2;
			result[2][2] = boneMat0[2][2] * flWeight0 + boneMat1[2][2] * flWeight1 + boneMat2[2][2] * flWeight2;
			result[2][3] = boneMat0[2][3] * flWeight0 + boneMat1[2][3] * flWeight1 + boneMat2[2][3] * flWeight2;
		}
		return;

	case 4:
		{
			Assert( 0 ); // results undefined for numbones == 4, as MAX_NUM_BONES_PER_VERT is 3
		}
		return;
	}

	Assert(0);
}

void ComputeSkinMatrixToMemorySSE( mstudioboneweight_t &boneweights, matrix3x4_t *pPoseToWorld, matrix3x4_t &result )
{
	// NOTE: pPoseToWorld, being cache aligned, doesn't need explicit initialization
#if defined( _WIN32 ) && !defined( _WIN64 ) && !defined( _X360 )
	switch( boneweights.numbones )
	{
	default:
	case 1:
		memcpy( &result, &pPoseToWorld[boneweights.bone[0]], sizeof(matrix3x4_t) );
		return;

	case 2:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			float *pWeights = boneweights.weight;

			_asm
			{
				mov		eax, DWORD PTR [pWeights]
				movss	xmm6, dword ptr[eax]		; boneweights.weight[0]
				movss	xmm7, dword ptr[eax + 4]	; boneweights.weight[1]

				mov		eax, DWORD PTR [boneMat0]
				mov		ecx, DWORD PTR [boneMat1]
				mov		edi, DWORD PTR [result]

				// Fill xmm6, and 7 with all the bone weights
				shufps	xmm6, xmm6, 0
					shufps	xmm7, xmm7, 0

					// Load up all rows of the three matrices
					movaps	xmm0, XMMWORD PTR [eax]
				movaps	xmm1, XMMWORD PTR [ecx]
				movaps	xmm2, XMMWORD PTR [eax + 16]
				movaps	xmm3, XMMWORD PTR [ecx + 16]
				movaps	xmm4, XMMWORD PTR [eax + 32]
				movaps	xmm5, XMMWORD PTR [ecx + 32]

				// Multiply the rows by the weights
				mulps	xmm0, xmm6
					mulps	xmm1, xmm7
					mulps	xmm2, xmm6
					mulps	xmm3, xmm7
					mulps	xmm4, xmm6
					mulps	xmm5, xmm7

					addps	xmm0, xmm1
					addps	xmm2, xmm3
					addps	xmm4, xmm5

					movaps	XMMWORD PTR [edi], xmm0
					movaps	XMMWORD PTR [edi + 16], xmm2
					movaps	XMMWORD PTR [edi + 32], xmm4
			}
		}

	case 3:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			matrix3x4_t &boneMat2 = pPoseToWorld[boneweights.bone[2]];
			float *pWeights = boneweights.weight;

			_asm
			{
				mov		eax, DWORD PTR [pWeights]
				movss	xmm5, dword ptr[eax]		; boneweights.weight[0]
				movss	xmm6, dword ptr[eax + 4]	; boneweights.weight[1]
				movss	xmm7, dword ptr[eax + 8]	; boneweights.weight[2]

				mov		eax, DWORD PTR [boneMat0]
				mov		ecx, DWORD PTR [boneMat1]
				mov		edx, DWORD PTR [boneMat2]
				mov		edi, DWORD PTR [result]

				// Fill xmm5, 6, and 7 with all the bone weights
				shufps	xmm5, xmm5, 0
					shufps	xmm6, xmm6, 0
					shufps	xmm7, xmm7, 0

					// Load up the first row of the three matrices
					movaps	xmm0, XMMWORD PTR [eax]
				movaps	xmm1, XMMWORD PTR [ecx]
				movaps	xmm2, XMMWORD PTR [edx]

				// Multiply the rows by the weights
				mulps	xmm0, xmm5
					mulps	xmm1, xmm6
					mulps	xmm2, xmm7

					addps	xmm0, xmm1
					addps	xmm0, xmm2
					movaps	XMMWORD PTR [edi], xmm0

					// Load up the second row of the three matrices
					movaps	xmm0, XMMWORD PTR [eax + 16]
				movaps	xmm1, XMMWORD PTR [ecx + 16]
				movaps	xmm2, XMMWORD PTR [edx + 16]

				// Multiply the rows by the weights
				mulps	xmm0, xmm5
					mulps	xmm1, xmm6
					mulps	xmm2, xmm7

					addps	xmm0, xmm1
					addps	xmm0, xmm2
					movaps	XMMWORD PTR [edi + 16], xmm0	

					// Load up the third row of the three matrices
					movaps	xmm0, XMMWORD PTR [eax + 32]
				movaps	xmm1, XMMWORD PTR [ecx + 32]
				movaps	xmm2, XMMWORD PTR [edx + 32]

				// Multiply the rows by the weights
				mulps	xmm0, xmm5
					mulps	xmm1, xmm6
					mulps	xmm2, xmm7

					addps	xmm0, xmm1
					addps	xmm0, xmm2
					movaps	XMMWORD PTR [edi + 32], xmm0	
			}
		}

	case 4:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			matrix3x4_t &boneMat2 = pPoseToWorld[boneweights.bone[2]];
			matrix3x4_t &boneMat3 = pPoseToWorld[boneweights.bone[3]];
			float *pWeights = boneweights.weight;

			_asm
			{
				mov		eax, DWORD PTR [pWeights]
				movss	xmm4, dword ptr[eax]		; boneweights.weight[0]
				movss	xmm5, dword ptr[eax + 4]	; boneweights.weight[1]
				movss	xmm6, dword ptr[eax + 8]	; boneweights.weight[2]
				movss	xmm7, dword ptr[eax + 12]	; boneweights.weight[3]

				mov		eax, DWORD PTR [boneMat0]
				mov		ecx, DWORD PTR [boneMat1]
				mov		edx, DWORD PTR [boneMat2]
				mov		esi, DWORD PTR [boneMat3]
				mov		edi, DWORD PTR [result]

				// Fill xmm5, 6, and 7 with all the bone weights
				shufps	xmm4, xmm4, 0
					shufps	xmm5, xmm5, 0
					shufps	xmm6, xmm6, 0
					shufps	xmm7, xmm7, 0

					// Load up the first row of the four matrices
					movaps	xmm0, XMMWORD PTR [eax]
				movaps	xmm1, XMMWORD PTR [ecx]
				movaps	xmm2, XMMWORD PTR [edx]
				movaps	xmm3, XMMWORD PTR [esi]

				// Multiply the rows by the weights
				mulps	xmm0, xmm4
					mulps	xmm1, xmm5
					mulps	xmm2, xmm6
					mulps	xmm3, xmm7

					addps	xmm0, xmm1
					addps	xmm2, xmm3
					addps	xmm0, xmm2
					movaps	XMMWORD PTR [edi], xmm0

					// Load up the second row of the three matrices
					movaps	xmm0, XMMWORD PTR [eax + 16]
				movaps	xmm1, XMMWORD PTR [ecx + 16]
				movaps	xmm2, XMMWORD PTR [edx + 16]
				movaps	xmm3, XMMWORD PTR [esi + 16]

				// Multiply the rows by the weights
				mulps	xmm0, xmm4
					mulps	xmm1, xmm5
					mulps	xmm2, xmm6
					mulps	xmm3, xmm7

					addps	xmm0, xmm1
					addps	xmm2, xmm3
					addps	xmm0, xmm2
					movaps	XMMWORD PTR [edi + 16], xmm0	

					// Load up the third row of the three matrices
					movaps	xmm0, XMMWORD PTR [eax + 32]
				movaps	xmm1, XMMWORD PTR [ecx + 32]
				movaps	xmm2, XMMWORD PTR [edx + 32]
				movaps	xmm3, XMMWORD PTR [esi + 32]

				// Multiply the rows by the weights
				mulps	xmm0, xmm4
					mulps	xmm1, xmm5
					mulps	xmm2, xmm6
					mulps	xmm3, xmm7

					addps	xmm0, xmm1
					addps	xmm2, xmm3
					addps	xmm0, xmm2
					movaps	XMMWORD PTR [edi + 32], xmm0	
			}
		}
	}
#elif POSIX || _WIN64
	ComputeSkinMatrixToMemory( boneweights, pPoseToWorld, result );
#elif defined( _X360 )
	ComputeSkinMatrixToMemory( boneweights, pPoseToWorld, result );
#endif
}

matrix3x4_t *ComputeSkinMatrixSSE( mstudioboneweight_t &boneweights, matrix3x4_t *pPoseToWorld, matrix3x4_t &scratchMatrix )
{
	// NOTE: pPoseToWorld, being cache aligned, doesn't need explicit initialization
#if defined( _WIN32 ) && !defined( _WIN64 ) && !defined( _X360 )
	switch( boneweights.numbones )
	{
	default:
	case 1:
		return &pPoseToWorld[boneweights.bone[0]];

	case 2:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			float *pWeights = boneweights.weight;

			_asm
			{
				mov		eax, DWORD PTR [pWeights]
				movss	xmm6, dword ptr[eax]		; boneweights.weight[0]
				movss	xmm7, dword ptr[eax + 4]	; boneweights.weight[1]

				mov		eax, DWORD PTR [boneMat0]
				mov		ecx, DWORD PTR [boneMat1]
				mov		edi, DWORD PTR [scratchMatrix]

				// Fill xmm6, and 7 with all the bone weights
				shufps	xmm6, xmm6, 0
				shufps	xmm7, xmm7, 0

				// Load up all rows of the three matrices
				movaps	xmm0, XMMWORD PTR [eax]
				movaps	xmm1, XMMWORD PTR [ecx]
				movaps	xmm2, XMMWORD PTR [eax + 16]
				movaps	xmm3, XMMWORD PTR [ecx + 16]
				movaps	xmm4, XMMWORD PTR [eax + 32]
				movaps	xmm5, XMMWORD PTR [ecx + 32]

				// Multiply the rows by the weights
				mulps	xmm0, xmm6
				mulps	xmm1, xmm7
				mulps	xmm2, xmm6
				mulps	xmm3, xmm7
				mulps	xmm4, xmm6
				mulps	xmm5, xmm7

				addps	xmm0, xmm1
				addps	xmm2, xmm3
				addps	xmm4, xmm5

				movaps	XMMWORD PTR [edi], xmm0
				movaps	XMMWORD PTR [edi + 16], xmm2
				movaps	XMMWORD PTR [edi + 32], xmm4
			}
		}
		return &scratchMatrix;

	case 3:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			matrix3x4_t &boneMat2 = pPoseToWorld[boneweights.bone[2]];
			float *pWeights = boneweights.weight;

			_asm
			{
				mov		eax, DWORD PTR [pWeights]
				movss	xmm5, dword ptr[eax]		; boneweights.weight[0]
				movss	xmm6, dword ptr[eax + 4]	; boneweights.weight[1]
				movss	xmm7, dword ptr[eax + 8]	; boneweights.weight[2]

				mov		eax, DWORD PTR [boneMat0]
				mov		ecx, DWORD PTR [boneMat1]
				mov		edx, DWORD PTR [boneMat2]
				mov		edi, DWORD PTR [scratchMatrix]

				// Fill xmm5, 6, and 7 with all the bone weights
				shufps	xmm5, xmm5, 0
				shufps	xmm6, xmm6, 0
				shufps	xmm7, xmm7, 0

				// Load up the first row of the three matrices
				movaps	xmm0, XMMWORD PTR [eax]
				movaps	xmm1, XMMWORD PTR [ecx]
				movaps	xmm2, XMMWORD PTR [edx]

				// Multiply the rows by the weights
				mulps	xmm0, xmm5
				mulps	xmm1, xmm6
				mulps	xmm2, xmm7

				addps	xmm0, xmm1
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi], xmm0
				
				// Load up the second row of the three matrices
				movaps	xmm0, XMMWORD PTR [eax + 16]
				movaps	xmm1, XMMWORD PTR [ecx + 16]
				movaps	xmm2, XMMWORD PTR [edx + 16]

				// Multiply the rows by the weights
				mulps	xmm0, xmm5
				mulps	xmm1, xmm6
				mulps	xmm2, xmm7

				addps	xmm0, xmm1
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi + 16], xmm0	

				// Load up the third row of the three matrices
				movaps	xmm0, XMMWORD PTR [eax + 32]
				movaps	xmm1, XMMWORD PTR [ecx + 32]
				movaps	xmm2, XMMWORD PTR [edx + 32]

				// Multiply the rows by the weights
				mulps	xmm0, xmm5
				mulps	xmm1, xmm6
				mulps	xmm2, xmm7

				addps	xmm0, xmm1
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi + 32], xmm0	
			}
		}
		return &scratchMatrix;

	case 4:
		{
			matrix3x4_t &boneMat0 = pPoseToWorld[boneweights.bone[0]];
			matrix3x4_t &boneMat1 = pPoseToWorld[boneweights.bone[1]];
			matrix3x4_t &boneMat2 = pPoseToWorld[boneweights.bone[2]];
			matrix3x4_t &boneMat3 = pPoseToWorld[boneweights.bone[3]];
			float *pWeights = boneweights.weight;

			_asm
			{
				mov		eax, DWORD PTR [pWeights]
				movss	xmm4, dword ptr[eax]		; boneweights.weight[0]
				movss	xmm5, dword ptr[eax + 4]	; boneweights.weight[1]
				movss	xmm6, dword ptr[eax + 8]	; boneweights.weight[2]
				movss	xmm7, dword ptr[eax + 12]	; boneweights.weight[3]

				mov		eax, DWORD PTR [boneMat0]
				mov		ecx, DWORD PTR [boneMat1]
				mov		edx, DWORD PTR [boneMat2]
				mov		esi, DWORD PTR [boneMat3]
				mov		edi, DWORD PTR [scratchMatrix]

				// Fill xmm5, 6, and 7 with all the bone weights
				shufps	xmm4, xmm4, 0
				shufps	xmm5, xmm5, 0
				shufps	xmm6, xmm6, 0
				shufps	xmm7, xmm7, 0

				// Load up the first row of the four matrices
				movaps	xmm0, XMMWORD PTR [eax]
				movaps	xmm1, XMMWORD PTR [ecx]
				movaps	xmm2, XMMWORD PTR [edx]
				movaps	xmm3, XMMWORD PTR [esi]

				// Multiply the rows by the weights
				mulps	xmm0, xmm4
				mulps	xmm1, xmm5
				mulps	xmm2, xmm6
				mulps	xmm3, xmm7

				addps	xmm0, xmm1
				addps	xmm2, xmm3
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi], xmm0
				
				// Load up the second row of the three matrices
				movaps	xmm0, XMMWORD PTR [eax + 16]
				movaps	xmm1, XMMWORD PTR [ecx + 16]
				movaps	xmm2, XMMWORD PTR [edx + 16]
				movaps	xmm3, XMMWORD PTR [esi + 16]

				// Multiply the rows by the weights
				mulps	xmm0, xmm4
				mulps	xmm1, xmm5
				mulps	xmm2, xmm6
				mulps	xmm3, xmm7

				addps	xmm0, xmm1
				addps	xmm2, xmm3
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi + 16], xmm0	

				// Load up the third row of the three matrices
				movaps	xmm0, XMMWORD PTR [eax + 32]
				movaps	xmm1, XMMWORD PTR [ecx + 32]
				movaps	xmm2, XMMWORD PTR [edx + 32]
				movaps	xmm3, XMMWORD PTR [esi + 32]

				// Multiply the rows by the weights
				mulps	xmm0, xmm4
				mulps	xmm1, xmm5
				mulps	xmm2, xmm6
				mulps	xmm3, xmm7

				addps	xmm0, xmm1
				addps	xmm2, xmm3
				addps	xmm0, xmm2
				movaps	XMMWORD PTR [edi + 32], xmm0	
			}
		}
		return &scratchMatrix;
	}
#else
#ifndef LINUX
#pragma message("ComputeSkinMatrixSSE C implementation only")
#endif

return ComputeSkinMatrix( boneweights, pPoseToWorld, scratchMatrix );
#endif

	Assert( 0 );
	return NULL;
}

//-----------------------------------------------------------------------------
// Designed for inter-module draw optimized calling, requires R_InitLightEffectWorld3()
// Compute the lighting at a point and normal
// Uses the set function pointer
// Final lighting is in gamma space
//-----------------------------------------------------------------------------
static lightpos_t lightpos[MAXLOCALLIGHTS];
inline void CStudioRender::R_ComputeLightAtPoint3( const Vector &pos, const Vector &normal, Vector &color )
{
	if ( m_pRC->m_Config.fullbright )
	{
		color.Init( 1.0f, 1.0f, 1.0f );
		return;
	}

	// Set up lightpos[i].dot, lightpos[i].falloff, and lightpos[i].delta for all lights
	R_LightStrengthWorld( pos, m_pRC->m_NumLocalLights, m_pRC->m_LocalLights, lightpos );

	// calculate ambient values from the ambient cube given a normal.
	R_LightAmbient_4D( normal, m_pRC->m_LightBoxColors, color );

	// Calculate color given lightpos_t lightpos, a normal, and the ambient
	// color from the ambient cube calculated above.
	Assert(R_LightEffectsWorld3);
	R_LightEffectsWorld3( m_pRC->m_LocalLights, lightpos, normal, color );
}


// define SPECIAL_SSE_MESH_PROCESSOR to enable code which contains a special optimized SSE lighting loop, significantly
// improving software vertex processing performace.
#if defined( _WIN32 ) && !defined( _X360 )
#define SPECIAL_SSE_MESH_PROCESSOR
#endif

#ifdef SPECIAL_SSE_MESH_PROCESSOR
//#define VERIFY_SSE_LIGHTING

// false: MAX(0,L*N) true: .5*(L.N)+.5. set based on material
static bool SSELightingHalfLambert;							

// These variables are used by the special SSE lighting path. The
// lighting path calculates them everytime it processes a mesh so their
// is no need to keep them in sync with changes to the other light variables
static fltx4 OneOver_ThetaDot_Minus_PhiDot[MAXLOCALLIGHTS]; // 1/(theta-phi)

void CStudioRender::R_MouthLighting( fltx4 fIllum, const FourVectors& normal, const FourVectors& forward, FourVectors &light )
{
	fltx4 dot = SubSIMD(Four_Zeros,normal*forward);
	dot=MaxSIMD(Four_Zeros,dot);
	dot=MulSIMD(fIllum,dot);
	light *= dot;
}

inline void CStudioRender::R_ComputeLightAtPoints3( const FourVectors &pos, const FourVectors &normal, FourVectors &color )
{
	if ( m_pRC->m_Config.fullbright )
	{
		color.DuplicateVector( Vector( 1.0f, 1.0f, 1.0f ) );
		return;
	}

	R_LightAmbient_4D( normal, m_pRC->m_LightBoxColors, color );
	// now, add in contribution from all lights
	for ( int i = 0; i < m_pRC->m_NumLocalLights; i++)
	{
		FourVectors delta;
		LightDesc_t const *wl = m_pRC->m_LocalLights+i;
		Assert((wl->m_Type==MATERIAL_LIGHT_POINT) || (wl->m_Type==MATERIAL_LIGHT_SPOT) || (wl->m_Type==MATERIAL_LIGHT_DIRECTIONAL));
		switch (wl->m_Type)
		{
			case MATERIAL_LIGHT_POINT:
			case MATERIAL_LIGHT_SPOT:
				delta.DuplicateVector(wl->m_Position);
				delta-=pos;
				break;
				
			case MATERIAL_LIGHT_DIRECTIONAL:
				delta.DuplicateVector(wl->m_Direction);
				delta*=-1.0;
				break;
				
		}
		fltx4 falloff = R_WorldLightDistanceFalloff( wl, delta);
		delta.VectorNormalizeFast();
		fltx4 strength=delta*normal;
		if (SSELightingHalfLambert)
		{
			strength=AddSIMD(MulSIMD(strength,Four_PointFives),Four_PointFives);
		}
		else
			strength=MaxSIMD(Four_Zeros,delta*normal);
		
		switch(wl->m_Type)
		{
			case MATERIAL_LIGHT_POINT:
				// half-lambert
				break;
				
 			case MATERIAL_LIGHT_SPOT:
			{
				fltx4 dot2=SubSIMD(Four_Zeros,delta*wl->m_Direction); // dot position with spot light dir for cone falloff

				fltx4 cone_falloff_scale=MulSIMD(OneOver_ThetaDot_Minus_PhiDot[i],
													 SubSIMD(dot2,ReplicateX4(wl->m_PhiDot)));
				cone_falloff_scale=MinSIMD(cone_falloff_scale,Four_Ones);
				if ((wl->m_Falloff!=0.0) && (wl->m_Falloff!=1.0))
				{
					// !!speed!! could compute integer exponent needed by powsimd and store in light
					cone_falloff_scale=PowSIMD(cone_falloff_scale,wl->m_Falloff);
				}
				strength=MulSIMD(cone_falloff_scale,strength);

				// now, zero out lighting where dot2<phidot. This will mask out any invalid results
				// from pow function, etc
 				fltx4 OutsideMask=CmpGtSIMD(dot2,ReplicateX4(wl->m_PhiDot)); // outside light cone?
 				strength=AndSIMD(OutsideMask,strength);
			}
			break;
			
			case MATERIAL_LIGHT_DIRECTIONAL:
				break;

		}
		strength=MulSIMD(strength,falloff);
		color.x=AddSIMD(color.x,MulSIMD(strength,ReplicateX4(wl->m_Color.x)));
		color.y=AddSIMD(color.y,MulSIMD(strength,ReplicateX4(wl->m_Color.y)));
		color.z=AddSIMD(color.z,MulSIMD(strength,ReplicateX4(wl->m_Color.z)));
	}
}

#endif // SPECIAL_SSE_MESH_PROCESSOR

//-----------------------------------------------------------------------------
// Optimized for low-end hardware
//-----------------------------------------------------------------------------
#pragma warning (disable:4701)

// NOTE: I'm using this crazy wrapper because using straight template functions
// doesn't appear to work with function tables 
template< int nHasTangentSpace, int nDoFlex, int nLighting > 
class CProcessMeshWrapper
{
public:

	static void R_TransformVert( const Vector *pSrcPos, const Vector *pSrcNorm, const Vector4D *pSrcTangentS,
		matrix3x4_t *pSkinMat, VectorAligned &pos, Vector &norm, Vector4DAligned &tangentS )
	{
		// NOTE: Could add SSE stuff here, if we knew what SSE stuff could make it faster

		pos.x  = pSrcPos->x  * (*pSkinMat)[0][0] + pSrcPos->y  * (*pSkinMat)[0][1] + pSrcPos->z  * (*pSkinMat)[0][2] + (*pSkinMat)[0][3];
		norm.x = pSrcNorm->x * (*pSkinMat)[0][0] + pSrcNorm->y * (*pSkinMat)[0][1] + pSrcNorm->z * (*pSkinMat)[0][2];

		pos.y  = pSrcPos->x  * (*pSkinMat)[1][0] + pSrcPos->y  * (*pSkinMat)[1][1] + pSrcPos->z  * (*pSkinMat)[1][2] + (*pSkinMat)[1][3];
		norm.y = pSrcNorm->x * (*pSkinMat)[1][0] + pSrcNorm->y * (*pSkinMat)[1][1] + pSrcNorm->z * (*pSkinMat)[1][2];

		pos.z  = pSrcPos->x  * (*pSkinMat)[2][0] + pSrcPos->y  * (*pSkinMat)[2][1] + pSrcPos->z  * (*pSkinMat)[2][2] + (*pSkinMat)[2][3];
		norm.z = pSrcNorm->x * (*pSkinMat)[2][0] + pSrcNorm->y * (*pSkinMat)[2][1] + pSrcNorm->z * (*pSkinMat)[2][2];

		if ( nHasTangentSpace )
		{
			tangentS.x = pSrcTangentS->x * (*pSkinMat)[0][0] + pSrcTangentS->y * (*pSkinMat)[0][1]	+ pSrcTangentS->z * (*pSkinMat)[0][2];
			tangentS.y = pSrcTangentS->x * (*pSkinMat)[1][0] + pSrcTangentS->y * (*pSkinMat)[1][1]	+ pSrcTangentS->z * (*pSkinMat)[1][2];
			tangentS.z = pSrcTangentS->x * (*pSkinMat)[2][0] + pSrcTangentS->y * (*pSkinMat)[2][1]	+ pSrcTangentS->z * (*pSkinMat)[2][2];
			tangentS.w = pSrcTangentS->w;
		}
	}

	static void R_StudioSoftwareProcessMesh( const mstudio_meshvertexdata_t *vertData, matrix3x4_t *pPoseToWorld,
		CCachedRenderData &vertexCache, CMeshBuilder& meshBuilder, int numVertices, unsigned short* pGroupToMesh, unsigned int nAlphaMask,
											 IMaterial* pMaterial)		
	{
		Vector color;
		Vector4D *pStudioTangentS;
		Vector4DAligned tangentS;
		Vector *pSrcPos;
		Vector *pSrcNorm;
		Vector4D *pSrcTangentS = NULL;

		ALIGN16 ModelVertexDX8_t dstVertex;
		dstVertex.m_vecUserData.Init( 1.0f, 0.0f, 0.0f, 1.0f );

		ALIGN16 matrix3x4_t temp;
		ALIGN16 matrix3x4_t *pSkinMat;

		int ntemp[PREFETCH_VERT_COUNT];

		Assert( numVertices > 0 );

		mstudiovertex_t *pVertices = vertData->Vertex( 0 );

		if (nHasTangentSpace)
		{
			pStudioTangentS = vertData->TangentS( 0 );
			Assert( pStudioTangentS->w == -1.0f || pStudioTangentS->w == 1.0f );
		}

		// Mouth related stuff...
		float fIllum = 1.0f;
		Vector forward;
		if (nLighting == LIGHTING_MOUTH)
		{
			g_StudioRender.R_MouthComputeLightingValues( fIllum, forward );
		}

		if ((nLighting == LIGHTING_MOUTH) || (nLighting == LIGHTING_SOFTWARE))
		{
			g_StudioRender.R_InitLightEffectsWorld3();
		}
#ifdef _DEBUG
		// In debug, clear it out to ensure we aren't accidentially calling 
		// the last setup for R_ComputeLightForPoint3.
		else
		{
			g_StudioRender.R_LightEffectsWorld3 = NULL;
		}
#endif

#if defined( _WIN32 ) && !defined( _X360 )
		// Precaches the data
		_mm_prefetch( (char*)((int)pGroupToMesh & (~0x1F)), _MM_HINT_NTA );
#endif
		for ( int i = 0; i < PREFETCH_VERT_COUNT; ++i )
		{
			ntemp[i] = pGroupToMesh[i];
#if defined( _WIN32 ) && !defined( _X360 )
			char *pMem = (char*)&pVertices[ntemp[i]];
			_mm_prefetch( pMem, _MM_HINT_NTA );
			_mm_prefetch( pMem + 32, _MM_HINT_NTA );
			if ( nHasTangentSpace )
			{
				_mm_prefetch( (char*)&pStudioTangentS[ntemp[i]], _MM_HINT_NTA );
			}
#endif
		}

		int n, idx;
		for ( int j=0; j < numVertices; ++j )
		{
#if defined( _WIN32 ) && !defined( _X360 )
			char *pMem = (char*)&pGroupToMesh[j + PREFETCH_VERT_COUNT + 1];
			_mm_prefetch( (char*)((int)pMem & (~0x1F)), _MM_HINT_NTA );
#endif
			idx = j & (PREFETCH_VERT_COUNT-1);
			n = ntemp[idx];

			mstudiovertex_t &vert = pVertices[n];

			ntemp[idx] = pGroupToMesh[j + PREFETCH_VERT_COUNT];

			// Compute the skinning matrix
			pSkinMat = ComputeSkinMatrixSSE( vert.m_BoneWeights, pPoseToWorld, temp );

			// transform into world space
			if ( nDoFlex && vertexCache.IsVertexFlexed(n) )
			{
				CachedPosNormTan_t* pFlexedVertex = vertexCache.GetFlexVertex(n);
				pSrcPos = &pFlexedVertex->m_Position.AsVector3D();
				pSrcNorm = &pFlexedVertex->m_Normal.AsVector3D();

				if ( nHasTangentSpace )
				{
					pSrcTangentS = &pFlexedVertex->m_TangentS;
					Assert( pSrcTangentS->w == -1.0f || pSrcTangentS->w == 1.0f );
				}
			}
			else
			{
				pSrcPos = &vert.m_vecPosition;
				pSrcNorm = &vert.m_vecNormal;

				if ( nHasTangentSpace )
				{
					pSrcTangentS = &pStudioTangentS[n];
					Assert( pSrcTangentS->w == -1.0f || pSrcTangentS->w == 1.0f );
				}
			}

			// Transform the vert into world space
			R_TransformVert( pSrcPos, pSrcNorm, pSrcTangentS, pSkinMat, 
				*(VectorAligned*)&dstVertex.m_vecPosition, dstVertex.m_vecNormal, *(Vector4DAligned*)&dstVertex.m_vecUserData );

#if defined( _WIN32 ) && !defined( _X360 )
			_mm_prefetch( (char*)&pVertices[ntemp[idx]], _MM_HINT_NTA);
			_mm_prefetch( (char*)&pVertices[ntemp[idx]] + 32, _MM_HINT_NTA );
			if ( nHasTangentSpace )
			{
				_mm_prefetch( (char*)&pStudioTangentS[ntemp[idx]], _MM_HINT_NTA );
			}
#endif

			dstVertex.m_vecTexCoord = vert.m_vecTexCoord; 

#if !defined( _X360 )
			Assert( dstVertex.m_vecUserData.w == -1.0f || dstVertex.m_vecUserData.w == 1.0f );
			meshBuilder.FastVertexSSE( dstVertex );
#else
			meshBuilder.VertexDX8ToX360( dstVertex );
#endif
		}
		meshBuilder.FastAdvanceNVertices( numVertices );
	}

#ifdef SPECIAL_SSE_MESH_PROCESSOR

#ifdef VERIFY_SSE_LIGHTING
	static int NotCloseEnough( float a, float b )
	{
		// check if 2 linear lighting values are close enough between the sse and non see lighting model
		// no point being more precise than 1% since it all maps to 8 bit anyway
		float thresh=0.1f*fabs( a );
		if ( thresh < 0.1f )
			thresh = 0.1f;
		return ( fabs( a-b ) > thresh );
	}
#endif

	// this special version of the vertex processor does 4 vertices at once, so that they can be lit using SSE instructions. This provides
	// a >2x speedup in the lit case
	static void R_PerformVectorizedLightingSSE( const FourVectors &forward, fltx4 fIllum, ModelVertexDX8_t *dst, unsigned int nAlphaMask)
	{
		if ( nLighting == LIGHTING_SOFTWARE )
		{
#ifdef VERIFY_SSE_LIGHTING
// 			if ( (g_StudioRender.m_NumLocalLights==1) &&
// 				 ( (g_StudioRender.m_LocalLights[0].m_Type==MATERIAL_LIGHT_SPOT)))
// 			{
// 				// ihvtest doesn't use different exponents for its spots,
// 				// so i mess with the exponents when testing
// 				static int ctr=0;
// 				static float exps[8]={0,1,2,3,4,4.5,5.25,2.5};
// 				ctr=(ctr+1)&7;
// 				g_StudioRender.m_LocalLights[0].m_Falloff=exps[ctr];
// 			}
#endif
			FourVectors Position;
			Position.LoadAndSwizzleAligned(dst[0].m_vecPosition,dst[1].m_vecPosition,dst[2].m_vecPosition,dst[3].m_vecPosition);
			FourVectors Normal(dst[0].m_vecNormal,dst[1].m_vecNormal,dst[2].m_vecNormal,dst[3].m_vecNormal);
			FourVectors Color;
			g_StudioRender.R_ComputeLightAtPoints3( Position, Normal, Color);

			for (int i=0; i<4; i++)
			{
				Vector color;
#ifdef VERIFY_SSE_LIGHTING
				// debug - check sse version against "real" version
				g_StudioRender.R_ComputeLightAtPoint3( dst[i].m_vecPosition,dst[i].m_vecNormal, color );
				if ( NotCloseEnough(color.x,Color.X(i)) ||
					 NotCloseEnough(color.y,Color.Y(i)) ||
					 NotCloseEnough(color.z,Color.Z(i)))
				{
					Assert(0);
					// recompute so can step in debugger
					g_StudioRender.R_ComputeLightAtPoints3( Position,Normal,Color);
					g_StudioRender.R_ComputeLightAtPoint3( dst[i].m_vecPosition,dst[i].m_vecNormal, color );
				}
#endif
				unsigned char r = LinearToLightmap( Color.X(i) );
				unsigned char g = LinearToLightmap( Color.Y(i) );
				unsigned char b = LinearToLightmap( Color.Z(i) );
				
				dst[i].m_nColor = b | (g << 8) | (r << 16) | nAlphaMask;
			}
		}
		else if ( nLighting == LIGHTING_MOUTH )
		{
			FourVectors Position;
			Position.LoadAndSwizzleAligned(dst[0].m_vecPosition,dst[1].m_vecPosition,dst[2].m_vecPosition,dst[3].m_vecPosition);
			FourVectors Normal(dst[0].m_vecNormal,dst[1].m_vecNormal,dst[2].m_vecNormal,dst[3].m_vecNormal);
			FourVectors Color;

			g_StudioRender.R_ComputeLightAtPoints3( Position, Normal, Color);
			g_StudioRender.R_MouthLighting( fIllum, Normal, forward, Color );
			for (int i=0; i<4; i++)
			{
				unsigned char r = LinearToLightmap( Color.X(i) );
				unsigned char g = LinearToLightmap( Color.Y(i) );
				unsigned char b = LinearToLightmap( Color.Z(i) );
				
				dst[i].m_nColor = b | (g << 8) | (r << 16) | nAlphaMask;
			}
		}
	}
#endif // SPECIAL_SSE_MESH_PROCESSOR
};


//-----------------------------------------------------------------------------
// Draws the mesh using software vertex transformation
//-----------------------------------------------------------------------------
typedef CProcessMeshWrapper< false, false, LIGHTING_HARDWARE >		ProcessMesh00H_t;
typedef CProcessMeshWrapper< false, false, LIGHTING_SOFTWARE >		ProcessMesh00S_t;
typedef CProcessMeshWrapper< false, false, LIGHTING_MOUTH >			ProcessMesh00M_t;

typedef CProcessMeshWrapper< false, true, LIGHTING_HARDWARE >		ProcessMesh01H_t;
typedef CProcessMeshWrapper< false, true, LIGHTING_SOFTWARE >		ProcessMesh01S_t;
typedef CProcessMeshWrapper< false, true, LIGHTING_MOUTH >			ProcessMesh01M_t;

typedef CProcessMeshWrapper< true, false, LIGHTING_HARDWARE >		ProcessMesh10H_t;
typedef CProcessMeshWrapper< true, false, LIGHTING_SOFTWARE >		ProcessMesh10S_t;
typedef CProcessMeshWrapper< true, false, LIGHTING_MOUTH >			ProcessMesh10M_t;

typedef CProcessMeshWrapper< true, true, LIGHTING_HARDWARE >		ProcessMesh11H_t;
typedef CProcessMeshWrapper< true, true, LIGHTING_SOFTWARE >		ProcessMesh11S_t;
typedef CProcessMeshWrapper< true, true, LIGHTING_MOUTH >			ProcessMesh11M_t;

static SoftwareProcessMeshFunc_t g_SoftwareProcessMeshFunc[] =
{
	ProcessMesh00H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh00S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh00M_t::R_StudioSoftwareProcessMesh,
	ProcessMesh01H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh01S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh01M_t::R_StudioSoftwareProcessMesh,
	ProcessMesh10H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh10S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh10M_t::R_StudioSoftwareProcessMesh,
	ProcessMesh11H_t::R_StudioSoftwareProcessMesh,
	ProcessMesh11S_t::R_StudioSoftwareProcessMesh,
	ProcessMesh11M_t::R_StudioSoftwareProcessMesh,
};

inline const mstudio_meshvertexdata_t * GetFatVertexData( mstudiomesh_t * pMesh, studiohdr_t * pStudioHdr )
{
	if ( !pMesh->pModel()->CacheVertexData( pStudioHdr ) )
	{
		// not available yet
		return NULL;
	}
	const mstudio_meshvertexdata_t *pVertData = pMesh->GetVertexData( pStudioHdr );
	Assert( pVertData );
	if ( !pVertData )
	{
		static unsigned int warnCount = 0;
		if ( warnCount++ < 20 )
			Warning( "ERROR: model verts have been compressed or you don't have them in memory on a console, cannot render! (use \"-no_compressed_vvds\")" );
	}
	return pVertData;
}

void CStudioRender::R_StudioSoftwareProcessMesh( mstudiomesh_t* pmesh, CMeshBuilder& meshBuilder, 
		int numVertices, unsigned short* pGroupToMesh, StudioModelLighting_t lighting, bool doFlex, float r_blend,
		bool bNeedsTangentSpace, IMaterial *pMaterial )
{
	unsigned int nAlphaMask = RoundFloatToInt( r_blend * 255.0f ); 
	nAlphaMask = clamp( nAlphaMask, (uint)0, (uint)255 );
	nAlphaMask <<= 24;

	// FIXME: Use function pointers to simplify this?!?
	int idx = bNeedsTangentSpace * 6 + doFlex * 3 + lighting;

	const mstudio_meshvertexdata_t *pVertData = GetFatVertexData( pmesh, m_pStudioHdr );
	if ( pVertData )
	{
		// invoke the software mesh processing handler
		g_SoftwareProcessMeshFunc[idx]( pVertData, m_PoseToWorld, m_VertexCache, meshBuilder, numVertices, pGroupToMesh, nAlphaMask, pMaterial ); 
	}
}

static void R_SlowTransformVert( const Vector *pSrcPos, const Vector *pSrcNorm,
	matrix3x4_t *pSkinMat, VectorAligned &pos, VectorAligned &norm )
{
	pos.x  = pSrcPos->x *  (*pSkinMat)[0][0] + pSrcPos->y *  (*pSkinMat)[0][1] + pSrcPos->z *  (*pSkinMat)[0][2] + (*pSkinMat)[0][3];
	norm.x = pSrcNorm->x * (*pSkinMat)[0][0] + pSrcNorm->y * (*pSkinMat)[0][1] + pSrcNorm->z * (*pSkinMat)[0][2];

	pos.y  = pSrcPos->x *  (*pSkinMat)[1][0] + pSrcPos->y *  (*pSkinMat)[1][1] + pSrcPos->z *  (*pSkinMat)[1][2] + (*pSkinMat)[1][3];
	norm.y = pSrcNorm->x * (*pSkinMat)[1][0] + pSrcNorm->y * (*pSkinMat)[1][1] + pSrcNorm->z * (*pSkinMat)[1][2];

	pos.z  = pSrcPos->x *  (*pSkinMat)[2][0] + pSrcPos->y *  (*pSkinMat)[2][1] + pSrcPos->z *  (*pSkinMat)[2][2] + (*pSkinMat)[2][3];
	norm.z = pSrcNorm->x * (*pSkinMat)[2][0] + pSrcNorm->y * (*pSkinMat)[2][1] + pSrcNorm->z * (*pSkinMat)[2][2];
}

static void R_SlowTransformVert( const Vector *pSrcPos, const Vector *pSrcNorm, const Vector4D *pSrcTangentS,
	matrix3x4_t *pSkinMat, VectorAligned &pos, VectorAligned &norm, VectorAligned &tangentS )
{
	pos.x      = pSrcPos->x *      (*pSkinMat)[0][0] + pSrcPos->y *      (*pSkinMat)[0][1] + pSrcPos->z *      (*pSkinMat)[0][2] + (*pSkinMat)[0][3];
	norm.x     = pSrcNorm->x *     (*pSkinMat)[0][0] + pSrcNorm->y *     (*pSkinMat)[0][1] + pSrcNorm->z *     (*pSkinMat)[0][2];
	tangentS.x = pSrcTangentS->x * (*pSkinMat)[0][0] + pSrcTangentS->y * (*pSkinMat)[0][1] + pSrcTangentS->z * (*pSkinMat)[0][2];

	pos.y      = pSrcPos->x *      (*pSkinMat)[1][0] + pSrcPos->y *      (*pSkinMat)[1][1] + pSrcPos->z *      (*pSkinMat)[1][2] + (*pSkinMat)[1][3];
	norm.y     = pSrcNorm->x *     (*pSkinMat)[1][0] + pSrcNorm->y *     (*pSkinMat)[1][1] + pSrcNorm->z *     (*pSkinMat)[1][2];
	tangentS.y = pSrcTangentS->x * (*pSkinMat)[1][0] + pSrcTangentS->y * (*pSkinMat)[1][1] + pSrcTangentS->z * (*pSkinMat)[1][2];

	pos.z      = pSrcPos->x *      (*pSkinMat)[2][0] + pSrcPos->y *      (*pSkinMat)[2][1] + pSrcPos->z *      (*pSkinMat)[2][2] + (*pSkinMat)[2][3];
	norm.z     = pSrcNorm->x *     (*pSkinMat)[2][0] + pSrcNorm->y *     (*pSkinMat)[2][1] + pSrcNorm->z *     (*pSkinMat)[2][2];
	tangentS.z = pSrcTangentS->x * (*pSkinMat)[2][0] + pSrcTangentS->y * (*pSkinMat)[2][1] + pSrcTangentS->z * (*pSkinMat)[2][2];
}

void CStudioRender::R_StudioSoftwareProcessMesh_NormalsBatched(IMatRenderContext *pRenderContext, mstudiomesh_t* pmesh, studiomeshgroup_t* pGroup,
		StudioModelLighting_t lighting, bool doFlex, float r_blend, bool bShowNormals, bool bShowTangent )
{
	//Batch up and render normals and tangents so that we don't blow the maximum vertex buffer size.
	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh( false );

	int nMaxVertices, nMaxIndices;	
	pRenderContext->GetMaxToRender( pMesh, false, &nMaxVertices, &nMaxIndices );
	int numPrimativesPerVertex = ( (bShowNormals ? 1 : 0) + (bShowTangent ? 2 : 0) );
	
	if ( numPrimativesPerVertex == 0 )
	{
		return;
	}

	int numLineSegVertsPerVertex = 2 * numPrimativesPerVertex;
	const int maxVertsPerPass = nMaxVertices / numLineSegVertsPerVertex;
	
	int startVertex = 0;
	int numVertsLeftToDraw = pGroup->m_NumVertices;
	while ( numVertsLeftToDraw > 0 )
	{
		int numVertsInThisPass = numVertsLeftToDraw;
		if ( numVertsInThisPass > maxVertsPerPass )
		{
			numVertsInThisPass = maxVertsPerPass;
		}

		meshBuilder.Begin( pMesh, MATERIAL_LINES, numVertsInThisPass * numPrimativesPerVertex  );
		R_StudioSoftwareProcessMesh_Normals( pmesh, meshBuilder, startVertex, numVertsInThisPass, pGroup->m_pGroupIndexToMeshIndex, lighting, doFlex, r_blend, bShowNormals, bShowTangent, bShowTangent );
		meshBuilder.End( );
		pMesh->Draw();

		numVertsLeftToDraw -= numVertsInThisPass;
		startVertex += numVertsInThisPass;
	}

}

void CStudioRender::R_StudioSoftwareProcessMesh_Normals( mstudiomesh_t* pmesh, CMeshBuilder& meshBuilder, int startVertex,
		int numVertices, unsigned short* pGroupToMesh, StudioModelLighting_t lighting, bool doFlex, float r_blend,
		bool bShowNormals, bool bShowTangentS, bool bShowTangentT )
{
	ALIGN16 matrix3x4_t temp;
	ALIGN16 matrix3x4_t *pSkinMat;

	Vector *pSrcPos = NULL;
	Vector *pSrcNorm = NULL;
	Vector4D *pSrcTangentS = NULL;
	VectorAligned norm, pos, tangentS, tangentT;

	// Gets at the vertex data
	const mstudio_meshvertexdata_t *vertData = GetFatVertexData( pmesh, m_pStudioHdr );
	if ( !vertData )
	{
		// not available
		return;
	}

	// Don't even try to show tangent data if we don't have any
	if ( !vertData->HasTangentData() )
	{
		bShowTangentS = bShowTangentT = false;
	}

	mstudiovertex_t *pVertices = vertData->Vertex( 0 );

	Vector4D *pTangentS = NULL;
	Vector4D tang;
	if ( bShowTangentS || bShowTangentT )
	{
		pTangentS = vertData->TangentS( 0 );
	}

	for ( int j=startVertex; j < startVertex + numVertices; j++ )
	{
		int n = pGroupToMesh[j];

		mstudiovertex_t &vert = pVertices[n];
		if ( bShowTangentS || bShowTangentT )
		{
			tang = pTangentS[n];
		}

		pSkinMat = ComputeSkinMatrix( vert.m_BoneWeights, m_PoseToWorld, temp );

		// transform into world space
		if ( m_VertexCache.IsVertexFlexed(n) )
		{
			CachedPosNormTan_t* pFlexedVertex = m_VertexCache.GetFlexVertex(n);
			pSrcPos = &pFlexedVertex->m_Position.AsVector3D();
			pSrcNorm = &pFlexedVertex->m_Normal.AsVector3D();

			if ( bShowTangentS || bShowTangentT )
			{
				pSrcTangentS = &pFlexedVertex->m_TangentS;
			}
		}
		else
		{
			pSrcPos = &vert.m_vecPosition;
			pSrcNorm = &vert.m_vecNormal;
			if ( bShowTangentS || bShowTangentT )
			{
				pSrcTangentS = &tang;
			}
		}

		// Transform the vert into world space
		if ( ( bShowTangentS || bShowTangentT ) && ( pSrcTangentS != NULL ) )
		{
			R_SlowTransformVert( pSrcPos, pSrcNorm, pSrcTangentS, pSkinMat, pos, norm, tangentS );
		}
		else
		{
			R_SlowTransformVert( pSrcPos, pSrcNorm, pSkinMat, pos, norm );
		}

		if ( bShowNormals )
		{
			meshBuilder.Position3fv( pos.Base() );
			meshBuilder.Color3f( 0.0f, 0.0f, 1.0f );
			meshBuilder.AdvanceVertex();

			Vector normalPos;
			normalPos = pos + norm * 0.5f;
			meshBuilder.Position3fv( normalPos.Base() );
			meshBuilder.Color3f( 0.0f, 0.0f, 1.0f );
			meshBuilder.AdvanceVertex();
		}

		if ( ( bShowTangentS || bShowTangentT ) && ( pSrcTangentS != NULL) )
		{
			if ( bShowTangentS )
			{
				meshBuilder.Position3fv( pos.Base() );
				meshBuilder.Color3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.AdvanceVertex();

				Vector vTangentSPos;
				vTangentSPos = pos + tangentS * 0.5f;
				meshBuilder.Position3fv( vTangentSPos.Base() );
				meshBuilder.Color3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.AdvanceVertex();
			}

			if ( bShowTangentT )
			{
				meshBuilder.Position3fv( pos.Base() );
				meshBuilder.Color3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.AdvanceVertex();

				// Compute tangentT from normal and tangentS
				CrossProduct( norm, tangentS, tangentT );

				Vector vTangentTPos;
				vTangentTPos = pos + tangentT * 0.5f;
				meshBuilder.Position3fv( vTangentTPos.Base() );
				meshBuilder.Color3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.AdvanceVertex();
			}

		} // end tacking on tangentS and tangetT line segments
	}
}

#pragma warning (default:4701)


static int r_studioProcess_maxVerts = 100;

//DLL_IMPORT CLinkedMiniProfiler *g_pOtherMiniProfilers;
//CLinkedMiniProfiler g_mp_flexV("flexV", &g_pOtherMiniProfilers);
//CLinkedMiniProfiler g_mp_flexW("flexW", &g_pOtherMiniProfilers);

void CStudioRender::R_StudioProcessFlexedMesh_StreamOffset( mstudiomesh_t* pmesh, int lod )
{
	VPROF_BUDGET( "ProcessFlexedMesh_SO", _T("HW_Morphing") );

	if ( m_VertexCache.IsFlexComputationDone() )
		return;

	int vertCount = pmesh->vertexdata.numLODVertexes[lod];
	m_VertexCache.SetupComputation( pmesh, true );
	mstudioflex_t *pflex = pmesh->pFlex( 0 );

	for (int i = 0; i < pmesh->numflexes; i++)
	{
		float w1 = RampFlexWeight( pflex[i], m_pFlexWeights[ pflex[i].flexdesc ] );
		float w2 = RampFlexWeight( pflex[i], m_pFlexDelayedWeights[ pflex[i].flexdesc ] );

		float w3, w4;
		if ( pflex[i].flexpair != 0)
		{
			w3 = RampFlexWeight( pflex[i], m_pFlexWeights[ pflex[i].flexpair ] );
			w4 = RampFlexWeight( pflex[i], m_pFlexDelayedWeights[ pflex[i].flexpair ] );
		}
		else
		{
			w3 = w1;
			w4 = w2;
		}

		// Move on if the weights for this flex are sufficiently small
		if (w1 > -0.001 && w1 < 0.001 && w2 > -0.001 && w2 < 0.001)
		{
			if (w3 > -0.001 && w3 < 0.001 && w4 > -0.001 && w4 < 0.001)
			{
				continue;
			}
		}

		if ( pflex[i].vertanimtype == STUDIO_VERT_ANIM_NORMAL )
		{
			//CMiniProfilerGuard mpguard(&g_mp_flexV,pflex[i].numverts);
			// the most likely path
			mstudiovertanim_t *pvanim = pflex[i].pVertanim( 0 );
#if defined(TEST_DUMP_BIG_FLEXES)
			if(pflex[i].numverts > r_studioProcess_maxVerts)
			{
				int numVerts = pflex[i].numverts;
				r_studioProcess_maxVerts = numVerts; 
				char szFileName[64];
				sprintf(szFileName, "d:\\BigFlex%u.gl", r_studioProcess_maxVerts);
				FileHandle_t fh = g_pFullFileSystem->Open(szFileName, "wt");
				const mstudio_meshvertexdata_t * pVertData = pmesh->GetVertexData(m_pStudioHdr);
				g_pFullFileSystem->FPrintf(fh, "// %d vertices, here goes:\n", numVerts);
				for(int i = 0; i < numVerts; ++i)
				{
					int vertIndex = pvanim[i].index;
					Vector pos = *pVertData->Position(vertIndex);

					Vector flexPos = pos + pvanim[i].GetDeltaFloat() + *(pVertData->Normal(vertIndex)) * 0.1f;
					g_pFullFileSystem->FPrintf(
						fh,
						"2\n%g %g %g 1 1 1\n%g %g %g 1 1 0\n",
						pos.x,pos.y,pos.z,
						flexPos.x, flexPos.y, flexPos.z
					);
				}
				g_pFullFileSystem->Close(fh);
			}
#endif
			m_VertexCache.ComputeFlexedVertex_StreamOffset_Optimized( m_pStudioHdr, &pflex[i], pvanim, vertCount, w1, w2, w3, w4 );
		}
		else
		{
			//CMiniProfilerGuard mpguard(&g_mp_flexW,pflex[i].numverts);
			mstudiovertanim_wrinkle_t *pvanim = pflex[i].pVertanimWrinkle( 0 );
			m_VertexCache.ComputeFlexedVertexWrinkle_StreamOffset_Optimized( m_pStudioHdr, &pflex[i], pvanim, vertCount, w1, w2, w3, w4 );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//  ** Only execute this function if device supports stream offset **
//
// Input  : pGroup - pointer to a studio mesh group
// Output : none
//-----------------------------------------------------------------------------
void CStudioRender::R_StudioFlexMeshGroup( studiomeshgroup_t *pGroup )
{
	VPROF_BUDGET( "R_StudioFlexMeshGroup", VPROF_BUDGETGROUP_MODEL_RENDERING );

	CMeshBuilder meshBuilder;
	int nVertexOffsetInBytes = 0;
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	IMesh *pMesh = pRenderContext->GetFlexMesh();
	meshBuilder.Begin( pMesh, MATERIAL_HETEROGENOUS, pGroup->m_NumVertices, 0, &nVertexOffsetInBytes );

	// Just pos and norm deltas (tangents use same deltas as normals)
	for ( int j=0; j < pGroup->m_NumVertices; j++)
	{
		int n = pGroup->m_pGroupIndexToMeshIndex[j];
		if ( m_VertexCache.IsThinVertexFlexed(n) )
		{
			CachedPosNorm_t *pIn = m_VertexCache.GetThinFlexVertex(n);
			meshBuilder.Position3fv( pIn->m_Position.Base() );
			meshBuilder.NormalDelta3fv( pIn->m_Normal.Base() );
			meshBuilder.Wrinkle1f( pIn->m_Position.w );
		}
		else
		{
			meshBuilder.Position3f( 0.0f, 0.0f, 0.0f );
			meshBuilder.NormalDelta3f( 0.0f, 0.0f, 0.0f );
			meshBuilder.Wrinkle1f( 0.0f );
		}
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS | VTX_HAVENORMAL, 0>();
	}

	meshBuilder.End( false, false );

	pGroup->m_pMesh->SetFlexMesh( pMesh, nVertexOffsetInBytes );
}

//-----------------------------------------------------------------------------
// Processes a flexed mesh to be hw skinned
//-----------------------------------------------------------------------------
void CStudioRender::R_StudioProcessFlexedMesh( mstudiomesh_t* pmesh, CMeshBuilder& meshBuilder, 
											   int numVertices, unsigned short* pGroupToMesh )
{
	PROFILE_STUDIO("FlexMeshBuilder");

	Vector4D *pStudioTangentS;

	// get the vertex data
	const mstudio_meshvertexdata_t *vertData = GetFatVertexData( pmesh, m_pStudioHdr );
	if ( !vertData )
	{
		// not available
		return;
	}
	mstudiovertex_t *pVertices = vertData->Vertex( 0 );

	if ( vertData->HasTangentData() )
	{
		pStudioTangentS = vertData->TangentS( 0 );
		Assert( pStudioTangentS->w == -1.0f || pStudioTangentS->w == 1.0f );

		for ( int j=0; j < numVertices ; j++)
		{
			int n = pGroupToMesh[j];
			mstudiovertex_t &vert = pVertices[n];

			// FIXME: For now, flexed hw-skinned meshes can only have one bone
			// The data must exist in the 0th hardware matrix

			// Here, we are doing HW skinning, so we need to simply copy over the flex
			if ( m_VertexCache.IsVertexFlexed(n) )
			{
				CachedPosNormTan_t* pFlexedVertex = m_VertexCache.GetFlexVertex(n);
				meshBuilder.Position3fv( pFlexedVertex->m_Position.Base() );
				meshBuilder.BoneWeight( 0, 1.0f );
				meshBuilder.BoneWeight( 1, 0.0f );
				meshBuilder.BoneWeight( 2, 0.0f );
				meshBuilder.BoneWeight( 3, 0.0f );
				meshBuilder.BoneMatrix( 0, 0 );
				meshBuilder.BoneMatrix( 1, 0 );
				meshBuilder.BoneMatrix( 2, 0 );
				meshBuilder.BoneMatrix( 3, 0 );
				meshBuilder.Normal3fv( pFlexedVertex->m_Normal.Base() );
				meshBuilder.TexCoord2fv( 0, vert.m_vecTexCoord.Base() );
				Assert( pFlexedVertex->m_TangentS.w == -1.0f || pFlexedVertex->m_TangentS.w == 1.0f );
				meshBuilder.UserData( pFlexedVertex->m_TangentS.Base() );
			}
			else
			{
				meshBuilder.Position3fv( vert.m_vecPosition.Base() );
				meshBuilder.BoneWeight( 0, 1.0f );
				meshBuilder.BoneWeight( 1, 0.0f );
				meshBuilder.BoneWeight( 2, 0.0f );
				meshBuilder.BoneWeight( 3, 0.0f );
				meshBuilder.BoneMatrix( 0, 0 );
				meshBuilder.BoneMatrix( 1, 0 );
				meshBuilder.BoneMatrix( 2, 0 );
				meshBuilder.BoneMatrix( 3, 0 );
				meshBuilder.Normal3fv( vert.m_vecNormal.Base() );
				meshBuilder.TexCoord2fv( 0, vert.m_vecTexCoord.Base() );
				Assert( pStudioTangentS[n].w == -1.0f || pStudioTangentS[n].w == 1.0f );
				meshBuilder.UserData( pStudioTangentS[n].Base() );
			}

			meshBuilder.AdvanceVertexF<VTX_HAVEPOS | VTX_HAVENORMAL, 1>();
		}
	}
	else
	{
		// no TangentS, replicated code to save inner conditional
		for ( int j=0; j < numVertices ; j++)
		{
			int n = pGroupToMesh[j];
			mstudiovertex_t &vert = pVertices[n];

			// FIXME: For now, flexed hw-skinned meshes can only have one bone
			// The data must exist in the 0th hardware matrix

			// Here, we are doing HW skinning, so we need to simply copy over the flex
			if ( m_VertexCache.IsVertexFlexed(n) )
			{
				CachedPosNormTan_t* pFlexedVertex = m_VertexCache.GetFlexVertex(n);
				meshBuilder.Position3fv( pFlexedVertex->m_Position.Base() );
				meshBuilder.BoneWeight( 0, 1.0f );
				meshBuilder.BoneWeight( 1, 0.0f );
				meshBuilder.BoneWeight( 2, 0.0f );
				meshBuilder.BoneWeight( 3, 0.0f );
				meshBuilder.BoneMatrix( 0, 0 );
				meshBuilder.BoneMatrix( 1, 0 );
				meshBuilder.BoneMatrix( 2, 0 );
				meshBuilder.BoneMatrix( 3, 0 );
				meshBuilder.Normal3fv( pFlexedVertex->m_Normal.Base() );
			}
			else
			{
				meshBuilder.Position3fv( vert.m_vecPosition.Base() );
				meshBuilder.BoneWeight( 0, 1.0f );
				meshBuilder.BoneWeight( 1, 0.0f );
				meshBuilder.BoneWeight( 2, 0.0f );
				meshBuilder.BoneWeight( 3, 0.0f );
				meshBuilder.BoneMatrix( 0, 0 );
				meshBuilder.BoneMatrix( 1, 0 );
				meshBuilder.BoneMatrix( 2, 0 );
				meshBuilder.BoneMatrix( 3, 0 );
				meshBuilder.Normal3fv( vert.m_vecNormal.Base() );
			}
			meshBuilder.TexCoord2fv( 0, vert.m_vecTexCoord.Base() );
			meshBuilder.AdvanceVertexF<VTX_HAVEPOS | VTX_HAVENORMAL, 1>();
		}
	}
}

//-----------------------------------------------------------------------------
// Restores the static mesh
//-----------------------------------------------------------------------------
template<VertexCompressionType_t T> void CStudioRender::R_StudioRestoreMesh( mstudiomesh_t* pmesh, studiomeshgroup_t* pMeshData )
{
#ifdef IS_WINDOWS_PC
	Vector4D *pStudioTangentS;

	// get at the vertex data
	const mstudio_meshvertexdata_t *vertData = GetFatVertexData( pmesh, m_pStudioHdr );
	if ( !vertData )
	{
		// not available
		return;
	}
	mstudiovertex_t *pVertices = vertData->Vertex( 0 );

	if (vertData->HasTangentData())
	{
		pStudioTangentS = vertData->TangentS( 0 );
	}
	else
	{
		pStudioTangentS = NULL;
	}

	CMeshBuilder meshBuilder;

	meshBuilder.BeginModify( pMeshData->m_pMesh );
	meshBuilder.SetCompressionType( T );
	for ( int j=0; j < meshBuilder.VertexCount() ; j++)
	{
		meshBuilder.SelectVertex(j);
		int n = pMeshData->m_pGroupIndexToMeshIndex[j];
		mstudiovertex_t &vert = pVertices[n];

		meshBuilder.Position3fv( vert.m_vecPosition.Base() );
		meshBuilder.CompressedNormal3fv<T>( vert.m_vecNormal.Base() );
		meshBuilder.TexCoord2fv( 0, vert.m_vecTexCoord.Base() );

		if (pStudioTangentS)
		{
			Assert( pStudioTangentS[n].w == -1.0f || pStudioTangentS[n].w == 1.0f );
			meshBuilder.CompressedUserData<T>( pStudioTangentS[n].Base() );
		}

		meshBuilder.Color4ub( 255, 255, 255, 255 );
	}
	meshBuilder.EndModify();
#endif
}



//-----------------------------------------------------------------------------
// Draws a mesh using hardware + software skinning
//-----------------------------------------------------------------------------
int CStudioRender::R_StudioDrawGroupHWSkin( IMatRenderContext *pRenderContext, studiomeshgroup_t* pGroup, IMesh* pMesh, ColorMeshInfo_t * pColorMeshInfo )
{
	PROFILE_STUDIO("HwSkin");
	int numFacesRendered = 0;

#if PIX_ENABLE
	char szPIXEventName[128];
	sprintf( szPIXEventName, "R_StudioDrawGroupHWSkin (%s)", m_pStudioHdr->name );	// PIX
	PIXEVENT( pRenderContext, szPIXEventName );
#endif

	if ( m_pStudioHdr->numbones == 1 )
	{
		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->LoadMatrix( m_PoseToWorld[0] );

		// a single bone means all verts rigidly assigned
		// any bonestatechange would needlessly re-load the same matrix
		// xbox can skip further hw skinning, seems ok for pc too
		pRenderContext->SetNumBoneWeights( 0 );
	}

	if ( pColorMeshInfo )
		pMesh->SetColorMesh( pColorMeshInfo->m_pMesh, pColorMeshInfo->m_nVertOffsetInBytes );
	else
		pMesh->SetColorMesh( NULL, 0 );

	Vector4D vecDiffuseModulation;
	ComputeDiffuseModulation( &vecDiffuseModulation );

	for (int j = 0; j < pGroup->m_NumStrips; ++j)
	{
		OptimizedModel::StripHeader_t* pStrip = &pGroup->m_pStripData[j];

		if ( m_pStudioHdr->numbones > 1 )
		{
			// Reset bone state if we're hardware skinning
			pRenderContext->SetNumBoneWeights( pStrip->numBones );

			for (int k = 0; k < pStrip->numBoneStateChanges; ++k)
			{
				OptimizedModel::BoneStateChangeHeader_t* pStateChange = pStrip->pBoneStateChange(k);
				if ( pStateChange->newBoneID < 0 )
					break;

				pRenderContext->LoadBoneMatrix( pStateChange->hardwareID, m_PoseToWorld[pStateChange->newBoneID] );
			}
		}

		pMesh->SetPrimitiveType( GetPrimitiveTypeForStripHeaderFlags( pStrip->flags ) );

		pMesh->DrawModulated( vecDiffuseModulation, pStrip->indexOffset, pStrip->numIndices );
		numFacesRendered += pGroup->m_pUniqueFaces[j];
	}
	pMesh->SetColorMesh( NULL, 0 );

	return numFacesRendered;
}

int CStudioRender::R_StudioDrawGroupSWSkin( studiomeshgroup_t* pGroup, IMesh* pMesh )
{
	int numFacesRendered = 0;
	
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	// Disable skinning
	pRenderContext->SetNumBoneWeights( 0 );

	Vector4D vecDiffuseModulation;
	ComputeDiffuseModulation( &vecDiffuseModulation );

	for (int j = 0; j < pGroup->m_NumStrips; ++j)
	{
		OptimizedModel::StripHeader_t* pStrip = &pGroup->m_pStripData[j];

		// Choose our primitive type
		pMesh->SetPrimitiveType( GetPrimitiveTypeForStripHeaderFlags( pStrip->flags ) );

		pMesh->DrawModulated( vecDiffuseModulation, pStrip->indexOffset, pStrip->numIndices );
		numFacesRendered += pGroup->m_pUniqueFaces[j];
	}

	return numFacesRendered;
}


//-----------------------------------------------------------------------------
// Sets up the hw flex mesh
//-----------------------------------------------------------------------------
void CStudioRender::ComputeFlexWeights( int nFlexCount, mstudioflex_t *pFlex, MorphWeight_t *pWeights )
{
	for ( int i = 0; i < nFlexCount; ++i, ++pFlex )
	{
		MorphWeight_t &weight = pWeights[i];

		weight.m_pWeight[MORPH_WEIGHT] = RampFlexWeight( *pFlex, m_pFlexWeights[ pFlex->flexdesc ] );
		weight.m_pWeight[MORPH_WEIGHT_LAGGED] = RampFlexWeight( *pFlex, m_pFlexDelayedWeights[ pFlex->flexdesc ] );

		if ( pFlex->flexpair != 0 )
		{
			weight.m_pWeight[MORPH_WEIGHT_STEREO] = RampFlexWeight( *pFlex, m_pFlexWeights[ pFlex->flexpair ] );
			weight.m_pWeight[MORPH_WEIGHT_STEREO_LAGGED] = RampFlexWeight( *pFlex, m_pFlexDelayedWeights[ pFlex->flexpair ] );
		}
		else
		{
			weight.m_pWeight[MORPH_WEIGHT_STEREO] = weight.m_pWeight[MORPH_WEIGHT];
			weight.m_pWeight[MORPH_WEIGHT_STEREO_LAGGED] = weight.m_pWeight[MORPH_WEIGHT_LAGGED];
		}
	}
}


//-----------------------------------------------------------------------------
// Draws the mesh as tristrips using hardware
//-----------------------------------------------------------------------------
int CStudioRender::R_StudioDrawStaticMesh( IMatRenderContext *pRenderContext, mstudiomesh_t* pmesh, 
				studiomeshgroup_t* pGroup, StudioModelLighting_t lighting, 
				float r_blend, IMaterial* pMaterial, int lod, ColorMeshInfo_t *pColorMeshes  )
{
	MatSysQueueMark( g_pMaterialSystem, "R_StudioDrawStaticMesh\n" );
	VPROF( "R_StudioDrawStaticMesh" );

	int numFacesRendered = 0;

	bool bDoSoftwareLighting = !pColorMeshes && 
		((m_pRC->m_Config.bSoftwareSkin != 0) || m_pRC->m_Config.bDrawNormals || m_pRC->m_Config.bDrawTangentFrame ||
		(pMaterial ? pMaterial->NeedsSoftwareSkinning() : false) ||
		(m_pRC->m_Config.bSoftwareLighting != 0) ||
		((lighting != LIGHTING_HARDWARE) && (lighting != LIGHTING_MOUTH) ));

	// software lighting case
	if ( bDoSoftwareLighting )
	{
		if ( m_pRC->m_Config.bNoSoftware )
			return 0;

		bool bTangentSpace = pMaterial ? pMaterial->NeedsTangentSpace() : false;
		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->LoadIdentity();

		// Hardcode the vertex format to a well-known format to make sw skin code faster
		VertexFormat_t fmt = VERTEX_FORMAT_STANDARD;

		Assert( ( pGroup->m_Flags & MESHGROUP_IS_DELTA_FLEXED ) == 0 );

		CMeshBuilder meshBuilder;
		IMesh* pMesh = pRenderContext->GetDynamicMeshEx( fmt, false, 0, pGroup->m_pMesh );
		meshBuilder.Begin( pMesh, MATERIAL_HETEROGENOUS, pGroup->m_NumVertices, 0 );

		R_StudioSoftwareProcessMesh( pmesh, meshBuilder, 
			pGroup->m_NumVertices, pGroup->m_pGroupIndexToMeshIndex, 
			lighting, false, r_blend, bTangentSpace, pMaterial );

		meshBuilder.End();

		numFacesRendered = R_StudioDrawGroupSWSkin( pGroup, pMesh );
		MatSysQueueMark( g_pMaterialSystem, "END R_StudioDrawStaticMesh\n" );
		return numFacesRendered;
	}

	// Needed when we switch back and forth between hardware + software lighting
#ifdef IS_WINDOWS_PC
	if ( IsPC() && pGroup->m_MeshNeedsRestore )
	{
		VertexCompressionType_t compressionType = CompressionType( pGroup->m_pMesh->GetVertexFormat() );
		switch ( compressionType )
		{
			case VERTEX_COMPRESSION_ON:
				R_StudioRestoreMesh<VERTEX_COMPRESSION_ON>( pmesh, pGroup );
				break;
			case VERTEX_COMPRESSION_NONE:
			default:
				R_StudioRestoreMesh<VERTEX_COMPRESSION_NONE>( pmesh, pGroup );
				break;
		}
		pGroup->m_MeshNeedsRestore = false;
	}
#endif

	// Build separate flex stream containing deltas, which will get copied into another vertex stream
	bool bUseHWFlex = m_pRC->m_Config.m_bEnableHWMorph && pGroup->m_pMorph && !m_bDrawTranslucentSubModels;
	bool bUseSOFlex = g_pMaterialSystemHardwareConfig->SupportsStreamOffset() && !bUseHWFlex;
	if ( (pGroup->m_Flags & MESHGROUP_IS_DELTA_FLEXED) && m_pRC->m_Config.bFlex )
	{
		PIXEVENT( pRenderContext, "Delta Flex Processing" );
		if ( bUseHWFlex )
		{
			pRenderContext->BindMorph( pGroup->m_pMorph );
		}
		if ( bUseSOFlex )
		{
			R_StudioProcessFlexedMesh_StreamOffset( pmesh, lod );
			R_StudioFlexMeshGroup( pGroup );
		}
	}

	// Draw it baby
	if ( pColorMeshes && ( pGroup->m_ColorMeshID != -1 ) )
	{
		// draw using specified color mesh
		numFacesRendered = R_StudioDrawGroupHWSkin( pRenderContext, pGroup, pGroup->m_pMesh, &(pColorMeshes[pGroup->m_ColorMeshID]) );
	}
	else
	{
		numFacesRendered = R_StudioDrawGroupHWSkin( pRenderContext, pGroup, pGroup->m_pMesh, NULL );
	}

	if ( ( pGroup->m_Flags & MESHGROUP_IS_DELTA_FLEXED ) && m_pRC->m_Config.bFlex )
	{
		if ( bUseHWFlex )
		{
			pRenderContext->BindMorph( NULL );
		}
		if ( bUseSOFlex )
		{
			pGroup->m_pMesh->DisableFlexMesh();	// clear flex stream
		}
	}

	MatSysQueueMark( g_pMaterialSystem, "END2 R_StudioDrawStaticMesh\n" );
	return numFacesRendered;
}


//-----------------------------------------------------------------------------
// Draws a dynamic mesh
//-----------------------------------------------------------------------------
int CStudioRender::R_StudioDrawDynamicMesh( IMatRenderContext *pRenderContext, mstudiomesh_t* pmesh, 
				studiomeshgroup_t* pGroup, StudioModelLighting_t lighting, 
				float r_blend, IMaterial* pMaterial, int lod )
{
	VPROF( "R_StudioDrawDynamicMesh" );

	bool bDoFlex = ((pGroup->m_Flags & MESHGROUP_IS_DELTA_FLEXED) != 0) && m_pRC->m_Config.bFlex;
	bool bQuadList = ( pGroup->m_pStripData[0].flags & OptimizedModel::STRIP_IS_QUADLIST_EXTRA ) ||
					 ( pGroup->m_pStripData[0].flags & OptimizedModel::STRIP_IS_QUADLIST_REG ) != 0;

	bool bDoSoftwareLighting = (m_pRC->m_Config.bSoftwareLighting != 0) ||
		((lighting != LIGHTING_HARDWARE) && (lighting != LIGHTING_MOUTH) );

	bool bSWSkin = bDoSoftwareLighting || m_pRC->m_Config.bDrawNormals || m_pRC->m_Config.bDrawTangentFrame ||
		((pGroup->m_Flags & MESHGROUP_IS_HWSKINNED) == 0) ||
		m_pRC->m_Config.bSoftwareSkin || bQuadList ||
		( pMaterial ? pMaterial->NeedsSoftwareSkinning() : false );

	if ( !bDoFlex && !bSWSkin )
	{
		return R_StudioDrawStaticMesh( pRenderContext, pmesh, pGroup, lighting, r_blend, pMaterial, lod, NULL );
	}

	// ----  Drawers before this might not need the vertices, so don't pay the penalty of getting them ----
	// --------  Everybody else past this point (flex and/or sw skinning) expects to read vertices --------

	const mstudio_meshvertexdata_t *vertData = GetFatVertexData( pmesh, m_pStudioHdr );
	if ( !vertData )
	{
		return 0;	// not available
	}

	MatSysQueueMark( g_pMaterialSystem, "R_StudioDrawDynamicMesh\n" );

	int numFacesRendered = 0;

#ifdef _DEBUG
	const char *pDebugMaterialName = NULL;
	if ( pMaterial )
	{
		pDebugMaterialName = pMaterial->GetName();
	}
#endif
	
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->LoadIdentity();

	// Software flex verts (not a delta stream)
	if ( bDoFlex )
	{
		R_StudioFlexVerts( pmesh, lod, bQuadList ); 
	}

	// Map quad mesh to Bicubic Bezier Patches
	if ( bQuadList )
	{
		GenerateBicubicPatches( pmesh, pGroup, bDoFlex );
	}

	IMesh* pMesh;
	bool bTangentSpace = pMaterial ? pMaterial->NeedsTangentSpace() : false;
	VertexFormat_t fmt = bQuadList ? VERTEX_FORMAT_SUBDQUAD : VERTEX_FORMAT_STANDARD;

	CMeshBuilder meshBuilder;
	pMesh = pRenderContext->GetDynamicMeshEx( fmt, false, 0, pGroup->m_pMesh);
	
	if ( bQuadList )
	{
		int TotalFaces = 0;
		for ( int s=0; s<pGroup->m_NumStrips; ++s )
		{
			TotalFaces += pGroup->m_pUniqueFaces[s];
		}

		// We're de-indexing the quad mesh, so we need to multiply the number of vertices by 4 here
		meshBuilder.Begin( pMesh, MATERIAL_HETEROGENOUS, TotalFaces * 4, 0 );
		SoftwareProcessQuadMesh( pmesh, meshBuilder, TotalFaces,
								 pGroup->m_pGroupIndexToMeshIndex,
								 pGroup->m_pTopologyIndices, bTangentSpace, bDoFlex );
	}
	else if ( bSWSkin )
	{
		meshBuilder.Begin( pMesh, MATERIAL_HETEROGENOUS, pGroup->m_NumVertices, 0 );
		R_StudioSoftwareProcessMesh( pmesh, meshBuilder, pGroup->m_NumVertices,
									 pGroup->m_pGroupIndexToMeshIndex, lighting, bDoFlex,
									 r_blend, bTangentSpace, pMaterial );
	}
	else if ( bDoFlex )
	{
		meshBuilder.Begin( pMesh, MATERIAL_HETEROGENOUS, pGroup->m_NumVertices, 0 );
		R_StudioProcessFlexedMesh( pmesh, meshBuilder, pGroup->m_NumVertices,
								   pGroup->m_pGroupIndexToMeshIndex );
	}
	
	meshBuilder.End();

	if ( !bSWSkin )
	{
		numFacesRendered = R_StudioDrawGroupHWSkin( pRenderContext, pGroup, pMesh );
	}
	else
	{
		numFacesRendered = R_StudioDrawGroupSWSkin( pGroup, pMesh );
	}

	pRenderContext->SetNumBoneWeights( 0 );
	pRenderContext->Bind( m_pMaterialTangentFrame );

	R_StudioSoftwareProcessMesh_NormalsBatched( pRenderContext, pmesh, pGroup, lighting, bDoFlex, r_blend, m_pRC->m_Config.bDrawNormals, m_pRC->m_Config.bDrawTangentFrame );
	
	pRenderContext->Bind( pMaterial );

	MatSysQueueMark( g_pMaterialSystem, "END R_StudioDrawDynamicMesh\n" );

	return numFacesRendered;
}


//-----------------------------------------------------------------------------
// Sets the material vars for the eye vertex shader
//-----------------------------------------------------------------------------
static unsigned int eyeOriginCache = 0;
static unsigned int eyeUpCache = 0;
static unsigned int irisUCache = 0;
static unsigned int irisVCache = 0;
static unsigned int glintUCache = 0;
static unsigned int glintVCache = 0;
void CStudioRender::SetEyeMaterialVars( IMaterial* pMaterial, mstudioeyeball_t* peyeball, 
		Vector const& eyeOrigin, const matrix3x4_t& irisTransform, const matrix3x4_t& glintTransform )
{
	if ( !pMaterial )
		return;

	IMaterialVar* pVar = pMaterial->FindVarFast( "$eyeorigin", &eyeOriginCache );
	if (pVar)
	{
		pVar->SetVecValue( eyeOrigin.Base(), 3 );
	}

	pVar = pMaterial->FindVarFast( "$eyeup", &eyeUpCache );
	if (pVar)
	{
		pVar->SetVecValue( peyeball->up.Base(), 3 );
	}
	pVar = pMaterial->FindVarFast( "$irisu", &irisUCache );
	if (pVar)
	{
		pVar->SetVecValue( irisTransform[0], 4 );
	}

	pVar = pMaterial->FindVarFast( "$irisv", &irisVCache );
	if (pVar)
	{
		pVar->SetVecValue( irisTransform[1], 4 );
	}

	pVar = pMaterial->FindVarFast( "$glintu", &glintUCache );
	if (pVar)
	{
		pVar->SetVecValue( glintTransform[0], 4 );
	}

	pVar = pMaterial->FindVarFast( "$glintv", &glintVCache );
	if (pVar)
	{
		pVar->SetVecValue( glintTransform[1], 4 );
	}
}


//-----------------------------------------------------------------------------
// Specialized routine to draw the eyeball
//-----------------------------------------------------------------------------
static unsigned int glintCache = 0;
int CStudioRender::R_StudioDrawEyeball( IMatRenderContext *pRenderContext, mstudiomesh_t* pmesh, studiomeshdata_t* pMeshData,
	StudioModelLighting_t lighting, IMaterial *pMaterial, int lod )
{
	if ( !m_pRC->m_Config.bEyes )
	{
		return 0;
	}

	int j;
	int numFacesRendered = 0;

	// See if any meshes in the group want to go down the static path...
	bool bFlexStatic = false;
	bool bIsHardwareSkinnedData = false;
	bool bQuadList = false;
	for (j = 0; j < pMeshData->m_NumGroup; ++j)
	{
		studiomeshgroup_t* pGroup = &pMeshData->m_pMeshGroup[j];

		if ( pGroup->m_Flags & MESHGROUP_IS_DELTA_FLEXED )
			bFlexStatic = true;

		if ( pGroup->m_Flags & MESHGROUP_IS_HWSKINNED )
			bIsHardwareSkinnedData = true;

		if ( pGroup->m_pStripData[0].flags & OptimizedModel::STRIP_IS_QUADLIST_EXTRA ||
			 pGroup->m_pStripData[0].flags & OptimizedModel::STRIP_IS_QUADLIST_REG )
		{
			bIsHardwareSkinnedData = false;
			bQuadList = true;
		}
	}

	// Take the static path for new flexed models on DX9 hardware
	bool bShouldHardwareSkin = bIsHardwareSkinnedData && bFlexStatic && 
		( lighting != LIGHTING_SOFTWARE ) && ( !m_pRC->m_Config.bSoftwareSkin );

	// EXPLICITLY DISABLING NEED FOR CPU SIDE VERTS ON CONSOLES!!!!
	// PORTAL2 CONSOLE: Vertex/Index data will never be read again (no model decals or load-time lighting), so discard the VVD data and create a new header
	// If we ever have a flexed eye vert on a model on the console, badness will ensue (ie. won't flex).
	if ( IsGameConsole() )
	{
		bShouldHardwareSkin = true;
		bFlexStatic = false;
	}

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->LoadIdentity();

	// Software flex eyeball verts (not a delta stream)
	if ( bFlexStatic && !bShouldHardwareSkin )
	{
		R_StudioFlexVerts( pmesh, lod, bQuadList );
	}

	mstudioeyeball_t *peyeball = m_pSubModel->pEyeball(pmesh->materialparam);

	// We'll need this to compute normals
	Vector org;
	VectorTransform( peyeball->org, m_pBoneToWorld[peyeball->bone], org );

	// Compute the glint projection
	matrix3x4_t glintMat;
	ComputeGlintTextureProjection( &m_pEyeballState[pmesh->materialparam], m_pRC->m_ViewRight, m_pRC->m_ViewUp, glintMat );
	
	if ( !m_pRC->m_Config.bWireframe )
	{
		// Compute the glint procedural texture
		IMaterialVar* pGlintVar = pMaterial->FindVarFast( "$glint", &glintCache );
		if (pGlintVar)
		{
			R_StudioEyeballGlint( &m_pEyeballState[pmesh->materialparam], pGlintVar, m_pRC->m_ViewRight, m_pRC->m_ViewUp, m_pRC->m_ViewOrigin );
		}
		SetEyeMaterialVars( pMaterial, peyeball, org, m_pEyeballState[pmesh->materialparam].mat, glintMat );
	}

	if ( bShouldHardwareSkin )
	{
		for ( j = 0; j < pMeshData->m_NumGroup; ++j )
		{
			studiomeshgroup_t* pGroup = &pMeshData->m_pMeshGroup[j];
			numFacesRendered += R_StudioDrawStaticMesh( pRenderContext, pmesh, pGroup, lighting, m_pRC->m_AlphaMod, pMaterial, lod, NULL );
		}

		return numFacesRendered;
	}

	// FIXME: We could compile a static vertex buffer in this case
	// if there's no flexed verts.
	const mstudio_meshvertexdata_t *vertData = GetFatVertexData( pmesh, m_pStudioHdr );
	if ( !vertData )
	{
		// not available
		return 0;
	}
	mstudiovertex_t *pVertices = vertData->Vertex( 0 );

	pRenderContext->SetNumBoneWeights( 0 );
	m_VertexCache.SetupComputation( pmesh );

	int nAlpnaInt = RoundFloatToInt( m_pRC->m_AlphaMod * 255 );
	unsigned char a = clamp( nAlpnaInt, 0, 255 );

	Vector position, normal, color;

	// setup the call
	R_InitLightEffectsWorld3();

	Vector4D vecDiffuseModulation;
	ComputeDiffuseModulation( &vecDiffuseModulation );

	// Render the puppy
	CMeshBuilder meshBuilder;
	bool bTangentSpace = pMaterial ? pMaterial->NeedsTangentSpace() : false;
	VertexFormat_t fmt = bQuadList ? VERTEX_FORMAT_SUBDQUAD : VERTEX_FORMAT_STANDARD;

	bool useHWLighting = m_pRC->m_Config.m_bSupportsVertexAndPixelShaders && !m_pRC->m_Config.bSoftwareLighting;
	// Draw all the various mesh groups...
	for ( j = 0; j < pMeshData->m_NumGroup; ++j )
	{
		studiomeshgroup_t* pGroup = &pMeshData->m_pMeshGroup[j];

		IMesh* pMesh = pRenderContext->GetDynamicMeshEx( fmt, false, 0, pGroup->m_pMesh );

		if ( bQuadList )
		{
			int TotalFaces = 0;
			for ( int s=0; s<pGroup->m_NumStrips; ++s )
			{
				TotalFaces += pGroup->m_pUniqueFaces[s];
			}

			// Map quad mesh to Bicubic Bezier Patches
			GenerateBicubicPatches( pmesh, pGroup, bFlexStatic );
			meshBuilder.Begin( pMesh, MATERIAL_SUBD_QUADS_EXTRA, TotalFaces, 0 );
			SoftwareProcessQuadMesh( pmesh, meshBuilder, TotalFaces,
									 pGroup->m_pGroupIndexToMeshIndex,
									 pGroup->m_pTopologyIndices, bTangentSpace, bFlexStatic );
		}
		else
		{
			// garymcthack!  need to look at the strip flags to figure out what it is.
			meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, pmesh->numvertices, 0 );
	//		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, pmesh->numvertices, 0 );
			//VPROF_INCREMENT_COUNTER( "TransformFlexVerts", pGroup->m_NumVertices );

			for ( int i=0; i < pGroup->m_NumVertices; ++i)
			{
				int n = pGroup->m_pGroupIndexToMeshIndex[i];
				mstudiovertex_t	&vert = pVertices[n];

				CachedPosNorm_t* pWorldVert = m_VertexCache.CreateWorldVertex(n);

				// transform into world space
				if ( m_VertexCache.IsVertexFlexed(n) )
				{
					CachedPosNormTan_t* pFlexVert = m_VertexCache.GetFlexVertex(n);
					R_StudioTransform( pFlexVert->m_Position.AsVector3D(), &vert.m_BoneWeights, m_PoseToWorld, pWorldVert->m_Position.AsVector3D() );
					R_StudioRotate( pFlexVert->m_Normal.AsVector3D(), &vert.m_BoneWeights, m_PoseToWorld, pWorldVert->m_Normal.AsVector3D() );
					Assert( pWorldVert->m_Normal.x >= -1.05f && pWorldVert->m_Normal.x <= 1.05f );
					Assert( pWorldVert->m_Normal.y >= -1.05f && pWorldVert->m_Normal.y <= 1.05f );
					Assert( pWorldVert->m_Normal.z >= -1.05f && pWorldVert->m_Normal.z <= 1.05f );
				}
				else
				{
					R_StudioTransform( vert.m_vecPosition, &vert.m_BoneWeights, m_PoseToWorld, pWorldVert->m_Position.AsVector3D() );
					R_StudioRotate( vert.m_vecNormal, &vert.m_BoneWeights, m_PoseToWorld, pWorldVert->m_Normal.AsVector3D() );
					Assert( pWorldVert->m_Normal.x >= -1.05f && pWorldVert->m_Normal.x <= 1.05f );
					Assert( pWorldVert->m_Normal.y >= -1.05f && pWorldVert->m_Normal.y <= 1.05f );
					Assert( pWorldVert->m_Normal.z >= -1.05f && pWorldVert->m_Normal.z <= 1.05f );
				}

				// Don't bother to light in software when we've got vertex + pixel shaders.
				meshBuilder.Position3fv( pWorldVert->m_Position.Base() );

				if (useHWLighting)
				{
					meshBuilder.Normal3fv( pWorldVert->m_Normal.Base() );
				}
				else
				{
					R_StudioEyeballNormal( peyeball, org, pWorldVert->m_Position.AsVector3D(), pWorldVert->m_Normal.AsVector3D() );

					// This isn't really used, but since the meshbuilder checks for messed up
					// normals, let's do this here in debug mode.
					// WRONGO YOU FRIGGIN IDIOT!!!!!!!!!!
					// DX7 needs these for the flashlight.
					meshBuilder.Normal3fv( pWorldVert->m_Normal.Base() );
					R_ComputeLightAtPoint3( pWorldVert->m_Position.AsVector3D(), pWorldVert->m_Normal.AsVector3D(), color );

					unsigned char r = LinearToLightmap( color.x );
					unsigned char g = LinearToLightmap( color.y );
					unsigned char b = LinearToLightmap( color.z );

					meshBuilder.Color4ub( r, g, b, a );
				}

				meshBuilder.TexCoord2fv( 0, vert.m_vecTexCoord.Base() );
				meshBuilder.AdvanceVertexF<VTX_HAVEPOS | VTX_HAVENORMAL | VTX_HAVECOLOR, 1>();
			}
		}

		meshBuilder.End();
		pMesh->DrawModulated( vecDiffuseModulation );

		for ( int k=0; k<pGroup->m_NumStrips; k++ )
		{
			numFacesRendered += pGroup->m_pUniqueFaces[k];
		}

		
		pRenderContext->SetNumBoneWeights( 0 );
		pRenderContext->Bind( m_pMaterialTangentFrame );
		
		R_StudioSoftwareProcessMesh_NormalsBatched( pRenderContext, pmesh, pGroup, lighting, true, false, m_pRC->m_Config.bDrawNormals, m_pRC->m_Config.bDrawTangentFrame );
	
		pRenderContext->Bind( pMaterial );

	}

	return numFacesRendered;
}



//-----------------------------------------------------------------------------
// Draws a mesh
//-----------------------------------------------------------------------------
int CStudioRender::R_StudioDrawMesh( IMatRenderContext *pRenderContext, mstudiomesh_t* pmesh, studiomeshdata_t* pMeshData,
									 StudioModelLighting_t lighting, IMaterial *pMaterial, 
									 ColorMeshInfo_t *pColorMeshes, int lod )
{
	VPROF( "R_StudioDrawMesh" );

	int numFacesRendered = 0;

	// Draw all the various mesh groups...
	for ( int j = 0; j < pMeshData->m_NumGroup; ++j )
	{
		studiomeshgroup_t* pGroup = &pMeshData->m_pMeshGroup[j];

		// Use the hardware if the mesh is hw skinned and we can put flexes on another stream 
		// Otherwise, we gotta do some expensive locks
		bool bIsHardwareSkinnedData = ( pGroup->m_Flags & MESHGROUP_IS_HWSKINNED ) != 0;
		bool bIsQuadMesh = ( pMeshData->m_pMeshGroup->m_pStripData[0].flags & OptimizedModel::STRIP_IS_QUADLIST_EXTRA ) ||
						   ( pMeshData->m_pMeshGroup->m_pStripData[0].flags & OptimizedModel::STRIP_IS_QUADLIST_REG ) != 0;
		bool bShouldHardwareSkin = bIsHardwareSkinnedData && !bIsQuadMesh && ( lighting != LIGHTING_SOFTWARE );

		if ( bShouldHardwareSkin && !m_pRC->m_Config.bDrawNormals && !m_pRC->m_Config.bDrawTangentFrame && !m_pRC->m_Config.bWireframe )
		{
			if ( !m_pRC->m_Config.bNoHardware )
			{
				numFacesRendered += R_StudioDrawStaticMesh( pRenderContext, pmesh, pGroup, lighting, m_pRC->m_AlphaMod, pMaterial, lod, pColorMeshes );
			}
		}
		else
		{
			if ( !m_pRC->m_Config.bNoSoftware )
			{
				numFacesRendered += R_StudioDrawDynamicMesh( pRenderContext, pmesh, pGroup, lighting, m_pRC->m_AlphaMod, pMaterial, lod );
			}
		}
	}
	return numFacesRendered;
}


//-----------------------------------------------------------------------------
// Inserts translucent mesh into list
//-----------------------------------------------------------------------------
template< class T >
void InsertRenderable( int mesh, T val, int count, int* pIndices, T* pValList )
{
	// Compute insertion point...
	int i;
	for ( i = count; --i >= 0; )
	{
		if (val < pValList[i])
			break;

		// Shift down
		pIndices[i + 1] = pIndices[i];
		pValList[i+1] = pValList[i];
	}

	// Insert at insertion point
	++i;
	pValList[i] = val;
	pIndices[i] = mesh;
}


//-----------------------------------------------------------------------------
// Sorts the meshes
//-----------------------------------------------------------------------------
int CStudioRender::SortMeshes( int* pIndices, IMaterial **ppMaterials, 
	short* pskinref, Vector const& vforward, Vector const& r_origin )
{
	int numMeshes = 0;
	if (m_bDrawTranslucentSubModels)
	{
//		float* pDist = (float*)stackalloc( m_pSubModel->nummeshes * sizeof(float) );

		// Sort each model piece by it's center, if it's translucent
		for (int i = 0; i < m_pSubModel->nummeshes; ++i)
		{
			// Don't add opaque materials
			mstudiomesh_t*	pmesh = m_pSubModel->pMesh(i);
			IMaterial *pMaterial = ppMaterials[pskinref[pmesh->material]];
			if( !pMaterial || !pMaterial->IsTranslucent() )
				continue;

			// FIXME: put the "center" of the mesh into delta
//			Vector delta;
//			VectorSubtract( delta, r_origin, delta );
//			float dist = DotProduct( delta, vforward );

			// Add it to our lists
//			InsertRenderable( i, dist, numMeshes, pIndices, pDist );

			// One more mesh
			++numMeshes;
		}
	}
	else
	{
		IMaterial** ppMat = (IMaterial**)stackalloc( m_pSubModel->nummeshes * sizeof(IMaterial*) );

		// Sort by material type
		for (int i = 0; i < m_pSubModel->nummeshes; ++i)
		{
			mstudiomesh_t*	pmesh = m_pSubModel->pMesh(i);
			IMaterial *pMaterial = ppMaterials[pskinref[pmesh->material]];
			if( !pMaterial )
				continue;

			// Don't add translucent materials
			if (( !m_pRC->m_Config.bWireframe ) && pMaterial->IsTranslucent() )
				continue;

			// Add it to our lists
			InsertRenderable( i, pMaterial, numMeshes, pIndices, ppMat );

			// One more mesh
			++numMeshes;
		}
	}

	return numMeshes;
}

//-----------------------------------------------------------------------------
// R_StudioDrawPoints
//
// Returns the number of triangles rendered.
//-----------------------------------------------------------------------------
#pragma warning (disable:4189)
int CStudioRender::R_StudioDrawPoints( IMatRenderContext *pRenderContext, int skin, void /*IClientEntity*/ *pClientEntity, 
	IMaterial **ppMaterials, int *pMaterialFlags, int boneMask, int lod, ColorMeshInfo_t *pColorMeshes )
{
	VPROF( "R_StudioDrawPoints" );
	int			i;
	int numFacesRendered = 0;

#if 0 // garymcthack
	if ( m_pSubModel->numfaces == 0 )
		return 0;
#endif

	// happens when there's a model load failure
	if ( m_pStudioMeshes == 0 )
		return 0;

	if ( m_pRC->m_Config.bWireframe && m_bDrawTranslucentSubModels )
		return 0;
	
	// ConDMsg("%d: %d %d\n", pimesh->numFaces, pimesh->numVertices, pimesh->numNormals );
	if ( m_pRC->m_Config.skin )
	{
		skin = m_pRC->m_Config.skin;
		if ( skin >= m_pStudioHdr->numskinfamilies )
		{
			skin = 0;
		}
	}

	// get skinref array
	short *pskinref	= m_pStudioHdr->pSkinref( 0 );
	if ( skin > 0 && skin < m_pStudioHdr->numskinfamilies )
	{
		pskinref += ( skin * m_pStudioHdr->numskinref );
	}

	// FIXME: Activate sorting on a mesh level
//	int* pIndices = (int*)stackalloc( m_pSubModel->nummeshes * sizeof(int) ); 
//	int numMeshes = SortMeshes( pIndices, ppMaterials, pskinref, vforward, r_origin );

	bool bHasMaterialOverride = ( m_pRC->m_pForcedMaterial[ 0 ] || ( m_pRC->m_nForcedMaterialType == OVERRIDE_DEPTH_WRITE ) );

	// draw each mesh
	for ( i = 0; i < m_pSubModel->nummeshes; ++i)
	{
		mstudiomesh_t *pmesh = m_pSubModel->pMesh(i);
		studiomeshdata_t *pMeshData = &m_pStudioMeshes[pmesh->meshid];
		Assert( pMeshData );

		if ( !pMeshData->m_NumGroup )
			continue;

		if ( !pMaterialFlags )
			continue;

		StudioModelLighting_t lighting = LIGHTING_HARDWARE;
		int materialFlags = pMaterialFlags[pskinref[pmesh->material]];

		IMaterial* pMaterial = R_StudioSetupSkinAndLighting( pRenderContext, pskinref[ pmesh->material ], ppMaterials, materialFlags, pClientEntity, pColorMeshes, lighting );
		if ( !pMaterial )
			continue;

#ifdef _DEBUG
		char const *materialName = pMaterial->GetName();
#endif

		// Set up flex data - this is the CPU flex cache...do we really need this at all if we're morphing?
		m_VertexCache.SetMesh( i );
		   
		// The following are special cases that can't be covered with the normal static/dynamic methods due to optimization reasons
		// NOTE: If we have a material override, we don't need to do eyeballs differently
		int nType = bHasMaterialOverride ? 0 : pmesh->materialtype;
		switch( nType )
		{
			case 1:	// eyeballs
				numFacesRendered += R_StudioDrawEyeball( pRenderContext, pmesh, pMeshData, lighting, pMaterial, lod );
				break;
			default:
				numFacesRendered += R_StudioDrawMesh( pRenderContext, pmesh, pMeshData, lighting, pMaterial, pColorMeshes, lod );
				break;
		}
	}

	// Reset this state so it doesn't hose other parts of rendering
	pRenderContext->SetNumBoneWeights( 0 );

	return numFacesRendered;
}
#pragma warning (default:4189)
