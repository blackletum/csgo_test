﻿//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef PORTALRENDER_H
#define PORTALRENDER_H

#ifdef _WIN32
#pragma once
#endif

#include "iviewrender.h"
#include "view_shared.h"
#include "viewrender.h"
#include "shaderapi/ishaderapi.h"

#define MAX_PORTAL_RECURSIVE_VIEWS 11 //maximum number of recursions we allow when drawing views through portals. Seeing as how 5 is extremely choppy under best conditions and is barely visible, 10 is a safe limit. Adding one because 0 tends to be the primary view in most arrays of this size

class C_Prop_Portal;

class CPortalRenderable
{
public:
	CPortalRenderable( void );
	virtual ~CPortalRenderable( void );


	//----------------------------------------------------------------------------
	//Stencil-based drawing helpers, these are ONLY used in stencil drawing mode
	//----------------------------------------------------------------------------
	virtual void	DrawPreStencilMask( IMatRenderContext *pRenderContext ) { }; //Do whatever drawing you need before cutting the stencil hole
	virtual void	DrawStencilMask( IMatRenderContext *pRenderContext ) { }; //Draw to wherever you should see through the portal. The mask will later be filled with the portal view.
	virtual void	DrawPostStencilFixes( IMatRenderContext *pRenderContext ) { }; //After done drawing to the portal mask, we need to fix the depth buffer as well as fog. So draw your mesh again, writing to z and with the fog color alpha'd in by distance
   

	//----------------------------------------------------------------------------
	//Rendering of views beyond the portal
	//----------------------------------------------------------------------------
	virtual void	RenderPortalViewToBackBuffer( CViewRender *pViewRender, const CViewSetup &cameraView ) { };
	virtual void	RenderPortalViewToTexture( CViewRender *pViewRender, const CViewSetup &cameraView ) { };


	//----------------------------------------------------------------------------
	//Visibility through portals
	//----------------------------------------------------------------------------
	virtual bool	DoesExitViewIntersectWaterPlane( float waterZ, int leafWaterDataID ) const { return false; };
	virtual SkyboxVisibility_t	SkyBoxVisibleFromPortal( void ) { return SKYBOX_NOT_VISIBLE; };

	//-----------------------------------------------------------------------------
	//Fog workarounds
	//-----------------------------------------------------------------------------
	virtual const Vector&	GetFogOrigin( void ) const { return vec3_origin; };
	virtual void			ShiftFogForExitPortalView() const;
	virtual float			GetPortalDistanceBias() const { return 0.0f; }

	//-----------------------------------------------------------------------------
	//Portal visibility testing
	//-----------------------------------------------------------------------------
	//Based on view, will the camera be able to see through the portal this frame? This will allow the stencil mask to start being tested for pixel visibility.
	virtual bool	ShouldUpdatePortalView_BasedOnView( const CViewSetup &currentView, const CUtlVector<VPlane> &currentComplexFrustum ) { return false; }; 
	
	//Stencil mode only: You stated the portal was visible based on view, and this is how much of the screen your stencil mask took up last frame. Still want to draw this frame? Values less than zero indicate a lack of data from last frame
	virtual bool	ShouldUpdatePortalView_BasedOnPixelVisibility( float fScreenFilledByStencilMaskLastFrame_Normalized ) { return (fScreenFilledByStencilMaskLastFrame_Normalized != 0.0f); }; // < 0 is unknown visibility, > 0 is known to be partially visible


	//-----------------------------------------------------------------------------
	// Misc
	//-----------------------------------------------------------------------------
	virtual CPortalRenderable* GetLinkedPortal() const { return NULL; };
	const VMatrix&	MatrixThisToLinked() const;
	virtual bool	ShouldUpdateDepthDoublerTexture( const CViewSetup &viewSetup ) { return false; };
	virtual void	DrawPortal( IMatRenderContext *pRenderContext ) { }; //sort of like what you'd expect to happen in C_BaseAnimating::DrawModel() if portals were fully compatible with models
	virtual int		BindPortalMaterial( IMatRenderContext *pRenderContext, int nPassIndex, bool *pAllowRingMeshOptimizationOut ) { Assert( 0 ); return 0; }

	virtual C_BaseEntity *PortalRenderable_GetPairedEntity( void ) { return NULL; }; //Pairing a portal with an entity is common but not required. Accessing that entity allows the CPortalRender system to better optimize.
	VMatrix			m_matrixThisToLinked; //Always going to need a matrix

	// Poor man's RTTI
	FORCEINLINE bool IsPropPortal() const { return m_bIsPropPortal; }

	//-----------------------------------------------------------------------------
	//SFM related
	//-----------------------------------------------------------------------------
	bool			m_bIsPlaybackPortal;
	virtual void	HandlePortalPlaybackMessage( KeyValues *pKeyValues ) { };

protected:

	CPortalRenderable *FindRecordedPortal( int nPortalId ); //routed through here to get friend access to CPortalRender

	//routed through here to get friend access to CViewRender
	void CopyToCurrentView( CViewRender *pViewRender, const CViewSetup &viewSetup ); 
	void ViewDrawScene_PortalStencil( CViewRender *pViewRender, const CViewSetup &viewIn, ViewCustomVisibility_t *pCustomVisibility );
	void Draw3dSkyboxworld_Portal( CViewRender *pViewRender, const CViewSetup &viewIn, int &nClearFlags, bool &bDrew3dSkybox, SkyboxVisibility_t &nSkyboxVisible, ITexture *pRenderTarget = NULL );
	void ViewDrawScene( CViewRender *pViewRender, bool bDrew3dSkybox, SkyboxVisibility_t nSkyboxVisible, const CViewSetup &viewIn, int nClearFlags, view_id_t viewID, bool bDrawViewModel = false, int baseDrawFlags = 0, ViewCustomVisibility_t *pCustomVisibility = NULL );
	void SetViewRecursionLevel( int iViewRecursionLevel );
	void SetRemainingViewDepth( int iRemainingViewDepth );
	void SetViewEntranceAndExitPortals( CPortalRenderable *pEntryPortal, CPortalRenderable *pExitPortal );

	bool m_bIsPropPortal;

private:
	int m_iPortalViewIDNodeIndex; //each PortalViewIDNode_t has a child node link for each CPortalRenderable in CPortalRender::m_ActivePortals. This portal follows the same indexed link from each node
	// m_iPortalViewIDNodeIndex is the index into CPortalRender::m_AllPortals
	friend class CPortalRender;
};

//-----------------------------------------------------------------------------
// inline state querying methods
//-----------------------------------------------------------------------------
inline const VMatrix& CPortalRenderable::MatrixThisToLinked() const
{
	return m_matrixThisToLinked;
}

//-----------------------------------------------------------------------------
// Portal rendering materials
//-----------------------------------------------------------------------------
struct PortalRenderingMaterials_t
{
	CMaterialReference	m_Wireframe;
	CMaterialReference	m_WriteZ_Model;
	CMaterialReference	m_TranslucentVertexColor;
	CMaterialReference	m_PortalDepthDoubler;
	unsigned int		m_nDepthDoubleViewMatrixVarCache;
};

struct PortalViewIDNode_t
{
	CUtlVector<PortalViewIDNode_t *> ChildNodes; //links will only be non-null if they're useful (can see through the portal at that depth and view setup)
	int iPrimaryViewID;
	//skybox view id is always primary + 1

	//In stencil mode this wraps CPortalRenderable::DrawStencilMask() and gives previous frames' results to CPortalRenderable::RenderPortalViewToBackBuffer()
	//In texture mode there's no good spot to auto-wrap occlusion tests. So you'll need to wrap it yourself for that.
	OcclusionQueryObjectHandle_t occlusionQueryHandle;
	int iWindowPixelsAtQueryTime;
	int iOcclusionQueryPixelsRendered;
	float fScreenFilledByPortalSurfaceLastFrame_Normalized;
};

struct GhostPortalRenderInfo_t
{
	C_Prop_Portal *m_pPortal;
	int m_nGhostPortalQuadIndex;
	IMaterial *m_pGhostMaterial;
};

struct ClampedPortalMeshRenderInfo_t
{
	int nStartIndex;
	int nIndexCount;
};

//-----------------------------------------------------------------------------
// Portal rendering management class
//-----------------------------------------------------------------------------
class CPortalRender	: public CAutoGameSystem
{
public:
	CPortalRender();
	~CPortalRender();

	// Inherited from IGameSystem
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPreEntity();

	// Are we currently rendering a portal?
	bool IsRenderingPortal() const;

	// Returns the current View IDs. Portal View IDs will change often (especially with recursive views) and should not be cached
	int GetCurrentViewId() const;
	int GetCurrentSkyboxViewId() const;

	// Returns view recursion level
	int GetViewRecursionLevel() const;

	float GetPixelVisilityForPortalSurface( const CPortalRenderable *pPortal ) const; //normalized for how many of the screen's possible pixels it takes up, less than zero indicates a lack of data from last frame

	// Returns the remaining number of portals to render within other portals
	// lets portals know that they should do "end of the line" kludges to cover up that portals don't go infinitely recursive
	int	GetRemainingPortalViewDepth() const;

	inline CPortalRenderable *GetCurrentViewEntryPortal( void ) const { return m_pRenderingViewForPortal; }; //if rendering a portal view, this is the portal the current view enters into
	inline CPortalRenderable *GetCurrentViewExitPortal( void ) const { return m_pRenderingViewExitPortal; }; //if rendering a portal view, this is the portal the current view exits from

	//it's a good idea to force cheaper water when the ratio of performance gain to noticability is high
	//0 = force no reflection/refraction
	//1/2 = downgrade to simple/world reflections as seen in advanced video options
	//3 = no downgrade
	int ShouldForceCheaperWaterLevel() const;

	bool ShouldObeyStencilForClears() const;

#ifdef _PS3
	void ReloadZcullMemory();
#endif // _PS3

	//sometimes we have to tweak some systems to render water properly with portals
	void WaterRenderingHandler_PreReflection() const;
	void WaterRenderingHandler_PostReflection() const;
	void WaterRenderingHandler_PreRefraction() const;
	void WaterRenderingHandler_PostRefraction() const;

	// return value indicates that something was done, and render lists should be rebuilt afterwards
	bool DrawPortalsUsingStencils( CViewRender *pViewRender ); 
	bool DrawPortalsUsingStencils_Old( CViewRender *pViewRender ); 
	
	void OverlayPortalRenderTargets( float w, float h );

	void UpdateDepthDoublerTexture( const CViewSetup &viewSetup ); //our chance to update all depth doubler texture before the view model is added to the back buffer
	static bool DepthDoublerPIPDisableCheck( void ); //the depth doubler texture is unusable for a picture-in-picture view. Rather than sort out that ugly mess, just disable it for that case.

	void EnteredPortal( int nPlayerSlot, CPortalRenderable *pEnteredPortal ); //does a bit of internal maintenance whenever the player/camera has logically passed the portal threshold

	// adds, removes a portal to the set of renderable portals
	void AddPortal( CPortalRenderable *pPortal );
	void RemovePortal( CPortalRenderable *pPortal ); 

	// Methods to query about the exit portal associated with the currently rendering portal
	void ShiftFogForExitPortalView() const;
	float GetCurrentPortalDistanceBias() const;
	const Vector &GetExitPortalFogOrigin() const;
	SkyboxVisibility_t IsSkyboxVisibleFromExitPortal() const;
	bool DoesExitPortalViewIntersectWaterPlane( float waterZ, int leafWaterDataID ) const;

	void HandlePortalPlaybackMessage( KeyValues *pKeyValues );

	CPortalRenderable* FindRecordedPortal( IClientRenderable *pRenderable );

	CViewSetup m_RecursiveViewSetups[MAX_PORTAL_RECURSIVE_VIEWS]; //before we recurse into a view, we backup the view setup here for reference

	// tests if the parameter ID is being used by portal pixel vis queries
	bool IsPortalViewID( view_id_t id );
	
	inline CUtlVector<VPlane> &GetRecursiveViewComplexFrustums( int nIdx ) { return m_RecursiveViewComplexFrustums[ nIdx ]; }

	void DrawEarlyZPortals( CViewRender *pViewRender );

private:
	struct RecordedPortalInfo_t
	{
		CPortalRenderable *m_pActivePortal;
		int m_nPortalId;
		IClientRenderable *m_pPlaybackRenderable;
	};

	PortalViewIDNode_t m_HeadPortalViewIDNode; //pseudo node. Primary view id will be VIEW_MAIN. The child links are what we really care about
	PortalViewIDNode_t* m_PortalViewIDNodeChain[MAX_PORTAL_RECURSIVE_VIEWS]; //the view id node chain we're following, 0 always being &m_HeadPortalViewIDNode (offsetting by 1 seems like it'd cause bugs in the long run)
	
	void UpdatePortalPixelVisibility( void ); //updates pixel visibility for portal surfaces

	// Handles a portal update message
	void HandlePortalUpdateMessage( KeyValues *pKeyValues );

	// Finds a recorded portal
	int FindRecordedPortalIndex( int nPortalId );
	CPortalRenderable* FindRecordedPortal( int nPortalId );

	void DrawPortalGhostLocations( IMatRenderContext *pRenderContext, IMesh *pPortalQuadMesh, const GhostPortalRenderInfo_t *pGhostPortalRenderInfos, int nPortalCount ) const;
	void RenderPortalEffects( IMatRenderContext *pRenderContext, IMesh *pPortalQuadMesh, const CUtlVector< CPortalRenderable* > &actualActivePortals,
		const CUtlVector< int > &actualActivePortalQuadVBIndex ) const;

private:

	PortalRenderingMaterials_t	m_Materials;
	int							m_iViewRecursionLevel;
	int							m_iRemainingPortalViewDepth; //let's portals know that they should do "end of the line" kludges to cover up that portals don't go infinitely recursive

	// Data that's only valid while inside DrawPortalsUsingStencil()
	CUtlStack<int>				m_stencilValueStack;
	CUtlStack<int>				m_parentPortalIdStack;
	ICachedPerFrameMeshData		*m_pCachedPortalQuadMeshData;
	VertexFormat_t				m_portalQuadMeshVertexFmt;
	CUtlVector< ClampedPortalMeshRenderInfo_t > m_clampedPortalMeshRenderInfos;
	CUtlVector< bool >			m_portalIsOpening;

	CPortalRenderable			*m_pRenderingViewForPortal; //the specific pointer for the portal that we're rending a view for
	CPortalRenderable			*m_pRenderingViewExitPortal; //the specific pointer for the portal that our view exits from

	CUtlVector<CPortalRenderable *>		m_AllPortals; //All portals currently in memory, active or not
	CUtlVector<CPortalRenderable *>		m_ActivePortals;
	CUtlVector< RecordedPortalInfo_t >	m_RecordedPortals;

	ShaderStencilState_t		m_StencilState;

	CUtlVector<VPlane>					m_RecursiveViewComplexFrustums[MAX_PORTAL_RECURSIVE_VIEWS]; 

	CUtlVector< GhostPortalRenderInfo_t > m_portalGhostRenderInfos;

public:
	//frustums with more (or less) than 6 planes. Store each recursion level's custom frustum here so further recursions can be better optimized.
	//When going into further recursions, if you've failed to fill in a complex frustum, the standard frustum will be copied in.
	//So all parent levels are guaranteed to contain valid data
	PortalRenderingMaterials_t& m_MaterialsAccess;

	friend class CPortalRenderable;
	friend void OnRenderStart();
};

extern CPortalRender* g_pPortalRender;


inline CPortalRenderable *CPortalRenderable::FindRecordedPortal( int nPortalId )
{ 
	return g_pPortalRender->FindRecordedPortal( nPortalId );
}


typedef CPortalRenderable *(*PortalRenderableCreationFunc)( void );

//only ever create global/static instances of this
class CPortalRenderableCreator_AutoRegister
{
public:
	CPortalRenderableCreator_AutoRegister( const char *szType, PortalRenderableCreationFunc creationFunc )
		: m_szPortalType( szType ), m_creationFunc( creationFunc )
	{
		m_pNext = s_pRegisteredTypes;
		s_pRegisteredTypes = this;
	}
private:
	const char *m_szPortalType;
	PortalRenderableCreationFunc m_creationFunc;
	const CPortalRenderableCreator_AutoRegister *m_pNext;
	static CPortalRenderableCreator_AutoRegister *s_pRegisteredTypes;
	friend class CPortalRender;
};



//-----------------------------------------------------------------------------
// inline friend access redirects
//-----------------------------------------------------------------------------
inline void CPortalRenderable::CopyToCurrentView( CViewRender *pViewRender, const CViewSetup &viewSetup )
{
	pViewRender->m_CurrentView = viewSetup;
}

inline void CPortalRenderable::ViewDrawScene_PortalStencil( CViewRender *pViewRender, const CViewSetup &viewIn, ViewCustomVisibility_t *pCustomVisibility )
{
	pViewRender->ViewDrawScene_PortalStencil( viewIn, pCustomVisibility );
}

inline void CPortalRenderable::Draw3dSkyboxworld_Portal( CViewRender *pViewRender, const CViewSetup &viewIn, int &nClearFlags, bool &bDrew3dSkybox, SkyboxVisibility_t &nSkyboxVisible, ITexture *pRenderTarget )
{
	pViewRender->Draw3dSkyboxworld_Portal( viewIn, nClearFlags, bDrew3dSkybox, nSkyboxVisible, pRenderTarget );
}

inline void CPortalRenderable::ViewDrawScene( CViewRender *pViewRender, bool bDrew3dSkybox, SkyboxVisibility_t nSkyboxVisible, const CViewSetup &viewIn, int nClearFlags, view_id_t viewID, bool bDrawViewModel, int baseDrawFlags, ViewCustomVisibility_t *pCustomVisibility )
{
	pViewRender->ViewDrawScene( bDrew3dSkybox, nSkyboxVisible, viewIn, nClearFlags, viewID, bDrawViewModel, baseDrawFlags, pCustomVisibility );
}

inline void CPortalRenderable::SetViewRecursionLevel( int iViewRecursionLevel )
{
	g_pPortalRender->m_iViewRecursionLevel = iViewRecursionLevel;
}

inline void CPortalRenderable::SetRemainingViewDepth( int iRemainingViewDepth )
{
	g_pPortalRender->m_iRemainingPortalViewDepth = iRemainingViewDepth;
}

inline void CPortalRenderable::SetViewEntranceAndExitPortals( CPortalRenderable *pEntryPortal, CPortalRenderable *pExitPortal )
{
	g_pPortalRender->m_pRenderingViewForPortal = pEntryPortal;
	g_pPortalRender->m_pRenderingViewExitPortal = pExitPortal;
}

#endif //#ifndef PORTALRENDER_H

