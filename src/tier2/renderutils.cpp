﻿//===== Copyright ¿½ 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A set of utilities to render standard shapes
//
//===========================================================================//

#include "tier2/renderutils.h"
#include "tier2/tier2.h"
#include "tier1/KeyValues.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "tier1/callqueue.h"
#include "tier0/vprof.h"
#include "tier0/basetypes.h"
#include "togl/rendermechanism.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

#if !defined(M_PI)
	#define M_PI			3.14159265358979323846
#endif

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
static bool s_bMaterialsInitialized = false;
static IMaterial *s_pWireframe;
static IMaterial *s_pWireframeIgnoreZ;
static IMaterial *s_pVertexColor;
static IMaterial *s_pVertexColorIgnoreZ;


//-----------------------------------------------------------------------------
// Initializes standard materials
//-----------------------------------------------------------------------------
void InitializeStandardMaterials()
{
	LOCAL_THREAD_LOCK();

	if ( s_bMaterialsInitialized )
		return;

	s_bMaterialsInitialized = true;


	KeyValues *pVMTKeyValues = new KeyValues( "wireframe" );
	pVMTKeyValues->SetInt( "$vertexcolor", 1 );
	s_pWireframe = g_pMaterialSystem->CreateMaterial( "__utilWireframe", pVMTKeyValues );
	s_pWireframe->IncrementReferenceCount();

	pVMTKeyValues = new KeyValues( "wireframe" );
	pVMTKeyValues->SetInt( "$vertexcolor", 1 );
	pVMTKeyValues->SetInt( "$vertexalpha", 1 );
	pVMTKeyValues->SetInt( "$ignorez", 1 );
	s_pWireframeIgnoreZ = g_pMaterialSystem->CreateMaterial( "__utilWireframeIgnoreZ", pVMTKeyValues );
	s_pWireframeIgnoreZ->IncrementReferenceCount();

	pVMTKeyValues = new KeyValues( "unlitgeneric" );
	pVMTKeyValues->SetInt( "$vertexcolor", 1 );
	pVMTKeyValues->SetInt( "$vertexalpha", 1 );
	s_pVertexColor = g_pMaterialSystem->CreateMaterial( "__utilVertexColor", pVMTKeyValues );
	s_pVertexColor->IncrementReferenceCount();

	pVMTKeyValues = new KeyValues( "unlitgeneric" );
	pVMTKeyValues->SetInt( "$vertexcolor", 1 );
	pVMTKeyValues->SetInt( "$vertexalpha", 1 );
	pVMTKeyValues->SetInt( "$ignorez", 1 );
	s_pVertexColorIgnoreZ = g_pMaterialSystem->CreateMaterial( "__utilVertexColorIgnoreZ", pVMTKeyValues );
	s_pVertexColorIgnoreZ->IncrementReferenceCount();
}

void ShutdownStandardMaterials()
{
	if ( !s_bMaterialsInitialized )
		return;

	s_bMaterialsInitialized = false;

	s_pWireframe->DecrementReferenceCount();
	s_pWireframe = NULL;

	s_pWireframeIgnoreZ->DecrementReferenceCount();
	s_pWireframeIgnoreZ = NULL;

	s_pVertexColor->DecrementReferenceCount();
	s_pVertexColor = NULL;

	s_pVertexColorIgnoreZ->DecrementReferenceCount();
	s_pVertexColorIgnoreZ = NULL;
}


//-----------------------------------------------------------------------------
// Renders a wireframe sphere
//-----------------------------------------------------------------------------
void RenderWireframeSphere( const Vector &vCenter, float flRadius, int nTheta, int nPhi, Color c, bool bZBuffer )
{
	InitializeStandardMaterials();

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	ICallQueue *pCallQueue = pRenderContext->GetCallQueue();
	if ( pCallQueue )
	{
		pCallQueue->QueueCall( RenderWireframeSphere, RefToVal( vCenter ), flRadius, nTheta, nPhi, c, bZBuffer );
		return;
	}

	// Make one more coordinate because (u,v) is discontinuous.
	++nTheta;

	int nVertices = nPhi * nTheta; 
	int nIndices = ( nTheta - 1 ) * 4 * ( nPhi - 1 );

	pRenderContext->Bind( bZBuffer ? s_pWireframe : s_pWireframeIgnoreZ );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_LINES, nVertices, nIndices );

	unsigned char chRed = c.r();
	unsigned char chGreen = c.g();
	unsigned char chBlue = c.b();
	unsigned char chAlpha = c.a();

	int i, j;
	for ( i = 0; i < nPhi; ++i )
	{
		for ( j = 0; j < nTheta; ++j )
		{
			float u = j / ( float )( nTheta - 1 );
			float v = i / ( float )( nPhi - 1 );
			float theta = 2.0f * M_PI * u;
			float phi = M_PI * v;

			meshBuilder.Position3f( vCenter.x + ( flRadius * sin(phi) * cos(theta) ),
				vCenter.y + ( flRadius * sin(phi) * sin(theta) ), 
				vCenter.z + ( flRadius * cos(phi) ) );
			meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
			meshBuilder.AdvanceVertex();
		}
	}

	for ( i = 0; i < nPhi - 1; ++i )
	{
		for ( j = 0; j < nTheta - 1; ++j )
		{
			int idx = nTheta * i + j;

			meshBuilder.Index( idx );
			meshBuilder.AdvanceIndex();

			meshBuilder.Index( idx + nTheta );
			meshBuilder.AdvanceIndex();

			meshBuilder.Index( idx );
			meshBuilder.AdvanceIndex();

			meshBuilder.Index( idx + 1 );
			meshBuilder.AdvanceIndex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Draws a sphere
//-----------------------------------------------------------------------------
void RenderSphereInternal( const Vector &vCenter, float flRadius, int nTheta, int nPhi, Color c, IMaterial *pMaterial, bool bInsideOut )
{
	InitializeStandardMaterials();

	CMatRenderContextPtr pRenderContext( materials );
	ICallQueue *pCallQueue = pRenderContext->GetCallQueue();
	if ( pCallQueue )
	{
		pCallQueue->QueueCall( RenderSphereInternal, RefToVal( vCenter ), flRadius, nTheta, nPhi, c, pMaterial, bInsideOut );
		return;
	}

	unsigned char chRed = c.r();
	unsigned char chGreen = c.g();
	unsigned char chBlue = c.b();
	unsigned char chAlpha = c.a();

	// Two extra degenerate triangles per row (except the last one)
	int nTriangles = 2 * nTheta * ( nPhi - 1 ); 
	int nIndices = 2 * ( nTheta + 1 ) * ( nPhi - 1 );
	if ( nTriangles == 0 )
		return;

	pRenderContext->Bind( pMaterial );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( );
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, nTriangles, nIndices );

	// Build the index buffer.
	float flOONPhi = 1.0f / (nPhi-1);
	float flOONTheta = 1.0f / (nTheta-1);

	int i, j;
	for ( i = 0; i < nPhi; ++i )
	{
		for ( j = 0; j < nTheta; ++j )
		{
			float u = j / ( float )( nTheta - 1 );
			float v = i / ( float )( nPhi - 1 );
			float theta = 2.0f * M_PI * u;
			float phi = M_PI * v;

			Vector vecPos;
			vecPos.x = flRadius * sin(phi) * cos(theta);
			vecPos.y = flRadius * sin(phi) * sin(theta); 
			vecPos.z = flRadius * cos(phi);

			Vector vecNormal = vecPos;
			VectorNormalize(vecNormal);

			Vector4D vecTangent( -vecPos.y, vecPos.x, 0.0f, 1.0f );
			VectorNormalize( vecTangent.AsVector3D() );
			vecPos += vCenter;

			meshBuilder.Position3f( vecPos.x, vecPos.y, vecPos.z );
			meshBuilder.Normal3f( vecNormal.x, vecNormal.y, vecNormal.z );
			meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
			meshBuilder.UserData( vecTangent.Base() );
			meshBuilder.TexCoord2f( 0, j * flOONTheta, i * flOONPhi );
			meshBuilder.AdvanceVertex();
		}
	}

	// Emit the triangle strips.
	int idx = 0;
	for ( i = 0; i < nPhi - 1; ++i )
	{
		if ( bInsideOut )
		{
			for ( j = nTheta-1; j >= 0; --j )
			{
				idx = nTheta * i + j;

				meshBuilder.Index( idx + nTheta );
				meshBuilder.AdvanceIndex();

				meshBuilder.Index( idx );
				meshBuilder.AdvanceIndex();
			}
		}
		else
		{
			for ( j = 0; j < nTheta; ++j )
			{
				idx = nTheta * i + j;

				meshBuilder.Index( idx + nTheta );
				meshBuilder.AdvanceIndex();

				meshBuilder.Index( idx );
				meshBuilder.AdvanceIndex();
			}

			// Emit a degenerate triangle to skip to the next row without a connecting triangle
			if ( i < nPhi - 2 )
			{
				meshBuilder.Index( idx );
				meshBuilder.AdvanceIndex();

				meshBuilder.Index( idx + nTheta + 1 );
				meshBuilder.AdvanceIndex();
			}
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}

void RenderSphere( const Vector &vCenter, float flRadius, int nTheta, int nPhi, Color c, IMaterial *pMaterial, bool bInsideOut )
{
	RenderSphereInternal( vCenter, flRadius, nTheta, nPhi, c, pMaterial, bInsideOut );
}

void RenderSphere( const Vector &vCenter, float flRadius, int nTheta, int nPhi, Color c, bool bZBuffer, bool bInsideOut )
{
	InitializeStandardMaterials();

	IMaterial *pMaterial = bZBuffer ? s_pVertexColor : s_pVertexColorIgnoreZ;
	Color cActual( c.r(), c.g(), c.b(), c.a() );
	RenderSphereInternal( vCenter, flRadius, nTheta, nPhi, cActual, pMaterial, bInsideOut );
}


//-----------------------------------------------------------------------------
// Box vertices
//-----------------------------------------------------------------------------
static int s_pBoxFaceIndices[6][4] = 
{
	{ 0, 4, 6, 2 }, // -x
	{ 5, 1, 3, 7 }, // +x
	{ 0, 1, 5, 4 }, // -y
	{ 2, 6, 7, 3 }, // +y
	{ 0, 2, 3, 1 },	// -z
	{ 4, 5, 7, 6 }	// +z
};

static int s_pBoxFaceIndicesInsideOut[6][4] = 
{
	{ 0, 2, 6, 4 }, // -x
	{ 5, 7, 3, 1 }, // +x
	{ 0, 4, 5, 1 }, // -y
	{ 2, 3, 7, 6 }, // +y
	{ 0, 1, 3, 2 },	// -z
	{ 4, 6, 7, 5 }	// +z
};

static void GenerateBoxVertices( const Vector &vOrigin, const QAngle& angles, const Vector &vMins, const Vector &vMaxs, Vector pVerts[8] )
{
	// Build a rotation matrix from orientation
	matrix3x4_t fRotateMatrix;
	AngleMatrix( angles, fRotateMatrix );

	Vector vecPos;
	for ( int i = 0; i < 8; ++i )
	{
		vecPos[0] = ( i & 0x1 ) ? vMaxs[0] : vMins[0];
		vecPos[1] = ( i & 0x2 ) ? vMaxs[1] : vMins[1];
		vecPos[2] = ( i & 0x4 ) ? vMaxs[2] : vMins[2];

		VectorRotate( vecPos, fRotateMatrix, pVerts[i] );
		pVerts[i] += vOrigin;
	}
}


//-----------------------------------------------------------------------------
// Renders a wireframe box relative to an origin 
//-----------------------------------------------------------------------------
void RenderWireframeBox( const Vector &vOrigin, const QAngle& angles, const Vector &vMins, const Vector &vMaxs, Color c, bool bZBuffer )
{
	InitializeStandardMaterials();

	CMatRenderContextPtr pRenderContext( materials );
	ICallQueue *pCallQueue = pRenderContext->GetCallQueue();
	if ( pCallQueue )
	{
		pCallQueue->QueueCall( RenderWireframeBox, RefToVal( vOrigin ), RefToVal( angles ), RefToVal( vMins ), RefToVal( vMaxs ), c, bZBuffer );
		return;
	}

	pRenderContext->Bind( bZBuffer ? s_pWireframe : s_pWireframeIgnoreZ );

	Vector p[8];
	GenerateBoxVertices( vOrigin, angles, vMins, vMaxs, p );

	unsigned char chRed = c.r();
	unsigned char chGreen = c.g();
	unsigned char chBlue = c.b();
	unsigned char chAlpha = c.a();

	IMesh *pMesh = pRenderContext->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 24 );

	// Draw the box
	for ( int i = 0; i < 6; i++ )
	{
		int *pFaceIndex = s_pBoxFaceIndices[i];

		for ( int j = 0; j < 4; ++j )
		{
			meshBuilder.Position3fv( p[pFaceIndex[j]].Base() );
			meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( p[pFaceIndex[ (j == 3) ? 0 : j+1 ] ].Base() );
			meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
			meshBuilder.AdvanceVertex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Renders a solid box 
//-----------------------------------------------------------------------------
void RenderBoxInternal( const Vector& vOrigin, const QAngle& angles, const Vector& vMins, const Vector& vMaxs, Color c, IMaterial *pMaterial, bool bInsideOut )
{
	InitializeStandardMaterials();

	CMatRenderContextPtr pRenderContext( materials );
	ICallQueue *pCallQueue = pRenderContext->GetCallQueue();
	if ( pCallQueue )
	{
		pCallQueue->QueueCall( RenderBoxInternal, RefToVal( vOrigin ), RefToVal( angles ), RefToVal( vMins ), RefToVal( vMaxs ), c, pMaterial, bInsideOut );
		return;
	}


	pRenderContext->Bind( pMaterial );

	Vector p[8];
	GenerateBoxVertices( vOrigin, angles, vMins, vMaxs, p );

	unsigned char chRed = c.r();
	unsigned char chGreen = c.g();
	unsigned char chBlue = c.b();
	unsigned char chAlpha = c.a();

	IMesh *pMesh = pRenderContext->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 12 );

	// Draw the box
	Vector vecNormal;
	for ( int i = 0; i < 6; i++ )
	{
		vecNormal.Init();
		vecNormal[ i/2 ] = ( i & 0x1 ) ? 1.0f : -1.0f;
				 
		int *ppFaceIndices = bInsideOut ? s_pBoxFaceIndicesInsideOut[i] : s_pBoxFaceIndices[i];
		for ( int j = 1; j < 3; ++j )
		{
			int i0 = ppFaceIndices[0];
			int i1 = ppFaceIndices[j];
			int i2 = ppFaceIndices[j+1];

			meshBuilder.Position3fv( p[i0].Base() );
			meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
			meshBuilder.Normal3fv( vecNormal.Base() );
			meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( p[i2].Base() );
			meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
			meshBuilder.Normal3fv( vecNormal.Base() );
			meshBuilder.TexCoord2f( 0, 1.0f, ( j == 1 ) ? 1.0f : 0.0f );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( p[i1].Base() );
			meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
			meshBuilder.Normal3fv( vecNormal.Base() );
			meshBuilder.TexCoord2f( 0, ( j == 1 ) ? 0.0f : 1.0f, 1.0f );
			meshBuilder.AdvanceVertex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}

void RenderBox( const Vector& vOrigin, const QAngle& angles, const Vector& vMins, const Vector& vMaxs, Color c, IMaterial *pMaterial, bool bInsideOut )
{
	RenderBoxInternal( vOrigin, angles, vMins, vMaxs, c, pMaterial, bInsideOut );
}

void RenderBox( const Vector& vOrigin, const QAngle& angles, const Vector& vMins, const Vector& vMaxs, Color c, bool bZBuffer, bool bInsideOut )
{
	InitializeStandardMaterials();

	IMaterial *pMaterial = bZBuffer ? s_pVertexColor : s_pVertexColorIgnoreZ;
	Color cActual( c.r(), c.g(), c.b(), c.a() );
	RenderBoxInternal( vOrigin, angles, vMins, vMaxs, cActual, pMaterial, bInsideOut );
}


//-----------------------------------------------------------------------------
// Renders axes, red->x, green->y, blue->z
//-----------------------------------------------------------------------------
void RenderAxesAtOrigin( const Vector &vOrigin, float flScale, bool bZBuffer )
{
	RenderAxes( vOrigin, flScale, bZBuffer );
}

void RenderAxes( const Vector &vOrigin, float flScale, bool bZBuffer )
{
	InitializeStandardMaterials();

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	ICallQueue *pCallQueue = pRenderContext->GetCallQueue();
	if ( pCallQueue )
	{
		pCallQueue->QueueCall( RenderAxesAtOrigin, RefToVal( vOrigin ), flScale, bZBuffer );
		return;
	}

	pRenderContext->Bind( bZBuffer ? s_pWireframe : s_pWireframeIgnoreZ );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 3 );

	meshBuilder.Position3f( vOrigin.x, vOrigin.y, vOrigin.z );
	meshBuilder.Color4ub( 255, 0, 0, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vOrigin.x + flScale, vOrigin.y, vOrigin.z );
	meshBuilder.Color4ub( 255, 0, 0, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vOrigin.x, vOrigin.y, vOrigin.z );
	meshBuilder.Color4ub( 0, 255, 0, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vOrigin.x, vOrigin.y + flScale, vOrigin.z );
	meshBuilder.Color4ub( 0, 255, 0, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vOrigin.x, vOrigin.y, vOrigin.z );
	meshBuilder.Color4ub( 0, 0, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vOrigin.x, vOrigin.y, vOrigin.z + flScale );
	meshBuilder.Color4ub( 0, 0, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

void RenderAxesWithTransform( const matrix3x4_t &transform, float flScale, bool bZBuffer )
{
	RenderAxes( transform, flScale, bZBuffer );
}

void RenderAxes( const matrix3x4_t &transform, float flScale, bool bZBuffer )
{
	InitializeStandardMaterials();

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	ICallQueue *pCallQueue = pRenderContext->GetCallQueue();
	if ( pCallQueue )
	{
		pCallQueue->QueueCall( RenderAxesWithTransform, RefToVal( transform ), flScale, bZBuffer );
		return;
	}

	Vector xAxis, yAxis, zAxis, vOrigin, temp;
	MatrixGetColumn( transform, 0, xAxis );
	MatrixGetColumn( transform, 1, yAxis );
	MatrixGetColumn( transform, 2, zAxis );
	MatrixGetColumn( transform, 3, vOrigin );

	pRenderContext->Bind( bZBuffer ? s_pWireframe : s_pWireframeIgnoreZ );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 3 );

	meshBuilder.Position3fv( vOrigin.Base() );
	meshBuilder.Color4ub( 255, 0, 0, 255 );
	meshBuilder.AdvanceVertex();

	VectorMA( vOrigin, flScale, xAxis, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( 255, 0, 0, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vOrigin.x, vOrigin.y, vOrigin.z );
	meshBuilder.Color4ub( 0, 255, 0, 255 );
	meshBuilder.AdvanceVertex();

	VectorMA( vOrigin, flScale, yAxis, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( 0, 255, 0, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vOrigin.x, vOrigin.y, vOrigin.z );
	meshBuilder.Color4ub( 0, 0, 255, 255 );
	meshBuilder.AdvanceVertex();

	VectorMA( vOrigin, flScale, zAxis, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( 0, 0, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Render a line
//-----------------------------------------------------------------------------
void RenderLine( const Vector& v1, const Vector& v2, Color c, bool bZBuffer )
{
	InitializeStandardMaterials();

	CMatRenderContextPtr pRenderContext( materials );
	ICallQueue *pCallQueue = pRenderContext->GetCallQueue();
	if ( pCallQueue )
	{
		pCallQueue->QueueCall( RenderLine, RefToVal( v1 ), RefToVal( v2 ), c, bZBuffer );
		return;
	}

	pRenderContext->Bind( bZBuffer ? s_pWireframe : s_pWireframeIgnoreZ );

	unsigned char chRed = c.r();
	unsigned char chGreen = c.g();
	unsigned char chBlue = c.b();
	unsigned char chAlpha = c.a();

	IMesh *pMesh = pRenderContext->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

	meshBuilder.Position3fv( v1.Base() );
	meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( v2.Base() );
	meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

// todo: draw a capsule procedurally instead of using these baked-in unit capsule verts
#define CAPSULE_VERTS 74
#define CAPSULE_LINES 117

float g_capsuleVertPositions[CAPSULE_VERTS][3] = {
	{ -0.01, -0.01, 1.0 },	{ 0.51, 0.0, 0.86 },	{ 0.44, 0.25, 0.86 },	{ 0.25, 0.44, 0.86 },	{ -0.01, 0.51, 0.86 },	{ -0.26, 0.44, 0.86 },	{ -0.45, 0.25, 0.86 },	{ -0.51, 0.0, 0.86 },	{ -0.45, -0.26, 0.86 },
	{ -0.26, -0.45, 0.86 },	{ -0.01, -0.51, 0.86 },	{ 0.25, -0.45, 0.86 },	{ 0.44, -0.26, 0.86 },	{ 0.86, 0.0, 0.51 },	{ 0.75, 0.43, 0.51 },	{ 0.43, 0.75, 0.51 },	{ -0.01, 0.86, 0.51 },	{ -0.44, 0.75, 0.51 },
	{ -0.76, 0.43, 0.51 },	{ -0.87, 0.0, 0.51 },	{ -0.76, -0.44, 0.51 },	{ -0.44, -0.76, 0.51 },	{ -0.01, -0.87, 0.51 },	{ 0.43, -0.76, 0.51 },	{ 0.75, -0.44, 0.51 },	{ 1.0, 0.0, 0.01 },		{ 0.86, 0.5, 0.01 },
	{ 0.49, 0.86, 0.01 },	{ -0.01, 1.0, 0.01 },	{ -0.51, 0.86, 0.01 },	{ -0.87, 0.5, 0.01 },	{ -1.0, 0.0, 0.01 },	{ -0.87, -0.5, 0.01 },	{ -0.51, -0.87, 0.01 },	{ -0.01, -1.0, 0.01 },	{ 0.49, -0.87, 0.01 },
	{ 0.86, -0.51, 0.01 },	{ 1.0, 0.0, -0.02 },	{ 0.86, 0.5, -0.02 },	{ 0.49, 0.86, -0.02 },	{ -0.01, 1.0, -0.02 },	{ -0.51, 0.86, -0.02 },	{ -0.87, 0.5, -0.02 },	{ -1.0, 0.0, -0.02 },	{ -0.87, -0.5, -0.02 },
	{ -0.51, -0.87, -0.02 },{ -0.01, -1.0, -0.02 },	{ 0.49, -0.87, -0.02 },	{ 0.86, -0.51, -0.02 },	{ 0.86, 0.0, -0.51 },	{ 0.75, 0.43, -0.51 },	{ 0.43, 0.75, -0.51 },	{ -0.01, 0.86, -0.51 },	{ -0.44, 0.75, -0.51 },
	{ -0.76, 0.43, -0.51 },	{ -0.87, 0.0, -0.51 },	{ -0.76, -0.44, -0.51 },{ -0.44, -0.76, -0.51 },{ -0.01, -0.87, -0.51 },{ 0.43, -0.76, -0.51 },	{ 0.75, -0.44, -0.51 },	{ 0.51, 0.0, -0.87 },	{ 0.44, 0.25, -0.87 },
	{ 0.25, 0.44, -0.87 },	{ -0.01, 0.51, -0.87 },	{ -0.26, 0.44, -0.87 },	{ -0.45, 0.25, -0.87 },	{ -0.51, 0.0, -0.87 },	{ -0.45, -0.26, -0.87 },{ -0.26, -0.45, -0.87 },{ -0.01, -0.51, -0.87 },{ 0.25, -0.45, -0.87 },
	{ 0.44, -0.26, -0.87 },	{ 0.0, 0.0, -1.0 },
};

int g_capsuleLineIndices[CAPSULE_LINES] = { -1,
	14,		0,	4,	16,	28,	40,	52,	64,	73,	70,	58,	46,	34,	22,	10,		-1,
	14,		0,	1,	13,	25,	37,	49,	61,	73,	67,	55,	43,	31,	19,	7,		-1,
	12,		61,	62,	63,	64,	65,	66,	67,	68,	69,	70,	71,	72,				-1,
	12,		49,	50,	51,	52,	53,	54,	55,	56,	57,	58,	59,	60,				-1,
	12,		37,	38,	39,	40,	41,	42,	43,	44,	45,	46,	47,	48,				-1,
	12,		25,	26,	27,	28,	29,	30,	31,	32,	33,	34,	35,	36,				-1,
	12,		13,	14,	15,	16,	17,	18,	19,	20,	21,	22,	23,	24,				-1,
	12,		1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,				-1
};

void RenderCapsule( const Vector &vStart, const Vector &vEnd, const float &flRadius, Color c, IMaterial *pMaterial )
{
	InitializeStandardMaterials();

	CMatRenderContextPtr pRenderContext( materials );
	ICallQueue *pCallQueue = pRenderContext->GetCallQueue();
	if ( pCallQueue )
	{
		pCallQueue->QueueCall( RenderCapsule, RefToVal( vStart ), RefToVal( vEnd ), RefToVal( flRadius ), c, pMaterial );
		return;
	}

	//RenderLine( vStart, vEnd, c, false );

	Vector vecCapsuleCoreNormal = ( vStart - vEnd ).Normalized();

	matrix3x4_t matCapsuleRotationSpace;
	VectorMatrix( Vector(0,0,1), matCapsuleRotationSpace );

	matrix3x4_t matCapsuleSpace;
	VectorMatrix( vecCapsuleCoreNormal, matCapsuleSpace );

	Vector v[CAPSULE_VERTS];
	Vector vecLen = (vEnd - vStart);
	for ( int i=0; i<CAPSULE_VERTS; i++ )
	{
		Vector vecCapsuleVert = Vector( g_capsuleVertPositions[i][0], g_capsuleVertPositions[i][1], g_capsuleVertPositions[i][2] );
		
		VectorRotate( vecCapsuleVert, matCapsuleRotationSpace, vecCapsuleVert );
		VectorRotate( vecCapsuleVert, matCapsuleSpace, vecCapsuleVert );

		vecCapsuleVert *= flRadius;

		if ( g_capsuleVertPositions[i][2] > 0 )
		{
			vecCapsuleVert += vecLen;
		}

		v[i] = vecCapsuleVert + vStart;
	}

	unsigned char chRed = c.r();
	unsigned char chGreen = c.g();
	unsigned char chBlue = c.b();
	unsigned char chAlpha = c.a();

	pRenderContext->Bind( s_pWireframeIgnoreZ );

	IMesh* pMesh = pRenderContext->GetDynamicMesh( );
	CMeshBuilder meshBuilder;

	for ( int i=0; i<CAPSULE_LINES; i++ )
	{
		if ( g_capsuleLineIndices[i] == -1 )
		{
			if ( i > 0 )
			{
				meshBuilder.End( false, true );

				if ( i == CAPSULE_LINES - 1 )
					break;
			}
			
			i++;
			meshBuilder.Begin( pMesh, MATERIAL_LINE_LOOP, g_capsuleLineIndices[i] );
			i++;
		}

		meshBuilder.Position3fv (v[g_capsuleLineIndices[i]].Base());
		meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
		meshBuilder.AdvanceVertex();
	}
}

//-----------------------------------------------------------------------------
// Draws a triangle
//-----------------------------------------------------------------------------
void RenderTriangleInternal( const Vector& p1, const Vector& p2, const Vector& p3, Color c, IMaterial *pMaterial )
{
	InitializeStandardMaterials();

	CMatRenderContextPtr pRenderContext( materials );
	ICallQueue *pCallQueue = pRenderContext->GetCallQueue();
	if ( pCallQueue )
	{
		pCallQueue->QueueCall( RenderTriangleInternal, RefToVal( p1 ), RefToVal( p2 ), RefToVal( p3 ), c, pMaterial );
		return;
	}

	pRenderContext->Bind( pMaterial );

	unsigned char chRed = c.r();
	unsigned char chGreen = c.g();
	unsigned char chBlue = c.b();
	unsigned char chAlpha = c.a();

	Vector vecNormal;
	Vector vecDelta1, vecDelta2;
	VectorSubtract( p2, p1, vecDelta1 );
	VectorSubtract( p3, p1, vecDelta2 );
	CrossProduct( vecDelta1, vecDelta2, vecNormal );
	VectorNormalize( vecNormal );

	IMesh *pMesh = pRenderContext->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 1 );

	meshBuilder.Position3fv( p1.Base() );
	meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
	meshBuilder.Normal3fv( vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( p2.Base() );
	meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
	meshBuilder.Normal3fv( vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( p3.Base() );
	meshBuilder.Color4ub( chRed, chGreen, chBlue, chAlpha );
	meshBuilder.Normal3fv( vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

void RenderTriangle( const Vector& p1, const Vector& p2, const Vector& p3, Color c, IMaterial *pMaterial )
{
	RenderTriangleInternal( p1, p2, p3, c, pMaterial );
}

void RenderTriangle( const Vector& p1, const Vector& p2, const Vector& p3, Color c, bool bZBuffer )
{
	InitializeStandardMaterials();

	IMaterial *pMaterial = bZBuffer ? s_pVertexColor : s_pVertexColorIgnoreZ;
	Color cActual( c.r(), c.g(), c.b(), c.a() );
	RenderTriangleInternal( p1, p2, p3, cActual, pMaterial );
}



//-----------------------------------------------------------------------------
// Renders an extruded box
//-----------------------------------------------------------------------------
static void DrawAxes( const Vector& origin, Vector* pts, int idx, Color c, CMeshBuilder& meshBuilder )
{
	Vector start, temp;
	VectorAdd( pts[idx], origin, start );
	meshBuilder.Position3fv( start.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	int endidx = (idx & 0x1) ? idx - 1 : idx + 1;
	VectorAdd( pts[endidx], origin, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( start.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	endidx = (idx & 0x2) ? idx - 2 : idx + 2;
	VectorAdd( pts[endidx], origin, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( start.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	endidx = (idx & 0x4) ? idx - 4 : idx + 4;
	VectorAdd( pts[endidx], origin, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();
}

static void DrawExtrusionFace( const Vector& start, const Vector& end,
							  Vector* pts, int idx1, int idx2, Color c, CMeshBuilder& meshBuilder )
{
	Vector temp;
	VectorAdd( pts[idx1], start, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	VectorAdd( pts[idx2], start, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	VectorAdd( pts[idx2], end, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	VectorAdd( pts[idx1], end, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();

	VectorAdd( pts[idx1], start, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.Color4ub( c.r(), c.g(), c.b(), c.a() );
	meshBuilder.AdvanceVertex();
}

void RenderWireframeSweptBox( const Vector &vStart, const Vector &vEnd, const QAngle &angles, const Vector &vMins, const Vector &vMaxs, Color c, bool bZBuffer )
{
	InitializeStandardMaterials();

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->Bind( bZBuffer ? s_pWireframe : s_pWireframeIgnoreZ );

	Color cActual( c.r(), c.g(), c.b(), c.a() );

	// Build a rotation matrix from angles
	matrix3x4_t fRotateMatrix;
	AngleMatrix( angles, fRotateMatrix );

	IMesh *pMesh = pRenderContext->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 30 );

	Vector vDelta;
	VectorSubtract( vEnd, vStart, vDelta );

	// Compute the box points, rotated but without the origin added
	Vector temp;
	Vector pts[8];
	float dot[8];
	int minidx = 0;
	for ( int i = 0; i < 8; ++i )
	{
		temp.x = (i & 0x1) ? vMaxs[0] : vMins[0];
		temp.y = (i & 0x2) ? vMaxs[1] : vMins[1];
		temp.z = (i & 0x4) ? vMaxs[2] : vMins[2];

		// Rotate the corner point
		VectorRotate( temp, fRotateMatrix, pts[i] );

		// Find the dot product with dir
		dot[i] = DotProduct( pts[i], vDelta );
		if ( dot[i] < dot[minidx] )
		{
			minidx = i;
		}
	}

	// Choose opposite corner
	int maxidx = minidx ^ 0x7;

	// Draw the start + end axes...
	DrawAxes( vStart, pts, minidx, cActual, meshBuilder );
	DrawAxes( vEnd, pts, maxidx, cActual, meshBuilder );

	// Draw the extrusion faces
	for (int j = 0; j < 3; ++j )
	{
		int dirflag1 = ( 1 << ((j+1)%3) );
		int dirflag2 = ( 1 << ((j+2)%3) );

		int idx1, idx2, idx3;
		idx1 = (minidx & dirflag1) ? minidx - dirflag1 : minidx + dirflag1;
		idx2 = (minidx & dirflag2) ? minidx - dirflag2 : minidx + dirflag2;
		idx3 = (minidx & dirflag2) ? idx1 - dirflag2 : idx1 + dirflag2;

		DrawExtrusionFace( vStart, vEnd, pts, idx1, idx3, cActual, meshBuilder );
		DrawExtrusionFace( vStart, vEnd, pts, idx2, idx3, cActual, meshBuilder );
	}
	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Draws a axis-aligned quad
//-----------------------------------------------------------------------------
void RenderQuad( IMaterial *pMaterial, float x, float y, float w, float h, 
	float z, float s0, float t0, float s1, float t1, const Color& clr )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ub( clr.r(), clr.g(), clr.b(), clr.a());
	meshBuilder.TexCoord2f( 0, s0, t0 );
	meshBuilder.Position3f( x, y, z );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( clr.r(), clr.g(), clr.b(), clr.a());
	meshBuilder.TexCoord2f( 0, s1, t0 );
	meshBuilder.Position3f( x + w, y, z );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( clr.r(), clr.g(), clr.b(), clr.a());
	meshBuilder.TexCoord2f( 0, s1, t1 );
	meshBuilder.Position3f( x + w, y + h, z );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( clr.r(), clr.g(), clr.b(), clr.a());
	meshBuilder.TexCoord2f( 0, s0, t1 );
	meshBuilder.Position3f( x, y + h, z );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Renders a screen space quad
//-----------------------------------------------------------------------------

void DrawScreenSpaceRectangle( IMaterial *pMaterial, 
								  int nDestX, int nDestY, int nWidth, int nHeight,	// Rect to draw into in screen space
								  float flSrcTextureX0, float flSrcTextureY0,		// which texel you want to appear at destx/y
								  float flSrcTextureX1, float flSrcTextureY1,		// which texel you want to appear at destx+width-1, desty+height-1
								  int nSrcTextureWidth, int nSrcTextureHeight,		// needed for fixup
								  void *pClientRenderable,							// Used to pass to the bind proxies
								  int nXDice, int nYDice,							// Amount to tessellate the mesh
								  float fDepth )									// what Z value to put in the verts (def 0.0)
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	if ( ( nWidth <= 0 ) || ( nHeight <= 0 ) || ( nSrcTextureWidth <= 0 ) || ( nSrcTextureHeight <= 0 ) )
		return;

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->Bind( pMaterial, pClientRenderable );

	int xSegments = MAX( nXDice, 1);
	int ySegments = MAX( nYDice, 1);

	CMeshBuilder meshBuilder;
	
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, xSegments * ySegments );

	int nScreenWidth, nScreenHeight;
	pRenderContext->GetRenderTargetDimensions( nScreenWidth, nScreenHeight );

	// TOGL now automatically accounts for the half pixel offset between D3D9 vs. GL (including OSX unless using the older OSX togl lib. in which case flOffset = 0.0f)
	float flOffset = .5f;
			
	float flLeftX = nDestX - flOffset;
	float flRightX = nDestX + nWidth - flOffset;

	float flTopY = nDestY - flOffset;
	float flBottomY = nDestY + nHeight - flOffset;

	float flSubrectWidth = flSrcTextureX1 - flSrcTextureX0;
	float flSubrectHeight = flSrcTextureY1 - flSrcTextureY0;

	float flTexelsPerPixelX = ( nWidth > 1 ) ? flSubrectWidth / ( nWidth - 1 ) : 0.0f;
	float flTexelsPerPixelY = ( nHeight > 1 ) ? flSubrectHeight / ( nHeight - 1 ) : 0.0f;

	float flLeftU = flSrcTextureX0 + 0.5f - ( 0.5f * flTexelsPerPixelX );
	float flRightU = flSrcTextureX1 + 0.5f + ( 0.5f * flTexelsPerPixelX );
	float flTopV = flSrcTextureY0 + 0.5f - ( 0.5f * flTexelsPerPixelY );
	float flBottomV = flSrcTextureY1 + 0.5f + ( 0.5f * flTexelsPerPixelY );

	float flOOTexWidth = 1.0f / nSrcTextureWidth;
	float flOOTexHeight = 1.0f / nSrcTextureHeight;
	flLeftU *= flOOTexWidth;
	flRightU *= flOOTexWidth;
	flTopV *= flOOTexHeight;
	flBottomV *= flOOTexHeight;

	// Get the current viewport size
	int vx, vy, vw, vh;
	pRenderContext->GetViewport( vx, vy, vw, vh );

	// map from screen pixel coords to -1..1
	flRightX = FLerp( -1, 1, 0, vw, flRightX );
	flLeftX = FLerp( -1, 1, 0, vw, flLeftX );
	flTopY = FLerp( 1, -1, 0, vh ,flTopY );
	flBottomY = FLerp( 1, -1, 0, vh, flBottomY );

	// Dice the quad up...
	if ( ( xSegments > 1 ) || ( ySegments > 1 ) )
	{
		// Screen height and width of a subrect
		float flWidth  = (flRightX - flLeftX) / (float) xSegments;
		float flHeight = (flTopY - flBottomY) / (float) ySegments;

		// UV height and width of a subrect
		float flUWidth  = (flRightU - flLeftU) / (float) xSegments;
		float flVHeight = (flBottomV - flTopV) / (float) ySegments;

		for ( int x=0; x < xSegments; x++ )
		{
			for ( int y=0; y < ySegments; y++ )
			{
				// Top left
				meshBuilder.Position3f( flLeftX   + (float) x * flWidth, flTopY - (float) y * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) x * flUWidth, flTopV + (float) y * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.AdvanceVertex();

				// Top right (x+1)
				meshBuilder.Position3f( flLeftX   + (float) (x+1) * flWidth, flTopY - (float) y * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) (x+1) * flUWidth, flTopV + (float) y * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.AdvanceVertex();

				// Bottom right (x+1), (y+1)
				meshBuilder.Position3f( flLeftX   + (float) (x+1) * flWidth, flTopY - (float) (y+1) * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) (x+1) * flUWidth, flTopV + (float)(y+1) * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.AdvanceVertex();

				// Bottom left (y+1)
				meshBuilder.Position3f( flLeftX   + (float) x * flWidth, flTopY - (float) (y+1) * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) x * flUWidth, flTopV + (float)(y+1) * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.AdvanceVertex();
			}
		}
	}
	else // just one quad
	{
		for ( int corner=0; corner<4; corner++ )
		{
			bool bLeft = (corner==0) || (corner==3);
			meshBuilder.Position3f( (bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, fDepth );
			meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
			meshBuilder.TexCoord2f( 0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV );
			meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
			meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
			meshBuilder.AdvanceVertex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}

void DrawNDCSpaceUntexturedPolygon( IMaterial *pMaterial, int nVertexCount, Vector2D *pScreenSpaceCoordinates, void *pClientRenderable )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->Bind( pMaterial, pClientRenderable );	

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );
	meshBuilder.Begin( pMesh, MATERIAL_POLYGON, nVertexCount );

	for ( int i = 0; i < nVertexCount; ++ i )
	{
		meshBuilder.Position3f( pScreenSpaceCoordinates[i].x, pScreenSpaceCoordinates[i].y, 0.0f );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}

