﻿#ifndef GLMDISPLAYDB_H
#define	GLMDISPLAYDB_H

#include "tier1/utlvector.h"

//===============================================================================
// modes, displays, and renderers
//===============================================================================

// GLMDisplayModeInfoFields is in glmdisplay.h

class GLMDisplayMode
{
public:
	GLMDisplayModeInfoFields	m_info;
	
	GLMDisplayMode( uint width, uint height, uint refreshHz );
	GLMDisplayMode() { };
	~GLMDisplayMode( void );

	void	Init( uint width, uint height, uint refreshHz );
	void	Dump( int which );
};

//===============================================================================

// GLMDisplayInfoFields is in glmdisplay.h

class GLMDisplayInfo
{
public:
	GLMDisplayInfoFields			m_info;
	CUtlVector< GLMDisplayMode* >	*m_modes;				// starts out NULL, set by PopulateModes
	GLMDisplayMode					m_DesktopMode;

#ifdef OSX
	GLMDisplayInfo( CGDirectDisplayID displayID, CGOpenGLDisplayMask displayMask );
#else
	GLMDisplayInfo( void );
#endif

	~GLMDisplayInfo( void );
	
	void	PopulateModes( void );

	void	Dump( int which );

#ifdef OSX
private:
	int m_display;
#endif
};

//===============================================================================

// GLMRendererInfoFields is in glmdisplay.h

class GLMRendererInfo
{
public:
	GLMRendererInfoFields			m_info;
#ifdef OSX
	CUtlVector< GLMDisplayInfo* >	*m_displays;			// starts out NULL, set by PopulateDisplays
#else
	GLMDisplayInfo					*m_display;
#endif

#ifdef OSX
	GLMRendererInfo			( GLMRendererInfoFields *info );
#else
	GLMRendererInfo			();
#endif
	~GLMRendererInfo		( void );

#ifndef OSX
	void	Init( GLMRendererInfoFields *info );
#endif
	void	PopulateDisplays();
	void	Dump( int which );
};

//===============================================================================

#ifdef OSX
// this is just a tuple describing fake adapters which are really renderer/display pairings.
// dxabstract bridges the gap between the d3d adapter-centric world and the GL renderer+display world.
// this makes it straightforward to handle cases like two video cards with two displays on one, and one on the other -
// you get three fake adapters which represent each useful screen.

// the constraint that dxa will have to follow though, is that if the user wants to change their 
// display selection for full screen, they would only be able to pick on that has the same underlying renderer.
// can't change fakeAdapter from one to another with different GL renderer under it.  Screen hop but no card hop.

struct GLMFakeAdapter
{
	int		m_rendererIndex;
	int		m_displayIndex;
};
#endif

class GLMDisplayDB
{
public:
#ifdef OSX
	CUtlVector< GLMRendererInfo* >		*m_renderers;			// starts out NULL, set by PopulateRenderers
	CUtlVector< GLMFakeAdapter >		m_fakeAdapters;
#else
	GLMRendererInfo	m_renderer;
#endif

	GLMDisplayDB	( void );
	~GLMDisplayDB	( void );	

	virtual void	PopulateRenderers( void );
	virtual void	PopulateFakeAdapters( uint realRendererIndex );		// fake adapters = one real adapter times however many displays are on it
	virtual void	Populate( void );
	
	// The info-get functions return false on success.
	virtual	int		GetFakeAdapterCount( void );
	virtual	bool	GetFakeAdapterInfo( int fakeAdapterIndex, int *rendererOut, int *displayOut, GLMRendererInfoFields *rendererInfoOut, GLMDisplayInfoFields *displayInfoOut );
	
	virtual	int		GetRendererCount( void );
	virtual	bool	GetRendererInfo( int rendererIndex, GLMRendererInfoFields *infoOut );
	
	virtual	int		GetDisplayCount( int rendererIndex );
	virtual	bool	GetDisplayInfo( int rendererIndex, int displayIndex, GLMDisplayInfoFields *infoOut );

	virtual	int		GetModeCount( int rendererIndex, int displayIndex );
	virtual	bool	GetModeInfo( int rendererIndex, int displayIndex, int modeIndex, GLMDisplayModeInfoFields *infoOut );
	
	virtual	void	Dump( void );
};

#endif // GLMDISPLAYDB_H
