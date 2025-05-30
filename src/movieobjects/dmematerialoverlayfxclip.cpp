﻿//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "movieobjects/dmematerialoverlayfxclip.h"
#include "datamodel/dmelementfactoryhelper.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "tier1/KeyValues.h"
#include "tier1/convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CDmeClip - common base class for filmclips, soundclips, and channelclips
//-----------------------------------------------------------------------------
IMPLEMENT_FX_CLIP_ELEMENT_FACTORY( DmeMaterialOverlayFXClip, CDmeMaterialOverlayFXClip, "Material Overlay Effect" );

void CDmeMaterialOverlayFXClip::OnConstruction()
{
	m_Material.Init( this, "material" );
	m_Color.Init( this, "overlaycolor" );
	m_nLeft.Init( this, "left" );
	m_nTop.Init( this, "top" );
	m_nWidth.Init( this, "width" );
	m_nHeight.Init( this, "height" );
	m_bFullScreen.Init( this, "fullscreen" );
	m_bUseSubRect.Init( this, "useSubRect" );
	m_flMovementAngle.Init( this, "movementAngle" );
	m_flMovementSpeed.Init( this, "movementSpeed" );
	m_nSubRectLeft.Init( this, "subRectLeft" );
	m_nSubRectTop.Init( this, "subRectTop" );
	m_nSubRectWidth.Init( this, "subRectWidth" );
	m_nSubRectHeight.Init( this, "subRectHeight" );

	m_Color.SetColor( 255, 255, 255, 255 );
	m_bFullScreen = true;
	m_nLeft = m_nTop = 0;
	m_nWidth = m_nHeight = 1;

	// Set up the Blt material
	KeyValues *pVMTKeyValues = new KeyValues( "accumbuff4sample" );
	pVMTKeyValues->SetString( "$INPUT", "Effects/FilmScan256" ); // dummy

	KeyValues *pProxiesKV = pVMTKeyValues->FindKey( "proxies", true );	// create a subkey
	pProxiesKV->FindKey( "sfm_blt", true );

	m_BltMaterial.Init( "accumbuff4sample", pVMTKeyValues );
	m_BltMaterial->Refresh();
}

void CDmeMaterialOverlayFXClip::OnDestruction()
{
	m_BltMaterial.Shutdown();
}


//-----------------------------------------------------------------------------
// Resolve
//-----------------------------------------------------------------------------
void CDmeMaterialOverlayFXClip::Resolve()
{
	if ( m_Material.IsDirty() )
	{
		m_OverlayMaterial.Shutdown();
		const char *pName = m_Material.Get();
		if ( pName && pName[0] )
		{
			m_OverlayMaterial.Init( pName, NULL, false );
		}
		m_Material.GetAttribute()->RemoveFlag( FATTRIB_DIRTY );
	}
}


//-----------------------------------------------------------------------------
// Helper for overlays
//-----------------------------------------------------------------------------
void CDmeMaterialOverlayFXClip::SetOverlayEffect( const char *pMaterialName )
{
	m_Material = pMaterialName;
}

void CDmeMaterialOverlayFXClip::SetAlpha( float flAlpha )
{
	m_Color.SetAlpha( flAlpha * 255 );
}

float CDmeMaterialOverlayFXClip::GetAlpha( void )
{
	return ( (float)m_Color.a() ) / 255.0f;
}

bool CDmeMaterialOverlayFXClip::HasOpaqueOverlay( void )
{
	if ( m_OverlayMaterial )
		return ( !m_OverlayMaterial->IsTranslucent() && ( m_Color.a() == 255 ) && m_bFullScreen );

	// no material overlay set
	return false;
}

IMaterial *CDmeMaterialOverlayFXClip::GetMaterial()
{
	return m_OverlayMaterial;
}


void CDmeMaterialOverlayFXClip::DrawQuad( int x, int y, int w, int h, float u0, float v0, float u1, float v1 )
{
	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.BoneWeight( 0, 1.0f );
	meshBuilder.BoneMatrix( 0, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0, u0, v0 );
	meshBuilder.TexCoord2f( 1, 0.0f, 0.0f );
	meshBuilder.TexCoord2f( 2, 0.0f, 0.0f );
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 3>();

	meshBuilder.Position3f( x, y+h, 0.0f );
	meshBuilder.BoneWeight( 0, 1.0f );
	meshBuilder.BoneMatrix( 0, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0, u0, v1 );
	meshBuilder.TexCoord2f( 1, 0.0f, 1.0f );
	meshBuilder.TexCoord2f( 2, 0.0f, 0.0f );
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 3>();

	meshBuilder.Position3f( x+w, y, 0.0f );
	meshBuilder.BoneWeight( 0, 1.0f );
	meshBuilder.BoneMatrix( 0, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0, u1, v0 );
	meshBuilder.TexCoord2f( 1, 1.0f, 0.0f );
	meshBuilder.TexCoord2f( 2, 0.0f, 0.0f );
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 3>();

	meshBuilder.Position3f( x+w, y+h, 0.0f );
	meshBuilder.BoneWeight( 0, 1.0f );
	meshBuilder.BoneMatrix( 0, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0, u1, v1 );
	meshBuilder.TexCoord2f( 1, 1.0f, 1.0f );
	meshBuilder.TexCoord2f( 2, 0.0f, 0.0f );
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS | VTX_HAVECOLOR, 3>();

	meshBuilder.End();
	pMesh->Draw();
}

void CDmeMaterialOverlayFXClip::DrawOneToOneQuad( int nWidth, int nHeight )
{
	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;

	// Epsilons for 1:1 texel to pixel mapping
	float fWidthEpsilon = 0.5f / ((float) nWidth);
	float fHeightEpsilon = 0.5f / ((float) nHeight);

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( -1.0f, 1.0f, 0.5f );	// Upper left
	meshBuilder.TexCoord2f( 0, 0.0f + fWidthEpsilon, 0.0f + fHeightEpsilon);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.Position3f( -1.0f,  -1.0f, 0.5f );	// Lower left
	meshBuilder.TexCoord2f( 0, 0.0f + fWidthEpsilon, 1.0f + fHeightEpsilon);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.Position3f( 1.0f, -1.0f, 0.5 );		// Lower right
	meshBuilder.TexCoord2f( 0, 1.0f + fWidthEpsilon, 1.0f + fHeightEpsilon);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.Position3f( 1.0f, 1.0f, 0.5 );		// Upper right
	meshBuilder.TexCoord2f( 0, 1.0f + fWidthEpsilon, 0.0f + fHeightEpsilon);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// All effects must be able to apply their effect
//-----------------------------------------------------------------------------
void CDmeMaterialOverlayFXClip::ApplyEffect( DmeTime_t time, Rect_t &currentRect, Rect_t &totalRect, ITexture *pTextures[MAX_FX_INPUT_TEXTURES] )
{
	if ( !m_OverlayMaterial || !m_BltMaterial || m_Color.a() == 0 )
		return;
	
	time = ToChildMediaTime( time, false );

	// Clip the overlay rectangle to the currently drawn one
	int x, y, w, h;
	int tx, ty, tw, th;
	if ( m_bFullScreen )
	{
		x = currentRect.x; 
		y = currentRect.y; 
		w = currentRect.width; 
		h = currentRect.height;
		tx = ty = 0;
		tw = totalRect.width;
		th = totalRect.height;
	}
	else
	{
		x = clamp( m_nLeft.Get(), currentRect.x, currentRect.x + currentRect.width );
		y = clamp( m_nTop.Get(), currentRect.y, currentRect.y + currentRect.height );
		int x1 = clamp( m_nLeft + m_nWidth, currentRect.x, currentRect.x + currentRect.width );
		int y1 = clamp( m_nTop + m_nHeight, currentRect.y, currentRect.y + currentRect.height );
		w = x1 - x;
		h = y1 - y;

		tx = m_nLeft;
		ty = m_nTop;
		tw = m_nWidth;
		th = m_nHeight;

		// Clipped...
		if ( w <= 0 || h <= 0 )
			return;
	}

	if ( tw == 0 || th == 0 )
		return;

	// Compute texture coordinate range of the entire texture
	int mw = m_OverlayMaterial->GetMappingWidth();
	int mh = m_OverlayMaterial->GetMappingHeight();

	// Compute the texture coords in texels we want over the entire image
	float uMin = 0;
	float uMax = mw;
	float vMin = 0;
	float vMax = mh;

	if ( m_bUseSubRect )
	{
		uMin = m_nSubRectLeft;
		vMin = m_nSubRectTop;
		uMax = uMin + m_nSubRectWidth;
		vMax = vMin + m_nSubRectHeight;
	}

	if ( m_flMovementSpeed )
	{
		float flRadians = M_PI * m_flMovementAngle / 180.0f;
		float dUdT = -m_flMovementSpeed * cos( flRadians );
		float dVdT = m_flMovementSpeed * sin( flRadians );
		float dU = time.GetSeconds() * dUdT;
		float dV = time.GetSeconds() * dVdT;
		uMin += dU; uMax += dU;
		vMin += dV; vMax += dV;
	}

	// This is the range of normalizes (u,v) coordinates over the *total* image
	uMin = ( uMin + 0.5f ) / mw;
	vMin = ( vMin + 0.5f ) / mh;
	uMax = ( uMax - 0.5f ) / mw;
	vMax = ( vMax - 0.5f ) / mh;

	// Now determine the subrange we should use given we're rendering a portion of the image
	float u0, v0, u1, v1, f;
	
	f = ( x - tx ) / tw;
	u0 = Lerp( f, uMin, uMax );

	f = ( x + w - tx ) / tw;
	u1 = Lerp( f, uMin, uMax );

	f = ( y - ty ) / th;
	v0 = Lerp( f, vMin, vMax );

	f = ( y + h - ty ) / th;
	v1 = Lerp( f, vMin, vMax );

	x -= currentRect.x;
	y -= currentRect.y;

	CMatRenderContextPtr pRenderContext( materials );

	if ( m_OverlayMaterial->NeedsPowerOfTwoFrameBufferTexture() )
	{
		pRenderContext->Bind( m_BltMaterial, pTextures[0] );
		pRenderContext->PushRenderTargetAndViewport( pTextures[1], 0, 0, w, h );		// Blt current input to temp surface
		DrawOneToOneQuad( w, h );
		pRenderContext->PopRenderTargetAndViewport();

		pRenderContext->SetFrameBufferCopyTexture( pTextures[1] );

		pRenderContext->PushRenderTargetAndViewport( pTextures[0], 0, 0, w, h );			// Render to result surface
	}

	float r, g, b, a;
	m_OverlayMaterial->GetColorModulation( &r, &g, &b );
	a = m_OverlayMaterial->GetAlphaModulation();

	m_OverlayMaterial->ColorModulate( m_Color.r() / 255.0f, m_Color.g() / 255.0f, m_Color.b() / 255.0f );
	m_OverlayMaterial->AlphaModulate( m_Color.a() / 255.0f );

	pRenderContext->Bind( m_OverlayMaterial );

	DrawQuad( x, y, w, h, u0, v0, u1, v1 );

	if ( m_OverlayMaterial->NeedsPowerOfTwoFrameBufferTexture() )
	{
		pRenderContext->PopRenderTargetAndViewport();
	}

	m_OverlayMaterial->ColorModulate( r, g, b );
	m_OverlayMaterial->AlphaModulate( a );
}

