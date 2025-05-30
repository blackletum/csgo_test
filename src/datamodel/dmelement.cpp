﻿//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "datamodel/dmelement.h"
#include "tier0/dbg.h"
#include "datamodel.h"
#include "tier1/utllinkedlist.h"
#include "tier1/utlbuffer.h"
#include "datamodel/dmattribute.h"
#include "Color.h"
#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"
#include "datamodel/dmelementfactoryhelper.h"
#include "datamodel/dmattributevar.h"
#include "dmattributeinternal.h"
#include "DmElementFramework.h"
#include <ctype.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// helper class to allow CDmeHandle access to g_pDataModelImp
//-----------------------------------------------------------------------------
void CDmeElementRefHelper::Ref( DmElementHandle_t hElement, HandleType_t handleType )
{
	g_pDataModelImp->OnElementReferenceAdded( hElement, handleType );
}

void CDmeElementRefHelper::Unref( DmElementHandle_t hElement, HandleType_t handleType )
{
	g_pDataModelImp->OnElementReferenceRemoved( hElement, handleType );
}


// turn memdbg off temporarily so we can get at placement new
#include "tier0/memdbgoff.h"
DEFINE_FIXEDSIZE_ALLOCATOR( DmAttributeList_t, 1024, CUtlMemoryPool::GROW_SLOW );
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// element reference struct - containing attribute referrers and handle refcount
//-----------------------------------------------------------------------------
void DmElementReference_t::AddAttribute( CDmAttribute *pAttribute )
{
	if ( m_attributes.m_hAttribute != DMATTRIBUTE_HANDLE_INVALID )
	{
		DmAttributeList_t *pLink = new DmAttributeList_t; // TODO - create a fixed size allocator for these
		pLink->m_hAttribute = m_attributes.m_hAttribute;
		pLink->m_pNext = m_attributes.m_pNext;
		m_attributes.m_pNext = pLink;
	}
	m_attributes.m_hAttribute = pAttribute->GetHandle();
}

void DmElementReference_t::RemoveAttribute( CDmAttribute *pAttribute )
{
	DmAttributeHandle_t hAttribute = pAttribute->GetHandle();
	if ( m_attributes.m_hAttribute == hAttribute )
	{
		DmAttributeList_t *pNext = m_attributes.m_pNext;
		if ( pNext )
		{
			m_attributes.m_hAttribute = pNext->m_hAttribute;
			m_attributes.m_pNext = pNext->m_pNext;
			delete pNext;
		}
		else
		{
			m_attributes.m_hAttribute = DMATTRIBUTE_HANDLE_INVALID;
		}
		return;
	}

	for ( DmAttributeList_t *pLink = &m_attributes; pLink->m_pNext; pLink = pLink->m_pNext )
	{
		DmAttributeList_t *pNext = pLink->m_pNext;
		if ( pNext->m_hAttribute == hAttribute )
		{
			pLink->m_pNext = pNext->m_pNext;
			delete pNext; // TODO - create a fixed size allocator for these
			return;
		}
	}

	Assert( 0 );
}

bool DmElementReference_t::FindAttribute( CDmAttribute *pAttribute )
{
	DmAttributeHandle_t hAttribute = pAttribute->GetHandle();
	for ( DmAttributeList_t *pLink = &m_attributes; pLink; pLink = pLink->m_pNext )
	{
		if ( pLink->m_hAttribute == hAttribute )
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Class factory 
//-----------------------------------------------------------------------------
IMPLEMENT_ELEMENT_FACTORY( DmElement, CDmElement );


//-----------------------------------------------------------------------------
// For backward compat: DmeElement -> creates a CDmElement class
//-----------------------------------------------------------------------------
CDmElementFactory< CDmElement > g_CDmeElement_Factory( "DmElement" );
CDmElementFactoryHelper g_CDmeElement_Helper( "DmeElement", &g_CDmeElement_Factory, true );


//-----------------------------------------------------------------------------
// Constructor, destructor 
//-----------------------------------------------------------------------------
CDmElement::CDmElement( DmElementHandle_t handle, const char *pElementType, const DmObjectId_t &id, const char *pElementName, DmFileId_t fileid ) : 
	m_ref( handle ), m_Type( g_pDataModel->GetSymbol( pElementType ) ), m_fileId( fileid ),
	m_pAttributes( NULL ), m_bDirty( false ), m_bOnChangedCallbacksEnabled( false ), m_nParityBits( 0 ), m_bOnlyInUndo( false )
{
	MEM_ALLOC_CREDIT();
	g_pDataModelImp->AddElementToFile( m_ref.m_hElement, m_fileId );
	{
		DMX_PROFILE_SCOPE( CDmElement_m_Name_InitAndSet );
		m_Name.InitAndSet( this, "name", pElementName, FATTRIB_TOPOLOGICAL );
	}
	CopyUniqueId( id, &m_Id );
}

CDmElement::~CDmElement()
{
	g_pDataModelImp->RemoveElementFromFile( m_ref.m_hElement, m_fileId );
}

void CDmElement::PerformConstruction()
{
	OnConstruction();
}

void CDmElement::PerformDestruction()
{
	OnDestruction();
}


//-----------------------------------------------------------------------------
// Purpose: Deletes all attributes
//-----------------------------------------------------------------------------
void CDmElement::Purge()
{
	// Don't create "undo" records for attribute changes here, since
	//  the entire element is getting deleted...
	CDisableUndoScopeGuard guard;

	while ( m_pAttributes )
	{
#if defined( _DEBUG )
		// So you can see what attribute is being destroyed
		const char *pName = m_pAttributes->GetName();
		NOTE_UNUSED( pName );
#endif
		CDmAttribute *pNext = m_pAttributes->NextAttribute();
		CDmAttribute::DestroyAttribute( m_pAttributes );
		m_pAttributes = pNext;
	}

	g_pDataModelImp->NotifyState( NOTIFY_CHANGE_TOPOLOGICAL );
}


void CDmElement::SetId( const DmObjectId_t &id )
{
	CopyUniqueId( id, &m_Id );
}


//-----------------------------------------------------------------------------
// RTTI implementation
//-----------------------------------------------------------------------------
void CDmElement::SetTypeSymbol( CUtlSymbolLarge sym )
{
	m_classType = sym;
}

bool CDmElement::IsA( CUtlSymbolLarge typeSymbol ) const 
{
	// NOTE: This pattern here is used to avoid a zillion virtual function
	// calls in the implementation of IsA. The IsA_Implementation is 
	// all static function calls.
	return IsA_Implementation( typeSymbol ); 
}

int	CDmElement::GetInheritanceDepth( CUtlSymbolLarge typeSymbol ) const
{
	// NOTE: This pattern here is used to avoid a zillion virtual function
	// calls in the implementation of IsA. The IsA_Implementation is 
	// all static function calls.
	return GetInheritanceDepth_Implementation( typeSymbol, 0 ); 
}

// Helper for GetInheritanceDepth
int CDmElement::GetInheritanceDepth( const char *pTypeName ) const
{
	CUtlSymbolLarge typeSymbol = g_pDataModel->GetSymbol( pTypeName );
	return GetInheritanceDepth( typeSymbol ); 
}


//-----------------------------------------------------------------------------
// Is the element dirty?
//-----------------------------------------------------------------------------
bool CDmElement::IsDirty() const
{
	return m_bDirty;
}

void CDmElement::MarkDirty( bool bDirty )
{
	if ( bDirty && !m_bDirty )
	{
		g_pDmElementFrameworkImp->AddElementToDirtyList( m_ref.m_hElement );
	}
	m_bDirty = bDirty;
}

void CDmElement::MarkAttributesClean()
{
	for ( CDmAttribute *pAttr = m_pAttributes; pAttr; pAttr = pAttr->NextAttribute() )
	{
		// No Undo for flag changes
		pAttr->RemoveFlag( FATTRIB_DIRTY );
	}
}

void CDmElement::DisableOnChangedCallbacks()
{
	m_bOnChangedCallbacksEnabled = false;
}

void CDmElement::EnableOnChangedCallbacks()
{
	m_bOnChangedCallbacksEnabled = true;
}

bool CDmElement::AreOnChangedCallbacksEnabled()
{
	return m_bOnChangedCallbacksEnabled;	
}

void CDmElement::FinishUnserialization()
{
	for( CDmAttribute *pAttribute = m_pAttributes; pAttribute; pAttribute = pAttribute->NextAttribute() )
	{
		pAttribute->OnUnserializationFinished();
	}

	// loop referencing attributes, and call OnAttributeChanged on them as well
	if ( m_ref.m_attributes.m_hAttribute != DMATTRIBUTE_HANDLE_INVALID )
	{
		for ( DmAttributeList_t *pAttrLink = &m_ref.m_attributes; pAttrLink; pAttrLink = pAttrLink->m_pNext )
		{
			CDmAttribute *pAttr = g_pDataModel->GetAttribute( pAttrLink->m_hAttribute );
			if ( !pAttr || pAttr->GetOwner()->GetFileId() == m_fileId )
				continue; // attributes in this file will already have OnAttributeChanged called on them

			pAttr->OnUnserializationFinished();
		}
	}

	// Mostly used for backward compatibility reasons
	CDmElement *pElement = g_pDataModel->GetElement( m_ref.m_hElement );
	pElement->OnElementUnserialized();

	// Force a resolve also, and set it up to remove it from the dirty list
	// after unserialization is complete
	pElement->Resolve();
	MarkDirty( false );
	MarkAttributesClean();
	g_pDataModelImp->NotifyState( NOTIFY_CHANGE_TOPOLOGICAL );
}

// Should only be called from datamodel, who will take care of changing the fileset entry as well
void CDmElement::ChangeHandle( DmElementHandle_t handle )
{
	m_ref.m_hElement = handle;
}

// returns element reference struct w/ list of referrers and handle count
DmElementReference_t *CDmElement::GetReference()
{
	return &m_ref;
}

void CDmElement::SetReference( const DmElementReference_t &ref )
{
	Assert( !m_ref.IsWeaklyReferenced() );
	m_ref = ref;
}


int CDmElement::EstimateMemoryUsage( CUtlHash< DmElementHandle_t > &visited, TraversalDepth_t depth, int *pCategories )
{
	if ( visited.Find( m_ref.m_hElement ) != visited.InvalidHandle() )
		return 0;
	visited.Insert( m_ref.m_hElement );

	int nDataModelUsage = g_pDataModelImp->EstimateMemoryOverhead( );
	int nReferenceUsage = m_ref.EstimateMemoryOverhead();
	CDmElement *pElement = g_pDataModel->GetElement( m_ref.m_hElement );
	int nInternalUsage = sizeof( *this ) - sizeof( CUtlSymbolLarge );	// NOTE: The utlSymbolLarge is the 'name' attribute var
	int nOuterUsage = pElement->AllocatedSize() - nInternalUsage;
	Assert( nOuterUsage >= 0 );

	if ( pCategories )
	{
		pCategories[MEMORY_CATEGORY_OUTER] += nOuterUsage;
		pCategories[MEMORY_CATEGORY_DATAMODEL] += nDataModelUsage;
		pCategories[MEMORY_CATEGORY_REFERENCES] += nReferenceUsage;
		pCategories[MEMORY_CATEGORY_ELEMENT_INTERNAL] += nInternalUsage;
	}

	int nAttributeDataUsage = 0;
	for ( CDmAttribute *pAttr = m_pAttributes; pAttr; pAttr = pAttr->NextAttribute() )
	{
		nAttributeDataUsage += pAttr->EstimateMemoryUsageInternal( visited, depth, pCategories );
	}

	return nInternalUsage + nDataModelUsage + nReferenceUsage + nOuterUsage + nAttributeDataUsage;
}

//-----------------------------------------------------------------------------
// parity methods for marking elements during traversal
//-----------------------------------------------------------------------------
bool CDmElement::GetParity( int bit /*=0*/ ) const
{
	return ( m_nParityBits & ( 1 << bit ) ) != 0;
}

void CDmElement::SetParity( bool bParity, int bit /*=0*/ )
{
	m_nParityBits = bParity ? m_nParityBits | ( 1 << bit ) : m_nParityBits & ~( 1 << bit );
}

void CDmElement::SetParity( bool bParity, TraversalDepth_t depth, int bit /*=0*/ )
{
	if ( GetParity( bit ) == bParity )
		return;

	SetParity( bParity, bit );

	for ( const CDmAttribute *pAttr = FirstAttribute(); pAttr != NULL; pAttr = pAttr->NextAttribute() )
	{
		if ( !ShouldTraverse( pAttr, depth ) )
			continue;

		if ( pAttr->GetType() == AT_ELEMENT )
		{
			CDmElement *pChild = pAttr->GetValueElement<CDmElement>();
			if ( !pChild )
				continue;
			pChild->SetParity( bParity, depth, bit );
		}
		else if ( pAttr->GetType() == AT_ELEMENT_ARRAY )
		{
			const CDmrElementArrayConst<> elementArrayAttr( pAttr );
			int nChildren = elementArrayAttr.Count();
			for ( int i = 0; i < nChildren; ++i )
			{
				CDmElement *pChild = elementArrayAttr[ i ];
				if ( !pChild )
					continue;
				pChild->SetParity( bParity, depth, bit );
			}
		}
	}
}


bool CDmElement::IsOnlyInUndo() const
{
	return m_bOnlyInUndo;
}

void CDmElement::SetOnlyInUndo( bool bOnlyInUndo )
{
	m_bOnlyInUndo = bOnlyInUndo;
}


//-----------------------------------------------------------------------------
// returns the first path to the element found traversing all element/element array attributes - not necessarily the shortest
//-----------------------------------------------------------------------------

// do we want a true visited set to avoid retraversing the same subtree over and over again?
// for most dag trees, it's probably a perf loss, since multiple instances are rare, (and searching the visited set costs log(n))
// for trees that include channels, it's probably a perf win, since many channels link into the same element most of the time
bool CDmElement::FindElement( const CDmElement *pElement, CUtlVector< ElementPathItem_t > &elementPath, TraversalDepth_t depth ) const
{
	if ( this == pElement )
		return true;

	ElementPathItem_t search( GetHandle() );
	if ( elementPath.Find( search ) != elementPath.InvalidIndex() )
		return false;

	int idx = elementPath.AddToTail( search );
	ElementPathItem_t &pathItem = elementPath[ idx ];

	for ( const CDmAttribute *pAttr = FirstAttribute(); pAttr != NULL; pAttr = pAttr->NextAttribute() )
	{
		if ( !ShouldTraverse( pAttr, depth ) )
			continue;

		if ( pAttr->GetType() == AT_ELEMENT )
		{
			pathItem.hAttribute = const_cast< CDmAttribute* >( pAttr )->GetHandle();
			pathItem.nIndex = -1;

			CDmElement *pChild = pAttr->GetValueElement<CDmElement>();
			if ( pChild && pChild->FindElement( pElement, elementPath, depth ) )
				return true;
		}
		else if ( pAttr->GetType() == AT_ELEMENT_ARRAY )
		{
			pathItem.hAttribute = const_cast< CDmAttribute* >( pAttr )->GetHandle();

			CDmrElementArrayConst<> elementArrayAttr( pAttr );
			int nChildren = elementArrayAttr.Count();
			for ( int i = 0; i < nChildren; ++i )
			{
				pathItem.nIndex = i;

				CDmElement *pChild = elementArrayAttr[ i ];
				if ( pChild && pChild->FindElement( pElement, elementPath, depth ) )
					return true;
			}
		}
	}

	elementPath.Remove( idx );
	return false;
}

bool CDmElement::FindReferer( DmElementHandle_t hElement, CUtlVector< ElementPathItem_t > &elementPath, TraversalDepth_t depth /* = TD_SHALLOW */ ) const
{
	DmElementHandle_t hThis = GetHandle();

	DmAttributeReferenceIterator_t hAttr = g_pDataModel->FirstAttributeReferencingElement( hThis );
	for ( ; hAttr != DMATTRIBUTE_REFERENCE_ITERATOR_INVALID; hAttr = g_pDataModel->NextAttributeReferencingElement( hAttr ) )
	{
		CDmAttribute *pAttr = g_pDataModel->GetAttribute( hAttr );
		if ( !pAttr )
			continue;

		if ( !ShouldTraverse( pAttr, depth ) )
			continue;

		DmElementHandle_t hOwner = pAttr->GetOwner()->GetHandle();
		if ( elementPath.Find( ElementPathItem_t( hOwner ) ) != elementPath.InvalidIndex() )
			return false;

		int i = elementPath.AddToTail();
		ElementPathItem_t &item = elementPath[ i ];
		item.hElement = hOwner;
		item.hAttribute = pAttr->GetHandle();
		item.nIndex = -1;
		if ( pAttr->GetType() == AT_ELEMENT_ARRAY )
		{
			CDmrElementArray<> array( pAttr );
			item.nIndex = array.Find( hThis );
		}

		if ( hOwner == hElement )
			return true;

		CDmElement *pOwner = GetElement< CDmElement >( hOwner );
		if ( pOwner->FindReferer( hElement, elementPath, depth ) )
			return true;

		elementPath.Remove( i );
	}

	return false;
}

void CDmElement::RemoveAllReferencesToElement( CDmElement *pElement )
{
	for ( CDmAttribute *pAttr = FirstAttribute(); pAttr != NULL; pAttr = pAttr->NextAttribute() )
	{
		if ( pAttr->GetType() == AT_ELEMENT )
		{
			CDmElement *pChild = pAttr->GetValueElement<CDmElement>();
			if ( pChild == pElement )
			{
				pAttr->SetValue( DMELEMENT_HANDLE_INVALID );
			}
		}
		else if ( pAttr->GetType() == AT_ELEMENT_ARRAY )
		{
			CDmrElementArray<> elementArrayAttr( pAttr );
			int nChildren = elementArrayAttr.Count();
			for ( int i = nChildren - 1; i >= 0; --i )
			{
				CDmElement *pChild = elementArrayAttr[ i ];
				if ( pChild == pElement )
				{
					elementArrayAttr.Remove( i );
				}
			}
		}
	}
}

int CDmElement::EstimateMemoryUsage( TraversalDepth_t depth /* = TD_DEEP */ )
{
	return g_pDataModel->EstimateMemoryUsage( GetHandle(), depth );
}


//-----------------------------------------------------------------------------
//
// Implementation Undo for copied objects
//
//-----------------------------------------------------------------------------
CDmElement* CDmElement::CopyInternal( TraversalDepth_t depth /* = TD_DEEP */ ) const
{
	CDmElement *pCopy = GetElement< CDmElement >( g_pDataModel->CreateElement( GetType(), GetName(), GetFileId() ) );
	if ( pCopy )
	{
		CopyAttributesTo( pCopy, depth );
	}
	return pCopy;
}

//-----------------------------------------------------------------------------
// Copy - implementation of shallow and deep element copying
//		- allows attributes to be marked to always (or never) copy
//-----------------------------------------------------------------------------
void CDmElement::CopyAttributesTo( CDmElement *pCopy, TraversalDepth_t depth ) const
{
	CDisableUndoScopeGuard sg;

	CUtlMap< DmElementHandle_t, DmElementHandle_t, int > refmap( DefLessFunc( DmElementHandle_t ) );
	CopyAttributesTo( pCopy, refmap, depth );

	CUtlHashFast< DmElementHandle_t > visited;
	uint nPow2Size = 1;
	while( nPow2Size < refmap.Count() )
	{
		nPow2Size <<= 1;
	}
	visited.Init( nPow2Size );
	pCopy->FixupReferences( visited, refmap, depth );
}


//-----------------------------------------------------------------------------
// Copy an element-type attribute
//-----------------------------------------------------------------------------
void CDmElement::CopyElementAttribute( const CDmAttribute *pSrcAttr, CDmAttribute *pDestAttr, CRefMap &refmap, TraversalDepth_t depth ) const
{
	DmElementHandle_t hSrc = pSrcAttr->GetValue<DmElementHandle_t>();
	CDmElement *pSrc = GetElement< CDmElement >( hSrc );

	if ( pSrc == NULL )
	{
		pDestAttr->SetValue( DMELEMENT_HANDLE_INVALID );
		return;
	}

	if ( pSrc->IsShared() )
	{
		pDestAttr->SetValue( pSrcAttr );
		return;
	}

	int idx = refmap.Find( hSrc );
	if ( idx != refmap.InvalidIndex() )
	{
		pDestAttr->SetValue( refmap[ idx ] );
		return;
	}

	if ( ShouldTraverse( pSrcAttr, depth ) )
	{
		DmElementHandle_t hDest = pDestAttr->GetValue<DmElementHandle_t>();
		if ( hDest == DMELEMENT_HANDLE_INVALID )
		{
			hDest = g_pDataModel->CreateElement( pSrc->GetType(), pSrc->GetName(), pSrc->GetFileId() );
			pDestAttr->SetValue( hDest );
		}

		CDmElement *pDest = GetElement< CDmElement >( hDest );
		pSrc->CopyAttributesTo( pDest, refmap, depth );
		return;
	}

	pDestAttr->SetValue( pSrcAttr );
}


//-----------------------------------------------------------------------------
// Copy an element array-type attribute
//-----------------------------------------------------------------------------
void CDmElement::CopyElementArrayAttribute( const CDmAttribute *pAttr, CDmAttribute *pCopyAttr, CRefMap &refmap, TraversalDepth_t depth ) const
{
	CDmrElementArrayConst<> srcAttr( pAttr );
	CDmrElementArray<> destAttr( pCopyAttr );
	destAttr.RemoveAll(); // automatically releases each handle

	bool bCopy = ShouldTraverse( pAttr, depth );

	int n = srcAttr.Count();
	destAttr.EnsureCapacity( n );

	for ( int i = 0; i < n; ++i )
	{
		DmElementHandle_t hSrc = srcAttr.GetHandle( i );
		CDmElement *pSrc = srcAttr[i];

		if ( pSrc == NULL )
		{
			destAttr.AddToTail( DMELEMENT_HANDLE_INVALID );
			continue;
		}

		if ( pSrc->IsShared() )
		{
			destAttr.AddToTail( srcAttr[ i ] );
			continue;
		}

		int idx = refmap.Find( hSrc );
		if ( idx != refmap.InvalidIndex() )
		{
			destAttr.AddToTail( refmap[ idx ] );
			continue;
		}

		if ( bCopy )
		{
			DmElementHandle_t hDest = g_pDataModel->CreateElement( pSrc->GetType(), pSrc->GetName(), pSrc->GetFileId() );
			destAttr.AddToTail( hDest );
			CDmElement *pDest = GetElement< CDmElement >( hDest );
			pSrc->CopyAttributesTo( pDest, refmap, depth );
			continue;
		}

		destAttr.AddToTail( srcAttr[ i ] );
	}
}

			
//-----------------------------------------------------------------------------
// internal recursive copy method
// builds refmap of old element's handle -> copy's handle, and uses it to fixup references
//-----------------------------------------------------------------------------
void CDmElement::CopyAttributesTo( CDmElement *pCopy, CRefMap &refmap, TraversalDepth_t depth ) const
{
	refmap.Insert( this->GetHandle(), pCopy->GetHandle() );

	// loop attrs, copying - element (and element array) attrs can be marked to always copy deep(er)
	for ( const CDmAttribute *pAttr = FirstAttribute(); pAttr != NULL; pAttr = pAttr->NextAttribute() )
	{
		DmAttributeType_t type = pAttr->GetType();
		const char *pAttrName = pAttr->GetName();
		CDmAttribute *pCopyAttr = pCopy->GetAttribute( pAttrName );

		if ( pCopyAttr == NULL )
		{
			pCopyAttr = pCopy->AddAttribute( pAttrName, type );

			int flags = pAttr->GetFlags();
			Assert( ( flags & FATTRIB_EXTERNAL ) == 0 );
			flags &= ~FATTRIB_EXTERNAL;

			pCopyAttr->ClearFlags();
			pCopyAttr->AddFlag( flags );
		}

		// Temporarily remove the read-only flag from the copy while we copy into it
		bool bReadOnly = pCopyAttr->IsFlagSet( FATTRIB_READONLY );
		if ( bReadOnly )
		{
			pCopyAttr->RemoveFlag( FATTRIB_READONLY );
		}

		if ( type == AT_ELEMENT )
		{
			CopyElementAttribute( pAttr, pCopyAttr, refmap, depth );
		}
		else if ( type == AT_ELEMENT_ARRAY )
		{
			CopyElementArrayAttribute( pAttr, pCopyAttr, refmap, depth );
		}
		else
		{
			pCopyAttr->SetValue( pAttr );
		}

		if ( bReadOnly )
		{
			pCopyAttr->AddFlag( FATTRIB_READONLY );
		}
	}
}

//-----------------------------------------------------------------------------
// FixupReferences
// fixes up any references that Copy wasn't able to figure out
// example:
//	during a shallow copy, a channel doesn't copy its target element,
//	but the targets parent might decide to copy it, (later on in the travesal)
//	so the channel needs to change to refer to the copy
//-----------------------------------------------------------------------------
void CDmElement::FixupReferences( CUtlHashFast< DmElementHandle_t > &visited, const CRefMap &refmap, TraversalDepth_t depth )
{
	if ( visited.Find( GetHandle() ) != visited.InvalidHandle() )
		return;

	visited.Insert( GetHandle(), DMELEMENT_HANDLE_INVALID ); // ignore data arguement - we're just using it as a set

	// loop attrs, copying - element (and element array) attrs can be marked to always copy deep(er)
	for ( CDmAttribute *pAttr = FirstAttribute(); pAttr != NULL; pAttr = pAttr->NextAttribute() )
	{
		DmAttributeType_t type = pAttr->GetType();
		bool bCopy = ShouldTraverse( pAttr, depth );

		if ( type == AT_ELEMENT )
		{
			DmElementHandle_t handle = pAttr->GetValue<DmElementHandle_t>();
			int idx = refmap.Find( handle );
			if ( idx == refmap.InvalidIndex() )
			{
				CDmElement *pElement = GetElement< CDmElement >( handle );
				if ( pElement == NULL || !bCopy )
					continue;

				pElement->FixupReferences( visited, refmap, depth );
			}
			else
			{
				pAttr->SetValue( refmap[ idx ] );
			}
		}
		else if ( type == AT_ELEMENT_ARRAY )
		{
			CDmrElementArray<> attrArray( pAttr );
			int nElements = attrArray.Count();
			for ( int i = 0; i < nElements; ++i )
			{
				DmElementHandle_t handle = attrArray.GetHandle( i );
				int idx = refmap.Find( handle );
				if ( idx == refmap.InvalidIndex() )
				{
					CDmElement *pElement = GetElement< CDmElement >( handle );
					if ( pElement == NULL || !bCopy )
						continue;

					pElement->FixupReferences( visited, refmap, depth );
				}
				else
				{
					attrArray.SetHandle( i, refmap[ idx ] );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Change type (only possible when versioning file formats)
//-----------------------------------------------------------------------------
void CDmElement::SetType( const char *pType )
{
	if ( !g_pDataModelImp->IsCreatingUntypedElements() )
	{
		Warning( "Unable to set type unless you're creating untyped elements!\n" );
		return;
	}

	m_Type = g_pDataModel->GetSymbol( pType );
}


//-----------------------------------------------------------------------------
// owning file
//-----------------------------------------------------------------------------
void CDmElement::SetFileId( DmFileId_t fileid )
{
	g_pDataModelImp->RemoveElementFromFile( m_ref.m_hElement, m_fileId );
	m_fileId = fileid;
	g_pDataModelImp->AddElementToFile( m_ref.m_hElement, fileid );
}


//-----------------------------------------------------------------------------
// recursively set fileid's, with option to only change elements in the matched file
//-----------------------------------------------------------------------------
void CDmElement::SetFileId( DmFileId_t fileid, TraversalDepth_t depth, bool bOnlyIfMatch /* = false */ )
{
	if ( depth != TD_NONE )
	{
		CUtlHashFast< DmElementHandle_t > visited;
		visited.Init( 4096 ); // this will make visited behave reasonably (perf-wise) for trees w/ around 4k elements in them
		SetFileId_R( visited, fileid, depth, GetFileId(), bOnlyIfMatch );
	}
	else
	{
		SetFileId( fileid );
	}
}

void CDmElement::SetFileId_R( CUtlHashFast< DmElementHandle_t > &visited, DmFileId_t fileid, TraversalDepth_t depth, DmFileId_t match, bool bOnlyIfMatch )
{
	if ( bOnlyIfMatch && match != GetFileId() )
		return;

	if ( visited.Find( GetHandle() ) != visited.InvalidHandle() )
		return;

	visited.Insert( GetHandle(), DMELEMENT_HANDLE_INVALID ); // ignore data arguement - we're just using it as a set

	SetFileId( fileid );

	for ( CDmAttribute *pAttr = FirstAttribute(); pAttr != NULL; pAttr = pAttr->NextAttribute() )
	{
		DmAttributeType_t type = pAttr->GetType();
		if ( !ShouldTraverse( pAttr, depth ) )
			continue;

		if ( type == AT_ELEMENT )
		{
			CDmElement *pElement = pAttr->GetValueElement<CDmElement>();
			if ( pElement )
			{
				pElement->SetFileId_R( visited, fileid, depth, match, bOnlyIfMatch );
			}
		}
		else if ( type == AT_ELEMENT_ARRAY )
		{
			CDmrElementArray<> attrArray( pAttr );
			int nElements = attrArray.Count();
			for ( int i = 0; i < nElements; ++i )
			{
				CDmElement *pElement = attrArray[ i ];
				if ( pElement )
				{
					pElement->SetFileId_R( visited, fileid, depth, match, bOnlyIfMatch );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
DmElementHandle_t CDmElement::GetHandle() const
{
	Assert( m_ref.m_hElement != DMELEMENT_HANDLE_INVALID );
	return m_ref.m_hElement;
}


//-----------------------------------------------------------------------------
// Iteration
//-----------------------------------------------------------------------------
int CDmElement::AttributeCount() const
{
	int nAttrs = 0;
	for ( CDmAttribute *pAttr = m_pAttributes; pAttr; pAttr = pAttr->NextAttribute() )
	{
		++nAttrs;
	}
	return nAttrs;
}

CDmAttribute* CDmElement::FirstAttribute()
{
	return m_pAttributes;
}

const CDmAttribute*	CDmElement::FirstAttribute() const
{
	return m_pAttributes;
}

bool CDmElement::HasAttribute( const char *pAttributeName, DmAttributeType_t type ) const
{
	CDmAttribute *pAttribute = FindAttribute( pAttributeName );
	if ( !pAttribute )
		return false;
	
	return ( type == AT_UNKNOWN || ( pAttribute->GetType() == type ) );
}


//-----------------------------------------------------------------------------
//
// Implementation of Undo for adding or removing an attribute to or from and 
// element. 
//
//-----------------------------------------------------------------------------
class CUndoAttributeAddRemove : public CUndoElement
{
	typedef CUndoElement BaseClass;

public:
	CUndoAttributeAddRemove( CDmElement *pElement, CDmAttribute *pAttribute, bool bRemove )
		: BaseClass( "CUndoAttributeAddRemove" ),
		m_bRemove( bRemove ),
		m_bHoldingPtr( bRemove ), 
		m_hElement( pElement->GetHandle() ),
		m_pAttribute( pAttribute )
	{
		m_symAttribute = g_pDataModel->GetSymbol( pAttribute->GetName() );
		Assert( pElement && pElement->GetFileId() != DMFILEID_INVALID );
	}

	~CUndoAttributeAddRemove()
	{
		if ( m_bHoldingPtr && m_pAttribute )
		{
			CDmAttributeAccessor::DestroyAttribute( m_pAttribute );
		}
	}

	virtual void Undo()
	{
		// Remove the attribute if this is an add undo, or add the attribute if a remove undo.
		AddOrRemoveAttributeFromElement( m_bRemove );
	}

	virtual void Redo()
	{		
		// Add the attribute if this an add undo, or remove the attribute if this a remove undo.
		AddOrRemoveAttributeFromElement( !m_bRemove );
	}

	virtual const char	*GetDesc()
	{
		static char buf[ 128 ];

		const char *base = BaseClass::GetDesc();
		Q_snprintf( buf, sizeof( buf ), "%s(%s)", base, m_symAttribute.String() );
		return buf;
	}

private:

	void AddOrRemoveAttributeFromElement( bool bAdd )
	{
		CDmElement *pElement = GetElement();
		if ( pElement == NULL )
			return;
		
		if ( bAdd )
		{
			CDmeElementAccessor::AddAttributeByPtr( pElement, m_pAttribute );
			m_bHoldingPtr = false;
		}
		else
		{
			CDmeElementAccessor::RemoveAttributeByPtrNoDelete( pElement, m_pAttribute );
			m_bHoldingPtr = true;
		}
	}

	CDmElement *GetElement() const 
	{
		return g_pDataModel->GetElement( m_hElement );
	}

	const bool				m_bRemove;
	bool					m_bHoldingPtr;
	const DmElementHandle_t	m_hElement;
	CUtlSymbolLarge			m_symAttribute;
	CDmAttribute *const		m_pAttribute;
};


//-----------------------------------------------------------------------------
// Containing object
//-----------------------------------------------------------------------------
void CDmElement::RemoveAttributeByPtrNoDelete( CDmAttribute *ptr )
{
	for ( CDmAttribute **ppAttr = &m_pAttributes; *ppAttr; ppAttr = ( *ppAttr )->GetNextAttributeRef() )
	{
		if ( ptr == *ppAttr )
		{
			MarkDirty();

			ptr->InvalidateHandle();
			*ppAttr = ( *ppAttr )->NextAttribute();

			g_pDataModelImp->NotifyState( NOTIFY_CHANGE_TOPOLOGICAL );
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Attribute removal
//-----------------------------------------------------------------------------
void CDmElement::RemoveAttribute( CDmAttribute **pAttrRef )
{
	CDmAttribute *pAttrToDelete = *pAttrRef;

	// Removal of external attributes is verboten
	Assert( !pAttrToDelete->IsFlagSet( FATTRIB_EXTERNAL ) );
	if( pAttrToDelete->IsFlagSet( FATTRIB_EXTERNAL ) )
		return;

	// This will cause element attributes to be properly removed from the attribute 
	// reference list of the element they refer to, resolving issues where dead
	// attributes would be encountered when walking the of referring elements.
	pAttrToDelete->SetToDefaultValue();

	MarkDirty();

	// UNDO Hook
	bool storedbyundo = false;
	if ( g_pDataModel->UndoEnabledForElement( this ) )
	{
		MEM_ALLOC_CREDIT_CLASS();
		CUndoAttributeAddRemove *pUndo = new CUndoAttributeAddRemove( this, pAttrToDelete, true );
		g_pDataModel->AddUndoElement( pUndo );
		storedbyundo = true;
	}

	*pAttrRef = ( *pAttrRef )->NextAttribute();

	if ( !storedbyundo )
	{
		CDmAttribute::DestroyAttribute( pAttrToDelete );
	}
	g_pDataModelImp->NotifyState( NOTIFY_CHANGE_TOPOLOGICAL );
}


void CDmElement::RemoveAttribute( const char *pAttributeName )
{
	CUtlSymbolLarge find = g_pDataModel->GetSymbol( pAttributeName );
	for ( CDmAttribute **ppAttr = &m_pAttributes; *ppAttr; ppAttr = ( *ppAttr )->GetNextAttributeRef() )
	{
		if ( find == ( *ppAttr )->GetNameSymbol() )
		{
			RemoveAttribute( ppAttr );
			return;
		}
	}
}

void CDmElement::RemoveAttributeByPtr( CDmAttribute *pAttribute )
{
	Assert( pAttribute );
	for ( CDmAttribute **ppAttr = &m_pAttributes; *ppAttr; ppAttr = ( *ppAttr )->GetNextAttributeRef() )
	{
		if ( pAttribute == *ppAttr )
		{
			RemoveAttribute( ppAttr );
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Sets an attribute from a string
//-----------------------------------------------------------------------------
void CDmElement::SetValueFromString( const char *pAttributeName, const char *pValue )
{
	CDmAttribute *pAttribute = FindAttribute( pAttributeName );
	if ( pAttribute )
	{
		pAttribute->SetValueFromString( pValue );
	}
}


//-----------------------------------------------------------------------------
// Writes an attribute as a string
//-----------------------------------------------------------------------------
const char *CDmElement::GetValueAsString( const char *pAttributeName, char *pBuffer, size_t nBufLen ) const
{
	Assert( pBuffer );
														    
	const CDmAttribute *pAttribute = FindAttribute( pAttributeName );
	if ( pAttribute )
		return pAttribute->GetValueAsString( pBuffer, nBufLen );

	pBuffer[ 0 ] = 0;
	return pBuffer;
}


//-----------------------------------------------------------------------------
// Adds, removes attributes
//-----------------------------------------------------------------------------
void CDmElement::AddAttributeByPtr( CDmAttribute *ptr )
{
	MarkDirty();

	for ( CDmAttribute *pAttr = m_pAttributes; pAttr; pAttr = pAttr->NextAttribute() )
	{
		if ( pAttr == ptr )
		{
			Assert( 0 );
			return;
		}
	}

	*( ptr->GetNextAttributeRef() ) = m_pAttributes;
	m_pAttributes = ptr;

	g_pDataModelImp->NotifyState( NOTIFY_CHANGE_TOPOLOGICAL );
}

CDmAttribute *CDmElement::CreateAttribute( const char *pAttributeName, DmAttributeType_t type )
{
	Assert( !HasAttribute( pAttributeName ) );
	MarkDirty( );

	CDmAttribute *pAttribute = NULL;
	{
		CDisableUndoScopeGuard guard;
		pAttribute = CDmAttribute::CreateAttribute( this, type, pAttributeName );
		*( pAttribute->GetNextAttributeRef() ) = m_pAttributes;
		m_pAttributes = pAttribute;
	}

	if ( g_pDataModel->UndoEnabledForElement( this ) )
	{
		MEM_ALLOC_CREDIT_CLASS();
		CUndoAttributeAddRemove *pUndo = new CUndoAttributeAddRemove( this, pAttribute, false );			
		g_pDataModel->AddUndoElement( pUndo );
	}

	g_pDataModelImp->NotifyState( NOTIFY_CHANGE_TOPOLOGICAL );
	
	return pAttribute;
}

CDmAttribute* CDmElement::AddExternalAttribute( const char *pAttributeName, DmAttributeType_t type, void *pMemory )
{
	DMX_PROFILE_SCOPE( AddExternalAttribute );

	{
		DMX_PROFILE_SCOPE( AddExternalAttribute_MarkDirty );
		MarkDirty( );
	}

	// Add will only add the attribute doesn't already exist
	{
		DMX_PROFILE_SCOPE( AddExternalAttribute_HasAttribute );
		if ( HasAttribute( pAttributeName ) )
		{
			Assert( 0 );
			return NULL;
		}
	}

	CDmAttribute *pAttribute = NULL;
	{
		CDisableUndoScopeGuard guard;
		{
			DMX_PROFILE_SCOPE( AddExternalAttribute_CreateExternalAttribute );
			pAttribute = CDmAttribute::CreateExternalAttribute( this, type, pAttributeName, pMemory );
		}

		*( pAttribute->GetNextAttributeRef() ) = m_pAttributes;
		m_pAttributes = pAttribute;
	}

	{
		DMX_PROFILE_SCOPE( AddExternalAttribute_CheckUndo );
		if ( g_pDataModel->UndoEnabledForElement( this ) )
		{
			MEM_ALLOC_CREDIT_CLASS();
			CUndoAttributeAddRemove *pUndo = new CUndoAttributeAddRemove( this, pAttribute, false );			
			g_pDataModel->AddUndoElement( pUndo );
		}
	}

	g_pDataModelImp->NotifyState( NOTIFY_CHANGE_TOPOLOGICAL );
	
	return pAttribute;
}


//-----------------------------------------------------------------------------
// Find an attribute in the list
//-----------------------------------------------------------------------------
CDmAttribute *CDmElement::FindAttribute( const char *pAttributeName ) const
{
	CUtlSymbolLarge find = g_pDataModel->GetSymbol( pAttributeName );

	for ( CDmAttribute *pAttr = m_pAttributes; pAttr; pAttr = pAttr->NextAttribute() )
	{
		if ( find == pAttr->GetNameSymbol() )
			return pAttr;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// attribute renaming
//-----------------------------------------------------------------------------
void CDmElement::RenameAttribute( const char *pAttributeName, const char *pNewName )
{
	CDmAttribute *pAttr = FindAttribute( pAttributeName );
	if ( pAttr )
	{
		pAttr->SetName( pNewName );
	}
}


//-----------------------------------------------------------------------------
// allows elements to chain OnAttributeChanged up to their parents (or at least, referrers)
//-----------------------------------------------------------------------------
void InvokeOnAttributeChangedOnReferrers( DmElementHandle_t hElement, CDmAttribute *pChangedAttr )
{
	DmAttributeReferenceIterator_t ai = g_pDataModel->FirstAttributeReferencingElement( hElement );
	for ( ; ai != DMATTRIBUTE_REFERENCE_ITERATOR_INVALID; ai = g_pDataModel->NextAttributeReferencingElement( ai ) )
	{
		CDmAttribute *pAttr = g_pDataModel->GetAttribute( ai );
		Assert( pAttr );
		if ( !pAttr )
			continue;

		if ( pAttr->IsFlagSet( FATTRIB_NEVERCOPY ) )
			continue;

		CDmElement *pOwner = pAttr->GetOwner();
		Assert( pOwner );
		if ( !pOwner )
			continue;

		pOwner->OnAttributeChanged( pChangedAttr );
	}
}


//-----------------------------------------------------------------------------
// Destroys an element and all elements it refers to via attributes
//-----------------------------------------------------------------------------
void DestroyElement( CDmElement *pElement, TraversalDepth_t depth )
{
	if ( !pElement )
		return;

	DmElementHandle_t hElement = pElement->GetHandle();

	CDmAttribute* pAttribute;
	for ( pAttribute = pElement->FirstAttribute(); pAttribute; pAttribute = pAttribute->NextAttribute() )
	{
		if ( !ShouldTraverse( pAttribute, depth ) )
			continue;

		g_pDataModelImp->MarkHandleInvalid( hElement ); // prevents recursing through this element again

		switch( pAttribute->GetType() )	
		{
		case AT_ELEMENT:
			{
				CDmElement *pChild = pAttribute->GetValueElement<CDmElement>();
				DestroyElement( pChild, depth );
			}
			break;

		case AT_ELEMENT_ARRAY:
			{
				CDmrElementArray<> array( pAttribute );
				int nElements = array.Count();
				for ( int i = 0; i < nElements; ++i )
				{
					CDmElement *pChild = array[ i ];
					DestroyElement( pChild, depth );
				}
			}
			break;
		}

		g_pDataModelImp->MarkHandleValid( hElement );
	}

	g_pDataModel->DestroyElement( hElement );
}


//-----------------------------------------------------------------------------
//
// generic element tree traversal helper class
//
//-----------------------------------------------------------------------------

CElementTreeTraversal::CElementTreeTraversal( CDmElement *pRoot, const char *pAttrName )
{
	Reset( pRoot, pAttrName );
}

void CElementTreeTraversal::Reset( CDmElement *pRoot, const char *pAttrName )
{
	m_state.RemoveAll();
	m_state.AddToTail( State_t( pRoot, NOT_VISITED ) );
	m_pAttrName = pAttrName;
}

CDmElement *CElementTreeTraversal::Next( bool bSkipChildren /*= false*/ )
{
	if ( m_state.Count() == 1 && m_state[ 0 ].nIndex == NOT_VISITED )
	{
		m_state[ 0 ].nIndex = VISITING;
		return m_state[ 0 ].pElement;
	}

	if ( bSkipChildren )
	{
		m_state.Remove( m_state.Count() - 1 );
	}

	while ( int nCount = m_state.Count() )
	{
		State_t &state = m_state[ nCount - 1 ];
		Assert( state.pElement );
		if ( !state.pElement )
			return NULL;

		CDmrElementArray<> children( state.pElement, m_pAttrName );
		if ( children.IsValid() )
		{
			int nChildren = children.Count();
			while ( ++state.nIndex < nChildren )
			{
				if ( CDmElement *pElement = children[ state.nIndex ] )
				{
					m_state.AddToTail( State_t( pElement, VISITING ) );
					return pElement;
				}
			}
		}

		m_state.Remove( nCount - 1 );
	}

	return NULL;
}

CDmElement *CElementTreeTraversal::GetElement()
{
	int nCount = m_state.Count();
	if ( nCount < 1 )
		return NULL;

	State_t &state = m_state[ nCount - 1 ];
	Assert( state.nIndex == NOT_VISITED || state.nIndex == VISITING );
	return state.nIndex == NOT_VISITED ? NULL : state.pElement;
}

CDmElement *CElementTreeTraversal::GetParent( int i = 0 )
{
	return i + 1 < m_state.Count() ? m_state[ m_state.Count() - i - 1 ].pElement : NULL;
}

int CElementTreeTraversal::GetChildIndex( int i )
{
	return i + 1 < m_state.Count() ? m_state[ m_state.Count() - i - 1 ].nIndex : NOT_VISITED;
}


//-----------------------------------------------------------------------------
//
// element-specific unique name generation methods
//
//-----------------------------------------------------------------------------

// returns startindex if none found, 1 if only "prefix" found, and n+1 if "prefixn" found
int GenerateUniqueNameIndex( const char *prefix, const CUtlVector< DmElementHandle_t > &array, int startindex /*= 0*/ )
{
	return V_GenerateUniqueNameIndex( prefix, ElementArrayNameAccessor< DmElementHandle_t >( array ), startindex );
}

bool GenerateUniqueName( char *name, int memsize, const char *prefix, const CUtlVector< DmElementHandle_t > &array )
{
	return V_GenerateUniqueName( name, memsize, prefix, ElementArrayNameAccessor< DmElementHandle_t >( array ) );
}

int SplitStringIntoBaseAndIntegerSuffix( const char *pName, int len, char *pBaseName )
{
	int baselen = len;
	while ( baselen > 0 && V_isdigit( pName[ baselen - 1 ] ) )
	{
		--baselen;
	}
	V_strncpy( pBaseName, pName, baselen + 1 );

	return ( baselen < len ) ? V_atoi( pName + baselen ) : 0;
}

void MakeElementNameUnique( CDmElement *pElement, const CUtlVector< DmElementHandle_t > &array )
{
	if ( !pElement )
		return;

	const char *pName = pElement->GetName();
	int len = V_strlen( pName );
	char *pBaseName = ( char* )stackalloc( len + 1 );
	int nStartingIndex = SplitStringIntoBaseAndIntegerSuffix( pName, len, pBaseName );

	int i = GenerateUniqueNameIndex( pBaseName, array, nStartingIndex );
	if ( i <= 0 )
	{
		pElement->SetName( pBaseName );
		return;
	}

	int newlen = len + 11; // reserve at least enough space for 10 digits and a terminating '\0'
	char *pNewName = ( char* )stackalloc( newlen );
	Q_snprintf( pNewName, newlen, "%s%d", pBaseName, i );

	pElement->SetName( pNewName );
}

void RemoveElementFromRefereringAttributes( CDmElement *pElement, bool bPreserveOrder /*= true*/ )
{
	for ( DmAttributeReferenceIterator_t i = g_pDataModel->FirstAttributeReferencingElement( pElement->GetHandle() );
		i != DMATTRIBUTE_REFERENCE_ITERATOR_INVALID;
		i = g_pDataModel->FirstAttributeReferencingElement( pElement->GetHandle() ) ) // always re-get the FIRST attribute, since we're removing from this list
	{
		CDmAttribute *pAttribute = g_pDataModel->GetAttribute( i );
		Assert( pAttribute );
		if ( !pAttribute )
			continue;

		if ( IsArrayType( pAttribute->GetType() ) )
		{
			CDmrElementArray<> array( pAttribute );
			int i = array.Find( pElement );
			Assert( i != array.InvalidIndex() );
			if ( bPreserveOrder )
			{
				array.Remove( i );
			}
			else
			{
				array.FastRemove( i );
			}
		}
		else
		{
			pAttribute->SetValue( DMELEMENT_HANDLE_INVALID );
		}
	}
}
