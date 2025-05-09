﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HUD_PDUMP_H
#define HUD_PDUMP_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include "hudelement.h"
#include "tier1/utlsortvector.h"

namespace vgui
{
	class IScheme;
};

class CPDumpPanel : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CPDumpPanel, vgui::Panel );

public:
	enum
	{
		DUMP_CLASSNAME_SIZE = 128,
		DUMP_STRING_SIZE = 128,
	};

	explicit CPDumpPanel( const char *pElementName );
	~CPDumpPanel();

	DECLARE_MULTIPLY_INHERITED();

	virtual void ApplySettings( KeyValues *inResourceData );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Paint( void );

	virtual bool ShouldDraw();

	// Remove dump info
	void		Clear();
	void		DumpEntity( C_BaseEntity *ent, int commands_acknowledged );

	void DumpComparision( const char *classname, const char *fieldname, const char *fieldtype,
		bool networked, bool noterrorchecked, bool differs, bool withintolerance, const char *value );
private:

	void PredictionDumpColor( bool legend, bool predictable, bool networked, bool errorchecked, bool differs, bool withintolerance,
		int& r, int& g, int& b, int& a );
	//-----------------------------------------------------------------------------
	// Purpose: Stores some info about the various fields of an entity for display
	//-----------------------------------------------------------------------------
	struct DumpInfo
	{
		int  index;
		char classname[ DUMP_CLASSNAME_SIZE ];
		bool networked;
		char fieldstring[ DUMP_STRING_SIZE ];
		bool differs;
		bool withintolerance;
		bool noterrorchecked;

		class CDumpInfoLess
		{
		public:
			bool Less( const DumpInfo &src1, const DumpInfo &src2, void *pCtx )
			{
				int str = Q_stricmp( src1.classname, src2.classname );
				if ( str < 0 )
					return true;
				else if ( str > 0 )
					return false;

				return src1.index < src2.index;
			}
		};
	};

	CUtlSortVector< DumpInfo, DumpInfo::CDumpInfoLess > m_DumpEntityInfo;

	EHANDLE			m_hDumpEntity;
	int				m_nCurrentIndex;

	CPanelAnimationVar( vgui::HFont, m_FontSmall, "ItemFont", "DebugOverlay" );
	CPanelAnimationVar( vgui::HFont, m_FontMedium, "LabelFont", "DebugOverlay" );
	CPanelAnimationVar( vgui::HFont, m_FontBig, "TitleFont", "DebugOverlay" );
};

CPDumpPanel *GetPDumpPanel();

#endif // HUD_PDUMP_H
