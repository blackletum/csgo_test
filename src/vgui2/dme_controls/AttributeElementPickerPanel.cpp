﻿//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "dme_controls/AttributeElementPickerPanel.h"
#include "dme_controls/AttributeTextEntry.h"
#include "dme_controls/AttributeWidgetFactory.h"
#include "datamodel/dmelement.h"
#include "tier1/KeyValues.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ComboBox.h"
#include "dme_controls/dmepicker.h"
#include "movieobjects/dmeeditortypedictionary.h"
#include "dme_controls/inotifyui.h"
#include "dme_controls/dmecontrols.h"
#include "dme_controls/dmecontrols_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;

// ----------------------------------------------------------------------------
CAttributeElementPickerPanel::CAttributeElementPickerPanel( vgui::Panel *parent, const AttributeWidgetInfo_t &info ) :
BaseClass( parent, info )
{
	m_hEdit = new vgui::Button( this, "Open", "...", this, "open" );

	m_pData = new CAttributeTextEntry( this, "AttributeValue" );
	m_pData->SetEnabled( !HasFlag( READONLY ) );
	m_pData->AddActionSignalTarget(this);
	m_pType->SetText( "element" );

	m_bShowMemoryUsage = info.m_bShowMemoryUsage;
	m_bShowUniqueID = info.m_bShowMemoryUsage;
}

void CAttributeElementPickerPanel::PostConstructor()
{
	Refresh();
}

// ----------------------------------------------------------------------------
vgui::Panel *CAttributeElementPickerPanel::GetDataPanel()
{
	return static_cast< vgui::Panel * >( m_pData );
}

void CAttributeElementPickerPanel::Apply()
{
	// FIXME: Implement when needed
	Assert( 0 );
}

void CAttributeElementPickerPanel::Refresh()
{
	char elemText[ 512 ];
	elemText[0] = 0;

	CDmElement *element = NULL;
	if ( !GetEditorInfo() || !GetEditorInfo()->GetValue<bool>( "hideText" ) )
	{
		if ( HasAttribute( ) )
		{
			element = GetAttributeValueElement( );
		}
		else
		{
			element = GetPanelElement();
		}
	}

	if ( element )
	{
		char idstr[ 37 ] = "";
		if( m_bShowUniqueID )
		{
			UniqueIdToString( element->GetId(), idstr, sizeof( idstr ) );
		}
		if ( m_bShowMemoryUsage )
		{
			Q_snprintf( elemText, sizeof( elemText ), "%s %s (%.3fMB total / %.3fKB self)", element->GetTypeString(),
				idstr, element->EstimateMemoryUsage( TD_DEEP ) / float( 1 << 20 ), element->EstimateMemoryUsage( TD_NONE ) / float( 1 << 10 ) );
		}
		else
		{
			Q_snprintf( elemText, sizeof( elemText ), "%s %s", element->GetTypeString(), idstr );
		}
	}

	m_pData->SetText( elemText );
	m_pData->SetEditable( false );
}


//-----------------------------------------------------------------------------
// Called when it's time to show the Dme picker
//-----------------------------------------------------------------------------
void CAttributeElementPickerPanel::ShowPickerDialog()
{
	CDmeEditorChoicesInfo *pInfo = CastElement<CDmeEditorChoicesInfo>( GetEditorInfo() );
	if ( !pInfo )
		return;

	// FIXME: Sucky. Should we make GetElementChoiceList return a DmeHandleVec_t? 
	ElementChoiceList_t choices;
	CUtlVector< DmePickerInfo_t > vec;
	if ( ElementPropertiesChoices()->GetElementChoiceList( pInfo->GetChoiceType(), GetPanelElement(), GetAttributeName(), IsArrayEntry(), choices ) )
	{
		int c = choices.Count();
		vec.EnsureCapacity( c );
		for ( int i = 0; i < c; ++i )
		{
			int j = vec.AddToTail( );
			vec[j].m_hElement = choices[i].m_pValue->GetHandle();
			vec[j].m_pChoiceString = choices[i].m_pChoiceString;
		}
	}

	CDmePickerFrame *pDmePickerDialog = new CDmePickerFrame( this, "Select DME Element" );
	pDmePickerDialog->AddActionSignalTarget( this );
	pDmePickerDialog->DoModal( vec );
}


//-----------------------------------------------------------------------------
// Called by the dme picker dialog if a dme was selected
//-----------------------------------------------------------------------------
void CAttributeElementPickerPanel::OnDmeSelected( KeyValues *pKeyValues )
{
	// We're either going to get an activity or sequence name
	CDmElement *pElement = GetElementKeyValue< CDmElement >( pKeyValues, "dme" );
	SetAttributeValueElement( pElement );
	Refresh( );
}


//-----------------------------------------------------------------------------
// Handle commands
//-----------------------------------------------------------------------------
void CAttributeElementPickerPanel::OnCommand( char const *cmd )
{
	if ( !Q_stricmp( cmd, "open" ) )
	{
		ShowPickerDialog();
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}


//-----------------------------------------------------------------------------
// Layout the panel
//-----------------------------------------------------------------------------
void CAttributeElementPickerPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int viewWidth, viewHeight;
	GetSize( viewWidth, viewHeight );

	IImage *arrowImage = vgui::scheme()->GetImage( "tools/ifm/icon_properties_linkarrow" , false);
	if( arrowImage )
	{
		m_hEdit->SetImage( arrowImage , 0 );
		m_hEdit->SetPaintBorderEnabled( false );
		m_hEdit->SetContentAlignment( vgui::Label::a_center );
		m_hEdit->SetBounds( (FirstColumnWidth - ColumnBorderWidth - 16) * 0.5 , ( viewHeight - 16 )* 0.5 , 16, 16 );
	}
	else
	{
		m_hEdit->SetBounds( 0, 0, 100, 20 );
	}
}

