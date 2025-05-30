﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

//#define LOG_DELTA_BITS_TO_FILE
#ifdef LOG_DELTA_BITS_TO_FILE
#undef fopen
#endif

#include "tier1/tokenset.h"

#include <algorithm>
#include <stdarg.h>
#include "dt_send.h"
#include "dt.h"
#include "dt_recv.h"
#include "dt_encode.h"
#include "convar.h"
#include "commonmacros.h"
#include "tier1/strtools.h"
#include "tier0/dbg.h"
#include "dt_stack.h"
#include "filesystem_engine.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PROPINDEX_NUMBITS 12
#define MAX_TOTAL_SENDTABLE_PROPS	( (1 << PROPINDEX_NUMBITS) - 1 ) // one value reserved for end marker
#define PROPINDEX_END_MARKER ( ( 1 << PROPINDEX_NUMBITS ) - 1 )


ConVar g_CV_DTWatchEnt( "dtwatchent", "-1", 0, "Watch this entities data table encoding." );
ConVar g_CV_DTWatchVar( "dtwatchvar", "", 0, "Watch the named variable." );
ConVar g_CV_DTWarning( "dtwarning", "0", 0, "Print data table warnings?" );
ConVar g_CV_DTWatchClass( "dtwatchclass", "", 0, "Watch all fields encoded with this table." );
ConVar g_CV_DTEncode( "dtwatchencode", "1", 0, "When watching show encode." );
ConVar g_CV_DTDecode( "dtwatchdecode", "1", 0, "When watching show decode." );
ConVar sv_new_delta_bits( "sv_new_delta_bits", "1" );


#ifdef LOG_DELTA_BITS_TO_FILE

class CDeltaBitsRun
{
public:
	CUtlVector<int> m_Props;
	CUtlVector<int> m_BitCounts;
};

CUtlVector<CDeltaBitsRun*> g_DeltaBitsRuns;
CDeltaBitsRun *g_pDeltaBitsRun = NULL;

inline void LogDeltaBitsStart()
{
	if ( g_pDeltaBitsRun )
		Error( "LogDeltaBitsStart" );

	g_pDeltaBitsRun = new CDeltaBitsRun;
	g_DeltaBitsRuns.AddToTail( g_pDeltaBitsRun );
}

inline void LogDeltaBitsEnd()
{
	if ( !g_pDeltaBitsRun )
		Error( "LogDeltaBitsEnd" );

	g_pDeltaBitsRun = NULL;
}

inline void LogDeltaBitsEntry( int iProp, int nBits )
{
	g_pDeltaBitsRun->m_Props.AddToTail( iProp );
	g_pDeltaBitsRun->m_BitCounts.AddToTail( nBits );
}


void FlushDeltaBitsTrackingData()
{
	FILE *fp = fopen( "c:\\deltabits.txt", "wt" );
	fprintf( fp, "%d\n", g_DeltaBitsRuns.Count() );

	for ( int i=0; i < g_DeltaBitsRuns.Count(); i++ )
	{
		CDeltaBitsRun *pRun = g_DeltaBitsRuns[i];

		fprintf( fp, "%d ", pRun->m_Props.Count() );
		for ( int z=0; z < pRun->m_Props.Count(); z++ )
		{
			fprintf( fp, "%d %d ", pRun->m_Props[z], pRun->m_BitCounts[z] );
		}
		fprintf( fp, "\n" );
	}

	fclose( fp );
}

#else

inline void LogDeltaBitsStart() {}
inline void LogDeltaBitsEnd() {}
inline void LogDeltaBitsEntry( int iProp, int nBits ) {}
void FlushDeltaBitsTrackingData() {}

#endif




// ----------------------------------------------------------------------------- //
//
// CBuildHierarchyStruct
//
// Used while building a CSendNode hierarchy.
//
// ----------------------------------------------------------------------------- //
class CBuildHierarchyStruct
{
public:
	const ExcludeProp	*m_pExcludeProps;
	int					m_nExcludeProps;

	const SendProp		*m_pDatatableProps[MAX_TOTAL_SENDTABLE_PROPS];
	int					m_nDatatableProps;
	
	const SendProp		*m_pProps[MAX_TOTAL_SENDTABLE_PROPS];
	unsigned char		m_PropProxyIndices[MAX_TOTAL_SENDTABLE_PROPS];
	int					m_nProps;

	unsigned char m_nPropProxies;
};


// ------------------------------------------------------------------------------------ //
// CDeltaBitsWriter.
// ------------------------------------------------------------------------------------ //

static FORCEINLINE unsigned int ReadPropIndex( bf_read *pBuf, bool bNewScheme )
{
	if ( bNewScheme )
	{
		if ( pBuf->ReadOneBit() )
		{
			return pBuf->ReadUBitLong( 3 );
		}
	}

	int ret = pBuf->ReadUBitLong( 7 );
	switch( ret & ( 32 | 64 ) )
	{
		case 32:
			ret = ( ret &~96 ) | ( pBuf->ReadUBitLong( 2 ) << 5 );
			Assert( ret >= 32);
			break;
				
		case 64:
			ret = ( ret &~96 ) | ( pBuf->ReadUBitLong( 4 ) << 5 );
			Assert( ret >= 128);
			break;
		case 96:
			ret = ( ret &~96 ) | ( pBuf->ReadUBitLong( 7 ) << 5 );
			Assert( ret >= 512);
			break;
	}

	return ret;
}

FORCEINLINE void WritePropIndex( bf_write *pBuf, unsigned int n, bool bNewScheme )
{
	Assert( n < (1 << PROPINDEX_NUMBITS ) );
	Assert( ( n & 0xfff ) == n );

	if ( bNewScheme )
	{
		if ( n < 8 )
		{
			pBuf->WriteOneBit( 1 );
			pBuf->WriteUBitLong( n, 3 );
			return;
		}
		else
		{
			pBuf->WriteOneBit( 0 );
		}
	}

	if ( n < 32 )
		pBuf->WriteUBitLong( n, 7 );
	else
		if ( n < 128 )
			pBuf->WriteUBitLong( ( n & 31 ) | 32 | ( ( n & ( 64 | 32 ) ) << 2 ), 9 );
		else
			if ( n < 512 )
				pBuf->WriteUBitLong( ( n & 31 ) | 64 | ( ( n & ( 256 | 128 | 64 | 32 ) ) << 2 ), 11 );
			else
				pBuf->WriteUBitLong( ( n & 31 ) | 96 | 
									 ( ( n & ( 2048 | 1024 | 512 | 256 | 128 | 64 | 32 ) ) << 2 ), 14 );
}


CDeltaBitsWriter::CDeltaBitsWriter( bf_write *pBuf )
{
	m_pBuf = pBuf;
	m_iLastProp = -1;
	LogDeltaBitsStart();
	
	//TODO: Get rid of this..
	m_bUsingNewScheme = sv_new_delta_bits.GetBool();
	if ( m_bUsingNewScheme )
		pBuf->WriteOneBit( 1 );
	else
		pBuf->WriteOneBit( 0 );
}


CDeltaBitsWriter::~CDeltaBitsWriter()
{
	if ( m_pBuf )
		Finish();
}

void CDeltaBitsWriter::Finish()
{
	m_pBuf->WriteOneBit( 0 );
	::WritePropIndex( m_pBuf, PROPINDEX_END_MARKER, m_bUsingNewScheme );
	LogDeltaBitsEnd();
	m_pBuf = NULL;
}

void CDeltaBitsWriter::WritePropIndex( int iProp )
{
	int diff = iProp - m_iLastProp;
	m_iLastProp = iProp;

	Assert( iProp < MAX_DATATABLE_PROPS );
	Assert( diff > 0 && diff < MAX_DATATABLE_PROPS );
	--diff; // It's always at least 1 so subtract 1.

	int startbit = m_pBuf->GetNumBitsWritten();

	if ( m_bUsingNewScheme )
	{
		if ( diff == 0 )
		{
			m_pBuf->WriteOneBit( 1 );
		}
		else
		{
			m_pBuf->WriteOneBit( 0 );
			::WritePropIndex( m_pBuf, diff, m_bUsingNewScheme );
		}
	}
	else
	{
		::WritePropIndex( m_pBuf, diff, m_bUsingNewScheme );
	}

	int nBitsToEncode = m_pBuf->GetNumBitsWritten() - startbit;

	LogDeltaBitsEntry( iProp, nBitsToEncode );
}


// ------------------------------------------------------------------------------------ //
// CDeltaBitsReader.
// ------------------------------------------------------------------------------------ //

CDeltaBitsReader::CDeltaBitsReader( bf_read *pBuf ) : m_nLastFieldPathBits( 0 )
{
	m_pBuf = pBuf;
	m_bFinished = false;
	m_iLastProp = -1;
	
	if ( pBuf )
		m_bUsingNewScheme = (pBuf->ReadOneBit() != 0);
	else
		m_bUsingNewScheme = false;
}


CDeltaBitsReader::~CDeltaBitsReader()
{
	// Make sure they read to the end unless they specifically said they don't care.
	if ( m_pBuf )
	{
		Assert( m_bFinished );
	}
}

int CDeltaBitsReader::GetFieldPathBits() const
{
	return m_nLastFieldPathBits;
}

int CDeltaBitsReader::ReadNextPropIndex()
{
	Assert( !m_bFinished );

	if ( m_bUsingNewScheme )
	{
		if ( m_pBuf->ReadOneBit() )
		{
			m_iLastProp++;
			m_nLastFieldPathBits = 1;
			return m_iLastProp;
		}
	}

	int nStartBit = m_pBuf->GetNumBitsRead();
	int nRead = ReadPropIndex( m_pBuf, m_bUsingNewScheme );
	m_nLastFieldPathBits = m_pBuf->GetNumBitsRead() - nStartBit;
	if ( nRead == PROPINDEX_END_MARKER )
	{
		m_bFinished = true;
		return -1;
	}

	int prop = 1 + nRead;
	prop += m_iLastProp;
	m_iLastProp = prop;

	Assert( m_iLastProp < MAX_DATATABLE_PROPS );

	return prop;
}


void CDeltaBitsReader::ForceFinished()
{
	m_bFinished = true;
	m_pBuf = NULL;
}


// ----------------------------------------------------------------------------- //
// CSendNode.
// ----------------------------------------------------------------------------- //

CSendNode::CSendNode()
{
	m_iDatatableProp = -1;
	m_pTable = NULL;
	
	m_iFirstRecursiveProp = m_nRecursiveProps = 0;

	m_DataTableProxyIndex = DATATABLE_PROXY_INDEX_INVALID; // set it to a questionable value.
}

CSendNode::~CSendNode()
{
	int c = GetNumChildren();
	for ( int i = c - 1 ; i >= 0 ; i-- )
	{
		delete GetChild( i );
	}
	m_Children.Purge();
}

// ----------------------------------------------------------------------------- //
// CSendTablePrecalc
// ----------------------------------------------------------------------------- //

bool PropOffsetLT( const unsigned short &a, const unsigned short &b )
{
	return a < b;
}

CSendTablePrecalc::CSendTablePrecalc() : 
	m_PropOffsetToIndexMap( 0, 0, PropOffsetLT )
{
	m_pDTITable = NULL;
	m_pSendTable = 0;
	m_nDataTableProxies = 0;
}


CSendTablePrecalc::~CSendTablePrecalc()
{
	if ( m_pSendTable )
		m_pSendTable->m_pPrecalc = 0;
}


const ExcludeProp* FindExcludeProp(
	char const *pTableName,
	char const *pPropName,
	const ExcludeProp *pExcludeProps, 
	int nExcludeProps)
{
	for ( int i=0; i < nExcludeProps; i++ )
	{
		if ( stricmp(pExcludeProps[i].m_pTableName, pTableName) == 0 && stricmp(pExcludeProps[i].m_pPropName, pPropName ) == 0 )
			return &pExcludeProps[i];
	}

	return NULL;
}


// Fill in a list of all the excluded props.
static bool SendTable_GetPropsExcluded( const SendTable *pTable, ExcludeProp *pExcludeProps, int &nExcludeProps, int nMaxExcludeProps )
{
	for(int i=0; i < pTable->m_nProps; i++)
	{
		SendProp *pProp = &pTable->m_pProps[i];

		if ( pProp->IsExcludeProp() )
		{
			char const *pName = pProp->GetExcludeDTName();

			ErrorIfNot( pName,
				("Found an exclude prop missing a name.")
			);
			
			ErrorIfNot( nExcludeProps < nMaxExcludeProps,
				("SendTable_GetPropsExcluded: Overflowed max exclude props with %s.", pName)
			);

			pExcludeProps[nExcludeProps].m_pTableName = pName;
			pExcludeProps[nExcludeProps].m_pPropName = pProp->GetName();
			nExcludeProps++;
		}
		else if ( pProp->GetDataTable() )
		{
			if( !SendTable_GetPropsExcluded( pProp->GetDataTable(), pExcludeProps, nExcludeProps, nMaxExcludeProps ) )
				return false;
		}
	}

	return true;
}


// Set the datatable proxy indices in all datatable SendProps.
static void SetDataTableProxyIndices_R( 
	CSendTablePrecalc *pMainTable, 
	CSendNode *pCurTable,
	CBuildHierarchyStruct *bhs )
{
	for ( int i=0; i < pCurTable->GetNumChildren(); i++ )
	{
		CSendNode *pNode = pCurTable->GetChild( i );
		const SendProp *pProp = bhs->m_pDatatableProps[pNode->m_iDatatableProp];

		if ( pProp->GetFlags() & SPROP_PROXY_ALWAYS_YES )
		{
			pNode->SetDataTableProxyIndex( DATATABLE_PROXY_INDEX_NOPROXY );
		}
		else
		{
			pNode->SetDataTableProxyIndex( pMainTable->GetNumDataTableProxies() );
			pMainTable->SetNumDataTableProxies( pMainTable->GetNumDataTableProxies() + 1 );
		}

		SetDataTableProxyIndices_R( pMainTable, pNode, bhs );
	}
}

// Set the datatable proxy indices in all datatable SendProps.
static void SetRecursiveProxyIndices_R( 
	SendTable *pBaseTable,
	CSendNode *pCurTable,
	int &iCurProxyIndex )
{
	if ( iCurProxyIndex >= CDatatableStack::MAX_PROXY_RESULTS )
		Error( "Too many proxies for datatable %s.", pBaseTable->GetName() );

	pCurTable->SetRecursiveProxyIndex( iCurProxyIndex );
	iCurProxyIndex++;
	
	for ( int i=0; i < pCurTable->GetNumChildren(); i++ )
	{
		CSendNode *pNode = pCurTable->GetChild( i );
		SetRecursiveProxyIndices_R( pBaseTable, pNode, iCurProxyIndex );
	}
}


void SendTable_BuildHierarchy( 
	CSendNode *pNode,
	const SendTable *pTable, 
	CBuildHierarchyStruct *bhs
	);


void SendTable_BuildHierarchy_IterateProps(
	CSendNode *pNode,
	const SendTable *pTable, 
	CBuildHierarchyStruct *bhs,
	const SendProp *pNonDatatableProps[MAX_TOTAL_SENDTABLE_PROPS],
	int &nNonDatatableProps )
{
	int i;
	for ( i=0; i < pTable->m_nProps; i++ )
	{
		const SendProp *pProp = &pTable->m_pProps[i];

		if ( pProp->IsExcludeProp() || 
			pProp->IsInsideArray() || 
			FindExcludeProp( pTable->GetName(), pProp->GetName(), bhs->m_pExcludeProps, bhs->m_nExcludeProps ) )
		{
			continue;
		}

		if ( pProp->GetType() == DPT_DataTable )
		{
			if ( pProp->GetFlags() & SPROP_COLLAPSIBLE )
			{
				// This is a base class.. no need to make a new CSendNode (and trigger a bunch of
				// unnecessary send proxy calls in the datatable stacks).
				SendTable_BuildHierarchy_IterateProps( 
					pNode,
					pProp->GetDataTable(), 
					bhs, 
					pNonDatatableProps, 
					nNonDatatableProps );
			}
			else
			{
				// Setup a child datatable reference.
				CSendNode *pChild = new CSendNode;

				// Setup a datatable prop for this node to reference (so the recursion
				// routines can get at the proxy).
				if ( bhs->m_nDatatableProps >= ARRAYSIZE( bhs->m_pDatatableProps ) )
					Error( "Overflowed datatable prop list in SendTable '%s'.", pTable->GetName() );
				
				bhs->m_pDatatableProps[bhs->m_nDatatableProps] = pProp;
				pChild->m_iDatatableProp = bhs->m_nDatatableProps;
				++bhs->m_nDatatableProps;

				pNode->m_Children.AddToTail( pChild );

				// Recurse into the new child datatable.
				SendTable_BuildHierarchy( pChild, pProp->GetDataTable(), bhs );
			}
		}
		else
		{
			if ( nNonDatatableProps >= MAX_TOTAL_SENDTABLE_PROPS )
				Error( "SendTable_BuildHierarchy: overflowed non-datatable props with '%s'.", pProp->GetName() );
			
			pNonDatatableProps[nNonDatatableProps] = pProp;
			++nNonDatatableProps;
		}
	}
}


void SendTable_BuildHierarchy( 
	CSendNode *pNode,
	const SendTable *pTable, 
	CBuildHierarchyStruct *bhs
	)
{
	pNode->m_pTable = pTable;
	pNode->m_iFirstRecursiveProp = bhs->m_nProps;
	
	if ( bhs->m_nPropProxies >= 255 )
	{
		Error( "Exceeded max number of datatable proxies in SendTable_BuildHierarchy()" );
	}

	unsigned char curPropProxy = bhs->m_nPropProxies;
	++bhs->m_nPropProxies;

	const SendProp *pNonDatatableProps[MAX_TOTAL_SENDTABLE_PROPS];
	int nNonDatatableProps = 0;
	
	// First add all the child datatables.
	SendTable_BuildHierarchy_IterateProps(
		pNode,
		pTable,
		bhs,
		pNonDatatableProps,
		nNonDatatableProps );

	
	// Now add the properties.

	// Make sure there's room, then just copy the pointers from the loop above.
	ErrorIfNot( bhs->m_nProps + nNonDatatableProps < ARRAYSIZE( bhs->m_pProps ),
		("SendTable_BuildHierarchy: overflowed prop buffer.")
	);
	
	for ( int i=0; i < nNonDatatableProps; i++ )
	{
		bhs->m_pProps[bhs->m_nProps] = pNonDatatableProps[i];
		bhs->m_PropProxyIndices[bhs->m_nProps] = curPropProxy;
		++bhs->m_nProps;
	}

	pNode->m_nRecursiveProps = bhs->m_nProps - pNode->m_iFirstRecursiveProp;
}

static int __cdecl SendProp_SortPriorities( const byte *p1, const byte *p2 )
{
	return *p1 > *p2;
}

void SendTable_SortByPriority(CBuildHierarchyStruct *bhs)
{
	CUtlVector<byte> priorities;

	// Build a list of priorities 

	// Default entry for SPROP_CHANGES_OFTEN
	priorities.AddToTail( SENDPROP_CHANGES_OFTEN_PRIORITY );

	for ( int i = 0; i < bhs->m_nProps; i++ )
	{
		const SendProp *p = bhs->m_pProps[i];

		if ( priorities.Find( p->GetPriority() ) < 0 )
		{
			priorities.AddToTail( p->GetPriority() );
		}
	}

	// We're using this one because CUtlVector::Sort utilizes qsort, which has different behavior on Windows than on Linux
	std::stable_sort( priorities.Base(), priorities.Base() + priorities.Count() );

	int start = 0;

	for ( int priorityIndex = 0; priorityIndex < priorities.Count(); ++priorityIndex )
	{
		byte priority = priorities[priorityIndex];
		int i;
	
		while( true )
		{
			for ( i = start; i < bhs->m_nProps; i++ )
			{
				const SendProp *p = bhs->m_pProps[i];
				unsigned char c = bhs->m_PropProxyIndices[i];

				if ( p->GetPriority() == priority ||
					 ( ( p->GetFlags() & SPROP_CHANGES_OFTEN ) && priority == SENDPROP_CHANGES_OFTEN_PRIORITY ) )
				{
					if ( i != start )
					{
						bhs->m_pProps[i] = bhs->m_pProps[start];
						bhs->m_PropProxyIndices[i] = bhs->m_PropProxyIndices[start];
						bhs->m_pProps[start] = p;
						bhs->m_PropProxyIndices[start] = c;
					}
					start++;
					break;
				}
			}
	
			if ( i == bhs->m_nProps )
				break; 
		}
	}
}

void CalcPathLengths_R( CSendNode *pNode, CUtlVector<int> &pathLengths, int curPathLength, int &totalPathLengths )
{
	pathLengths[pNode->GetRecursiveProxyIndex()] = curPathLength;
	totalPathLengths += curPathLength;
	
	for ( int i=0; i < pNode->GetNumChildren(); i++ )
	{
		CalcPathLengths_R( pNode->GetChild( i ), pathLengths, curPathLength+1, totalPathLengths );
	}
}


void FillPathEntries_R( CSendTablePrecalc *pPrecalc, CSendNode *pNode, CSendNode *pParent, int &iCurEntry )
{
	// Fill in this node's path.
	CSendTablePrecalc::CProxyPath &outProxyPath = pPrecalc->m_ProxyPaths[ pNode->GetRecursiveProxyIndex() ];
	outProxyPath.m_iFirstEntry = (unsigned short)iCurEntry;

	// Copy all the proxies leading to the parent.
	if ( pParent )
	{
		CSendTablePrecalc::CProxyPath &parentProxyPath = pPrecalc->m_ProxyPaths[pParent->GetRecursiveProxyIndex()];
		outProxyPath.m_nEntries = parentProxyPath.m_nEntries + 1;

		for ( int i=0; i < parentProxyPath.m_nEntries; i++ )
			pPrecalc->m_ProxyPathEntries[iCurEntry++] = pPrecalc->m_ProxyPathEntries[parentProxyPath.m_iFirstEntry+i];
		
		// Now add this node's own proxy.
		pPrecalc->m_ProxyPathEntries[iCurEntry].m_iProxy = pNode->GetRecursiveProxyIndex();
		pPrecalc->m_ProxyPathEntries[iCurEntry].m_iDatatableProp = pNode->m_iDatatableProp;
		++iCurEntry;
	}
	else
	{
		outProxyPath.m_nEntries = 0;
	}

	for ( int i=0; i < pNode->GetNumChildren(); i++ )
	{
		FillPathEntries_R( pPrecalc, pNode->GetChild( i ), pNode, iCurEntry );
	}
}


void SendTable_GenerateProxyPaths( CSendTablePrecalc *pPrecalc, int nProxyIndices )
{
	// Initialize the array.
	pPrecalc->m_ProxyPaths.SetSize( nProxyIndices );
	for ( int i=0; i < nProxyIndices; i++ )
		pPrecalc->m_ProxyPaths[i].m_iFirstEntry = pPrecalc->m_ProxyPaths[i].m_nEntries = 0xFFFF;
	
	// Figure out how long the path down the tree is to each node.
	int totalPathLengths = 0;
	CUtlVector<int> pathLengths;
	pathLengths.SetSize( nProxyIndices );
	memset( pathLengths.Base(), 0, sizeof( pathLengths[0] ) * nProxyIndices );
	CalcPathLengths_R( pPrecalc->GetRootNode(), pathLengths, 0, totalPathLengths );
	
	// 
	int iCurEntry = 0;
	pPrecalc->m_ProxyPathEntries.SetSize( totalPathLengths );
	FillPathEntries_R( pPrecalc, pPrecalc->GetRootNode(), NULL, iCurEntry );
}


bool CSendTablePrecalc::SetupFlatPropertyArray()
{
	SendTable *pTable = GetSendTable();

	// First go through and set SPROP_INSIDEARRAY when appropriate, and set array prop pointers.
	SetupArrayProps_R<SendTable, SendTable::PropType>( pTable );

	// Make a list of which properties are excluded.
	ExcludeProp excludeProps[MAX_EXCLUDE_PROPS];
	int nExcludeProps = 0;
	if( !SendTable_GetPropsExcluded( pTable, excludeProps, nExcludeProps, MAX_EXCLUDE_PROPS ) )
		return false;

	// Now build the hierarchy.
	CBuildHierarchyStruct bhs;
	bhs.m_pExcludeProps = excludeProps;
	bhs.m_nExcludeProps = nExcludeProps;
	bhs.m_nProps = bhs.m_nDatatableProps = 0;
	bhs.m_nPropProxies = 0;
	SendTable_BuildHierarchy( GetRootNode(), pTable, &bhs );

	SendTable_SortByPriority( &bhs );

	// Copy the SendProp pointers into the precalc.	
	MEM_ALLOC_CREDIT();
	m_Props.CopyArray( bhs.m_pProps, bhs.m_nProps );
	m_DatatableProps.CopyArray( bhs.m_pDatatableProps, bhs.m_nDatatableProps );
	m_PropProxyIndices.CopyArray( bhs.m_PropProxyIndices, bhs.m_nProps );

	// Assign the datatable proxy indices.
	SetNumDataTableProxies( 0 );
	SetDataTableProxyIndices_R( this, GetRootNode(), &bhs );
	
	int nProxyIndices = 0;
	SetRecursiveProxyIndices_R( pTable, GetRootNode(), nProxyIndices );

	SendTable_GenerateProxyPaths( this, nProxyIndices );

	return true;
}


// ---------------------------------------------------------------------------------------- //
// Helpers.
// ---------------------------------------------------------------------------------------- //

// Compares two arrays of bits.
// Returns true if they are equal.
bool AreBitArraysEqual(
	void const *pvBits1,
	void const *pvBits2,
	int nBits ) 
{
	int i, nBytes, bit1, bit2;

	unsigned char const *pBits1 = (unsigned char*)pvBits1;
	unsigned char const *pBits2 = (unsigned char*)pvBits2;

	// Compare bytes.
	nBytes = nBits >> 3;
	if( memcmp( pBits1, pBits2, nBytes ) != 0 )
		return false;

	// Compare remaining bits.
	for(i=nBytes << 3; i < nBits; i++)
	{
		bit1 = pBits1[i >> 3] & (1 << (i & 7));
		bit2 = pBits2[i >> 3] & (1 << (i & 7));
		if(bit1 != bit2)
			return false;
	}

	return true;
}


// Does a fast memcmp-based test to determine if the two bit arrays are different.
// Returns true if they are equal.
bool CompareBitArrays(
	void const *pPacked1,
	void const *pPacked2,
	int nBits1,
	int nBits2
	)
{
	if( nBits1 >= 0 && nBits1 == nBits2 )
	{
		if ( pPacked1 == pPacked2 )
		{
			return true;
		}
		else
		{
			return AreBitArraysEqual( pPacked1, pPacked2, nBits1 );
		}
	}
	else
		return false;
}

// Looks at the DTWatchEnt and DTWatchProp console variables and returns true
// if the user wants to watch this property.
bool ShouldWatchThisProp( const SendTable *pTable, int objectID, const char *pPropName )
{
	if ( !g_CV_DTEncode.GetBool() )
	{
		return false;
	}

	if(g_CV_DTWatchEnt.GetInt() != -1 &&
		g_CV_DTWatchEnt.GetInt() == objectID)
	{
		const char *pStr = g_CV_DTWatchVar.GetString();
		if ( pStr && pStr[0] != 0 )
		{
			return stricmp( pStr, pPropName ) == 0;
		}
		else
		{
			return true;
		}
	}

	if ( g_CV_DTWatchClass.GetString()[ 0 ] && Q_stristr( pTable->GetName(), g_CV_DTWatchClass.GetString() ) )
		return true;

	return false;
}

// Looks at the DTWatchEnt and DTWatchProp console variables and returns true
// if the user wants to watch this property.
bool ShouldWatchThisProp( const RecvTable *pTable, int objectID, const char *pPropName )
{
	if ( !g_CV_DTDecode.GetBool() )
	{
		return false;
	}

	if(g_CV_DTWatchEnt.GetInt() != -1 &&
		g_CV_DTWatchEnt.GetInt() == objectID)
	{
		const char *pStr = g_CV_DTWatchVar.GetString();
		if ( pStr && pStr[0] != 0 )
		{
			return stricmp( pStr, pPropName ) == 0;
		}
		else
		{
			return true;
		}
	}

	if ( g_CV_DTWatchClass.GetString()[ 0 ] && Q_stristr( pTable->GetName(), g_CV_DTWatchClass.GetString() ) )
		return true;

	return false;
}

bool Sendprop_UsingDebugWatch()
{
	if ( g_CV_DTWatchEnt.GetInt() != -1 )
		return true;

	if ( g_CV_DTWatchClass.GetString()[ 0 ] )
		return true; 

	return false;
}


// Prints a datatable warning into the console.
void DataTable_Warning( const char *pInMessage, ... )
{
	char msg[4096];
	va_list marker;
	
#if 0
	#if !defined(_DEBUG)
		if(!g_CV_DTWarning.GetInt())
			return;
	#endif
#endif

	va_start(marker, pInMessage);
	Q_vsnprintf( msg, sizeof( msg ), pInMessage, marker);
	va_end(marker);

	Warning( "DataTable warning: %s", msg );
}






