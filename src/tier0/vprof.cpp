﻿//===== Copyright 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Real-Time Hierarchical Profiling
//
// $NoKeywords: $
//===========================================================================//

#include "pch_tier0.h"

#include "tier0/memalloc.h"
#include "tier0/valve_off.h"

#if defined(_WIN32) && !defined(_X360)
#define WIN_32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <assert.h>

#ifdef _WIN32
#pragma warning(disable:4073)
#pragma init_seg( lib )
#endif

#pragma warning(push, 1)
#pragma warning(disable:4786)
#pragma warning(disable:4530)
#include <map>
#include <vector>
#include <algorithm>
#pragma warning(pop)

#include "tier0/valve_on.h"
#include "tier0/vprof.h"
#include "tier0/l2cache.h"
#include "tier0/tslist.h"
#include "tier0/icommandline.h"
#include "tier0/dynfunction.h"
#include "strtools.h"


#ifdef _X360

#include "xbox/xbox_console.h"

#elif defined(_PS3)
#include "ps3/ps3_console.h"

#else // NOT _X360:

#include "tier0/memdbgon.h"

#endif

// NOTE: Explicitly and intentionally using STL in here to not generate any
// cyclical dependencies between the low-level debug library and the higher
// level data structures (toml 01-27-03)
using namespace std;

#ifdef VPROF_ENABLED


#if defined(_X360) && !defined(_CERT)  // enable PIX CPU trace:
#include "tracerecording.h"
#pragma comment( lib, "tracerecording.lib" )
#pragma comment( lib, "xbdm.lib" )
#endif

//-----------------------------------------------------------------------------
bool g_VProfSignalSpike;

//-----------------------------------------------------------------------------

CVProfile g_VProfCurrentProfile;

int CVProfNode::s_iCurrentUniqueNodeID = 0;



CVProfNode::~CVProfNode()
{
#if !defined( _WIN32 ) && !defined( POSIX )
	delete m_pChild;
	delete m_pSibling;
#endif
}

CVProfNode *CVProfNode::GetSubNode( const tchar *pszName, int detailLevel, const tchar *pBudgetGroupName, int budgetFlags )
{
	// Try to find this sub node
	CVProfNode * child = m_pChild;
	while ( child ) 
	{
		if ( child->m_pszName == pszName ) 
		{
			return child;
		}
		child = child->m_pSibling;
	}

	// We didn't find it, so add it
	CVProfNode * node = new CVProfNode( pszName, detailLevel, this, pBudgetGroupName, budgetFlags );
	node->m_pSibling = m_pChild;
	m_pChild = node;
	return node;
}

CVProfNode *CVProfNode::GetSubNode( const tchar *pszName, int detailLevel, const tchar *pBudgetGroupName )
{
	return GetSubNode( pszName, detailLevel, pBudgetGroupName, BUDGETFLAG_OTHER );
}


//-------------------------------------

void CVProfNode::EnterScope()
{
	m_nCurFrameCalls++;
	if ( m_nRecursions++ == 0 ) 
	{
		m_Timer.Start();
#ifndef _X360
		if ( g_VProfCurrentProfile.UsePME() )
		{
			m_L2Cache.Start();
		}
#else // 360 code:
		if ( g_VProfCurrentProfile.UsePME() || ((m_iBitFlags & kRecordL2) != 0) ) 
		{
			m_PMCData.Start();
		}

		if ( (m_iBitFlags & kCPUTrace) != 0)
		{
			// this node is to be recorded. Which recording mode are we in?
			switch ( g_VProfCurrentProfile.GetCPUTraceMode() )
			{
			case CVProfile::kFirstHitNode:
			case CVProfile::kAllNodesInFrame_Recording:
			case CVProfile::kAllNodesInFrame_RecordingMultiFrame:
				// we are presently recording.
				if ( !XTraceStartRecording( g_VProfCurrentProfile.GetCPUTraceFilename() ) )
				{
					Msg( "XTraceStartRecording failed, error code %d\n", GetLastError() );
				}

			default:
				// no default.
				break;
			}

		}

#endif

#ifdef VPROF_VTUNE_GROUP
		g_VProfCurrentProfile.PushGroup( m_BudgetGroupID );
#endif
	}
}

//-------------------------------------

bool CVProfNode::ExitScope()
{
	if ( --m_nRecursions == 0 && m_nCurFrameCalls != 0 ) 
	{
		m_Timer.End();
		m_CurFrameTime += m_Timer.GetDuration();
#ifndef _X360
		if ( g_VProfCurrentProfile.UsePME() )
		{
			m_L2Cache.End();
			m_iCurL2CacheMiss += m_L2Cache.GetL2CacheMisses();
		}
#else // 360 code:
		if ( g_VProfCurrentProfile.UsePME() || ((m_iBitFlags & kRecordL2) != 0) ) 
		{
			m_PMCData.End();
			m_iCurL2CacheMiss   += m_PMCData.GetL2CacheMisses();
			m_iCurLoadHitStores += m_PMCData.GetLHS();
		}

		if ( (m_iBitFlags & kCPUTrace) != 0 )
		{
			// this node is enabled to be recorded. What mode are we in?
			switch ( g_VProfCurrentProfile.GetCPUTraceMode() )
			{
			case CVProfile::kFirstHitNode:
			{
				// one-off recording. stop now.
				if ( XTraceStopRecording() )
				{
					Msg( "CPU trace finished.\n" );
					if ( g_VProfCurrentProfile.TraceCompleteEvent() )
					{
						// signal VXConsole that trace is completed
						XBX_rTraceComplete();
					}
				}
				// don't trace again next frame, overwriting the file.
				g_VProfCurrentProfile.SetCPUTraceEnabled( CVProfile::kDisabled );
				break;
			}

			case CVProfile::kAllNodesInFrame_Recording:
			case CVProfile::kAllNodesInFrame_RecordingMultiFrame:
			{
				// one-off recording. stop now.
				if ( XTraceStopRecording() )
				{
					if ( g_VProfCurrentProfile.GetCPUTraceMode() == CVProfile::kAllNodesInFrame_RecordingMultiFrame )
					{
						Msg( "%.3f msec in %s\n", m_CurFrameTime.GetMillisecondsF(), g_VProfCurrentProfile.GetCPUTraceFilename() );
					}
					else
					{
						Msg( "CPU trace finished.\n" );
					}
				}
				
				// Spew time info for file to allow figuring it out later
				g_VProfCurrentProfile.LatchMultiFrame( m_CurFrameTime.GetLongCycles() );
				
#if 0 // This doesn't want to work on the xbox360-- MoveFile not available or file still being put down to disk?
				char suffix[ 32 ];
				_snprintf( suffix, sizeof( suffix ), "_%.3f_msecs", flMsecs );

				char fn[ 512 ];

				strncpy( fn, g_VProfCurrentProfile.GetCPUTraceFilename(), sizeof( fn ) );

				char *p = strrchr( fn, '.' );
				if ( *p )
				{	
					*p = 0;
				}
				strncat( fn, suffix, sizeof( fn ) );
				strncat( fn, ".pix2", sizeof( fn ) );
			
				BOOL bSuccess = MoveFile( g_VProfCurrentProfile.GetCPUTraceFilename(), fn );
				if ( !bSuccess )
				{
					DWORD eCode = GetLastError();
					Msg( "Error %d\n", eCode );
				}
#endif

				// we're still recording until the frame is done.
				// but, increment the index.
				g_VProfCurrentProfile.IncrementMultiTraceIndex();
				break;
			}

			}
			
			// g_VProfCurrentProfile.IsCPUTraceEnabled() && 


		}

#endif

#ifdef VPROF_VTUNE_GROUP
		g_VProfCurrentProfile.PopGroup();
#endif
	}
	return ( m_nRecursions == 0 );
}

//-------------------------------------

void CVProfNode::Pause()
{
	if ( m_nRecursions > 0 ) 
	{
		m_Timer.End();
		m_CurFrameTime += m_Timer.GetDuration();

#ifndef _X360
		if ( g_VProfCurrentProfile.UsePME() )
		{
			m_L2Cache.End();
			m_iCurL2CacheMiss += m_L2Cache.GetL2CacheMisses();
		}
#else // 360 code:
		if ( g_VProfCurrentProfile.UsePME() || ((m_iBitFlags & kRecordL2) != 0) ) 
		{
			m_PMCData.End();
			m_iCurL2CacheMiss   += m_PMCData.GetL2CacheMisses();
			m_iCurLoadHitStores += m_PMCData.GetLHS();
		}
#endif
	}
	if ( m_pChild ) 
	{
		m_pChild->Pause();
	}
	if ( m_pSibling ) 
	{
		m_pSibling->Pause();
	}
}

//-------------------------------------

void CVProfNode::Resume()
{
	if ( m_nRecursions > 0 ) 
	{
		m_Timer.Start();

#ifndef _X360
		if ( g_VProfCurrentProfile.UsePME() )
		{
			m_L2Cache.Start();
		}
#else
		if ( g_VProfCurrentProfile.UsePME() || ((m_iBitFlags & kRecordL2) != 0) ) 
		{
			m_PMCData.Start();
		}
#endif
	}
	if ( m_pChild ) 
	{
		m_pChild->Resume();
	}
	if ( m_pSibling ) 
	{
		m_pSibling->Resume();
	}
}

//-------------------------------------

void CVProfNode::Reset()
{
	m_nPrevFrameCalls = 0;
	m_PrevFrameTime.Init();

	m_nCurFrameCalls = 0;
	m_CurFrameTime.Init();
	
	m_nTotalCalls = 0;
	m_TotalTime.Init();
	
	m_PeakTime.Init();

	m_iPrevL2CacheMiss = 0;
	m_iCurL2CacheMiss = 0;
	m_iTotalL2CacheMiss = 0;

#ifdef _X360
	m_iPrevLoadHitStores = 0;
	m_iCurLoadHitStores = 0;
	m_iTotalLoadHitStores = 0;
#endif

	if ( m_pChild ) 
	{
		m_pChild->Reset();
	}
	if ( m_pSibling ) 
	{
		m_pSibling->Reset();
	}
}


//-------------------------------------

void CVProfNode::MarkFrame()
{
	m_nPrevFrameCalls = m_nCurFrameCalls;
	m_PrevFrameTime = m_CurFrameTime;
	m_iPrevL2CacheMiss = m_iCurL2CacheMiss;
#ifdef _X360
	m_iPrevLoadHitStores = m_iCurLoadHitStores;
#endif
	m_nTotalCalls += m_nCurFrameCalls;
	m_TotalTime += m_CurFrameTime;
	
	if ( m_PeakTime.IsLessThan( m_CurFrameTime ) )
	{
		m_PeakTime = m_CurFrameTime;
	}
	
	m_CurFrameTime.Init();
	m_nCurFrameCalls = 0;
	m_iTotalL2CacheMiss += m_iCurL2CacheMiss;
	m_iCurL2CacheMiss = 0;
#ifdef _X360
	m_iTotalLoadHitStores += m_iCurLoadHitStores;
	m_iCurLoadHitStores = 0;
#endif
	if ( m_pChild ) 
	{
		m_pChild->MarkFrame();
	}
	if ( m_pSibling ) 
	{
		m_pSibling->MarkFrame();
	}
}

//-------------------------------------

void CVProfNode::ResetPeak()
{
	m_PeakTime.Init();

	if ( m_pChild ) 
	{
		m_pChild->ResetPeak();
	}
	if ( m_pSibling ) 
	{
		m_pSibling->ResetPeak();
	}
}


void CVProfNode::SetCurFrameTime( unsigned long milliseconds )
{
	m_CurFrameTime.Init( (float)milliseconds );
}
#ifdef DBGFLAG_VALIDATE
//-----------------------------------------------------------------------------
// Purpose: Ensure that all of our internal structures are consistent, and
//			account for all memory that we've allocated.
// Input:	validator -		Our global validator object
//			pchName -		Our name (typically a member var in our container)
//-----------------------------------------------------------------------------
void CVProfNode::Validate( CValidator &validator, tchar *pchName )
{
	validator.Push( _T("CVProfNode"), this, pchName );

	m_L2Cache.Validate( validator, _T("m_L2Cache") );

	if ( m_pSibling )
		m_pSibling->Validate( validator, _T("m_pSibling") );
	if ( m_pChild )
		m_pChild->Validate( validator, _T("m_pChild") );

	validator.Pop( );
}
#endif // DBGFLAG_VALIDATE



//-----------------------------------------------------------------------------

struct TimeSums_t
{
	const tchar *pszProfileScope;
	unsigned	calls;
	double 		time;
	double 		timeLessChildren;
	double		peak;
};

static bool TimeCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	return ( lhs.time > rhs.time );
}

static bool TimeLessChildrenCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	return ( lhs.timeLessChildren > rhs.timeLessChildren );
}

static bool PeakCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	return ( lhs.peak > rhs.peak );
}

static bool AverageTimeCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	double avgLhs = ( lhs.calls ) ? lhs.time / (double)lhs.calls : 0.0;
	double avgRhs = ( rhs.calls ) ? rhs.time / (double)rhs.calls : 0.0;
	return ( avgLhs > avgRhs );
}

static bool AverageTimeLessChildrenCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	double avgLhs = ( lhs.calls ) ? lhs.timeLessChildren / (double)lhs.calls : 0.0;
	double avgRhs = ( rhs.calls ) ? rhs.timeLessChildren / (double)rhs.calls : 0.0;
	return ( avgLhs > avgRhs );

}
static bool PeakOverAverageCompare( const TimeSums_t &lhs, const TimeSums_t &rhs )
{
	double avgLhs = ( lhs.calls ) ? lhs.time / (double)lhs.calls : 0.0;
	double avgRhs = ( rhs.calls ) ? rhs.time / (double)rhs.calls : 0.0;
	
	double lhsPoA = ( avgLhs != 0 ) ? lhs.peak / avgLhs : 0.0;
	double rhsPoA = ( avgRhs != 0 ) ? rhs.peak / avgRhs : 0.0;
	
	return ( lhsPoA > rhsPoA );
}

map<CVProfNode *, double> 	g_TimesLessChildren;
int							g_TotalFrames;
map<const tchar *, uintp>	g_TimeSumsMap;
vector<TimeSums_t> 			g_TimeSums;
CVProfNode *				g_pStartNode;
const tchar *				g_pszSumNode;

//-------------------------------------

void CVProfile::SumTimes( CVProfNode *pNode, int budgetGroupID )
{
	if ( !pNode )
		return; // this generally only happens on a failed FindNode()

	bool bSetStartNode;
	if ( !g_pStartNode && _tcscmp( pNode->GetName(), g_pszSumNode ) == 0 )
	{
		g_pStartNode = pNode;
		bSetStartNode = true;
	}
	else
		bSetStartNode = false;

	if ( GetRoot() != pNode )
	{
		if ( g_pStartNode && pNode->GetTotalCalls() > 0 && ( budgetGroupID == -1 || pNode->GetBudgetGroupID() == budgetGroupID ) )
		{
			double timeLessChildren = pNode->GetTotalTimeLessChildren();
			
			g_TimesLessChildren.insert( make_pair( pNode, timeLessChildren ) );
			
			map<const tchar *, uintp>::iterator iter;
			iter = g_TimeSumsMap.find( pNode->GetName() ); // intenionally using address of string rather than string compare (toml 01-27-03)
			if ( iter == g_TimeSumsMap.end() )
			{
				TimeSums_t timeSums = { pNode->GetName(), static_cast<unsigned int>(pNode->GetTotalCalls()), pNode->GetTotalTime(), timeLessChildren, pNode->GetPeakTime() };
				g_TimeSumsMap.insert( make_pair( pNode->GetName(), g_TimeSums.size() ) );
				g_TimeSums.push_back( timeSums );
			}
			else
			{
				TimeSums_t &timeSums = g_TimeSums[iter->second];
				timeSums.calls += pNode->GetTotalCalls();
				timeSums.time += pNode->GetTotalTime();
				timeSums.timeLessChildren += timeLessChildren;
				if ( pNode->GetPeakTime() > timeSums.peak )
					timeSums.peak = pNode->GetPeakTime();
			}
		}
			
		if( ( !g_pStartNode || pNode != g_pStartNode ) && pNode->GetSibling() )
		{
			SumTimes( pNode->GetSibling(), budgetGroupID );
		}
	}
	
	if( pNode->GetChild() )
	{
		SumTimes( pNode->GetChild(), budgetGroupID );
	}
		
	if ( bSetStartNode )
		g_pStartNode = NULL;
}

//-------------------------------------

CVProfNode *CVProfile::FindNode( CVProfNode *pStartNode, const tchar *pszNode )
{
	if ( _tcscmp( pStartNode->GetName(), pszNode ) != 0 )
	{
		CVProfNode *pFoundNode = NULL;
		if ( pStartNode->GetSibling() )
		{
			pFoundNode = FindNode( pStartNode->GetSibling(), pszNode );
		}

		if ( !pFoundNode && pStartNode->GetChild() )
		{
			pFoundNode = FindNode( pStartNode->GetChild(), pszNode );
		}

		return pFoundNode;
	}
	return pStartNode;
}

//-------------------------------------
#ifdef _X360

void CVProfile::PMCDisableAllNodes(CVProfNode *pStartNode)
{
	if (pStartNode == NULL)
	{
		pStartNode = GetRoot();
	}

	pStartNode->EnableL2andLHS(false);

	if ( pStartNode->GetSibling() )
	{
		PMCDisableAllNodes(pStartNode->GetSibling());
	}

	if ( pStartNode->GetChild() )
	{
		PMCDisableAllNodes(pStartNode->GetChild());
	}
}

// recursively set l2/lhs recording state for a node and all children AND SIBLINGS
static void PMCRecursiveL2Set(CVProfNode *pNode, bool enableState)
{
	if ( pNode )
	{
		pNode->EnableL2andLHS(enableState);
		if ( pNode->GetSibling() )
		{
			PMCRecursiveL2Set( pNode->GetSibling(), enableState );
		}
		if ( pNode->GetChild() )
		{
			PMCRecursiveL2Set( pNode->GetChild(), enableState );
		}
	}
}

bool CVProfile::PMCEnableL2Upon(const tchar *pszNodeName, bool bRecursive)
{
	// PMCDisableAllNodes();
	CVProfNode *pNode = FindNode( GetRoot(), pszNodeName );
	if (pNode)
	{
		pNode->EnableL2andLHS(true);
		if (bRecursive)
		{
			PMCRecursiveL2Set(pNode->GetChild(), true);
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool CVProfile::PMCDisableL2Upon(const tchar *pszNodeName, bool bRecursive)
{
	// PMCDisableAllNodes();
	CVProfNode *pNode = FindNode( GetRoot(), pszNodeName );
	if ( pNode )
	{
		pNode->EnableL2andLHS( false );
		if ( bRecursive )
		{
			PMCRecursiveL2Set( pNode->GetChild(), false );
		}
		return true;
	}
	else
	{
		return false;
	}
}

static void DumpEnabledPMCNodesInner(CVProfNode* pNode)
{
	if (!pNode)
		return;

	if (pNode->IsL2andLHSEnabled())
	{
		Msg( _T("\t%s\n"), pNode->GetName() );
	}

	// depth first printing clearer
	if ( pNode->GetChild() )
	{
		DumpEnabledPMCNodesInner(pNode->GetChild());
	}

	if ( pNode->GetSibling() )
	{
		DumpEnabledPMCNodesInner(pNode->GetChild());
	}
}

void CVProfile::DumpEnabledPMCNodes( void )
{
	Msg( _T("Nodes enabled for PMC counters:\n") );
	CVProfNode *pNode = GetRoot();
	DumpEnabledPMCNodesInner( pNode );

	Msg( _T("(end)\n") );
}

CVProfNode *CVProfile::CPUTraceGetEnabledNode(CVProfNode *pStartNode) 
{
	if (!pStartNode)
	{
		pStartNode = GetRoot();
	}

	if ( (pStartNode->m_iBitFlags & CVProfNode::kCPUTrace) != 0 )
	{
		return pStartNode;
	}

	if (pStartNode->GetSibling())
	{
		CVProfNode *retval = CPUTraceGetEnabledNode(pStartNode->GetSibling());
		if (retval)
			return retval;
	}
	
	if (pStartNode->GetChild())
	{
		CVProfNode *retval = CPUTraceGetEnabledNode(pStartNode->GetChild());
		if (retval)
			return retval;
	}

	return NULL;
}

const char *CVProfile::SetCPUTraceFilename( const char *filename )
{
	strncpy( m_CPUTraceFilename, filename, sizeof( m_CPUTraceFilename ) );
	return GetCPUTraceFilename();	
}

/// Returns a pointer to an internal static, so you don't need to 
/// make temporary char buffers for this to write into. What of it?
/// You're not hanging on to that pointer. That would be foolish. 
const char *CVProfile::GetCPUTraceFilename()
{
	static char retBuf[256];

	switch ( m_iCPUTraceEnabled )
	{
	case kAllNodesInFrame_WaitingForMark:
	case kAllNodesInFrame_Recording:
		_snprintf( retBuf, sizeof( retBuf ), "e:\\%.128s%.4d.pix2", m_CPUTraceFilename, m_iSuccessiveTraceIndex );
		break;

	case kAllNodesInFrame_WaitingForMarkMultiFrame:
	case kAllNodesInFrame_RecordingMultiFrame:
		_snprintf( retBuf, sizeof( retBuf ), "e:\\%.128s_%.4d_%.4d.pix2", m_CPUTraceFilename, m_nFrameCount, m_iSuccessiveTraceIndex );
		break;

	default:
		_snprintf( retBuf, sizeof( retBuf ), "e:\\%.128s.pix2", m_CPUTraceFilename );
	}

	return retBuf;
}

bool CVProfile::TraceCompleteEvent( void )
{
	return m_bTraceCompleteEvent;
}

CVProfNode *CVProfile::CPUTraceEnableForNode(const tchar *pszNodeName)
{
	// disable whatever may be enabled already (we can only trace one node at a time)
	CPUTraceDisableAllNodes();

	CVProfNode *which = FindNode(GetRoot(), pszNodeName);
	if (which)
	{
		which->m_iBitFlags |= CVProfNode::kCPUTrace;
		return which;
	}
	else
		return NULL;
}

void CVProfile::CPUTraceDisableAllNodes(CVProfNode *pStartNode)
{
	if (!pStartNode)
	{
		pStartNode = GetRoot();
	}

	pStartNode->m_iBitFlags &= ~CVProfNode::kCPUTrace;

	if (pStartNode->GetSibling())
	{
		CPUTraceDisableAllNodes(pStartNode->GetSibling());
	}

	if (pStartNode->GetChild())
	{
		CPUTraceDisableAllNodes(pStartNode->GetChild());
	}

}

#endif

//-------------------------------------

void CVProfile::SumTimes( const tchar *pszStartNode, int budgetGroupID )
{
	if ( GetRoot()->GetChild() )
	{
		if ( pszStartNode == NULL )
			g_pStartNode = GetRoot();
		else
			g_pStartNode = NULL;

		g_pszSumNode = pszStartNode;
		SumTimes( GetRoot(), budgetGroupID );
		g_pStartNode = NULL;
	}

}

//-------------------------------------

void CVProfile::DumpNodes( CVProfNode *pNode, int indent, bool bAverageAndCountOnly )
{
	if ( !pNode )
		return; // this generally only happens on a failed FindNode()

	bool fIsRoot = ( pNode == GetRoot() );

	if ( fIsRoot || pNode == g_pStartNode )
	{
		if( bAverageAndCountOnly )
		{
			Msg( _T(" Avg Time/Frame (ms)\n") );
			Msg( _T("[ func+child      func ]       Count\n") );
			Msg( _T("  ---------- ---------      --------\n") );
		}
		else
		{
			Msg( _T("       Sum (ms)            Avg Time/Frame (ms)     Avg Time/Call (ms)\n") );
			Msg( _T("[ func+child      func ]  [ func+child   func ]  [ func+child   func ]    Count   Peak\n") );
			Msg( _T("  ---------- ---------      ---------- ------      ---------- ------   -------- ------\n") );
		}
	}

	if ( !fIsRoot )
	{
		map<CVProfNode *, double>::iterator iterTimeLessChildren = g_TimesLessChildren.find( pNode );
		
		double dNodeTime = 0;
		if(iterTimeLessChildren != g_TimesLessChildren.end())
			dNodeTime = iterTimeLessChildren->second;

		if( bAverageAndCountOnly )
		{
			Msg( _T("  %10.3f %9.2f      %8d"), 
						 ( pNode->GetTotalCalls() > 0 ) ? pNode->GetTotalTime() / (double)NumFramesSampled() : 0, 
						 ( pNode->GetTotalCalls() > 0 ) ? dNodeTime / (double)NumFramesSampled() : 0, 
						 pNode->GetTotalCalls()  );
		}
		else
		{
			Msg( _T("  %10.3f %9.2f      %10.3f %6.2f      %10.3f %6.2f   %8d %6.2f"), 
						 pNode->GetTotalTime(), dNodeTime,
						 ( pNode->GetTotalCalls() > 0 ) ? pNode->GetTotalTime() / (double)NumFramesSampled() : 0, 
						 ( pNode->GetTotalCalls() > 0 ) ? dNodeTime / (double)NumFramesSampled() : 0, 
						 ( pNode->GetTotalCalls() > 0 ) ? pNode->GetTotalTime() / (double)pNode->GetTotalCalls() : 0, 
						 ( pNode->GetTotalCalls() > 0 ) ? dNodeTime / (double)pNode->GetTotalCalls() : 0, 
						 pNode->GetTotalCalls(), pNode->GetPeakTime()  );
		}
		
		Msg( _T("  ") );
		for ( int i = 1; i < indent; i++ )
		{
			Msg( _T("|  ") );
		}

		Msg( _T("%s\n"), pNode->GetName() );
	}

	if( pNode->GetChild() )
	{
		DumpNodes( pNode->GetChild(), indent + 1, bAverageAndCountOnly );
	}
	
	if( !( fIsRoot || pNode == g_pStartNode ) && pNode->GetSibling() )
	{
		DumpNodes( pNode->GetSibling(), indent, bAverageAndCountOnly );
	}
}

//-------------------------------------

#if defined( VPROF_VXCONSOLE_EXISTS )
static void CalcBudgetGroupTimes_Recursive( CVProfNode *pNode, unsigned int *groupTimes, int numGroups, float flScale )
{
	int			groupID;
	CVProfNode	*nodePtr;

	groupID = pNode->GetBudgetGroupID();
	if ( groupID >= numGroups )
	{
		return;
	}

	groupTimes[groupID] += flScale*pNode->GetPrevTimeLessChildren();	

	nodePtr = pNode->GetSibling();
	if ( nodePtr )
	{
		CalcBudgetGroupTimes_Recursive( nodePtr, groupTimes, numGroups, flScale );
	}

	nodePtr = pNode->GetChild();
	if ( nodePtr )
	{
		CalcBudgetGroupTimes_Recursive( nodePtr, groupTimes, numGroups, flScale );
	}
}

static void CalcBudgetGroupL2CacheMisses_Recursive( CVProfNode *pNode, unsigned int *groupTimes, int numGroups, float flScale )
{
	int			groupID;
	CVProfNode	*nodePtr;

	groupID = pNode->GetBudgetGroupID();
	if ( groupID >= numGroups )
	{
		return;
	}

	groupTimes[groupID] += flScale*pNode->GetPrevL2CacheMissLessChildren();	

	nodePtr = pNode->GetSibling();
	if ( nodePtr )
	{
		CalcBudgetGroupL2CacheMisses_Recursive( nodePtr, groupTimes, numGroups, flScale );
	}

	nodePtr = pNode->GetChild();
	if ( nodePtr )
	{
		CalcBudgetGroupL2CacheMisses_Recursive( nodePtr, groupTimes, numGroups, flScale );
	}
}

static void CalcBudgetGroupLHS_Recursive( CVProfNode *pNode, unsigned int *groupTimes, int numGroups, float flScale )
{
	int			groupID;
	CVProfNode	*nodePtr;

	groupID = pNode->GetBudgetGroupID();
	if ( groupID >= numGroups )
	{
		return;
	}

	groupTimes[groupID] += flScale*pNode->GetPrevLoadHitStoreLessChildren();	

	nodePtr = pNode->GetSibling();
	if ( nodePtr )
	{
		CalcBudgetGroupLHS_Recursive( nodePtr, groupTimes, numGroups, flScale );
	}

	nodePtr = pNode->GetChild();
	if ( nodePtr )
	{
		CalcBudgetGroupLHS_Recursive( nodePtr, groupTimes, numGroups, flScale );
	}
}


void CVProfile::VXConsoleReportMode( VXConsoleReportMode_t mode )
{
	m_ReportMode = mode;
}

void CVProfile::VXConsoleReportScale( VXConsoleReportMode_t mode, float flScale )
{
	m_pReportScale[mode] = flScale;
}


//-----------------------------------------------------------------------------
// Send the all the counter attributes once to VXConsole at profiling start
//-----------------------------------------------------------------------------
void CVProfile::VXProfileStart()
{
	const char		*names[XBX_MAX_PROFILE_COUNTERS];
	COLORREF		colors[XBX_MAX_PROFILE_COUNTERS];
	int				numGroups;
	int				counterGroup;
	const char		*pGroupName;
	int				i;
	int				r,g,b,a;

	// vprof system must be running
	if ( m_enabled <= 0 || !m_UpdateMode )
	{
		return;
	}

	if ( m_UpdateMode & VPROF_UPDATE_BUDGET )
	{
		// update budget profiling
		numGroups = g_VProfCurrentProfile.GetNumBudgetGroups();
		if ( numGroups > XBX_MAX_PROFILE_COUNTERS )
		{
			numGroups = XBX_MAX_PROFILE_COUNTERS;
		}
		for ( i=0; i<numGroups; i++ )
		{
			names[i] = g_VProfCurrentProfile.GetBudgetGroupName( i );
			g_VProfCurrentProfile.GetBudgetGroupColor( i, r, g, b, a );
			colors[i] = XMAKECOLOR( r, g, b );
		}

		// send all the profile attributes
		XBX_rSetProfileAttributes( "cpu", numGroups, names, colors );
	}

	if ( m_UpdateMode & (VPROF_UPDATE_TEXTURE_GLOBAL|VPROF_UPDATE_TEXTURE_PERFRAME) )
	{		
		// update texture profiling
		numGroups = 0;
		counterGroup = (m_UpdateMode & VPROF_UPDATE_TEXTURE_GLOBAL) ? COUNTER_GROUP_TEXTURE_GLOBAL : COUNTER_GROUP_TEXTURE_PER_FRAME;
		for ( i=0; i<g_VProfCurrentProfile.GetNumCounters(); i++ )
		{
			if ( g_VProfCurrentProfile.GetCounterGroup( i ) == counterGroup )
			{	
				// strip undesired prefix
				pGroupName = g_VProfCurrentProfile.GetCounterName( i );
				if ( !stricmp( pGroupName, "texgroup_frame_" ) )
				{
					pGroupName += 15;
				}
				else if ( !stricmp( pGroupName, "texgroup_global_" ) )
				{
					pGroupName += 16;
				}
				names[numGroups] = pGroupName;

				g_VProfCurrentProfile.GetBudgetGroupColor( numGroups, r, g, b, a );
				colors[numGroups] = XMAKECOLOR( r, g, b );

				numGroups++;
				if ( numGroups == XBX_MAX_PROFILE_COUNTERS )
				{
					break;
				}
			}
		}

		// send all the profile attributes
		XBX_rSetProfileAttributes( "texture", numGroups, names, colors );
	}
}

//-----------------------------------------------------------------------------
// Send the counters to VXConsole
//-----------------------------------------------------------------------------
void CVProfile::VXProfileUpdate()
{
	int				i;
	int				counterGroup;
	int				numGroups;
	unsigned int	groupData[XBX_MAX_PROFILE_COUNTERS];

	// vprof system must be running
	if ( m_enabled <= 0 || !m_UpdateMode )
	{
		return;
	}

	if ( m_UpdateMode & VPROF_UPDATE_BUDGET )
	{
		// send the cpu counters
		numGroups = g_VProfCurrentProfile.GetNumBudgetGroups();
		if ( numGroups > XBX_MAX_PROFILE_COUNTERS )
		{
			numGroups = XBX_MAX_PROFILE_COUNTERS;
		}
		memset( groupData, 0, numGroups * sizeof( unsigned int ) );

		CVProfNode *pNode = g_VProfCurrentProfile.GetRoot();
		if ( pNode && pNode->GetChild() )
		{
			switch ( m_ReportMode )
			{
			default:
			case VXCONSOLE_REPORT_TIME:
				CalcBudgetGroupTimes_Recursive( pNode->GetChild(), groupData, numGroups, m_pReportScale[VXCONSOLE_REPORT_TIME] );
				break;

			case VXCONSOLE_REPORT_L2CACHE_MISSES:
				CalcBudgetGroupL2CacheMisses_Recursive( pNode->GetChild(), groupData, numGroups, m_pReportScale[VXCONSOLE_REPORT_L2CACHE_MISSES] );
				break;

			case VXCONSOLE_REPORT_LOAD_HIT_STORE:
				CalcBudgetGroupLHS_Recursive( pNode->GetChild(), groupData, numGroups, m_pReportScale[VXCONSOLE_REPORT_LOAD_HIT_STORE] );
				break;
			}
		}

		XBX_rSetProfileData( "cpu", numGroups, groupData );
	}

	if ( m_UpdateMode & ( VPROF_UPDATE_TEXTURE_GLOBAL|VPROF_UPDATE_TEXTURE_PERFRAME ) )
	{
		// send the texture counters
		numGroups = 0;
		counterGroup = ( m_UpdateMode & VPROF_UPDATE_TEXTURE_GLOBAL ) ? COUNTER_GROUP_TEXTURE_GLOBAL : COUNTER_GROUP_TEXTURE_PER_FRAME;
		for ( i = 0; i < g_VProfCurrentProfile.GetNumCounters(); i++ )
		{
			if ( g_VProfCurrentProfile.GetCounterGroup( i ) == counterGroup )
			{
				// get the size in bytes
				groupData[numGroups++] = g_VProfCurrentProfile.GetCounterValue( i );
				if ( numGroups == XBX_MAX_PROFILE_COUNTERS )
				{
					break;
				}
			}
		}

		XBX_rSetProfileData( "texture", numGroups, groupData );
	}
}

void CVProfile::VXEnableUpdateMode( int event, bool bEnable )
{
	// enable or disable the updating of specified events
	if ( bEnable )
	{
		m_UpdateMode |= event;
	}
	else
	{
		m_UpdateMode &= ~event;
	}

	// force a resend of possibly affected attributes
	VXProfileStart();
}

#define MAX_VPROF_NODES_IN_LIST 4096
static void VXBuildNodeList_r( CVProfNode *pNode, xVProfNodeItem_t *pNodeList, int *pNumNodes )
{
	if ( !pNode )
	{
		return; 
	}
	if ( *pNumNodes >= MAX_VPROF_NODES_IN_LIST )
	{
		// list full
		return;
	}

	// add to list
	pNodeList[*pNumNodes].pName = (const char *)pNode->GetName();

	pNodeList[*pNumNodes].pBudgetGroupName = g_VProfCurrentProfile.GetBudgetGroupName( pNode->GetBudgetGroupID() );
	int r, g, b, a;
	g_VProfCurrentProfile.GetBudgetGroupColor( pNode->GetBudgetGroupID(), r, g, b, a );
	pNodeList[*pNumNodes].budgetGroupColor = XMAKECOLOR( r, g, b );

	pNodeList[*pNumNodes].totalCalls = pNode->GetTotalCalls();
	pNodeList[*pNumNodes].inclusiveTime = pNode->GetTotalTime();
	pNodeList[*pNumNodes].exclusiveTime = pNode->GetTotalTimeLessChildren();
	(*pNumNodes)++;

	CVProfNode	*nodePtr = pNode->GetSibling();
	if ( nodePtr )
	{
		VXBuildNodeList_r( nodePtr, pNodeList, pNumNodes );
	}

	nodePtr = pNode->GetChild();
	if ( nodePtr )
	{
		VXBuildNodeList_r( nodePtr, pNodeList, pNumNodes );
	}
}
void CVProfile::VXSendNodes( void )
{
	Pause();

	xVProfNodeItem_t *pNodeList = (xVProfNodeItem_t *)stackalloc( MAX_VPROF_NODES_IN_LIST * sizeof(xVProfNodeItem_t) );
	int numNodes = 0;
	VXBuildNodeList_r( GetRoot(), pNodeList, &numNodes );

	// send to vxconsole
	XBX_rVProfNodeList( numNodes, pNodeList );

	Resume();
}
#endif

//-------------------------------------
static void DumpSorted( const tchar *pszHeading, double totalTime, bool (*pfnSort)( const TimeSums_t &, const TimeSums_t & ), int maxLen = 999999 )
{
	unsigned i;
	vector<TimeSums_t> sortedSums;
	sortedSums = g_TimeSums;
	sort( sortedSums.begin(), sortedSums.end(), pfnSort );
	
	Msg( _T("%s\n"), pszHeading);
    Msg( _T("  Scope                                                      Calls Calls/Frame  Time+Child    Pct        Time    Pct   Avg/Frame    Avg/Call Avg-NoChild        Peak\n"));
    Msg( _T("  ---------------------------------------------------- ----------- ----------- ----------- ------ ----------- ------ ----------- ----------- ----------- -----------\n"));
    for ( i = 0; i < sortedSums.size() && i < (unsigned)maxLen; i++ )
    {
		double avg = ( sortedSums[i].calls ) ? sortedSums[i].time / (double)sortedSums[i].calls : 0.0;
		double avgLessChildren = ( sortedSums[i].calls ) ? sortedSums[i].timeLessChildren / (double)sortedSums[i].calls : 0.0;
		
        Msg( _T("  %52.52s%12d%12.3f%12.3f%7.2f%12.3f%7.2f%12.3f%12.3f%12.3f%12.3f\n"), 
             sortedSums[i].pszProfileScope,
             sortedSums[i].calls,
			 (float)sortedSums[i].calls / (float)g_TotalFrames,
			 sortedSums[i].time,
			 min( ( sortedSums[i].time / totalTime ) * 100.0, 100.0 ),
			 sortedSums[i].timeLessChildren,
			 min( ( sortedSums[i].timeLessChildren / totalTime ) * 100.0, 100.0 ),
			 sortedSums[i].time / (float)g_TotalFrames,
			 avg,
			 avgLessChildren,
			 sortedSums[i].peak );
	}
}

#if defined( _X360 )
// Dump information on all nodes with PMC recording
static void DumpPMC( CVProfNode *pNode, bool &bPrintHeader, uint64 L2thresh = 1, uint64 LHSthresh = 1 )
{
	if (!pNode) return;

	uint64 l2 = pNode->GetL2CacheMisses();
	uint64 lhs = pNode->GetLoadHitStores();
	if ( l2  > L2thresh && 
		 lhs > LHSthresh )
	{
		// met threshold.
		if (bPrintHeader)
		{
			// print header
			Msg( _T("-- 360 PMC information --\n") );
			Msg( _T("Scope                                                  L2/call  L2/frame  LHS/call LHS/frame\n") );
			Msg( _T("---------------------------------------------------- --------- --------- --------- ---------\n") );

			bPrintHeader = false;
		}

		// print
		float calls = pNode->GetTotalCalls();
		float frames = g_TotalFrames;
		Msg( _T("%52.52s %9.2f %9.2f %9.2f %9.2f\n"), pNode->GetName(), l2/calls, l2/frames, lhs/calls, lhs/frames );
	}

	if ( pNode->GetSibling() )
	{
		DumpPMC( pNode->GetSibling(), bPrintHeader, L2thresh, LHSthresh );
	}

	if ( pNode->GetChild() )
	{
		DumpPMC( pNode->GetChild(), bPrintHeader, L2thresh, LHSthresh );
	}
}
#endif

//-------------------------------------

void CVProfile::OutputReport( int type, const tchar *pszStartNode, int budgetGroupID )
{
	Msg( _T("******** BEGIN VPROF REPORT ********\n"));
#ifdef _MSC_VER
#if (_MSC_VER < 1300)
	Msg( _T("  (note: this report exceeds the output capacity of MSVC debug window. Use console window or console log.) \n"));
#endif
#endif

	g_TotalFrames = max( NumFramesSampled() - 1, 1 );
	
	if ( NumFramesSampled() == 0 || GetTotalTimeSampled() == 0)
		Msg( _T("No samples\n") );
	else
	{
		if ( type & VPRT_SUMMARY )
		{
			Msg( _T("-- Summary --\n") );
			Msg( _T("%d frames sampled for %.2f seconds\n"), g_TotalFrames, GetTotalTimeSampled() / 1000.0 );
			Msg( _T("Average %.2f fps, %.2f ms per frame\n"), 1000.0 / ( GetTotalTimeSampled() / g_TotalFrames ), GetTotalTimeSampled() / g_TotalFrames );
			Msg( _T("Peak %.2f ms frame\n"), GetPeakFrameTime() );
			
			double timeAccountedFor = 100.0 - ( m_Root.GetTotalTimeLessChildren() / m_Root.GetTotalTime() );
			Msg( _T("%.0f pct of time accounted for\n"), min( 100.0, timeAccountedFor ) );
			Msg( _T("\n") );
		}

		if ( pszStartNode == NULL )
		{
			pszStartNode = GetRoot()->GetName();
		}

		SumTimes( pszStartNode, budgetGroupID );
		
		// Dump the hierarchy
		if ( type & VPRT_HIERARCHY )
		{
			Msg( _T("-- Hierarchical Call Graph --\n"));
			if ( pszStartNode == NULL )
				g_pStartNode = NULL;
			else
				g_pStartNode = FindNode( GetRoot(), pszStartNode );

			DumpNodes( (!g_pStartNode) ? GetRoot() : g_pStartNode, 0, false );
			Msg( _T("\n") );
		}
		
		if ( type & VPRT_HIERARCHY_TIME_PER_FRAME_AND_COUNT_ONLY )
		{
			Msg( _T("-- Hierarchical Call Graph --\n"));
			if ( pszStartNode == NULL )
				g_pStartNode = NULL;
			else
				g_pStartNode = FindNode( GetRoot(), pszStartNode );

			DumpNodes( (!g_pStartNode) ? GetRoot() : g_pStartNode, 0, true );
			Msg( _T("\n") );
		}

		int maxLen = ( type & VPRT_LIST_TOP_ITEMS_ONLY ) ? 25 : 999999;

		if ( type & VPRT_LIST_BY_TIME )
		{
			DumpSorted( _T("-- Profile scopes sorted by time (including children) --"), GetTotalTimeSampled(), TimeCompare, maxLen );
			Msg( _T("\n") );
		}
		if ( type & VPRT_LIST_BY_TIME_LESS_CHILDREN )
		{
			DumpSorted( _T("-- Profile scopes sorted by time (without children) --"), GetTotalTimeSampled(), TimeLessChildrenCompare, maxLen );
			Msg( _T("\n") );
		}
		if ( type & VPRT_LIST_BY_AVG_TIME )
		{
			DumpSorted( _T("-- Profile scopes sorted by average time (including children) --"), GetTotalTimeSampled(), AverageTimeCompare, maxLen );
			Msg( _T("\n") );
		}
		if ( type & VPRT_LIST_BY_AVG_TIME_LESS_CHILDREN )
		{
			DumpSorted( _T("-- Profile scopes sorted by average time (without children) --"), GetTotalTimeSampled(), AverageTimeLessChildrenCompare, maxLen );
			Msg( _T("\n") );
		}
		if ( type & VPRT_LIST_BY_PEAK_TIME )
		{
			DumpSorted( _T("-- Profile scopes sorted by peak --"), GetTotalTimeSampled(), PeakCompare, maxLen);
			Msg( _T("\n") );
		}
		if ( type & VPRT_LIST_BY_PEAK_OVER_AVERAGE )
		{
			DumpSorted( _T("-- Profile scopes sorted by peak over average (including children) --"), GetTotalTimeSampled(), PeakOverAverageCompare, maxLen );
			Msg( _T("\n") );
		}
		
		// TODO: Functions by time less children
		// TODO: Functions by time averages
		// TODO: Functions by peak
		// TODO: Peak deviation from average
		g_TimesLessChildren.clear();
		g_TimeSumsMap.clear();
		g_TimeSums.clear();

#ifdef _X360
		bool bPrintedHeader = true;
		DumpPMC( FindNode( GetRoot(), pszStartNode ), bPrintedHeader );
#endif

	}
	Msg( _T("******** END VPROF REPORT ********\n"));

}

//=============================================================================

CVProfile::CVProfile() 
 :	m_Root( _T("Root"), 0, NULL, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, 0 ),
	m_pCurNode( &m_Root ), 
 	m_nFrames( 0 ),
 	m_enabled( 0 ),  // don't change this. if m_enabled is anything but zero coming out of this constructor, vprof will break.
 	m_pausedEnabledDepth( 0 ),
	m_fAtRoot( true )
{

#ifdef VPROF_VTUNE_GROUP
	m_GroupIDStackDepth = 1;
	m_GroupIDStack[0] = 0; // VPROF_BUDGETGROUP_OTHER_UNACCOUNTED
#endif

	m_TargetThreadId = ThreadGetCurrentId();
	
	// Go ahead and allocate 32 slots for budget group names
	MEM_ALLOC_CREDIT();
	m_pBudgetGroups = new CVProfile::CBudgetGroup[32];
	m_nBudgetGroupNames = 0;
	m_nBudgetGroupNamesAllocated = 32;

	// Add these here so that they will always be in the same order.
	// VPROF_BUDGETGROUP_OTHER_UNACCOUNTED has to be FIRST!!!!
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_UNACCOUNTED,		BUDGETFLAG_OTHER | BUDGETFLAG_SERVER );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_WORLD_RENDERING,			BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_DISPLACEMENT_RENDERING,	BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_GAME,						BUDGETFLAG_OTHER | BUDGETFLAG_SERVER );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_PLAYER,					BUDGETFLAG_OTHER | BUDGETFLAG_SERVER );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_NPCS,						BUDGETFLAG_OTHER | BUDGETFLAG_SERVER );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_SERVER_ANIM,				BUDGETFLAG_OTHER | BUDGETFLAG_SERVER );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_CLIENT_ANIMATION,			BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_PHYSICS,					BUDGETFLAG_OTHER | BUDGETFLAG_SERVER );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_STATICPROP_RENDERING,		BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_MODEL_RENDERING,			BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_MODEL_FAST_PATH_RENDERING,BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_LIGHTCACHE,				BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_BRUSHMODEL_RENDERING,		BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_SHADOW_RENDERING,			BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_DETAILPROP_RENDERING,		BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_PARTICLE_RENDERING,		BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_ROPES,					BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_DLIGHT_RENDERING,			BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_NETWORKING,			BUDGETFLAG_OTHER | BUDGETFLAG_SERVER );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_SOUND,				BUDGETFLAG_OTHER | BUDGETFLAG_SERVER );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_VGUI,				BUDGETFLAG_OTHER | BUDGETFLAG_SERVER );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OTHER_FILESYSTEM,			BUDGETFLAG_OTHER | BUDGETFLAG_SERVER );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_PREDICTION,				BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_INTERPOLATION,			BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_SWAP_BUFFERS,				BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OCCLUSION,				BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_OVERLAYS,					BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_TOOLS,					BUDGETFLAG_OTHER | BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_TEXTURE_CACHE,			BUDGETFLAG_CLIENT );
	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_REPLAY,					BUDGETFLAG_SERVER );
//	BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_DISP_HULLTRACES );

	m_bPMEInit = false;
	m_bPMEEnabled = false;

#ifdef VPROF_VXCONSOLE_EXISTS
	m_bTraceCompleteEvent = false;
	m_iSuccessiveTraceIndex = 0;
	m_ReportMode = VXCONSOLE_REPORT_TIME;
	m_pReportScale[VXCONSOLE_REPORT_TIME] = 1000.0f;
	m_pReportScale[VXCONSOLE_REPORT_L2CACHE_MISSES] = 1.0f;
	m_pReportScale[VXCONSOLE_REPORT_LOAD_HIT_STORE] = 0.1f;
	m_nFrameCount = 0;
	m_nFramesRemaining = 1;
	m_WorstCycles = 0;
	m_WorstTraceFilename[ 0 ] = 0;
	m_UpdateMode = 0;
#endif
#ifdef _X360
	m_iCPUTraceEnabled = kDisabled;
#endif
}


CVProfile::~CVProfile()
{
	Term();
}


void CVProfile::FreeNodes_R( CVProfNode *pNode )
{
	CVProfNode *pNext;
	for ( CVProfNode *pChild = pNode->GetChild(); pChild; pChild = pNext )
	{
		pNext = pChild->GetSibling();
		FreeNodes_R( pChild );
	}
	
	if ( pNode == GetRoot() )
	{
		pNode->m_pChild = NULL;
	}
	else
	{
		delete pNode;
	}
}


void CVProfile::Term()
{
	int i;
	for( i = 0; i < m_nBudgetGroupNames; i++ )
	{
		delete [] m_pBudgetGroups[i].m_pName;
	}
	delete m_pBudgetGroups;
	m_nBudgetGroupNames = m_nBudgetGroupNamesAllocated = 0;
	m_pBudgetGroups = NULL;

	int n;
	for( n = 0; n < m_NumCounters; n++ )
	{
		delete [] m_CounterNames[n];
		m_CounterNames[n] = NULL;
	}
	m_NumCounters = 0;

	// Free the nodes.
	if ( GetRoot() )
	{
		FreeNodes_R( GetRoot() );
	}
}


#define COLORMIN 160
#define COLORMAX 255

static int g_ColorLookup[4] = 
{
	COLORMIN, 
	COLORMAX,
	COLORMIN+(COLORMAX-COLORMIN)/3,
	COLORMIN+((COLORMAX-COLORMIN)*2)/3, 
};

#define GET_BIT( val, bitnum ) ( ( val >> bitnum ) & 0x1 )

void CVProfile::GetBudgetGroupColor( int budgetGroupID, int &r, int &g, int &b, int &a )
{
	budgetGroupID = budgetGroupID % ( 1 << 6 );

	int index;
	index = GET_BIT( budgetGroupID, 0 ) | ( GET_BIT( budgetGroupID, 5 ) << 1 );
	r = g_ColorLookup[index];
	index = GET_BIT( budgetGroupID, 1 ) | ( GET_BIT( budgetGroupID, 4 ) << 1 );
	g = g_ColorLookup[index];
	index = GET_BIT( budgetGroupID, 2 ) | ( GET_BIT( budgetGroupID, 3 ) << 1 );
	b = g_ColorLookup[index];
	a = 255;
}

// return -1 if it doesn't exist.
int CVProfile::FindBudgetGroupName( const tchar *pBudgetGroupName )
{
	int i;
	for( i = 0; i < m_nBudgetGroupNames; i++ )
	{
		if( _tcsicmp( pBudgetGroupName, m_pBudgetGroups[i].m_pName ) == 0 )
		{
			return i;
		}
	}
	return -1;
}

int CVProfile::AddBudgetGroupName( const tchar *pBudgetGroupName, int budgetFlags )
{
	MEM_ALLOC_CREDIT();
	tchar *pNewString = new tchar[ _tcslen( pBudgetGroupName ) + 1 ];
	_tcscpy( pNewString, pBudgetGroupName );
	if( m_nBudgetGroupNames + 1 > m_nBudgetGroupNamesAllocated )
	{
		m_nBudgetGroupNamesAllocated *= 2;
		m_nBudgetGroupNamesAllocated = max( m_nBudgetGroupNames + 6, m_nBudgetGroupNamesAllocated );
		
		CBudgetGroup *pNew = new CBudgetGroup[ m_nBudgetGroupNamesAllocated ];
		for ( int i=0; i < m_nBudgetGroupNames; i++ )
			pNew[i] = m_pBudgetGroups[i];
		
		delete [] m_pBudgetGroups;
		m_pBudgetGroups = pNew;
	}

	m_pBudgetGroups[m_nBudgetGroupNames].m_pName = pNewString;
	m_pBudgetGroups[m_nBudgetGroupNames].m_BudgetFlags = budgetFlags;
	m_nBudgetGroupNames++;
	if( m_pNumBudgetGroupsChangedCallBack )
	{
		(*m_pNumBudgetGroupsChangedCallBack)();
	}

#if defined( VPROF_VXCONSOLE_EXISTS )
	// re-start with all the known budgets
	VXProfileStart();
#endif
	return m_nBudgetGroupNames - 1;
}

int CVProfile::BudgetGroupNameToBudgetGroupID( const tchar *pBudgetGroupName, int budgetFlagsToORIn )
{
	int budgetGroupID = FindBudgetGroupName( pBudgetGroupName );
	if( budgetGroupID == -1 )
	{
		budgetGroupID = AddBudgetGroupName( pBudgetGroupName, budgetFlagsToORIn );
	}
	else
	{
		m_pBudgetGroups[budgetGroupID].m_BudgetFlags |= budgetFlagsToORIn;
	}

	return budgetGroupID;
}

int CVProfile::BudgetGroupNameToBudgetGroupID( const tchar *pBudgetGroupName )
{
	return BudgetGroupNameToBudgetGroupID( pBudgetGroupName, BUDGETFLAG_OTHER );
}

int CVProfile::GetNumBudgetGroups( void )
{
	return m_nBudgetGroupNames;
}

void CVProfile::RegisterNumBudgetGroupsChangedCallBack( void (*pCallBack)(void) )
{
	m_pNumBudgetGroupsChangedCallBack = pCallBack;
}

void CVProfile::HideBudgetGroup( int budgetGroupID, bool bHide )
{
	if( budgetGroupID != -1 )
	{
		if ( bHide )
			m_pBudgetGroups[budgetGroupID].m_BudgetFlags |= BUDGETFLAG_HIDDEN;
		else
			m_pBudgetGroups[budgetGroupID].m_BudgetFlags &= ~BUDGETFLAG_HIDDEN;
	}
}

int *CVProfile::FindOrCreateCounter( const tchar *pName, CounterGroup_t eCounterGroup )
{	
	Assert( m_NumCounters+1 < MAXCOUNTERS );
	if ( m_NumCounters + 1 >= MAXCOUNTERS || !InTargetThread() )
	{
		static int dummy;
		return &dummy;
	}
	int i;
	for( i = 0; i < m_NumCounters; i++ )
	{
		if( _tcsicmp( m_CounterNames[i], pName ) == 0 )
		{
			// found it!
			return &m_Counters[i];
		}
	}

	// NOTE: These get freed in ~CVProfile.
	MEM_ALLOC_CREDIT();
	tchar *pNewName = new tchar[_tcslen( pName ) + 1];
	_tcscpy( pNewName, pName );
	m_Counters[m_NumCounters] = 0;
	m_CounterGroups[m_NumCounters] = (char)eCounterGroup;
	m_CounterNames[m_NumCounters++] = pNewName;
	return &m_Counters[m_NumCounters-1];
}

void CVProfile::ResetCounters( CounterGroup_t eCounterGroup )
{
	int i;
	for( i = 0; i < m_NumCounters; i++ )
	{
		if ( m_CounterGroups[i] == eCounterGroup )
			m_Counters[i] = 0;
	}
}

int CVProfile::GetNumCounters() const
{
	return m_NumCounters;
}

const tchar *CVProfile::GetCounterName( int index ) const
{
	Assert( index >= 0 && index < m_NumCounters );
	return m_CounterNames[index];
}

int CVProfile::GetCounterValue( int index ) const
{
	Assert( index >= 0 && index < m_NumCounters );
	return m_Counters[index];
}

const tchar *CVProfile::GetCounterNameAndValue( int index, int &val ) const
{
	Assert( index >= 0 && index < m_NumCounters );
	val = m_Counters[index];
	return m_CounterNames[index];
}

CounterGroup_t CVProfile::GetCounterGroup( int index ) const
{
	Assert( index >= 0 && index < m_NumCounters );
	return (CounterGroup_t)m_CounterGroups[index];
}

#ifdef _X360
void CVProfile::LatchMultiFrame( int64 cycles )
{
	if ( cycles > m_WorstCycles )
	{
		strncpy( m_WorstTraceFilename, GetCPUTraceFilename(), sizeof( m_WorstTraceFilename ) );
		m_WorstCycles = cycles;
	}
}

void CVProfile::SpewWorstMultiFrame()
{
	CCycleCount cc( m_WorstCycles );
	Msg( "%s == %.3f msec\n", m_WorstTraceFilename, cc.GetMillisecondsF() );
}
#endif

#ifdef DBGFLAG_VALIDATE

#ifdef _WIN64
#error the below is presumably broken on 64 bit
#endif // _WIN64

const int k_cSTLMapAllocOffset = 4;
#define GET_INTERNAL_MAP_ALLOC_PTR( pMap ) \
	( * ( (void **) ( ( ( byte * ) ( pMap ) ) + k_cSTLMapAllocOffset ) ) )
//-----------------------------------------------------------------------------
// Purpose: Ensure that all of our internal structures are consistent, and
//			account for all memory that we've allocated.
// Input:	validator -		Our global validator object
//			pchName -		Our name (typically a member var in our container)
//-----------------------------------------------------------------------------
void CVProfile::Validate( CValidator &validator, tchar *pchName )
{
	validator.Push( _T("CVProfile"), this, pchName );

	m_Root.Validate( validator, _T("m_Root") );

	for ( int iBudgetGroup=0; iBudgetGroup < m_nBudgetGroupNames; iBudgetGroup++ )
		validator.ClaimMemory( m_pBudgetGroups[iBudgetGroup].m_pName );

	validator.ClaimMemory( m_pBudgetGroups );

	// The std template map class allocates memory internally, but offer no way to get
	// access to their pointer.  Since this is for debug purposes only and the
	// std template classes don't change, just look at the well-known offset.  This
	// is arguably sick and wrong, kind of like marrying a squirrel.
	validator.ClaimMemory( GET_INTERNAL_MAP_ALLOC_PTR( &g_TimesLessChildren ) );
	validator.ClaimMemory( GET_INTERNAL_MAP_ALLOC_PTR( &g_TimeSumsMap ) );

	validator.Pop( );
}
#endif // DBGFLAG_VALIDATE

#endif // VPROF_ENABLED


#ifdef RAD_TELEMETRY_ENABLED

#ifdef POSIX
extern "C" char *
__realpath_chk (const char *buf, char *resolved, size_t resolvedlen)
{
    return realpath (buf, resolved);
}
#endif

TelemetryData g_Telemetry;
static HTELEMETRY g_tmContext;
static TmU8 *g_pTmMemoryArena = NULL;
static bool g_TelemetryLoaded = false;

static unsigned int g_TelemetryFrameCount = 0;
static bool g_fTelemetryLevelChanged = false;

struct ThreadNameInfo_t
{
	TSLNodeBase_t base;
	ThreadId_t ThreadID;
	char szName[ 64 ];
};
static CTSSimpleList< ThreadNameInfo_t > g_ThreadNamesList;

static bool g_bThreadNameArrayChanged = false;
static int g_ThreadNameArrayCount = 0;
static ThreadNameInfo_t g_ThreadNameArray[32];

void TelemetryThreadSetDebugName( ThreadId_t id, const char *pszName )
{
	ThreadNameInfo_t *pThreadNameInfo = new ThreadNameInfo_t;

	if( id == ( uint32 )-1 )
	{
		id = ThreadGetCurrentId();
	}

	pThreadNameInfo->ThreadID = id;
	strncpy( pThreadNameInfo->szName, pszName, ARRAYSIZE( pThreadNameInfo->szName ) );
	pThreadNameInfo->szName[ ARRAYSIZE( pThreadNameInfo->szName ) - 1 ] = 0;
	g_ThreadNamesList.Push( pThreadNameInfo );

	g_bThreadNameArrayChanged = true;
}

const int TELEMETRY_ARENA_SIZE = 32 * 1024 * 1024; // How much memory we want Telemetry to use.

static bool TelemetryInitialize()
{
	if( g_tmContext )
	{
		TmConnectionStatus status = TM_GET_CONNECTION_STATUS( g_tmContext );

		if( status == TMCS_CONNECTED || status == TMCS_CONNECTING )
			return true;
	}

	TmErrorCode retVal;

	if( !g_TelemetryLoaded )
	{
		// Pass in 0 if you want to use the release mode DLL or 1 if you want to
		// use the checked DLL.  The checked DLL is compiled with optimizations but
		// does extra run time checks and reporting.
		int nLoadTelemetry = TM_LOAD_TELEMETRY( 0 );

		retVal = TM_STARTUP();
		if ( retVal != TM_OK )
		{
			Warning( "TelemetryInit() failed: tmStartup() returned %d, tmLoadTelemetry() returned %d.\n", retVal, nLoadTelemetry );
			return false;
		}

		if( !g_pTmMemoryArena )
		{
			g_pTmMemoryArena = new TmU8[ TELEMETRY_ARENA_SIZE ];
		}

		retVal = TM_INITIALIZE_CONTEXT( &g_tmContext, g_pTmMemoryArena, TELEMETRY_ARENA_SIZE );
		if ( retVal != TM_OK )
		{
			delete [] g_pTmMemoryArena;
			g_pTmMemoryArena = NULL;

			Warning( "TelemetryInit() failed: tmInitializeContext() returned %d.\n", retVal );
			return false;
		}

		g_TelemetryLoaded = true;
	}

	char *pGameName = "csgo";

#if defined( IS_WINDOWS_PC )
	char baseExeFilename[512];
	if( GetModuleFileName ( GetModuleHandle( NULL ), baseExeFilename, sizeof( baseExeFilename ) ) )
	{
		char *pExt = strrchr( baseExeFilename, '.' );

		if( pExt )
			*pExt = 0;

		char *pSeparator = strrchr( baseExeFilename, '\\' );

		pGameName = pSeparator ? ( pSeparator + 1 ) : baseExeFilename;
	}

	// If you've got \\perforce\symbols on your _NT_SYMBOL_PATH, tmOpen() can take a massively long
	//	time in the symInitialize() routine. Since we don't really need that, kill it here.
	putenv( "_NT_SYMBOL_PATH=" );
#endif

	const char *pServerAddress = g_Telemetry.ServerAddress[0] ? g_Telemetry.ServerAddress : "localhost";
	TmConnectionType tmType = TMCT_TCP; // !V_tier0_stricmp( pServerAddress, "FILE" ) ? TMCT_FILE : TMCT_TCP;

	Msg( "TELEMETRY: Calling tmOpen( %s )...\n", pServerAddress );

	char szBuildInfo[ 2048 ];
	_snprintf( szBuildInfo, ARRAYSIZE( szBuildInfo ), "%s: %s", __DATE__ __TIME__, Plat_GetCommandLine() );
	szBuildInfo[ ARRAYSIZE( szBuildInfo ) - 1 ] = 0;


	TmU32 TmOpenFlags = TMOF_DEFAULT | TMOF_MINIMAL_CONTEXT_SWITCHES;
	/* TmOpenFlags |= TMOF_DISABLE_CONTEXT_SWITCHES | TMOF_INIT_NETWORKING*/

	retVal = TM_OPEN( g_tmContext, pGameName, szBuildInfo, pServerAddress, tmType,
		TELEMETRY_DEFAULT_PORT, TmOpenFlags, 1000 );
	if ( retVal != TM_OK )
	{
		Warning( "TelemetryInitialize() failed: tmOpen returned %d.\n", retVal );
		return false;
	}

	Msg( "Telemetry initialized at level %u.\n", g_Telemetry.Level );
#ifdef LINUX
	printf( "Telemetry initialized at level %u.\n", g_Telemetry.Level );
#endif

	if( g_bThreadNameArrayChanged )
	{
		// Go through and add any new thread names we got in our thread safe list to our thread names array.
		for( ThreadNameInfo_t *pThreadNameInfo = g_ThreadNamesList.Pop();
			pThreadNameInfo;
			pThreadNameInfo = g_ThreadNamesList.Pop() )
		{
			if( g_ThreadNameArrayCount < ARRAYSIZE( g_ThreadNameArray ) )
			{
				g_ThreadNameArray[ g_ThreadNameArrayCount ] = *pThreadNameInfo;
				g_ThreadNameArrayCount++;
			}

			delete pThreadNameInfo;
		}

		for( int i = 0; i < g_ThreadNameArrayCount; i++ )
		{
			tmThreadName( g_tmContext, g_ThreadNameArray[i].ThreadID, g_ThreadNameArray[i].szName );
		}

		g_bThreadNameArrayChanged = false;
	}

	// Default Zone Filter value to .5ms if they haven't set it already.
	if( !g_Telemetry.ZoneFilterVal )
		g_Telemetry.ZoneFilterVal = 500;

	// Init plot data
	for( int i = 0; i < TELEMETRY_ZONE_PLOT_SLOT_MAX; ++i )
	{
		g_Telemetry.m_ZonePlot[i].m_Name			= NULL;
		g_Telemetry.m_ZonePlot[i].m_CurrFrameTime	= 0;
	}

	return true;
}

#if 0

//an instance of TM_API_STRUCT that points to stubbed out functions
class CTM_API_STRUCT_Stub : public TM_API_STRUCT
{
public:
	inline CTM_API_STRUCT_Stub( void ) {}; //this empty default constructor works around warning C4701 "potentially uninitialized local variable * used" in code below. We initialize it and use it in 2 different scopes that use the same boolean
	const CTM_API_STRUCT_Stub &operator=( const TM_API_STRUCT &Existing );
	void LinkToStubs( void ); //replaces all known function pointers with stubbed versions
};
#endif

static void TelemetryShutdown( bool InDtor = false )
{
	if( g_tmContext )
	{
		// Msg can't be called here as tier0 may have already been shut down...
		if( !InDtor )
		{
			Msg( "Shutting down telemetry.\n" );
		}

		TmConnectionStatus status = TM_GET_CONNECTION_STATUS( g_tmContext );
		if( status == TMCS_CONNECTED || status == TMCS_CONNECTING )
			TM_CLOSE( g_tmContext );

#if 0
		CTM_API_STRUCT_Stub stubbedApiStruct;

		//Tm__Zone usage saves off a copy of g_tmContext and will attempt to use that pointer when leaving scope.
		//This include threads that we're not especially great at shutting down yet.
		//If we happen to own the memory that the context points at, we'll just stub out the pointers instead of completely deleting the memory
		const bool bUseLazyShutdownStubs = IsPlatformWindowsPC() && 			
			( ( g_tmContext >= ( HTELEMETRY ) g_pTmMemoryArena ) && ( g_tmContext < ( HTELEMETRY )( g_pTmMemoryArena + TELEMETRY_ARENA_SIZE ) ) ); //we completely own the memory that the context points at

		if ( bUseLazyShutdownStubs )
		{
			if( !InDtor )
			{
				Msg( "Using lazy telemetry shutdown stub functions\n\tArena: %p, Context: %p\n", g_pTmMemoryArena, g_tmContext ); //previous testing has shown that the context is at the start of the arena
			}
			stubbedApiStruct = *(TM_API_STRUCT *)g_tmContext; // a bit of future proofing to ensure that we'll use whatever was in the old struct if we fail to initialize a particular stub
			stubbedApiStruct.LinkToStubs();
		}
#endif

		//discontinue new usage of the context before shutting it down (multithreading)
		memset( g_Telemetry.tmContext, 0, sizeof( g_Telemetry.tmContext ) );
		HTELEMETRY hShutdown = g_tmContext;
		g_tmContext = NULL;

		TM_SHUTDOWN_CONTEXT( hShutdown ); 
#if 0
		if ( bUseLazyShutdownStubs )
		{
			//there's a window where this context will be in an unknown state
			memcpy( hShutdown, &stubbedApiStruct, sizeof( TM_API_STRUCT ) );
		}
		else
#endif
            if ( !IsPlatformWindowsPC() ) //actual test should be "do we probably have outstanding threads"
		{
			delete [] g_pTmMemoryArena;
			g_pTmMemoryArena = NULL;
		}
		TM_SHUTDOWN();
		g_TelemetryLoaded = false;
	}
}

// Helper class to initialize Telemetry.
class CTelemetryRegister
{
public:
	CTelemetryRegister() 	{}
	~CTelemetryRegister() 	{ TelemetryShutdown( true ); }
} g_TelemetryRegister;

PLATFORM_INTERFACE void TelemetrySetLevel( unsigned int Level )
{
	DevMsg( "TelemetrySetLevel changed from 0x%x to 0x%x (ZoneFilterVal:%d)\n", g_Telemetry.Level, Level, g_Telemetry.ZoneFilterVal );

	if( Level != g_Telemetry.Level )
	{
		g_Telemetry.Level = Level;
		g_TelemetryFrameCount = g_Telemetry.FrameCount;
		g_fTelemetryLevelChanged = true;
	}
}

#if defined( IS_WINDOWS_PC )

#include <psapi.h>

typedef BOOL ( WINAPI *GetProcessMemoryInfo_t )( HANDLE Process, PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb );
static CDynamicFunction< GetProcessMemoryInfo_t > DynGetProcessMemoryInfo( "psapi.dll", "GetProcessMemoryInfo" );

#endif

static void TelemetryPlots()
{
	if( g_Telemetry.playbacktick )
	{
		TM_PLOT_U32( TELEMETRY_LEVEL1, TMPT_INTEGER, 0, g_Telemetry.playbacktick, "game/PlaybackTick" );
		g_Telemetry.playbacktick = 0;
	}

	if( g_Telemetry.dotatime )
	{
		TM_PLOT_F32( TELEMETRY_LEVEL1, TMPT_NONE, 0, g_Telemetry.dotatime, "game/DotaTime" );
		g_Telemetry.dotatime = 0.0f;
	}

	for( int i = 0; i < g_VProfCurrentProfile.GetNumCounters(); i++ )
	{
		if( g_VProfCurrentProfile.GetCounterGroup( i ) == COUNTER_GROUP_TELEMETRY )
		{
			int val;
			const char *name = g_VProfCurrentProfile.GetCounterNameAndValue( i, val );

			TM_PLOT_I32( TELEMETRY_LEVEL1, TMPT_INTEGER, 0, val, name );
		}
	}

	g_VProfCurrentProfile.ResetCounters( COUNTER_GROUP_TELEMETRY );

	// Send plot value collected using TM_ZONE_PLOT macro
	// Data sent as a percentage of a 16ms frame (so that Telemetry display the data nicely in the Timeline view)
	for( int i = 0; i < TELEMETRY_ZONE_PLOT_SLOT_MAX; ++i )
	{
		TelemetryZonePlotData* pData = &g_Telemetry.m_ZonePlot[i];
		if (pData->m_Name)
		{
			TM_PLOT_F32(
				TELEMETRY_LEVEL1,
				TMPT_TIME_MS, 
				TMPF_NONE, 
				(pData->m_CurrFrameTime * g_Telemetry.flRDTSCToMilliSeconds),
				"(frametimes)%s(ms)", pData->m_Name );
		}

		pData->m_Name			= NULL;
		pData->m_CurrFrameTime	= 0;
	}

	TM_PLOT_F32(
		TELEMETRY_LEVEL1,
		TMPT_TIME_MS, 
		TMPF_NONE,
		16.666f,
		"(frametimes)%s(ms)", "Ref 16ms" );
}

PLATFORM_INTERFACE void TelemetryTick()
{
	if( !g_Telemetry.Level && g_Telemetry.DemoTickStart && ( (uint32)g_Telemetry.playbacktick > g_Telemetry.DemoTickStart ) )
	{
		TelemetrySetLevel( 2 );
		g_Telemetry.DemoTickStart = 0;
	}
	if( g_Telemetry.Level && g_Telemetry.DemoTickEnd && ( (uint32)g_Telemetry.playbacktick > g_Telemetry.DemoTickEnd ) )
	{
		TelemetrySetLevel( 0 );
		g_Telemetry.DemoTickEnd = ( uint32 )-1;
	}

	if ( ( g_tmContext ) && ( g_Telemetry.Level > 0 ) )
	{
		TelemetryPlots();
	}

	static double s_d0 = Plat_FloatTime();
	static TmU64 s_t0 = TM_FAST_TIME();

	// People can NIL out contexts in the TelemetryData structure to control
	//	the level and what sections to log. We always need to do ticks though,
	//	so use the master context for this.
	if( g_tmContext )
	{
		TM_TICK( g_tmContext );
	}

	if( g_tmContext )
	{
		static uint32 s_ZoneFilterValLast = 0;

		if( s_ZoneFilterValLast != g_Telemetry.ZoneFilterVal )
		{
			g_fTelemetryLevelChanged = true;
			s_ZoneFilterValLast = g_Telemetry.ZoneFilterVal;
		}

		TmU64 s_t1 = TM_FAST_TIME();
		double s_d1 = Plat_FloatTime();

		g_Telemetry.flRDTSCToMilliSeconds = 1000.0f / ( ( s_t1 - s_t0 ) / ( s_d1 - s_d0 ) );
		// Msg( "g_Telemetry.flRDTSCToMilliSeconds: %f time:%f\n", g_Telemetry.flRDTSCToMilliSeconds, ( s_t1 - s_t0 ) * g_Telemetry.flRDTSCToMilliSeconds );

		s_d0 = s_d1;
		s_t0 = s_t1;

		if( g_TelemetryFrameCount && !TM_IS_PAUSED( g_tmContext ) )
		{
			g_TelemetryFrameCount--;
			if( !g_TelemetryFrameCount )
			{
				TelemetrySetLevel( 0 );
			}
		}
	}

	if( g_fTelemetryLevelChanged )
	{
		g_fTelemetryLevelChanged = false;
		memset( g_Telemetry.tmContext, 0, sizeof( g_Telemetry.tmContext ) );

		// Mask of all zeros just enables level 0.
		if( g_Telemetry.Level == 0x80000000 )
			g_Telemetry.Level = 1;

		unsigned int Level = g_Telemetry.Level;
		bool IsMask = ( Level & 0x80000000 ) ? true : false;

		Level &= ~0x80000000;

		if( Level == 0 )
		{
			// Calling shutdown here invalidates all the telemetry context handles, and background
			//  threads in the middle of Tm__Zone'd calls will crash. So pause things for now.
			TelemetryShutdown();
		}
		else
		{
			if( !TelemetryInitialize() )
			{
				g_Telemetry.Level = 0;
			}
			else
			{
				TM_PAUSE( g_tmContext, 0 );

				if( IsMask )
				{
					for( unsigned int i = 0; i < ARRAYSIZE( g_Telemetry.tmContext ); i++)
					{
						if( (1 << i) & Level)
						{
							g_Telemetry.tmContext[ i ] = g_tmContext;
						}
					}
				}
				else
				{
					Level = MIN( Level, ARRAYSIZE( g_Telemetry.tmContext ) );
					for( unsigned int i = 0; i < Level; i++ )
					{
						g_Telemetry.tmContext[i] = g_tmContext;
					}
				}
			}
		}

		//  	TM_SET_TIMELINE_SECTION_NAME( g_tmContext, "Level:0x%x", g_Telemetry.Level );

		// To disable various telemetry features, use the tmEnable() function as so:
		//	TM_ENABLE( g_tmContext, TMO_SUPPORT_PLOT, 0 );
	}
}

#if 0

const CTM_API_STRUCT_Stub &CTM_API_STRUCT_Stub::operator=( const TM_API_STRUCT &Existing )
{
	memcpy( this, &Existing, sizeof( TM_API_STRUCT ) );
	return *this;
}

#undef TM_API

typedef char const * tmStubCSTR;
#define tmStubRETURN_TmErrorCode			return TM_OK;
#define tmStubRETURN_TmU32					return 0;
#define tmStubRETURN_void					return;
#define tmStubRETURN_TmConnectionStatus		return TMCS_DISCONNECTED;
#define tmStubRETURN_TmU64					return 0;
#define tmStubRETURN_int					return 0;
#define tmStubRETURN_char					return 0;
#define tmStubRETURN_TmI32					return 0;
#define tmStubRETURN_tmStubCSTR				return "";
#define TM_API( ret, name, params ) inline ret RADEXPLINK name##Stub params { tmStubRETURN_##ret }

#if defined TM_PPU
#define TM_NUM_SPUS 6
TM_API( TmErrorCode, tmPPUCoreGetListener, ( HTELEMETRY cx, int const kNdx, TmU32 *pListener ) );
TM_API( TmErrorCode, tmPPUCoreRegisterSPUProgram, ( HTELEMETRY cx, TmU64 const kGuid, void const *imagebase, unsigned int const kImageSize, int const kRdOnlyOffset ) );
#endif

#if defined TM_IPC_HOST && defined __RADWIN__
TM_API( TmErrorCode, tmWin32CoreListenSHAREDMEM, ( HTELEMETRY cx, char const *name ) );
#endif

#if defined TM_SPU
TM_API( TmErrorCode, tmSPUCoreBindContextToListener, ( HTELEMETRY *pcx, void * mem, TmU32 kPPUListener, char const *imagename, ...) );
TM_API( TmErrorCode, tmSPUCoreUpdateTime, ( HTELEMETRY cx ) );
TM_API( TmErrorCode, tmSPUCoreFlushImage, ( HTELEMETRY cx ) );
#endif

TM_API( TmU32, tmCoreGetVersion, ( void ) );
TM_API( TmErrorCode, tmCoreCheckVersion, ( HTELEMETRY cx, TmU32 const major, TmU32 const minor, TmU32 const build, TmU32 const cust ) );
TM_API( TmErrorCode, tmCoreGetPlatformInformation, ( void* obj, TmPlatformInformation const kInfo, void* dst, TmU32 const kDstSize ) );
TM_API( TmErrorCode, tmCoreGetLastError, ( HTELEMETRY cx ) );
#ifndef TM_SPU
TM_API( TmErrorCode, tmCoreStartup, ( void ) );
TM_API( TmErrorCode, tmCoreInitializeContext, ( EXPOUT HTELEMETRY * pcx, void * pArena, TmU32 const kArenaSize ) );
TM_API( void, tmCoreShutdownContext, ( HTELEMETRY cx ) );
TM_API( void, tmCoreShutdown, ( void ) ); 
#endif
TM_API( TmErrorCode, tmCoreGetSessionName, ( HTELEMETRY cx, char *dst, int const kDstSize ) );

TM_API( TmConnectionStatus, tmCoreGetConnectionStatus, ( HTELEMETRY cx ) );
#ifndef TM_SPU
TM_API( TmErrorCode, tmCoreOpen, ( HTELEMETRY cx, char const * kpAppName,  
	char const * kpBuildInfo,
	char const * kpServerAddress, 
	TmConnectionType const kConnection,
	TmU16 const kServerPort,
	TmU32 const kFlags,
	int const kTimeoutMS ) );
#endif

TM_API( void, tmCoreClose, ( HTELEMETRY cx ) );

TM_API( void , tmCoreSetDebugZoneLevel, ( HTELEMETRY cx, int const v ) );
TM_API( void , tmCoreCheckDebugZoneLevel, ( HTELEMETRY cx, int const v ) );
TM_API( void , tmCoreUnwindToDebugZoneLevel, ( HTELEMETRY cx, int const v ) );

TM_API( tmStubCSTR, tmCoreDynamicString, ( HTELEMETRY cx, char const * s ) );
TM_API( void, tmCoreClearStaticString, ( HTELEMETRY cx, char const * s ) );

TM_API( void, tmCoreSetVariable, ( HTELEMETRY cx, char const *kpKey, TmU32* pFormatCode, char const *kpValueFmt, ... ) );
TM_API( void, tmCoreSetTimelineSectionName, ( HTELEMETRY cx, TmU32 *pFormatCode, char const * kpFmt, ... ) );
TM_API( void, tmCoreThreadName, ( HTELEMETRY cx, TmU32 const kThreadID, TmU32 *pFormatCode, char const * kpFmt, ... ) ); 
TM_API( void, tmCoreGetFormatCode, ( TmU32* pCode, char const * kpFmt ) );

TM_API( void, tmCoreEnable, ( HTELEMETRY cx, TmOption const kOption, int const kValue ) );
TM_API( int , tmCoreIsEnabled, ( HTELEMETRY cx, TmOption const kOption ) );

TM_API( void, tmCoreSetParameter, ( HTELEMETRY cx, TmParameter const kParam, void const *kpValue ) );

TM_API( void, tmCoreTick, ( HTELEMETRY cx ) );
TM_API( void, tmCoreFlush, ( HTELEMETRY cx ) );
TM_API( void, tmCorePause, ( HTELEMETRY cx, int const kPause ) );
TM_API( int, tmCoreIsPaused, ( HTELEMETRY cx ) );
TM_API( void, tmCoreEnter, (HTELEMETRY cx, TmU64 *matchid, TmU32 const kThreadId, TmU64 const kThreshold, TmU32 const kFlags, char const *kpLocation, TmU32 const kLine, TmU32* pFmtCode, char const *kpFmt, ... ) );
TM_API( void, tmCoreLeave, ( HTELEMETRY cx, TmU64 const kMatchID, TmU32 const kThreadId, char const *kpLocation, int const kLine ) ); 

TM_API( void,  tmCoreEmitAccumulationZone, ( HTELEMETRY cx, TmU64 * pAccum, TmU64 const kZoneTotal, TmU32 const kCount, TmU32 const kZoneFlags, char const *kpLocation, TmU32 const kLine, TmU32 *pFmtCode, char const * kpFmt, ... ) );

TM_API( TmU64, tmCoreGetLastContextSwitchTime, (HTELEMETRY cx) );

TM_API( void, tmCoreLockName, ( HTELEMETRY cx, void const *kpPtr, TmU32* pFmtCode, char const * kpFmt, ... ) );
TM_API( void, tmCoreSetLockState, ( HTELEMETRY cx, void const *kpPtr, TmLockState const kState, char const * kLocation, TmU32 const kLine, TmU32 *pFormatCode, char const *kpFmt, ... ) );
TM_API( int, tmCoreSetLockStateMinTime, ( HTELEMETRY cx, void* buf, void const *kpPtr, TmLockState const kState, char const * kLocation, TmU32 const kLine, TmU32 *pFormatCode, char const *kpFmt, ... ) );

TM_API( void, tmCoreBeginTimeSpan, ( HTELEMETRY cx, TmU64 const kId, TmU32 const kFlags, TmU64 const kTime, char const *kpLocation, TmU32 const kLine, TmU32 *pFmtCode, char const *kpFmt, ... ) );
TM_API( void, tmCoreEndTimeSpan, ( HTELEMETRY cx, TmU64 const kId, TmU32 const kFlags, TmU64 const kTime, char const *kpLocation, TmU32 const kLine, TmU32 *pFmtCode, char const *kpFmt, ... ) );

TM_API( void, tmCoreSignalLockCount, ( HTELEMETRY cx, char const *kpLocation, TmU32 const kLine, void const * kPtr, TmU32 const kCount, TmU32* pFmtCode, char const *kpName, ... ) );
TM_API( void, tmCoreTryLock,   ( HTELEMETRY cx, TmU64 *matchid, TmU64 const kThreshold, char const *kpLocation, TmU32 const kLine, void const * kPtr, TmU32* pFmtCode, char const *kpFmt, ... ) );
TM_API( void, tmCoreEndTryLock, ( HTELEMETRY cx, TmU64 const kMatchId, char const *kpLocation, int const kLine, TmU32* pFmt, void const * kPtr, TmLockResult const kResult ) );

TM_API( TmI32, tmCoreGetStati, ( HTELEMETRY cx, TmStat const kStat ) );

TM_API( void, tmCoreMessage, ( HTELEMETRY cx, TmU32 const kFlags, TmU32* pFmtCode, char const * kpFmt, ... ) );

TM_API( void, tmCoreAlloc, ( HTELEMETRY cx, void const * kPtr, TmU64 const kSize, char const *kpLocation, TmU32 const kLine, TmU32 *pFmtCode, char const *kpFmt, ... ) );
TM_API( void, tmCoreFree, ( HTELEMETRY cx, void const * kpPtr, char const *kpLocation, int const kLine, TmU32 *pFmtCode ) );

TM_API( void, tmCorePlot, ( HTELEMETRY cx, TmPlotType const kType, TmU32 const kFlags, float const kValue,  TmU32 *pFmtCode, char const * kpFmt, ... ) );
TM_API( void, tmCorePlotI32, ( HTELEMETRY cx, TmPlotType const kType, TmU32 const kFlags, TmI32 const kValue,  TmU32 *pFmtCode, char const * kpFmt, ... ) );
TM_API( void, tmCorePlotU32, ( HTELEMETRY cx, TmPlotType const kType, TmU32 const kFlags, TmU32 const kValue,  TmU32 *pFmtCode, char const * kpFmt, ... ) );
TM_API( void, tmCorePlotI64, ( HTELEMETRY cx, TmPlotType const kType, TmU32 const kFlags, TmI64 const kValue,  TmU32 *pFmtCode, char const * kpFmt, ... ) );
TM_API( void, tmCorePlotU64, ( HTELEMETRY cx, TmPlotType const kType, TmU32 const kFlags, TmU64 const kValue,  TmU32 *pFmtCode, char const * kpFmt, ... ) );
TM_API( void, tmCorePlotF64, ( HTELEMETRY cx, TmPlotType const kType, TmU32 const kFlags, double const kValue,  TmU32 *pFmtCode, char const * kpFmt, ... ) );

TM_API( void, tmCoreBlob, ( HTELEMETRY cx,   void const * kpData, int const kDataSize, char const *kpPluginIdentifier, TmU32* pFmtCode, char const * kpFmt, ... ) );
TM_API( void, tmCoreDisjointBlob, ( HTELEMETRY cx, int const kNumPieces, void const ** kpData, int const *kDataSize, char const *kpPluginIdentifier, TmU32* pFmtCode, char const * kpFmt, ... ) );

TM_API( void, tmCoreUpdateSymbolData, ( HTELEMETRY cx ) );

TM_API( int, tmCoreSendCallStack, ( HTELEMETRY cx, TmCallStack const * kpCallStack, int const kSkip ) );
TM_API( int, tmCoreGetCallStack, ( HTELEMETRY cx, TmCallStack * pCallStack ) );

#undef TM_API

void CTM_API_STRUCT_Stub::LinkToStubs( void )
{
#if defined( TM_API_S )
#undef TM_API_S
#endif

	//#define TM_API_S(name) name = GenerateStubFunction( TM_FUNCTION_TYPE(name)(NULL) );
#define TM_API_S(name) name = name##Stub;

#if ( TelemetryBuildNumber != 31 )
#error This section needs to get updated with the latest TM_API_STRUCT definitions whenever we update to a new telemetry sdk.
#endif

	//======================================================================================================================
	// Copy/paste the TM_API_S(*) contents of TM_API_STRUCT (from Rad's telemetry.h) here to stub out each of the functions
	//======================================================================================================================
	TM_API_S( tmCoreCheckVersion );
	TM_API_S( tmCoreUpdateSymbolData );
	TM_API_S( tmCoreGetLastContextSwitchTime );    
#ifndef __RADSPU__
	TM_API_S( tmCoreTick );
#endif
	TM_API_S( tmCoreFlush );
	TM_API_S( tmCoreDynamicString );
	TM_API_S( tmCoreClearStaticString );
	TM_API_S( tmCoreSetVariable );
	TM_API_S( tmCoreGetFormatCode );
	TM_API_S( tmCoreGetSessionName );
	TM_API_S( tmCoreGetLastError );
	TM_API_S( tmCoreShutdownContext );
	TM_API_S( tmCoreGetConnectionStatus );
	TM_API_S( tmCoreSetTimelineSectionName );
	TM_API_S( tmCoreEnable );
	TM_API_S( tmCoreIsEnabled );
#ifndef __RADSPU__
	TM_API_S( tmCoreOpen );
#endif
	TM_API_S( tmCoreClose );
	TM_API_S( tmCorePause );
	TM_API_S( tmCoreIsPaused );
	TM_API_S( tmCoreEnter );
	TM_API_S( tmCoreLeave );

	TM_API_S( tmCoreThreadName );
	TM_API_S( tmCoreLockName );
	TM_API_S( tmCoreTryLock );
	TM_API_S( tmCoreEndTryLock );
	TM_API_S( tmCoreSignalLockCount );
	TM_API_S( tmCoreSetLockState );

	TM_API_S( tmCoreAlloc );
	TM_API_S( tmCoreFree );
	TM_API_S( tmCoreGetStati );

	TM_API_S( tmCoreBeginTimeSpan );
	TM_API_S( tmCoreEndTimeSpan );

	TM_API_S( tmCorePlot );
	TM_API_S( tmCorePlotI32 );
	TM_API_S( tmCorePlotU32 );
	TM_API_S( tmCorePlotI64 );
	TM_API_S( tmCorePlotU64 );
	TM_API_S( tmCorePlotF64 );

	TM_API_S( tmCoreBlob );
	TM_API_S( tmCoreDisjointBlob );
	TM_API_S( tmCoreMessage );

	TM_API_S( tmCoreSendCallStack );
	TM_API_S( tmCoreGetCallStack );

	TM_API_S( tmCoreSetDebugZoneLevel );
	TM_API_S( tmCoreCheckDebugZoneLevel );
	TM_API_S( tmCoreUnwindToDebugZoneLevel );

	TM_API_S( tmCoreEmitAccumulationZone );

	TM_API_S( tmCoreSetLockStateMinTime );
	TM_API_S( tmCoreSetParameter );

#if defined __RADPS3__ && !defined TM_API_STATIC
	TM_API_S( tmPPUCoreGetListener );
	TM_API_S( tmPPUCoreRegisterSPUProgram );
#endif

#undef TM_API_S
}
#endif

#endif // RAD_TELEMETRY_ENABLED
