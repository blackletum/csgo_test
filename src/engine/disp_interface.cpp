﻿//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=====================================================================================//

#include "render_pch.h"
#include "decal_private.h"
#include "disp_defs.h"
#include "disp.h"
#include "gl_model_private.h"
#include "gl_matsysiface.h"
#include "gl_cvars.h"
#include "gl_rsurf.h"
#include "gl_lightmap.h"
#include "con_nprint.h"
#include "surfinfo.h"
#include "Overlay.h"
#include "cl_main.h"
#include "r_decal.h"
#include "materialsystem/materialsystem_config.h"
#include "debugoverlay.h"
#include "paint.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ----------------------------------------------------------------------------- //
// 	Shadow decals + fragments
// ----------------------------------------------------------------------------- //

static CUtlLinkedList< CDispShadowDecal,		DispShadowHandle_t, true >			s_DispShadowDecals;
static CUtlLinkedList< CDispShadowFragment,		DispShadowFragmentHandle_t, true >	s_DispShadowFragments;
static CUtlLinkedList< CDispDecal,				DispShadowHandle_t, true >			s_DispDecals;
static CUtlLinkedList< CDispDecalFragment,	DispShadowFragmentHandle_t, true >	s_DispDecalFragments;

// This mutex protects the s_DispShadowFragments and s_DispShadowDecals list during in AllocateShadowFragment() during
// threaded shadow clipping. It is NOT safe to Allocate and free shadow decal fragments from multiple threads.
static CThreadFastMutex s_ShadowFragmentAllocLock;

void CDispInfo::GetIntersectingSurfaces( GetIntersectingSurfaces_Struct *pStruct )
{
	if ( !m_Verts.Count() || !m_nIndices )
		return;

	// Walk through all of our triangles and add them one by one.
	SurfInfo *pOut = &pStruct->m_pInfos[ pStruct->m_nSetInfos ];
	for ( int iVert=0; iVert < m_MeshReader.NumIndices(); iVert += 3 )
	{
		// Is the list going to overflow?
		if ( pStruct->m_nSetInfos >= pStruct->m_nMaxInfos )
			break;
		
		Vector const &a = m_MeshReader.Position( m_MeshReader.Index(iVert+0) - m_iVertOffset );
		Vector const &b = m_MeshReader.Position( m_MeshReader.Index(iVert+1) - m_iVertOffset );
		Vector const &c = m_MeshReader.Position( m_MeshReader.Index(iVert+2) - m_iVertOffset );

		// Get the boundaries.
		Vector vMin;
		VectorMin( a, b, vMin );
		VectorMin( c, vMin, vMin );
		
		Vector vMax;
		VectorMax( a, b, vMax );
		VectorMax( c, vMax, vMax );

		// See if it touches the sphere.
		int iDim;
		for ( iDim=0; iDim < 3; iDim++ )
		{
			if ( ((*pStruct->m_pCenter)[iDim]+pStruct->m_Radius) < vMin[iDim] || 
				((*pStruct->m_pCenter)[iDim]-pStruct->m_Radius) > vMax[iDim] )
			{
				break;
			}
		}
		
		if ( iDim == 3 )
		{
			// Couldn't reject the sphere in the loop above, so add this surface.
			pOut->m_nVerts = 3;
			pOut->m_Verts[0] = a;
			pOut->m_Verts[1] = b;
			pOut->m_Verts[2] = c;
			pOut->m_Plane.m_Normal = ( c - a ).Cross( b - a );
			VectorNormalize( pOut->m_Plane.m_Normal );
			pOut->m_Plane.m_Dist = pOut->m_Plane.m_Normal.Dot( a );
			
			++pStruct->m_nSetInfos;
			++pOut;
		}
	}
}


// ----------------------------------------------------------------------------- //
// CDispInfo implementation of IDispInfo.
// ----------------------------------------------------------------------------- //
void CDispInfo::RenderWireframeInLightmapPage( int pageId )
{
#ifndef DEDICATED
    // render displacement as wireframe into lightmap pages
	SurfaceHandle_t surfID = GetParent();

	Assert( ( MSurf_MaterialSortID( surfID ) >= 0 ) && ( MSurf_MaterialSortID( surfID ) < g_WorldStaticMeshes.Count() ) );

	if( materialSortInfoArray[MSurf_MaterialSortID( surfID ) ].lightmapPageID != pageId )
		return;

	Shader_DrawLightmapPageSurface( surfID, 0.0f, 0.0f, 1.0f );
#endif
}


void CDispInfo::GetBoundingBox( Vector& bbMin, Vector& bbMax )
{
	bbMin = m_BBoxMin;
	bbMax = m_BBoxMax;
}


void CDispInfo::SetParent( SurfaceHandle_t surfID )
{
	m_ParentSurfID = surfID;
}


// returns surfID
SurfaceHandle_t CDispInfo::GetParent( void )
{
	return m_ParentSurfID;
}


unsigned int CDispInfo::ComputeDynamicLightMask( dlight_t *pLights )
{
	int lightMask = 0;

#ifndef DEDICATED
	if( !IS_SURF_VALID( m_ParentSurfID ) )
	{
		Assert( !"CDispInfo::ComputeDynamicLightMask: no parent surface" );
		return 0;
	}

	for ( int lnum = 0, testBit = 1, mask = r_dlightactive; lnum < MAX_DLIGHTS && mask != 0; lnum++, mask >>= 1, testBit <<= 1 )
	{
		if ( mask & 1 )
		{
			// not lit by this light
			if ( !(MSurf_DLightBits( m_ParentSurfID ) & testBit ) )
				continue;

			// This light doesn't affect the world
			if ( pLights[lnum].flags & DLIGHT_NO_WORLD_ILLUMINATION)
				continue;

			// This is used to ensure a maximum number of dlights in a frame
			if ( !R_CanUseVisibleDLight( lnum ) ) 
				continue;

			lightMask |= testBit;
		}
	}
#endif
	return lightMask;
}

void CDispInfo::AddDynamicLights( dlight_t *pLights, unsigned int mask )
{
#ifndef DEDICATED
	if( !IS_SURF_VALID( m_ParentSurfID ) )
	{
		Assert( !"CDispInfo::AddDynamicLights: no parent surface" );
		return;
	}

	for ( int lnum = 0; lnum < MAX_DLIGHTS && mask != 0; lnum++, mask >>= 1 )
	{
		if ( mask & 1 )
		{
			if ( (pLights[lnum].flags & DLIGHT_DISPLACEMENT_MASK) == 0)
			{
				if( NumLightMaps() == 1 )
				{
					AddSingleDynamicLight( pLights[lnum] );
				}
				else
				{
					AddSingleDynamicLightBumped( pLights[lnum] );
				}
			}
			else
			{
				AddSingleDynamicAlphaLight( pLights[lnum]);
			}
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Allocates fragments...
//-----------------------------------------------------------------------------
CDispDecalFragment* CDispInfo::AllocateDispDecalFragment( DispDecalHandle_t h, int nVerts )
{
	DispDecalFragmentHandle_t f = s_DispDecalFragments.Alloc(true);
	s_DispDecalFragments.LinkBefore( s_DispDecals[h].m_FirstFragment, f ); 
	s_DispDecals[h].m_FirstFragment = f;
	CDispDecalFragment* pf = &s_DispDecalFragments[f];
	
	// Initialize the vert count:
	pf->m_nVerts = nVerts;
	pf->m_pVerts = new CDecalVert[nVerts];

	return pf;
}


//-----------------------------------------------------------------------------
// Clears decal fragment lists
//-----------------------------------------------------------------------------
void CDispInfo::ClearDecalFragments( DispDecalHandle_t h )
{
	// Iterate over all fragments associated with each shadow decal
	CDispDecal& decal = s_DispDecals[h];
	DispDecalFragmentHandle_t f = decal.m_FirstFragment;
	DispDecalFragmentHandle_t next;

	while( f != DISP_DECAL_FRAGMENT_HANDLE_INVALID )
	{
		next = s_DispDecalFragments.Next(f);
		s_DispDecalFragments.Free(f);				// Destructs the decal, freeing the memory.
		f = next;
	}

	// Blat out the list
	decal.m_FirstFragment = DISP_DECAL_FRAGMENT_HANDLE_INVALID;

	// Mark is as not computed
	decal.m_Flags &= ~CDispDecalBase::FRAGMENTS_COMPUTED;

	// Update the number of triangles in the decal
	decal.m_nTris = 0;
	decal.m_nVerts = 0;
}

void CDispInfo::ClearAllDecalFragments()
{
	// Iterate over all shadow decals on the displacement
	DispDecalHandle_t h = m_FirstDecal;
	while( h != DISP_SHADOW_HANDLE_INVALID )
	{
		ClearDecalFragments( h );
		h = s_DispDecals.Next(h);
	}
}

// ----------------------------------------------------------------------------- //
// Add/remove decals
// ----------------------------------------------------------------------------- //
DispDecalHandle_t CDispInfo::NotifyAddDecal( decal_t *pDecal, float flSize )
{
	// Create a new decal, link it in
	DispDecalHandle_t h = s_DispDecals.Alloc( true );
	if ( h != s_DispDecals.InvalidIndex() )
	{
#ifndef DEDICATED
		int nDecalCount = 0;
		int iDecal = m_FirstDecal;
		int iLastDecal = s_DispDecals.InvalidIndex();
		while( iDecal != s_DispDecals.InvalidIndex() )
		{
			if ( 0 == ( s_DispDecals[ iDecal ].m_pDecal->flags & FDECAL_PLAYERSPRAY ) )
			{	// Do not count player sprays for the purpose of unlinking decals
				iLastDecal = iDecal;
				++nDecalCount;
			}
			iDecal = s_DispDecals.Next( iDecal );
		}
		
		if ( nDecalCount >= MAX_DISP_DECALS )
		{
			R_DecalUnlink( s_DispDecals[iLastDecal].m_pDecal, host_state.worldbrush );
		}
#endif
		
		s_DispDecals.LinkBefore( m_FirstDecal, h );
		m_FirstDecal = h;
		
		CDispDecal *pDispDecal = &s_DispDecals[h];
		pDispDecal->m_pDecal = pDecal;
		pDispDecal->m_FirstFragment = DISP_DECAL_FRAGMENT_HANDLE_INVALID;
		pDispDecal->m_nVerts = 0;
		pDispDecal->m_nTris = 0;
		pDispDecal->m_flSize = flSize;
		
		// Setup a basis for it.
		CDecalVert *pOutVerts = NULL;
		R_SetupDecalClip( pOutVerts, pDispDecal->m_pDecal, MSurf_Plane( m_ParentSurfID ).normal, pDispDecal->m_pDecal->material,
			pDispDecal->m_TextureSpaceBasis, pDispDecal->m_DecalWorldScale );
		
		// Recurse and precalculate which nodes this thing can touch.
		SetupDecalNodeIntersect( m_pPowerInfo->m_RootNode, 0, pDispDecal, 0 );
	}

	return h;
}

void CDispInfo::NotifyRemoveDecal( DispDecalHandle_t h )
{
	// Any fragments we got we don't need
	ClearDecalFragments(h);

	// Reset the head of the list
	if (m_FirstDecal == h)
		m_FirstDecal = s_DispDecals.Next(h);

	// Blow away the decal
	s_DispDecals.Free( h );
}


//-----------------------------------------------------------------------------
// Allocates fragments...
//-----------------------------------------------------------------------------
CDispShadowFragment* CDispInfo::AllocateShadowDecalFragment( DispShadowHandle_t h, int nCount )
{
	s_ShadowFragmentAllocLock.Lock();
	DispShadowFragmentHandle_t f = s_DispShadowFragments.Alloc(true);
	s_DispShadowFragments.LinkBefore( s_DispShadowDecals[h].m_FirstFragment, f ); 
	s_DispShadowDecals[h].m_FirstFragment = f;
	CDispShadowFragment* pf = &s_DispShadowFragments[f];
	pf->m_nVerts = nCount;
	pf->m_ShadowVerts = new ShadowVertex_t[nCount];
	s_ShadowFragmentAllocLock.Unlock();
	return pf;
}


//-----------------------------------------------------------------------------
// Clears shadow decal fragment lists
//-----------------------------------------------------------------------------
void CDispInfo::ClearShadowDecalFragments( DispShadowHandle_t h )
{
	// Iterate over all fragments associated with each shadow decal
	CDispShadowDecal& decal = s_DispShadowDecals[h];
	DispShadowFragmentHandle_t f = decal.m_FirstFragment;
	DispShadowFragmentHandle_t next;
	while( f != DISP_SHADOW_FRAGMENT_HANDLE_INVALID )
	{
		next = s_DispShadowFragments.Next(f);
		s_DispShadowFragments.Free(f);
		f = next;
	}

	// Blat out the list
	decal.m_FirstFragment = DISP_SHADOW_FRAGMENT_HANDLE_INVALID;

	// Mark is as not computed
	decal.m_Flags &= ~CDispDecalBase::FRAGMENTS_COMPUTED;

	// Update the number of triangles in the decal
	decal.m_nTris = 0;
	decal.m_nVerts = 0;
}

void CDispInfo::ClearAllShadowDecalFragments()
{
	// Iterate over all shadow decals on the displacement
	DispShadowHandle_t h = m_FirstShadowDecal;
	while( h != DISP_SHADOW_HANDLE_INVALID )
	{
		ClearShadowDecalFragments( h );
		h = s_DispShadowDecals.Next(h);
	}
}


// ----------------------------------------------------------------------------- //
// Add/remove shadow decals
// ----------------------------------------------------------------------------- //
DispShadowHandle_t CDispInfo::AddShadowDecal( ShadowHandle_t shadowHandle )
{
	// Create a new shadow decal, link it in
	DispShadowHandle_t h = s_DispShadowDecals.Alloc( true );
	s_DispShadowDecals.LinkBefore( m_FirstShadowDecal, h );
	m_FirstShadowDecal = h;

	CDispShadowDecal* pShadowDecal = &s_DispShadowDecals[h];
	pShadowDecal->m_nTris = 0;
	pShadowDecal->m_nVerts = 0;
	pShadowDecal->m_Shadow = shadowHandle;
	pShadowDecal->m_FirstFragment = DISP_SHADOW_FRAGMENT_HANDLE_INVALID;

	return h;
}

void CDispInfo::RemoveShadowDecal( DispShadowHandle_t h )
{
	// Any fragments we got we don't need
	ClearShadowDecalFragments(h);

	// Reset the head of the list
	if (m_FirstShadowDecal == h)
		m_FirstShadowDecal = s_DispShadowDecals.Next(h);

	// Blow away the decal
	s_DispShadowDecals.Free( h );
}


// ----------------------------------------------------------------------------- //
// This little beastie generate decal fragments
// ----------------------------------------------------------------------------- //
void CDispInfo::GenerateDecalFragments_R( CVertIndex const &nodeIndex, 
	int iNodeBitIndex, unsigned short decalHandle, CDispDecalBase *pDispDecal, int iLevel )
{
	// Get the node info for this node...
	Assert( iNodeBitIndex < m_pPowerInfo->m_NodeCount );
	DispNodeInfo_t const& nodeInfo = m_pNodeInfo[iNodeBitIndex];

	int iNodeIndex = VertIndex( nodeIndex );
	
	// Don't bother adding decals if the node doesn't have decal info.
	if( !pDispDecal->m_NodeIntersect.Get( iNodeBitIndex ) )
		return;

	// Recurse into child nodes, but only if they have triangles.
	if ( ( iLevel+1 < m_Power ) && (nodeInfo.m_Flags & DispNodeInfo_t::CHILDREN_HAVE_TRIANGLES) )
	{
		int iChildNodeBit = iNodeBitIndex + 1;
		for( int iChild=0; iChild < 4; iChild++ )
		{
			CVertIndex const &childNode = m_pPowerInfo->m_pChildVerts[iNodeIndex].m_Verts[iChild];

			bool bActiveChild = m_ActiveVerts.Get( VertIndex( childNode ) ) != 0;
			if ( bActiveChild )
				GenerateDecalFragments_R( childNode, iChildNodeBit, decalHandle, pDispDecal, iLevel + 1
				);
			iChildNodeBit += m_pPowerInfo->m_NodeIndexIncrements[iLevel];
		}
	}

	// Create the decal fragments on the node triangles
	bool isShadow = (pDispDecal->m_Flags & CDispDecalBase::DECAL_SHADOW) != 0;

	int index = nodeInfo.m_FirstTesselationIndex;
	for ( int i = 0; i < nodeInfo.m_Count; i += 3 )
	{
		if (isShadow)
			TestAddDecalTri( index + i, decalHandle, static_cast<CDispShadowDecal*>(pDispDecal)
			);
		else
			TestAddDecalTri( index + i, decalHandle, static_cast<CDispDecal*>(pDispDecal) );
	}
}

void CDispInfo::GenerateDecalFragments( CVertIndex const &nodeIndex, 
	int iNodeBitIndex, unsigned short decalHandle, CDispDecalBase *pDispDecal )
{
	GenerateDecalFragments_R( nodeIndex, iNodeBitIndex, decalHandle, pDispDecal, 0 );
	pDispDecal->m_Flags |= CDispDecalBase::FRAGMENTS_COMPUTED;
}

// ----------------------------------------------------------------------------- //
// Compute shadow fragments for a particular shadow
// ----------------------------------------------------------------------------- //
bool CDispInfo::ComputeShadowFragments( DispShadowHandle_t h, int& vertexCount, int& indexCount )
{
#ifndef DEDICATED
	CDispShadowDecal* pShadowDecal = &s_DispShadowDecals[h];

	// If we already have fragments, that means the data's already cached.
	if ((pShadowDecal->m_Flags & CDispDecalBase::FRAGMENTS_COMPUTED) != 0)
	{
		vertexCount = pShadowDecal->m_nVerts;
		indexCount = 3 * pShadowDecal->m_nTris;
		return true;
	}

	Assert( pShadowDecal->m_nTris == 0);
	Assert( pShadowDecal->m_FirstFragment == DISP_SHADOW_FRAGMENT_HANDLE_INVALID);

	ShadowInfo_t const& info = g_pShadowMgr->GetInfo( pShadowDecal->m_Shadow );

	unsigned short pIntersectingNodes[ 85 ];	// shorts to save stack space
	int nNumIntersectingNodes = 0;

	VMatrix matZScale( SetupMatrixScale( Vector( 1.0f, 1.0f, 1.0f/info.m_MaxDist ) ) );
	VMatrix normalizedWorldToShadow;
	MatrixMultiply( matZScale, info.m_WorldToShadow, normalizedWorldToShadow );

	Frustum_t shadowFrustum;
	VMatrix shadowToWorld;
	MatrixInverseGeneral( normalizedWorldToShadow, shadowToWorld );
	FrustumPlanesFromMatrix( shadowToWorld, shadowFrustum );

	FindNodesInShadowFrustum( shadowFrustum, pIntersectingNodes, &nNumIntersectingNodes, 0, 0 );

	if ( nNumIntersectingNodes == 0 )
	{
		return false;
	}

	AddNodeTrisToDecal( pShadowDecal, h, pIntersectingNodes, nNumIntersectingNodes );

	pShadowDecal->m_Flags |= CDispDecalBase::FRAGMENTS_COMPUTED;

	// Compute the index + vertex counts
	vertexCount = pShadowDecal->m_nVerts;
	indexCount = 3 * pShadowDecal->m_nTris;
#endif

	return true;
}


struct VisitedNodeData_t
{
	unsigned char iNodeBit;
	unsigned char iLevel;
};

// ----------------------------------------------------------------------------- //
// Hierarchical node culling using AABB tree
// ----------------------------------------------------------------------------- //
void CDispInfo::FindNodesInShadowFrustum( const Frustum_t& frustum, unsigned short* pNodeArray, int* pNumNodes, int iNodeBit, int iLevel )
{
	VisitedNodeData_t nodeList[85];
	nodeList[0].iNodeBit = 0;
	nodeList[0].iLevel = 0;
	int listIndex = 0;
	int maxIndex = 0;

	while ( listIndex <= maxIndex )
	{
		int iNodeBit = nodeList[listIndex].iNodeBit;
		int iLevel = nodeList[listIndex].iLevel;
		++listIndex;

		DispNodeInfo_t& nodeInfo = m_pNodeInfo[iNodeBit];

		if ( nodeInfo.m_mins.x == FLT_MAX )
		{
			// empty node
			continue;
		}

		// TODO: SIMD intersection routine? Replace recursion with queue?
		if ( frustum.CullBox( nodeInfo.m_mins, nodeInfo.m_maxs ) )
		{
			continue;
		}

		if ( iLevel+1 < m_Power )
		{
			// Need to check child nodes. put them at end of list
			int iChildNodeBit = iNodeBit + 1;
			for( int iChild=0; iChild < 4; iChild++ )
			{
				++maxIndex;
				nodeList[maxIndex].iNodeBit = iChildNodeBit;
				nodeList[maxIndex].iLevel = iLevel + 1;
				iChildNodeBit += m_pPowerInfo->m_NodeIndexIncrements[iLevel];
			}
		}
		else
		{
			// leaf node
			//CDebugOverlay::AddBoxOverlay( Vector( 0.0f, 0.0f, 0.0f ), nodeInfo.m_mins, nodeInfo.m_maxs, QAngle( 0, 0, 0 ), 255, 255, 255, 0, 0.0f );
			pNodeArray[*pNumNodes] = iNodeBit;
			++(*pNumNodes);
		}
	}
}


// ----------------------------------------------------------------------------- //
// Takes an array of node indices and adds all of their tris to a decal
// ----------------------------------------------------------------------------- //
void CDispInfo::AddNodeTrisToDecal( CDispShadowDecal *pDispDecal, unsigned short decalHandle, unsigned short* pNodeIndices, int nNumIndices )
{
	for ( int j = 0; j < nNumIndices; ++j )
	{
		// Get the node info for this node...
		DispNodeInfo_t const& nodeInfo = m_pNodeInfo[ pNodeIndices[j] ];
		int index = nodeInfo.m_FirstTesselationIndex;
		for ( int i = 0; i < nodeInfo.m_Count; i += 3 )
		{
			TestAddDecalTri( index + i, decalHandle, pDispDecal );
		}
	}
}
//-----------------------------------------------------------------------------
// Generate vertex lists
//-----------------------------------------------------------------------------

bool CDispInfo::GetTag()
{
	return m_Tag == m_pDispArray->m_CurTag;
}


void CDispInfo::SetTag()
{
	m_Tag = m_pDispArray->m_CurTag;
}

//-----------------------------------------------------------------------------
// Helpers for global functions.
//-----------------------------------------------------------------------------

void DispInfo_BuildPrimLists( int nSortGroup, SurfaceHandle_t *pList, int listCount, bool bDepthOnly,
	CDispInfo *visibleDisps[MAX_MAP_DISPINFO], int &nVisibleDisps )
{
	VPROF("DispInfo_BuildPrimLists");

	nVisibleDisps = 0;
	for( int i = 0; i < listCount; i++ )
	{
		CDispInfo *pDisp = static_cast<CDispInfo*>( pList[i]->pDispInfo );
		if( !pDisp->Render( pDisp->m_pMesh, false ) )
			continue;

		// Add it to the list of visible displacements.
		if( nVisibleDisps < MAX_MAP_DISPINFO )
		{
			visibleDisps[nVisibleDisps++] = pDisp;
		}
	}
}

void DispInfo_GetVisibleDispsAndAddOverlayFragmentsToRenderList( int nSortGroup, SurfaceHandle_t *pList, int listCount,
	CDispInfo *visibleDisps[MAX_MAP_DISPINFO], int &nVisibleDisps )
{
	VPROF( "DispInfo_GetVisibleDispsAndAddOverlayFragmentsToRenderList" );

	nVisibleDisps = 0;
#ifndef DEDICATED
	for( int i = 0; i < listCount; i++ )
	{
		CDispInfo *pDisp = static_cast<CDispInfo*>( pList[i]->pDispInfo );

		// Add it to the list of visible displacements.
		if( nVisibleDisps < MAX_MAP_DISPINFO )
		{
			visibleDisps[nVisibleDisps++] = pDisp;
		}

		OverlayMgr()->AddFragmentListToRenderList( nSortGroup, MSurf_OverlayFragmentList( pList[i] ), true );
	}
#endif
}

ConVar disp_dynamic( "disp_dynamic", "0" );

void DispInfo_DrawPrimLists( IMatRenderContext *pRenderContext, ERenderDepthMode_t DepthMode )
{
#ifndef DEDICATED
	VPROF("DispInfo_DrawPrimLists");

	int nDispGroupsSize = g_DispGroups.Count();

	int nFullbright = g_pMaterialSystemConfig->nFullbright;

	for( int iGroup=0; iGroup < nDispGroupsSize; iGroup++ )
	{
		CDispGroup *pGroup = g_DispGroups[iGroup];
		if( pGroup->m_nVisible == 0 )
			continue;

		if ( DepthMode != DEPTH_MODE_NORMAL )
		{
			// Select proper override material
			int nAlphaTest = (int) pGroup->m_pMaterial->IsAlphaTested();
			int nNoCull = (int) pGroup->m_pMaterial->IsTwoSided();

			IMaterial *pDepthWriteMaterial;

			if ( DepthMode == DEPTH_MODE_SHADOW )
			{
				pDepthWriteMaterial = g_pMaterialDepthWrite[ nAlphaTest ][ nNoCull ];
			}
			else
			{
				pDepthWriteMaterial = g_pMaterialSSAODepthWrite[ nAlphaTest ][ nNoCull ];
			}

			if ( nAlphaTest == 1 )
			{
				static unsigned int originalTextureVarCache = 0;
				IMaterialVar *pOriginalTextureVar = pGroup->m_pMaterial->FindVarFast( "$basetexture", &originalTextureVarCache );
				static unsigned int originalTextureFrameVarCache = 0;
				IMaterialVar *pOriginalTextureFrameVar = pGroup->m_pMaterial->FindVarFast( "$frame", &originalTextureFrameVarCache );
				static unsigned int originalAlphaRefCache = 0;
				IMaterialVar *pOriginalAlphaRefVar = pGroup->m_pMaterial->FindVarFast( "$AlphaTestReference", &originalAlphaRefCache );

				static unsigned int textureVarCache = 0;
				IMaterialVar *pTextureVar = pDepthWriteMaterial->FindVarFast( "$basetexture", &textureVarCache );
				static unsigned int textureFrameVarCache = 0;
				IMaterialVar *pTextureFrameVar = pDepthWriteMaterial->FindVarFast( "$frame", &textureFrameVarCache );
				static unsigned int alphaRefCache = 0;
				IMaterialVar *pAlphaRefVar = pDepthWriteMaterial->FindVarFast( "$AlphaTestReference", &alphaRefCache );

				if( pTextureVar && pOriginalTextureVar )
				{
					pTextureVar->SetTextureValue( pOriginalTextureVar->GetTextureValue() );
				}

				if( pTextureFrameVar && pOriginalTextureFrameVar )
				{
					pTextureFrameVar->SetIntValue( pOriginalTextureFrameVar->GetIntValue() );
				}

				if( pAlphaRefVar && pOriginalAlphaRefVar )
				{
					pAlphaRefVar->SetFloatValue( pOriginalAlphaRefVar->GetFloatValue() );
				}
			}

			pRenderContext->Bind( pDepthWriteMaterial );
		}
		else
		{
			pRenderContext->Bind( pGroup->m_pMaterial );
		}

		if ( nFullbright != 1 && DepthMode == DEPTH_MODE_NORMAL )
		{
			pRenderContext->BindLightmapPage( pGroup->m_LightmapPageID );
		}
		else
		{
			// If this code gets removed again, I'm chopping fingers off!
			if( pGroup->m_pMaterial->GetPropertyFlag( MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS ) )
			{
				pRenderContext->BindLightmapPage( MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE_BUMP );
			}
			else
			{
				pRenderContext->BindLightmapPage( MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE );
			}
		}

		
		int nMeshesSize = pGroup->m_Meshes.Count();

		for( int iMesh=0; iMesh < nMeshesSize; iMesh++ )
		{
			CGroupMesh *pMesh = pGroup->m_Meshes[iMesh];
			if( pMesh->m_nVisible == 0 )
				continue;

			if ( disp_dynamic.GetInt() )
			{
				for ( int iVisible=0; iVisible < pMesh->m_nVisible; iVisible++ )
				{
					pMesh->m_VisibleDisps[iVisible]->SpecifyDynamicMesh();
				}
			}
			else
			{
				pMesh->m_pMesh->Draw( pMesh->m_Visible.Base(), pMesh->m_nVisible );
			}

			pMesh->m_nVisible = 0;
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void DecalDispSurfacesInit( void )
{
#ifndef DEDICATED
	g_aDispDecalSortPool.RemoveAll();
	++g_nDispDecalSortCheckCount;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Batch up visible displacement decals and build them if necessary.
//-----------------------------------------------------------------------------
void DispInfo_BatchDecals( CDispInfo **pVisibleDisps, int nVisibleDisps )
{
#ifndef DEDICATED

	// Performance analysis.
	VPROF( "DispInfo_BatchDecals" );

	// Increment the decal sort check count and clear the pool.
	DecalDispSurfacesInit();

	// Do we have any visible displacements?
	if( !nVisibleDisps )
		return;

	for( int iDisp = 0; iDisp < nVisibleDisps; ++iDisp )
	{
		// Get the current visible displacement and see if it has any decals.
		CDispInfo *pDisp = pVisibleDisps[iDisp];
		if( pDisp->m_FirstDecal == DISP_DECAL_HANDLE_INVALID )
			continue;

		DispDecalHandle_t hDecal = pDisp->m_FirstDecal;
		while ( hDecal != DISP_DECAL_HANDLE_INVALID )
		{
			CDispDecal &decal = s_DispDecals[hDecal];

			// If we're an immediate cleanup decal, flag this for cleanup next frame. We do this by virtue of considering 
			// drawing, rather than actually drawing it. We do still want to draw it, though.
			if ( decal.m_pDecal->flags & FDECAL_IMMEDIATECLEANUP )
			{
				extern void R_DecalAddToDestroyList( decal_t *pDecal );
				R_DecalAddToDestroyList( decal.m_pDecal );
			}

			// Create the displacement fragment if necessary.
			if ( ( decal.m_Flags & CDispDecalBase::FRAGMENTS_COMPUTED ) == 0 )
			{
				pDisp->GenerateDecalFragments( pDisp->m_pPowerInfo->m_RootNode, 0, hDecal, &decal );
			}

			// Don't draw if there's no triangles.
			if ( decal.m_nTris == 0 )
			{
				hDecal = s_DispDecals.Next( hDecal );
				continue;
			}

			// Get the decal material.
			IMaterial *pMaterial = decal.m_pDecal->material;
			if ( !pMaterial )
			{
				DevMsg( "DispInfo_BatchDecals: material is NULL, decal %i.\n", hDecal );
				hDecal = s_DispDecals.Next( hDecal );
				continue;
			}

			// Lightmap decals.
			int iTreeType = -1;
			if ( pMaterial->GetPropertyFlag( MATERIAL_PROPERTY_NEEDS_LIGHTMAP ) )
			{
				// Permanent lightmapped decals.
				if ( decal.m_pDecal->flags & FDECAL_PERMANENT )
				{
					iTreeType = PERMANENT_LIGHTMAP;
				}
				// Non-permanent lightmapped decals.
				else
				{
					iTreeType = LIGHTMAP;
				}
			}
			// Non-lightmapped decals.
			else
			{
				iTreeType = NONLIGHTMAP;
			}

			// There is only one group at a time.
			int iGroup = 0;

			intp iPool = g_aDispDecalSortPool.Alloc( true );
			g_aDispDecalSortPool[iPool] = decal.m_pDecal;
			
			int iSortTree = decal.m_pDecal->m_iSortTree;
			int iSortMaterial = decal.m_pDecal->m_iSortMaterial;

			DecalMaterialBucket_t &materialBucket = g_aDispDecalSortTrees[iSortTree].m_aDecalSortBuckets[iGroup][iTreeType].Element( iSortMaterial );
			if ( materialBucket.m_nCheckCount == g_nDispDecalSortCheckCount )
			{	
				// intp iTail = materialBucket.m_iTail;
				// g_aDispDecalSortPool.LinkAfter( iTail, iPool );
				// materialBucket.m_iTail = iPool;
				// Displacement decals on the ground happen to render back-to-front, so link before head
				intp iHead = materialBucket.m_iHead;
				g_aDispDecalSortPool.LinkBefore( iHead, iPool );
				materialBucket.m_iHead = iPool;
			}
			else
			{
				materialBucket.m_iHead = materialBucket.m_iTail = iPool;
				materialBucket.m_nCheckCount = g_nDispDecalSortCheckCount;
			}

			hDecal = s_DispDecals.Next( hDecal );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
inline void DispInfo_DrawDecalMeshList( IMatRenderContext *pRenderContext, DecalMeshList_t &meshList )
{
	bool bMatFullbright = ( g_pMaterialSystemConfig->nFullbright == 1 );

	int nBatchCount = meshList.m_aBatches.Count();
	for ( int iBatch = 0; iBatch < nBatchCount; ++iBatch )
	{
		const DecalBatchList_t &batch = meshList.m_aBatches[iBatch];

		if ( !bMatFullbright )
		{
			pRenderContext->BindLightmapPage( batch.m_iLightmapPage );
		}
		else
		{
			pRenderContext->BindLightmapPage( MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE );
		}
		
		pRenderContext->Bind( batch.m_pMaterial, batch.m_pProxy );
		meshList.m_pMesh->Draw( batch.m_iStartIndex, batch.m_nIndexCount );
	}
}

void DispInfo_DrawDecalsGroup( IMatRenderContext *pRenderContext, int iGroup, int iTreeType )
{
#ifndef DEDICATED
	int nSortTreeCount = g_aDecalSortTrees.Count();
	if ( !nSortTreeCount )
		return;

	DecalMeshList_t		meshList;
	CMeshBuilder		meshBuilder;

	int nVertCount = 0;
	int nIndexCount = 0;

	int nDecalSortMaxVerts = g_nMaxDecals * 5;

	// NOTE: This is sort of a hack. The decal wireframe material is 20 bytes; this assumes
	// we're going to render vertices no larger than 80 bytes/vert.
	nDecalSortMaxVerts = MIN( nDecalSortMaxVerts, pRenderContext->GetMaxVerticesToRender( g_materialDecalWireframe ) / 4 );

	int nDecalSortMaxIndices = nDecalSortMaxVerts * 3;
	nDecalSortMaxIndices = MIN( nDecalSortMaxIndices, pRenderContext->GetMaxIndicesToRender() );

	bool bMatWireframe = ShouldDrawInWireFrameMode();

	for ( int iSortTree = 0; iSortTree < nSortTreeCount; ++iSortTree )
	{
		bool bMeshInit = true;
		uint16 unPlayerDecalStickerKitID = 0;	// player decals must be split into separate calls by actual basetexture, but overall keep the state so they are bucketed by same material
		
		const CUtlVector<DecalMaterialBucket_t> &materialBucketList = g_aDispDecalSortTrees[iSortTree].m_aDecalSortBuckets[iGroup][iTreeType];
		int nBucketCount = materialBucketList.Count();
		for ( int iBucket = 0; iBucket < nBucketCount; ++iBucket )
		{
			if ( materialBucketList.Element( iBucket ).m_nCheckCount != g_nDispDecalSortCheckCount )
				continue;
			
			intp iHead = materialBucketList.Element( iBucket ).m_iHead;
			if ( !g_aDispDecalSortPool.IsValidIndex( iHead ) )
				continue;

			decal_t *pDecalHead = g_aDispDecalSortPool.Element( iHead );
			Assert( pDecalHead->material );
			if ( !pDecalHead->material )
				continue;

			// Vertex format.
			VertexFormat_t vertexFormat = pDecalHead->material->GetVertexFormat();
			if ( vertexFormat == 0 )
				continue;
	
			// New bucket = new batch.
			DecalBatchList_t *pBatch = NULL;
			bool bBatchInit = true;
			
			int nCount;
			intp iElement = iHead;
			while ( iElement != g_aDispDecalSortPool.InvalidIndex() )
			{
				decal_t *pDecal = g_aDispDecalSortPool.Element( iElement );
				iElement = g_aDispDecalSortPool.Next( iElement );

				CDispDecal &decal = s_DispDecals[pDecal->m_DispDecal];

				// Now draw all the fragments with this material.
				IMaterial* pMaterial = decal.m_pDecal->material;
				if ( !pMaterial )
				{
					DevMsg( "DispInfo_DrawDecalsGroup: material is NULL decal %i.\n", pDecal->m_DispDecal );
					continue;
				}

				DispDecalFragmentHandle_t hFrag = decal.m_FirstFragment;
				while ( hFrag != DISP_DECAL_FRAGMENT_HANDLE_INVALID )
				{
					CDispDecalFragment &fragment = s_DispDecalFragments[hFrag];
					hFrag = s_DispDecalFragments.Next( hFrag );
					nCount = fragment.m_nVerts;

					// Overflow - new mesh, batch.
					if ( ( ( nVertCount + nCount ) >= nDecalSortMaxVerts ) || ( nIndexCount + ( nCount - 2 ) >= nDecalSortMaxIndices )
						|| ( !bMeshInit && ( pDecal->flags & FDECAL_PLAYERSPRAY ) && ( uint16( reinterpret_cast< uintp >( pDecal->userdata ) ) != unPlayerDecalStickerKitID ) ) )
					{
						// Finish this batch.
						if ( pBatch )
						{
							pBatch->m_nIndexCount = ( nIndexCount - pBatch->m_iStartIndex );
						}

						// End the mesh building phase and render.
						meshBuilder.End();
						DispInfo_DrawDecalMeshList( pRenderContext, meshList );

						// Reset.
						bMeshInit = true;
						pBatch = NULL;
						bBatchInit = true;
						unPlayerDecalStickerKitID = 0;
					}
					
					// Create the mesh.
					if ( bMeshInit )
					{
						// Reset the mesh list.
						meshList.m_pMesh = NULL;
						meshList.m_aBatches.RemoveAll();

						if ( !bMatWireframe )
						{
							meshList.m_pMesh = pRenderContext->GetDynamicMesh( false, NULL, NULL, pDecalHead->material );
						}
						else
						{
							meshList.m_pMesh = pRenderContext->GetDynamicMesh( false, NULL, NULL, g_materialDecalWireframe );
						}

						meshBuilder.Begin( meshList.m_pMesh, MATERIAL_TRIANGLES, nDecalSortMaxVerts, nDecalSortMaxIndices );
						
						nVertCount = 0;
						nIndexCount = 0;
						
						bMeshInit = false;

						unPlayerDecalStickerKitID = ( pDecal->flags & FDECAL_PLAYERSPRAY )
							? uint16( reinterpret_cast< uintp >( pDecal->userdata ) )
							: 0;	// Keep track of playerdecal proxy state for batches roll over
					}
					
					// Create the batch.
					if ( bBatchInit )
					{
						// Create a batch for this bucket = material/lightmap pair.
						int iBatchList = meshList.m_aBatches.AddToTail();
						pBatch = &meshList.m_aBatches[iBatchList];
						pBatch->m_iStartIndex = nIndexCount;
						
						if ( !bMatWireframe )
						{
							pBatch->m_pMaterial = pDecalHead->material;
							if ( pDecal->flags & FDECAL_PLAYERSPRAY )
								pBatch->m_pProxy = pDecal->userdata;	// Player sprays must use individual proxies, probably all materials can, but this is a safe change
							else
								pBatch->m_pProxy = pDecalHead->userdata;
							pBatch->m_iLightmapPage = materialSortInfoArray[MSurf_MaterialSortID( pDecalHead->surfID )].lightmapPageID;
						}
						else
						{
							pBatch->m_pMaterial = g_materialDecalWireframe;
						}
						
						bBatchInit = false;
					}
					Assert ( pBatch );
	
					// Setup verts.
					float flOffset = fragment.m_pDecal->lightmapOffset;
					for ( int iVert = 0; iVert < fragment.m_nVerts; ++iVert )
					{
						const CDecalVert &vert = fragment.m_pVerts[iVert];

						meshBuilder.Position3fv( vert.m_vPos.Base() );
						// FIXME!!  Really want the normal from the displacement, not from the base surface.
						Vector &normal = MSurf_Plane( fragment.m_pDecal->surfID ).normal;
						meshBuilder.Normal3fv( normal.Base() );
						meshBuilder.Color4ub( 255, 255, 255, 255 );
						meshBuilder.TexCoord2f( 0, vert.m_ctCoords.x, vert.m_ctCoords.y );
						meshBuilder.TexCoord2f( 1, vert.m_cLMCoords.x, vert.m_cLMCoords.y );
						meshBuilder.TexCoord1f( 2, flOffset );
						meshBuilder.AdvanceVertex();
					}

					// Setup indices.
					int nTriCount = ( nCount - 2 );
					CIndexBuilder &indexBuilder = meshBuilder;
					indexBuilder.FastPolygon( nVertCount, nTriCount );
					
					// Update counters.
					nVertCount += nCount;
					nIndexCount += ( nTriCount * 3 );
				}
				
				if ( pBatch )
				{
					pBatch->m_nIndexCount = ( nIndexCount - pBatch->m_iStartIndex );
				}
			}
		}
		
		if ( !bMeshInit )
		{
			meshBuilder.End();
			DispInfo_DrawDecalMeshList( pRenderContext, meshList );

			nVertCount = 0;
			nIndexCount = 0;
		}
	}
#endif
}

void DispInfo_DrawDecals( IMatRenderContext *pRenderContext, CDispInfo **visibleDisps, int nVisibleDisps )
{
#ifndef DEDICATED
	VPROF( "DispInfo_DrawDecals" );

	int iGroup = 0;

	// Draw world decals.
 	DispInfo_DrawDecalsGroup( pRenderContext, iGroup, PERMANENT_LIGHTMAP );

	// Draw lightmapped non-world decals.
	DispInfo_DrawDecalsGroup( pRenderContext, iGroup, LIGHTMAP );

	// Draw non-lit(mod2x) decals.
	DispInfo_DrawDecalsGroup( pRenderContext, iGroup, NONLIGHTMAP );
#endif
}

// ----------------------------------------------------------------------------- //
// Adds shadow rendering data to a particular mesh builder
// ----------------------------------------------------------------------------- //
int DispInfo_AddShadowsToMeshBuilder( CMeshBuilder& meshBuilder, DispShadowHandle_t h, int baseIndex )
{
#ifdef DEDICATED
	return 0;
#else

	ShadowDecalRenderInfo_t info;
	CDispShadowDecal* pShadowDecal = &s_DispShadowDecals[h];
	g_pShadowMgr->ComputeRenderInfo( &info, pShadowDecal->m_Shadow );

	// It had better be computed by now...
	Assert( pShadowDecal->m_Flags & CDispDecalBase::FRAGMENTS_COMPUTED );

#ifdef _DEBUG
	int triCount = 0;
	int vertCount = 0;
#endif

	Vector2D texCoord;
	DispShadowFragmentHandle_t f = pShadowDecal->m_FirstFragment;
	CIndexBuilder &indexBuilder = meshBuilder;
	int nVertices = 0;
	int nIndices = 0;

	while ( f != DISP_SHADOW_FRAGMENT_HANDLE_INVALID )
	{
		const CDispShadowFragment& fragment = s_DispShadowFragments[f];
		const ShadowVertex_t *pShadowVert = fragment.m_ShadowVerts;
			
		// Add in the vertices + indices, use two loops to minimize tests...
		int i;
		int triangleCount = fragment.m_nVerts-2;
		indexBuilder.FastPolygon( nIndices, baseIndex, triangleCount );
		nIndices += 3 * triangleCount;

		for ( i = 0; i < triangleCount; ++i, ++pShadowVert )
		{
			// Transform + offset the texture coords
			Vector2DMultiply( pShadowVert->m_ShadowSpaceTexCoord.AsVector2D(), info.m_vTexSize, texCoord );
			texCoord += info.m_vTexOrigin;
			meshBuilder.Position3fv( nVertices, pShadowVert->m_Position.Base() );
			meshBuilder.TexCoord3f( nVertices, 0, texCoord.x, texCoord.y, pShadowVert->m_ShadowSpaceTexCoord.z );
			meshBuilder.TexCoord3fv( nVertices, 1, info.m_vShadowFalloffParams.Base() );
			++nVertices;
		}

		Vector2DMultiply( pShadowVert->m_ShadowSpaceTexCoord.AsVector2D(), info.m_vTexSize, texCoord );
		texCoord += info.m_vTexOrigin;
		meshBuilder.Position3fv( nVertices, pShadowVert->m_Position.Base() );
		meshBuilder.TexCoord3f( nVertices, 0, texCoord.x, texCoord.y, pShadowVert->m_ShadowSpaceTexCoord.z );
		meshBuilder.TexCoord3fv( nVertices, 1, info.m_vShadowFalloffParams.Base() );
		++pShadowVert;
		++nVertices;

		Vector2DMultiply( pShadowVert->m_ShadowSpaceTexCoord.AsVector2D(), info.m_vTexSize, texCoord );
		texCoord += info.m_vTexOrigin;
		meshBuilder.Position3fv( nVertices, pShadowVert->m_Position.Base() );
		meshBuilder.TexCoord3f( nVertices, 0, texCoord.x, texCoord.y, pShadowVert->m_ShadowSpaceTexCoord.z );
		meshBuilder.TexCoord3fv( nVertices, 1, info.m_vShadowFalloffParams.Base() );
		++nVertices;

		baseIndex += fragment.m_nVerts;
		f = s_DispShadowFragments.Next(f);

#ifdef _DEBUG
		triCount += fragment.m_nVerts - 2;
		vertCount += fragment.m_nVerts;
#endif
	}

	meshBuilder.AdvanceVertices( nVertices );
	meshBuilder.AdvanceIndices( nIndices );

#ifdef _DEBUG
	Assert( triCount == pShadowDecal->m_nTris );
	Assert( vertCount == pShadowDecal->m_nVerts );
#endif

	return baseIndex;
#endif
}


HDISPINFOARRAY DispInfo_CreateArray( int nElements )
{
	CDispArray *pRet = new CDispArray;

	pRet->m_CurTag = 1;

	pRet->m_nDispInfos = nElements;
	if ( nElements )
	{
		pRet->m_pDispInfos = new CDispInfo[nElements];
	}
	else
	{
		pRet->m_pDispInfos = NULL;
	}
	for( int i=0; i < nElements; i++ )
		pRet->m_pDispInfos[i].m_pDispArray = pRet;

	return (HDISPINFOARRAY)pRet;
}


void DispInfo_DeleteArray( HDISPINFOARRAY hArray )
{
	CDispArray *pArray = static_cast<CDispArray*>( hArray );
	if( !pArray )
		return;

	delete [] pArray->m_pDispInfos;
	delete pArray;
}


IDispInfo* DispInfo_IndexArray( HDISPINFOARRAY hArray, int iElement )
{
	CDispArray *pArray = static_cast<CDispArray*>( hArray );
	if( !pArray )
		return NULL;

	Assert( iElement >= 0 && iElement < pArray->m_nDispInfos );
	return &pArray->m_pDispInfos[iElement];
}

int DispInfo_ComputeIndex( HDISPINFOARRAY hArray, IDispInfo* pInfo )
{
	CDispArray *pArray = static_cast<CDispArray*>( hArray );
	if( !pArray )
		return NULL;

	int iElement = size_cast< int >( ((intp)pInfo - (intp)(pArray->m_pDispInfos)) / sizeof(CDispInfo) );

	Assert( iElement >= 0 && iElement < pArray->m_nDispInfos );
	return iElement;
}

void DispInfo_ClearAllTags( HDISPINFOARRAY hArray )
{
	CDispArray *pArray = static_cast<CDispArray*>( hArray );
	if( !pArray )
		return;

	++pArray->m_CurTag;
	if( pArray->m_CurTag == 0xFFFF )
	{
		// Reset all the tags.
		pArray->m_CurTag = 1;
		for( int i=0; i < pArray->m_nDispInfos; i++ )
			pArray->m_pDispInfos[i].m_Tag = 0;
	}
}


//-----------------------------------------------------------------------------
// Renders normals for the displacements
//-----------------------------------------------------------------------------

static void DispInfo_DrawChainNormals( IMatRenderContext *pRenderContext, SurfaceHandle_t *pList, int listCount )
{
#ifndef DEDICATED
	// Only do it in debug because we're only storing the info then
	Vector p;

	pRenderContext->Bind( g_pMaterialWireframeVertexColor );

	for ( int i = 0; i < listCount; i++ )
	{
		CDispInfo *pDisp = static_cast<CDispInfo*>( pList[i]->pDispInfo );
		
		int nVerts = pDisp->NumVerts();

		IMesh *pMesh = pRenderContext->GetDynamicMesh( );
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_LINES, nVerts * 3 );

		for( int iVert=0; iVert < nVerts; iVert++ )
		{
			CDispRenderVert* pVert = pDisp->GetVertex(iVert);
			meshBuilder.Position3fv( pVert->m_vPos.Base() );
			meshBuilder.Color3ub( 0, 0, 255 );
			meshBuilder.AdvanceVertex();

			VectorMA( pVert->m_vPos, 5.0f, pVert->m_vNormal, p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 0, 0, 255 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pVert->m_vPos.Base() );
			meshBuilder.Color3ub( 255, 0, 0 );
			meshBuilder.AdvanceVertex();

			VectorMA( pVert->m_vPos, 5.0f, pVert->m_vSVector, p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 255, 0, 0 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pVert->m_vPos.Base() );
			meshBuilder.Color3ub( 0, 255, 0 );
			meshBuilder.AdvanceVertex();

			VectorMA( pVert->m_vPos, 5.0f, pVert->m_vTVector, p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 0, 255, 0 );
			meshBuilder.AdvanceVertex();
		}

		meshBuilder.End();
		pMesh->Draw();
	}
#endif
}


//-----------------------------------------------------------------------------
// Renders debugging information for displacements 
//-----------------------------------------------------------------------------

void DispInfo_RenderListDebug( IMatRenderContext *pRenderContext, SurfaceHandle_t *pList, int listCount )
{
#ifndef DEDICATED
	VPROF("DispInfo_DrawDebugInformation");
	if ( DispInfoRenderDebugModes() )
	{
		for( int i = 0; i < listCount; i++ )
		{
			CDispInfo *pDisp = static_cast<CDispInfo*>( pList[i]->pDispInfo );
			pDisp->Render( pDisp->m_pMesh, true );
		}
	}
	// Overlay with normals if we're in that mode
	if( mat_normals.GetInt() )
	{
		DispInfo_DrawChainNormals(pRenderContext, pList, listCount);
	}
#endif
}

//-----------------------------------------------------------------------------
// Renders all displacements in sorted order 
//-----------------------------------------------------------------------------
void DispInfo_RenderListWorld( IMatRenderContext *pRenderContext, int nSortGroup, SurfaceHandle_t *pList, int listCount, bool bOrtho, unsigned long flags, int DepthMode )
{
#ifndef DEDICATED
	if( !r_DrawDisp.GetInt() || !listCount || !( flags & DRAWWORLDLISTS_DRAW_WORLD_GEOMETRY ) )
		return;

	g_bDispOrthoRender = bOrtho;

	// Build up the CPrimLists for all the displacements.
	CDispInfo *visibleDisps[MAX_MAP_DISPINFO];
	int nVisibleDisps;

	DispInfo_BuildPrimLists( nSortGroup, pList, listCount, DepthMode != DEPTH_MODE_NORMAL, visibleDisps, nVisibleDisps );

	// Draw..
	DispInfo_DrawPrimLists( pRenderContext, (ERenderDepthMode_t) DepthMode );
#endif
}

//-----------------------------------------------------------------------------
// Renders all displacements in sorted order 
//-----------------------------------------------------------------------------
void DispInfo_RenderListDecalsAndOverlays( IMatRenderContext *pRenderContext, int nSortGroup, SurfaceHandle_t *pList, int listCount, bool bOrtho, unsigned long flags )
{
#ifndef DEDICATED
	if( !r_DrawDisp.GetInt() || !listCount || !( flags & DRAWWORLDLISTS_DRAW_DECALS_AND_OVERLAYS ) )
		return;

	g_bDispOrthoRender = bOrtho;

	// Build up the CPrimLists for all the displacements.
	CDispInfo *visibleDisps[MAX_MAP_DISPINFO];
	int nVisibleDisps;

	DispInfo_GetVisibleDispsAndAddOverlayFragmentsToRenderList( nSortGroup, pList, listCount, visibleDisps, nVisibleDisps );

	// Add all displacements to the shadow render list
	for ( int i = 0; i < listCount; i++ )
	{
		SurfaceHandle_t pCur = pList[i];
		ShadowDecalHandle_t decalHandle = MSurf_ShadowDecals( pCur );
		if ( decalHandle != SHADOW_DECAL_HANDLE_INVALID )
		{
			g_pShadowMgr->AddShadowsOnSurfaceToRenderList( decalHandle );
		}
	}

	bool bFlashlightMask = !( (flags & DRAWWORLDLISTS_DRAW_REFRACTION ) || (flags & DRAWWORLDLISTS_DRAW_REFLECTION ));

	// Draw flashlight lighting for displacements
	g_pShadowMgr->RenderFlashlights( bFlashlightMask, false );

	// Draw overlays
	OverlayMgr()->RenderOverlays( pRenderContext, nSortGroup );

	// Draw flashlight overlays	
	g_pShadowMgr->DrawFlashlightOverlays( pRenderContext, nSortGroup, bFlashlightMask );
	OverlayMgr()->ClearRenderLists( nSortGroup );

	// Draw decals

	DispInfo_BatchDecals( visibleDisps, nVisibleDisps );
	DispInfo_DrawDecals( pRenderContext, visibleDisps, nVisibleDisps );

	// Draw flashlight decals
	g_pShadowMgr->DrawFlashlightDecalsOnDisplacements( pRenderContext, nSortGroup, visibleDisps, nVisibleDisps, bFlashlightMask );
	g_pShadowMgr->RenderFlashlights( bFlashlightMask, true );

	// draw shadows
	g_pShadowMgr->RenderShadows( pRenderContext );
	g_pShadowMgr->ClearShadowRenderList();
#endif
}

