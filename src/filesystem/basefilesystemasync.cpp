﻿//========== Copyright (c) 2005, Valve Corporation, All rights reserved. ========
//
// Purpose: CBaseFileSystem Async Operation
//
//			The CBaseFileSystem methods implement the IFileSystem 
//			asynchronous entry points. The model for reads currently is a 
//			callback model where the callback can take place either in the 
//			context of the main thread or the worker thread. It would be 
//			easy to do a polled model later. Async operations return a 
//			handle which is used to refer to the operation later. The 
//			handle is actually a pointer to a reference counted "job" 
//			object that holds all the context, status, and results of an 
//			operation.
//
//=============================================================================

#include <limits.h>
#if defined( _WIN32 ) && !defined( _X360 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "tier1/convar.h"
#include "vstdlib/jobthread.h"
#include "tier1/utlmap.h"
#include "tier1/utlbuffer.h"
#include "tier0/icommandline.h"
#include "vstdlib/random.h"
#include "basefilesystem.h"
#include "filesystem/IQueuedLoader.h"

// VCR mode for now is handled by not running async.  This is primarily for
// performance reasons. VCR mode would preclude the use of a lock-free job
// retrieval. Can change if need in future, but it's best to do so if needed,
// and to make it a deliberate compile time choice to keep the fast path.
#undef WaitForSingleObject

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
ConVar async_mode( "async_mode", "0", 0, "Set the async filesystem mode (0 = async, 1 = synchronous)" );

//-----------------------------------------------------------------------------
// Async Modes
//-----------------------------------------------------------------------------
enum FSAsyncMode_t
{
	FSAM_ASYNC,
	FSAM_SYNC,
};

#ifndef DISABLE_ASYNC

// If we are not in CERT, we are adding some latency to the async accesses (between 15 and 25 ms).
// 70 ms is an usual average seek time on DVD / BluRay (or increasing much more the seek time for HDD).
ConVar async_simulate_delay( "async_simulate_delay", "0", 0, "Simulate a delay of up to a set msec per file operation" );
ConVar async_allow_held_files( "async_allow_held_files", "1", 0, "Allow AsyncBegin/EndRead()" );

static FSAsyncMode_t GetAsyncMode( void )
{
	return (FSAsyncMode_t)( async_mode.GetInt() );
}


static bool AsyncAllowHeldFiles( void )
{
	return async_allow_held_files.GetBool();
}


CON_COMMAND( async_suspend, "" )
{
	BaseFileSystem()->AsyncSuspend();
}

CON_COMMAND( async_resume, "" )
{
	BaseFileSystem()->AsyncResume();
}

#else


FORCEINLINE static FSAsyncMode_t GetAsyncMode( void )
{
	return FSAM_SYNC;
}

static bool AsyncAllowHeldFiles( void )
{
	return false;
}

#endif

FORCEINLINE static void SimulateDelay( void )
{
#ifndef DISABLE_ASYNC
	if ( async_simulate_delay.GetInt() == 0 || ThreadInMainThread() || g_pQueuedLoader->IsMapLoading() )
	{
	}
	else 
	{
		int nDelay = async_simulate_delay.GetInt();
		int nDelayMidRange = nDelay / 4;
		ThreadSleep( RandomInt( nDelay - nDelayMidRange, nDelay + nDelayMidRange ) );
	}
#endif
}

//-----------------------------------------------------------------------------
// Need to support old external. New implementation has less granular priority for efficiency
//-----------------------------------------------------------------------------

inline JobPriority_t ConvertPriority( int iFilesystemPriority )		
{
	if ( iFilesystemPriority == 0 )
	{
		return JP_NORMAL;
	}
	else if ( iFilesystemPriority > 0 )
	{
		return JP_HIGH;
	}
	return JP_LOW;
}

//-----------------------------------------------------------------------------
//
// Support for holding files open
//
//-----------------------------------------------------------------------------

struct AsyncOpenedFile_t : CRefCounted<CRefCountServiceNoDelete> // no mutex needed, under control of CAsyncOpenedFiles
{
	AsyncOpenedFile_t() : hFile(FILESYSTEM_INVALID_HANDLE) {}
	FileHandle_t hFile;
};

class CAsyncOpenedFiles
{
public:
	CAsyncOpenedFiles()
	{
		m_map.SetLessFunc( CaselessStringLessThan );
	}

	FSAsyncFile_t FindOrAdd( const char *pszFilename )
	{
		char szFixedName[MAX_FILEPATH];
		Q_strncpy( szFixedName, pszFilename, sizeof( szFixedName ) );
		Q_FixSlashes( szFixedName );

		Assert( (intp)FS_INVALID_ASYNC_FILE == m_map.InvalidIndex() );

		AUTO_LOCK_FM( m_mutex );

		int iEntry = m_map.Find( szFixedName );
		if ( iEntry == m_map.InvalidIndex() )
		{
			iEntry = m_map.Insert( strdup( szFixedName ), new AsyncOpenedFile_t );
		}
		else
		{
			m_map[iEntry]->AddRef();
		}
		return (FSAsyncFile_t)(intp)iEntry;
	}

	FSAsyncFile_t Find( const char *pszFilename )
	{
		char szFixedName[MAX_FILEPATH];
		Q_strncpy( szFixedName, pszFilename, sizeof( szFixedName ) );
		Q_FixSlashes( szFixedName );

		AUTO_LOCK_FM( m_mutex );

		int iEntry = m_map.Find( szFixedName );
		if ( iEntry != m_map.InvalidIndex() )
		{
			m_map[iEntry]->AddRef();
		}

		return (FSAsyncFile_t)(intp)iEntry;
	}

	AsyncOpenedFile_t *Get( FSAsyncFile_t item )
	{
		if ( item == FS_INVALID_ASYNC_FILE)
		{
			return NULL;
		}

		AUTO_LOCK_FM( m_mutex );

		int iEntry = (CUtlMap<CUtlString, AsyncOpenedFile_t>::IndexType_t)size_cast<int>( (intp) item );
		Assert( m_map.IsValidIndex( iEntry ) );
		m_map[iEntry]->AddRef();
		return m_map[iEntry];
	}

	void AddRef( FSAsyncFile_t item )
	{
		if ( item == FS_INVALID_ASYNC_FILE)
		{
			return;
		}

		AUTO_LOCK_FM( m_mutex );

		int iEntry = (CUtlMap<CUtlString, AsyncOpenedFile_t>::IndexType_t)size_cast<int>( (intp) item );
		Assert( m_map.IsValidIndex( iEntry ) );
		m_map[iEntry]->AddRef();
	}

	void Release( FSAsyncFile_t item )
	{
		if ( item == FS_INVALID_ASYNC_FILE)
		{
			return;
		}

		AUTO_LOCK_FM( m_mutex );

		int iEntry = (CUtlMap<CUtlString, AsyncOpenedFile_t>::IndexType_t)size_cast<int>( (intp) item );
		Assert( m_map.IsValidIndex( iEntry ) );
		if ( m_map[iEntry]->Release() == 0 )
		{
			if ( m_map[iEntry]->hFile != FILESYSTEM_INVALID_HANDLE )
			{
				BaseFileSystem()->Close( m_map[iEntry]->hFile );
			}
			delete m_map[iEntry];
			delete m_map.Key( iEntry );
			m_map.RemoveAt( iEntry );
		}
	}

private:
	CThreadFastMutex m_mutex;

	CUtlMap<const char *, AsyncOpenedFile_t *> m_map;
};

CAsyncOpenedFiles g_AsyncOpenedFiles;



#define FSASYNC_WRITE_PRIORITY	JP_LOW

//---------------------------------------------------------

ASSERT_INVARIANT( (int)FSASYNC_OK == (int)JOB_OK );
ASSERT_INVARIANT( (int)FSASYNC_STATUS_PENDING == (int)JOB_STATUS_PENDING );
ASSERT_INVARIANT( (int)FSASYNC_STATUS_INPROGRESS == (int)JOB_STATUS_INPROGRESS );
ASSERT_INVARIANT( (int)FSASYNC_STATUS_ABORTED == (int)JOB_STATUS_ABORTED );
ASSERT_INVARIANT( (int)FSASYNC_STATUS_UNSERVICED == (int)JOB_STATUS_UNSERVICED );

//---------------------------------------------------------
// A standard filesystem job
//---------------------------------------------------------
class CFileAsyncJob : public CJob
{
public:
	CFileAsyncJob( JobPriority_t priority = JP_NORMAL )
	  : CJob( priority )
	{
		SetFlags( GetFlags() | JF_IO );
	}

	virtual JobStatus_t GetResult( void **ppData, int *pSize ) { *ppData = NULL; *pSize = 0; return GetStatus(); }
	virtual bool IsWrite() const { return false; }
};

//---------------------------------------------------------
// A standard filesystem read job
//---------------------------------------------------------
class CFileAsyncReadJob : public CFileAsyncJob, 
						  protected FileAsyncRequest_t
{
public:
	CFileAsyncReadJob( const FileAsyncRequest_t &fromRequest )
	  : CFileAsyncJob( ConvertPriority( fromRequest.priority ) ),
		FileAsyncRequest_t( fromRequest ),
		m_pResultData( NULL ),
		m_nResultSize( 0 ),
		m_pRealContext( fromRequest.pContext ),
		m_pfnRealCallback( fromRequest.pfnCallback )
	{
#if defined( TRACK_BLOCKING_IO )
		m_Timer.Start();
#endif
		pszFilename = strdup( fromRequest.pszFilename );
		Q_FixSlashes( const_cast<char*>( pszFilename ) );

		pContext = this;
		pfnCallback = InterceptCallback;

		if ( hSpecificAsyncFile != FS_INVALID_ASYNC_FILE )
		{
			g_AsyncOpenedFiles.AddRef( hSpecificAsyncFile );
		}
#if (defined(_DEBUG) || defined(USE_MEM_DEBUG))
		m_pszAllocCreditFile = NULL;
		m_nAllocCreditLine = 0;
#endif
	}

	~CFileAsyncReadJob()
	{
		if ( hSpecificAsyncFile != FS_INVALID_ASYNC_FILE )
		{
			g_AsyncOpenedFiles.Release( hSpecificAsyncFile );
		}

		if ( pszFilename )
			free( (void *)pszFilename );
	}

	virtual char const	*Describe()
	{
		return pszFilename; 
	}

	const FileAsyncRequest_t *GetRequest() const
	{
		return this;
	}

	virtual JobStatus_t DoExecute()
	{
		SimulateDelay();
#if defined( TRACK_BLOCKING_IO )
		bool oldState = BaseFileSystem()->SetAllowSynchronousLogging( false );
#endif

#if (defined(_DEBUG) || defined(USE_MEM_DEBUG))
		if ( m_pszAllocCreditFile )
			MemAlloc_PushAllocDbgInfo( m_pszAllocCreditFile, m_nAllocCreditLine );
#endif

		JobStatus_t retval = BaseFileSystem()->SyncRead( *this );

#if (defined(_DEBUG) || defined(USE_MEM_DEBUG))
		if ( m_pszAllocCreditFile )
			MemAlloc_PopAllocDbgInfo();
#endif

#if defined( TRACK_BLOCKING_IO )
		m_Timer.End();
		FileBlockingItem item( FILESYSTEM_BLOCKING_ASYNCHRONOUS, Describe(), m_Timer.GetDuration().GetSeconds(), FileBlockingItem::FB_ACCESS_READ );
		BaseFileSystem()->RecordBlockingFileAccess( false, item );
		BaseFileSystem()->SetAllowSynchronousLogging( oldState );
#endif
		return retval;
	}

	virtual JobStatus_t GetResult( void **ppData, int *pSize ) 
	{ 
		if ( m_pResultData )
		{
			*ppData = m_pResultData;
			*pSize = m_nResultSize;
		}
		return GetStatus(); 
	}

	static void InterceptCallback( const FileAsyncRequest_t &request, int nBytesRead, FSAsyncStatus_t result )
	{
		CFileAsyncReadJob *pJob = (CFileAsyncReadJob *)request.pContext;
		if ( result == FSASYNC_OK && !( request.flags & FSASYNC_FLAGS_FREEDATAPTR ) )
		{
			pJob->m_pResultData = request.pData;
			pJob->m_nResultSize = nBytesRead;
		}

		if ( pJob->m_pfnRealCallback )
		{
			// Going to slam the values. Not used after this point. Make temps if that changes
			FileAsyncRequest_t &temp = const_cast<FileAsyncRequest_t &>(request);
			temp.pfnCallback = pJob->m_pfnRealCallback;
			temp.pContext = pJob->m_pRealContext;

			(*pJob->m_pfnRealCallback)( temp, nBytesRead, result );
		}
	}

	void SetAllocCredit( const char *pszFile, int line )
	{
#if (defined(_DEBUG) || defined(USE_MEM_DEBUG))
		m_pszAllocCreditFile = pszFile;
		m_nAllocCreditLine = line;
#endif
	}

private:
	void *					m_pResultData;
	int						m_nResultSize;
	void *					m_pRealContext;
	FSAsyncCallbackFunc_t	m_pfnRealCallback;
#if defined( TRACK_BLOCKING_IO )
	CFastTimer				m_Timer;
#endif
#if (defined(_DEBUG) || defined(USE_MEM_DEBUG))
	const char *			m_pszAllocCreditFile;
	int						m_nAllocCreditLine;
#endif
};

//---------------------------------------------------------
// Append to a file
//---------------------------------------------------------
static int g_nAsyncWriteJobs;

class CFileAsyncWriteJob : public CFileAsyncJob
{
public:
	CFileAsyncWriteJob( const char *pszFilename, const void *pData, unsigned nBytes, bool bFreeMemory, bool bAppend )
	  : CFileAsyncJob( FSASYNC_WRITE_PRIORITY ),
		m_pData( pData ),
		m_nBytes( nBytes ),
		m_bFreeMemory( bFreeMemory ),
		m_bAppend( bAppend )
	{
#if defined( TRACK_BLOCKING_IO )
		m_Timer.Start();
#endif
		m_pszFilename = strdup( pszFilename );
		g_nAsyncWriteJobs++;

		SetFlags( GetFlags() | JF_SERIAL );
	}

	~CFileAsyncWriteJob()
	{
		g_nAsyncWriteJobs--;
		free( (void *)m_pszFilename );
	}

	virtual char const *Describe() { return m_pszFilename; }

	virtual bool IsWrite() const { return true; }

	virtual JobStatus_t DoExecute()
	{
		SimulateDelay();
#if defined( TRACK_BLOCKING_IO )
		bool oldState = BaseFileSystem()->SetAllowSynchronousLogging( false );
#endif
		JobStatus_t retval = BaseFileSystem()->SyncWrite( m_pszFilename, m_pData, m_nBytes, false, m_bAppend );

#if defined( TRACK_BLOCKING_IO )
		m_Timer.End();
		FileBlockingItem item( FILESYSTEM_BLOCKING_ASYNCHRONOUS, Describe(), m_Timer.GetDuration().GetSeconds(), FileBlockingItem::FB_ACCESS_WRITE );
		BaseFileSystem()->RecordBlockingFileAccess( false, item );
		BaseFileSystem()->SetAllowSynchronousLogging( oldState );
#endif
		return retval;
	}

	virtual void DoCleanup()
	{
		if ( m_pData && m_bFreeMemory )
		{
			delete (char*)m_pData;
		}
	}

protected:
	bool		m_bFreeMemory;
private:
	const char *m_pszFilename;
	const void *m_pData;
	int			m_nBytes;
	bool		m_bAppend;
#if defined( TRACK_BLOCKING_IO )
	CFastTimer	m_Timer;
#endif
};

class CFileAsyncWriteFileJob : public CFileAsyncWriteJob
{
public:
	CFileAsyncWriteFileJob( const char *pszFilename, const CUtlBuffer *pData, unsigned nBytes, bool bFreeMemory, bool bAppend )
	  : CFileAsyncWriteJob( pszFilename, pData->Base(), nBytes, bFreeMemory, bAppend ),
		m_pBuffer( pData )
	{
	}

	virtual void DoCleanup()
	{
		if ( m_pBuffer && m_bFreeMemory )
		{
			delete m_pBuffer;
		}
	}

private:
	const CUtlBuffer *m_pBuffer;
};

//---------------------------------------------------------
// Append two files
//---------------------------------------------------------
class CFileAsyncAppendFileJob : public CFileAsyncJob
{
public:
	CFileAsyncAppendFileJob( const char *pszAppendTo, const char *pszAppendFrom )
		: CFileAsyncJob( FSASYNC_WRITE_PRIORITY )
	{
#if defined( TRACK_BLOCKING_IO )
		m_Timer.Start();
#endif
		m_pszAppendTo = strdup( pszAppendTo );
		m_pszAppendFrom = strdup( pszAppendFrom );
		Q_FixSlashes( const_cast<char*>( m_pszAppendTo ) );
		Q_FixSlashes( const_cast<char*>( m_pszAppendFrom ) );
		g_nAsyncWriteJobs++;

		SetFlags( GetFlags() | JF_SERIAL );
	}

	~CFileAsyncAppendFileJob()
	{
		g_nAsyncWriteJobs--;
	}

	virtual char const	*Describe() { return m_pszAppendTo; }

	virtual bool IsWrite() const { return true; }

	virtual JobStatus_t DoExecute()
	{
		SimulateDelay();
#if defined( TRACK_BLOCKING_IO )
		bool oldState = BaseFileSystem()->SetAllowSynchronousLogging( false );
#endif
		JobStatus_t retval = BaseFileSystem()->SyncAppendFile( m_pszAppendTo, m_pszAppendFrom );

#if defined( TRACK_BLOCKING_IO )
		m_Timer.End();
		FileBlockingItem item( FILESYSTEM_BLOCKING_ASYNCHRONOUS, Describe(), m_Timer.GetDuration().GetSeconds(), FileBlockingItem::FB_ACCESS_APPEND );
		BaseFileSystem()->RecordBlockingFileAccess( false, item );
		BaseFileSystem()->SetAllowSynchronousLogging( oldState );
#endif

		return retval;
	}

private:
	const char *m_pszAppendTo;
	const char *m_pszAppendFrom;
#if defined( TRACK_BLOCKING_IO )
	CFastTimer	m_Timer;
#endif
};

//---------------------------------------------------------
// Job to find out file size
//---------------------------------------------------------

class CFileAsyncFileSizeJob : public CFileAsyncReadJob 
{
public:
	CFileAsyncFileSizeJob( const FileAsyncRequest_t &fromRequest )
		: CFileAsyncReadJob( fromRequest )
	{
#if defined( TRACK_BLOCKING_IO )
		m_Timer.Start();
#endif
	}

	virtual JobStatus_t DoExecute()
	{
		SimulateDelay();
#if defined( TRACK_BLOCKING_IO )
		bool oldState = BaseFileSystem()->SetAllowSynchronousLogging( false );
#endif
		JobStatus_t retval = BaseFileSystem()->SyncGetFileSize( *this );
#if defined( TRACK_BLOCKING_IO )
		m_Timer.End();
		FileBlockingItem item( FILESYSTEM_BLOCKING_ASYNCHRONOUS, Describe(), m_Timer.GetDuration().GetSeconds(), FileBlockingItem::FB_ACCESS_SIZE );
		BaseFileSystem()->RecordBlockingFileAccess( false, item );
		BaseFileSystem()->SetAllowSynchronousLogging( oldState );
#endif
		return retval;
	}
#if defined( TRACK_BLOCKING_IO )
private:
	CFastTimer				m_Timer;
#endif
};




class CFileAsyncDirectoryScanJob : public CFileAsyncJob
{
	private:
		FSAsyncScanAddFunc_t		m_pAddResultCallback;
		FSAsyncScanCompleteFunc_t	m_pCompletionCallback;
		void*						m_pContext;
		bool						m_bRecurse;
		char						m_SearchSpec[MAX_PATH];
		char						m_CurPath[MAX_PATH];
		
		CFileAsyncDirectoryScanJob() : CFileAsyncJob() {};

	public:
		CFileAsyncDirectoryScanJob( const char* pSearchSpec, bool bRecurseDirs, void* pContext, FSAsyncScanAddFunc_t pAddCallback, FSAsyncScanCompleteFunc_t pDoneCallback ) : CFileAsyncJob( JP_NORMAL )
		{
			Assert( pSearchSpec != NULL && V_strlen( pSearchSpec )  < MAX_PATH && pAddCallback != NULL && pDoneCallback != NULL );
			m_pAddResultCallback = pAddCallback;
			m_pCompletionCallback = pDoneCallback;
			m_bRecurse = bRecurseDirs;
			m_pContext = pContext;
			V_strcpy( m_SearchSpec, pSearchSpec );
			V_memset( m_CurPath, 0, sizeof (m_CurPath) );
		
		}
		~CFileAsyncDirectoryScanJob()  {};

	virtual char const*		Describe() { return m_SearchSpec; }

	virtual bool			IsWrite() const { return false; }

	virtual JobStatus_t		DoExecute();

	virtual void			DoCleanup()   {};

};


JobStatus_t CFileAsyncDirectoryScanJob::DoExecute()
{

	FileFindHandle_t findSession = -1;
	
	// Save off the current path
	V_strcpy( m_CurPath, m_SearchSpec );
	V_StripFilename( m_CurPath );
	
	const char *pFileName = g_pFullFileSystem->FindFirst( m_SearchSpec, &findSession );

	while ( pFileName != NULL)
	{
		(*m_pAddResultCallback)( m_pContext, m_CurPath, ( char* ) pFileName );
		
		pFileName = g_pFullFileSystem->FindNext( findSession );
	}

	g_pFullFileSystem->FindClose( findSession );
	
	(*m_pCompletionCallback)( m_pContext, FSASYNC_OK );

	return FSASYNC_OK;

}




//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseFileSystem::InitAsync()
{
	Assert( !m_pThreadPool );
	if ( m_pThreadPool )
		return;
#ifndef OSX
	if ( IsX360() && Plat_IsInDebugSession() )
	{
		class CBreakThread : public CThread
		{
			virtual int Run()
			{
				for (;;)
				{
					Sleep(1000);
					static int wakeCount;
					wakeCount++;
					volatile static int bForceResume = false;
					if ( bForceResume )
					{
						bForceResume = false;
						BaseFileSystem()->AsyncResume();
					}
				}

				return 0;
			}
		};

		static CBreakThread breakThread;
		breakThread.SetName( "DebugBreakThread" );
		breakThread.Start( 1024 );
	}
#endif

	if ( CommandLine()->FindParm( "-noasync" ) )
	{
		Msg( "Async I/O disabled from command line\n" );
		return;
	}

	bool b360EmulatePS3 = ( IsX360() && CommandLine()->FindParm( "-ps3" ) );

	// create the i/o thread pool
	m_pThreadPool = CreateNewThreadPool();

	ThreadPoolStartParams_t params;
	params.iThreadPriority = 0;
	params.bIOThreads = true;

	if ( !IsX360() && !IsPS3() )
	{
		params.nThreads = MIN( GetCPUInformation().m_nLogicalProcessors, 3 ); // > 3 threads doing IO on one drive, are you crazy?
		params.nStackSize = 256 * 1024;
	}

	if ( IsX360() && !b360EmulatePS3 )
	{
		// override defaults
		// 360 has a single i/o thread on the farthest proc
		params.nThreads = 1;
		params.fDistribute = TRS_TRUE;
		params.bUseAffinityTable = true;
		params.iAffinityTable[0] = XBOX_PROCESSOR_3;
	}
	if ( IsPS3() || b360EmulatePS3 )
	{
		// override defaults
		// PS3 has a single i/o thread
		params.nThreads = 1;
		params.fDistribute = TRS_FALSE;
		params.bUseAffinityTable = false;
	}

	if ( !m_pThreadPool->Start( params, "FsAsyncIO" ) )
	{
		SafeRelease( m_pThreadPool );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseFileSystem::ShutdownAsync()
{
	if ( m_pThreadPool )
	{
		AsyncFlush();
		m_pThreadPool->Stop();
		SafeRelease( m_pThreadPool );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncReadMultiple( const FileAsyncRequest_t *pRequests, int nRequests, FSAsyncControl_t *phControls )
{
	return AsyncReadMultipleCreditAlloc( pRequests, nRequests, NULL, 0, phControls );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncReadMultipleCreditAlloc( const FileAsyncRequest_t *pRequests, int nRequests, const char *pszFile, int line, FSAsyncControl_t *phControls )
{
	bool bAsyncMode = ( GetAsyncMode() == FSAM_ASYNC );
	bool bSynchronous = ( !bAsyncMode || ( pRequests[0].flags & FSASYNC_FLAGS_SYNC ) || !m_pThreadPool );

	if ( !bAsyncMode )
	{
		AsyncFinishAll();
	}

	CFileAsyncReadJob *pJob;

	for ( int i = 0; i < nRequests; i++ )
	{
		if ( pRequests[i].nBytes >= 0 )
		{
			pJob = new CFileAsyncReadJob( pRequests[i] );
		}
		else
		{
			pJob =  new CFileAsyncFileSizeJob( pRequests[i] );
		}

#if (defined(_DEBUG) || defined(USE_MEM_DEBUG))
		pJob->SetAllocCredit( pszFile, line );
#endif
		if ( !bSynchronous )
		{
			// async mode, queue request
			m_pThreadPool->AddJob( pJob );
		}
		else
		{
			// synchronous mode, execute now
			pJob->Execute();
		}

		if ( phControls )
		{
			phControls[i] = (FSAsyncControl_t)pJob;
		}
		else
		{
			pJob->Release();
		}
	}

	return FSASYNC_OK;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncWrite(const char *pFileName, const void *pSrc, int nSrcBytes, bool bFreeMemory, bool bAppend, FSAsyncControl_t *pControl )
{
	bool bAsyncMode = ( GetAsyncMode() == FSAM_ASYNC );
	bool bSynchronous = ( !bAsyncMode || !m_pThreadPool );

	if ( !bAsyncMode )
	{
		AsyncFinishAll();
	}

	CJob *pJob = new CFileAsyncWriteJob( pFileName, pSrc, nSrcBytes, bFreeMemory, bAppend );

	if ( !bSynchronous )
	{
		m_pThreadPool->AddJob( pJob );
	}
	else
	{
		pJob->Execute();
	}

	if ( pControl )
	{
		*pControl = (FSAsyncControl_t)pJob;
	}
	else
	{
		pJob->Release();
	}

	return FSASYNC_OK;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncWriteFile(const char *pFileName, const CUtlBuffer *pBuff, int nSrcBytes, bool bFreeMemory, bool bAppend, FSAsyncControl_t *pControl )
{
	bool bAsyncMode = ( GetAsyncMode() == FSAM_ASYNC );
	bool bSynchronous = ( !bAsyncMode || !m_pThreadPool );

	if ( !bAsyncMode )
	{
		AsyncFinishAll();
	}

	CJob *pJob = new CFileAsyncWriteFileJob( pFileName, pBuff, nSrcBytes, bFreeMemory, bAppend );

	if ( !bSynchronous )
	{
		m_pThreadPool->AddJob( pJob );
	}
	else
	{
		pJob->Execute();
	}

	if ( pControl )
	{
		*pControl = (FSAsyncControl_t)pJob;
	}
	else
	{
		pJob->Release();
	}

	return FSASYNC_OK;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncAppendFile(const char *pAppendToFileName, const char *pAppendFromFileName, FSAsyncControl_t *pControl )
{
	bool bAsyncMode = ( GetAsyncMode() == FSAM_ASYNC );
	bool bSynchronous = ( !bAsyncMode || !m_pThreadPool );

	if ( !bAsyncMode )
	{
		AsyncFinishAll();
	}

	CJob *pJob = new CFileAsyncAppendFileJob( pAppendToFileName, pAppendFromFileName );

	if ( !bSynchronous )
	{
		m_pThreadPool->AddJob( pJob );
	}
	else
	{
		pJob->Execute();
	}

	if ( pControl )
	{
		*pControl = (FSAsyncControl_t)pJob;
	}
	else
	{
		pJob->Release();
	}

	return FSASYNC_OK;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CThreadMutex g_AsyncFinishMutex;
void CBaseFileSystem::AsyncFinishAll( int iToPriority )
{
	if ( m_pThreadPool)
	{
		AUTO_LOCK( g_AsyncFinishMutex );
		m_pThreadPool->ExecuteToPriority( ConvertPriority( iToPriority ) );
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

static bool AsyncWriteJobFilter( CJob *pJob )
{
	CFileAsyncJob *pFileJob = dynamic_cast<CFileAsyncJob *>(pJob);
	return ( pFileJob && pFileJob->IsWrite() );
}

void CBaseFileSystem::AsyncFinishAllWrites()
{
	if ( m_pThreadPool && g_nAsyncWriteJobs )
	{
		AUTO_LOCK( g_AsyncFinishMutex );
		m_pThreadPool->ExecuteAll( AsyncWriteJobFilter );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CBaseFileSystem::AsyncSuspend()
{
	if ( m_pThreadPool )
	{
		m_pThreadPool->SuspendExecution();
	}

	return true;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CBaseFileSystem::AsyncResume()
{
	if ( m_pThreadPool )
	{
		m_pThreadPool->ResumeExecution();
	}

	return true;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncBeginRead( const char *pszFile, FSAsyncFile_t *phFile )
{
#if !defined( _PS3) && !defined(FILESYSTEM_STEAM) && !defined(DEDICATED)
	if ( AsyncAllowHeldFiles() )
	{
		*phFile = g_AsyncOpenedFiles.FindOrAdd( pszFile );
		return FSASYNC_OK;
	}
#endif
	*phFile = FS_INVALID_ASYNC_FILE;
	return FSASYNC_OK;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncEndRead( FSAsyncFile_t hFile )
{
#if !defined( _PS3) && !defined(FILESYSTEM_STEAM) && !defined(DEDICATED)
	if ( hFile != FS_INVALID_ASYNC_FILE )
		g_AsyncOpenedFiles.Release( hFile );
#endif

	return FSASYNC_OK;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncFinish( FSAsyncControl_t hControl, bool wait )
{
	if ( wait  )
	{
		CJob *pJob = (CJob *)hControl;
		if ( !pJob )
		{
			return FSASYNC_ERR_FAILURE;
		}

#if defined( TRACK_BLOCKING_IO )
		CFastTimer timer;
		timer.Start();
		BaseFileSystem()->SetAllowSynchronousLogging( false );
#endif
		FSAsyncStatus_t retval = (FSAsyncStatus_t)pJob->Execute();

#if defined( TRACK_BLOCKING_IO )
		timer.End();
		FileBlockingItem item( FILESYSTEM_BLOCKING_ASYNCHRONOUS_BLOCK, pJob->Describe(), timer.GetDuration().GetSeconds(), FileBlockingItem::FB_ACCESS_READ );
		BaseFileSystem()->RecordBlockingFileAccess( false, item );
		BaseFileSystem()->SetAllowSynchronousLogging( true );
#endif
		return retval;
	}

	AsyncSetPriority( hControl, INT_MAX );
	return FSASYNC_OK;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncGetResult( FSAsyncControl_t hControl, void **ppData, int *pSize )
{
	if ( ppData )
	{
		*ppData = NULL;
	}
	if ( pSize )
	{
		*pSize = 0;
	}

	CFileAsyncJob *pJob = (CFileAsyncJob *)hControl;
	if ( !pJob )
	{
		return FSASYNC_ERR_FAILURE;
	}
	if ( pJob->IsFinished() )
	{
		return (FSAsyncStatus_t)pJob->GetResult( ppData, pSize );
	}
	return FSASYNC_STATUS_PENDING;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncAbort( FSAsyncControl_t hControl )
{
	CJob *pJob = (CJob *)hControl;
	if ( !pJob )
	{
		return FSASYNC_ERR_FAILURE;
	}
	return (FSAsyncStatus_t)pJob->Abort();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncStatus( FSAsyncControl_t hControl )
{
	CJob *pJob = (CJob *)hControl;
	if ( !pJob )
	{
		return FSASYNC_ERR_FAILURE;
	}
	return (FSAsyncStatus_t)pJob->GetStatus();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncFlush()
{
	if ( m_pThreadPool )
	{
		m_pThreadPool->AbortAll();
	}

	return FSASYNC_OK;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncSetPriority(FSAsyncControl_t hControl, int newPriority)
{
	if ( m_pThreadPool )
	{
		CJob *pJob = (CJob *)hControl;

		if ( !pJob )
		{
			return FSASYNC_ERR_FAILURE;
		}

		JobPriority_t internalPriority = ConvertPriority( newPriority );
		if ( internalPriority != pJob->GetPriority() )
		{
			m_pThreadPool->ChangePriority( pJob, internalPriority );
		}

	}
	return FSASYNC_OK;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseFileSystem::AsyncAddRef( FSAsyncControl_t hControl )
{
	CJob *pJob = (CJob *)hControl;
	if ( pJob )
	{
		pJob->AddRef();
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseFileSystem::AsyncRelease( FSAsyncControl_t hControl )
{
	CJob *pJob = (CJob *)hControl;
	if ( pJob )
	{
		pJob->Release();
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::AsyncDirectoryScan( const char* pSearchSpec, bool recurseFolders, void* pContext, FSAsyncScanAddFunc_t pfnAdd, FSAsyncScanCompleteFunc_t pfnDone, FSAsyncControl_t *pControl )
{
	bool bAsyncMode = ( GetAsyncMode() == FSAM_ASYNC );
	bool bSynchronous = ( !bAsyncMode || !m_pThreadPool );

	if ( !bAsyncMode )
	{
		AsyncFinishAll();
	}

	CJob *pJob = new CFileAsyncDirectoryScanJob( pSearchSpec, recurseFolders, pContext, pfnAdd, pfnDone );

	if ( !bSynchronous )
	{
		m_pThreadPool->AddJob( pJob );
	}
	else
	{
		pJob->Execute();
	}

	if ( pControl )
	{
		*pControl = (FSAsyncControl_t)pJob;
	}
	else
	{
		pJob->Release();
	}

	return FSASYNC_OK;


}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
static void *GetDest( const FileAsyncRequest_t &request, bool bTryUnbuffered, unsigned *pBytesBuffer, unsigned *pBytesRead )
{
	Assert( false ); // This function is not implemented
	return NULL;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

FSAsyncStatus_t CBaseFileSystem::SyncRead( const FileAsyncRequest_t &request )
{
	Assert( request.nBytes >=0 );

	if ( request.nBytes < 0 || request.nOffset < 0 )
	{
		Msg( "Invalid async read of %s\n", request.pszFilename );
		DoAsyncCallback( request, NULL, 0, FSASYNC_ERR_FILEOPEN );
		return FSASYNC_ERR_FILEOPEN;
	}

	FSAsyncStatus_t result;

	AsyncOpenedFile_t *pHeldFile = ( request.hSpecificAsyncFile != FS_INVALID_ASYNC_FILE ) ? g_AsyncOpenedFiles.Get( request.hSpecificAsyncFile ) : NULL;

	FileHandle_t hFile;
	
	if ( !pHeldFile || pHeldFile->hFile == FILESYSTEM_INVALID_HANDLE )
	{
		hFile = OpenEx( request.pszFilename, "rb", 0, request.pszPathID );
		if ( pHeldFile )
		{
			pHeldFile->hFile = hFile;
		}
	}
	else
	{
		hFile = pHeldFile->hFile;
	}

	if ( hFile )
	{
		// ------------------------------------------------------
		int nBytesToRead = ( request.nBytes ) ? request.nBytes : Size( hFile ) - request.nOffset;
		int nBytesBuffer;
		void *pDest;

		if ( nBytesToRead < 0 )
		{
			nBytesToRead = 0; // bad offset?
		}

		if ( request.pData )
		{
			// caller provided buffer
			Assert( !( request.flags & FSASYNC_FLAGS_NULLTERMINATE ) );
			pDest = request.pData;
			nBytesBuffer = nBytesToRead;
		}
		else
		{
			// allocate an optimal buffer
			unsigned nOffsetAlign;
			nBytesBuffer = nBytesToRead + ( ( request.flags & FSASYNC_FLAGS_NULLTERMINATE ) ? 1 : 0 );
			if ( GetOptimalIOConstraints( hFile, &nOffsetAlign, NULL, NULL) && ( request.nOffset % nOffsetAlign == 0 ) )
			{
				nBytesBuffer = GetOptimalReadSize( hFile, nBytesBuffer );
			}

			if ( !request.pfnAlloc )
			{
				pDest = AllocOptimalReadBuffer( hFile, nBytesBuffer, request.nOffset );
			}
			else
			{
				pDest = (*request.pfnAlloc)( request.pszFilename, nBytesBuffer );
			}
		}

		SetBufferSize( hFile, 0 ); // TODO: what if it's a pack file? restore buffer size?

		if ( request.nOffset > 0 )
		{
			Seek( hFile, request.nOffset, FILESYSTEM_SEEK_HEAD );
		}

		// perform the read operation
		int iPrevPriority = ThreadGetPriority();
		if ( iPrevPriority < 2 )
		{
			ThreadSetPriority( 2 );
		}
		int nBytesRead = ReadEx( pDest, nBytesBuffer, nBytesToRead, hFile );
		if ( iPrevPriority < 2 )
		{
			ThreadSetPriority( iPrevPriority );
		}

		if ( !pHeldFile )
		{
			Close( hFile );
		}

		if ( request.flags & FSASYNC_FLAGS_NULLTERMINATE )
		{
			((char *)pDest)[nBytesRead] = 0;
		}

		result = ( ( nBytesRead == 0 ) && ( nBytesToRead != 0 ) ) ? FSASYNC_ERR_READING : FSASYNC_OK;
		DoAsyncCallback( request, pDest, MIN( nBytesRead, nBytesToRead ), result );
	}
	else
	{
		DoAsyncCallback( request, NULL, 0, FSASYNC_ERR_FILEOPEN );
		result = FSASYNC_ERR_FILEOPEN;
	}

	if ( pHeldFile )
	{
		g_AsyncOpenedFiles.Release( request.hSpecificAsyncFile );
	}

	if ( m_fwLevel >= FILESYSTEM_WARNING_REPORTALLACCESSES_ASYNC )
	{
		LogAccessToFile( "async", request.pszFilename, "" );
	}

	return result;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::SyncGetFileSize( const FileAsyncRequest_t &request )
{
	int size = Size( request.pszFilename, request.pszPathID );

	DoAsyncCallback( request, NULL, size, ( size ) ? FSASYNC_OK : FSASYNC_ERR_FILEOPEN );

	return FSASYNC_OK;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::SyncWrite(const char *pszFilename, const void *pSrc, int nSrcBytes, bool bFreeMemory, bool bAppend )
{
	FileHandle_t hFile = OpenEx( pszFilename, ( bAppend ) ? "ab+" : "wb", IsX360() ? FSOPEN_NEVERINPACK : 0, NULL );
	if ( hFile )
	{
		SetBufferSize( hFile, 0 );
		Write( pSrc, nSrcBytes, hFile );
		Close( hFile );
		if ( bFreeMemory )
		{
			delete (char*)pSrc;
		}

		if ( m_fwLevel >= FILESYSTEM_WARNING_REPORTALLACCESSES_ASYNC )
		{
			LogAccessToFile( "asyncwrite", pszFilename, "" );
		}

		return FSASYNC_OK;
	}

	return FSASYNC_ERR_FILEOPEN;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
FSAsyncStatus_t CBaseFileSystem::SyncAppendFile(const char *pAppendToFileName, const char *pAppendFromFileName )
{
	FileHandle_t hDestFile = OpenEx( pAppendToFileName, "ab+", IsX360() ? FSOPEN_NEVERINPACK : 0, NULL );
	FSAsyncStatus_t result = FSASYNC_ERR_FAILURE;
	if ( hDestFile )
	{
		SetBufferSize( hDestFile, 0 );
		FileHandle_t hSourceFile = OpenEx( pAppendFromFileName, "rb", IsX360() ? FSOPEN_NEVERINPACK : 0, NULL );
		if ( hSourceFile )
		{
			SetBufferSize( hSourceFile, 0 );
			const int BUFSIZE = 128 * 1024;
			int fileSize = Size( hSourceFile );
			char *buf = (char *)malloc( BUFSIZE );
			int size;

			while ( fileSize > 0 )
			{
				if ( fileSize > BUFSIZE )
					size = BUFSIZE;
				else
					size = fileSize;
				Read( buf, size, hSourceFile );
				Write( buf, size, hDestFile );

				fileSize -= size;
			}

			free(buf);
			Close( hSourceFile );
			result = FSASYNC_OK;
		}
		Close( hDestFile );
	}

	if ( m_fwLevel >= FILESYSTEM_WARNING_REPORTALLACCESSES_ASYNC )
	{
		LogAccessToFile( "asyncappend", pAppendToFileName, "" );
	}

	return result;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseFileSystem::DoAsyncCallback( const FileAsyncRequest_t &request, void *pData, int nBytesRead, FSAsyncStatus_t result )
{
	void *pDataToFree = NULL;

	if ( request.pfnCallback )
	{
		if ( pData && request.pData != pData )
		{
			// Allocated the data here
			FileAsyncRequest_t temp = request;
			temp.pData = pData;
			{
				AUTOBLOCKREPORTER_FN( DoAsyncCallback, this, false, temp.pszFilename, FILESYSTEM_BLOCKING_CALLBACKTIMING, FileBlockingItem::FB_ACCESS_READ );
				(*request.pfnCallback)( temp, nBytesRead, result );
			}
			if ( !( request.flags & FSASYNC_FLAGS_ALLOCNOFREE ) )
			{
				pDataToFree = pData;
			}
		}
		else
		{
			{
				AUTOBLOCKREPORTER_FN( DoAsyncCallback, this, false, request.pszFilename, FILESYSTEM_BLOCKING_CALLBACKTIMING, FileBlockingItem::FB_ACCESS_READ );
				(*request.pfnCallback)( request, nBytesRead, result );
			}
			if ( ( request.flags & FSASYNC_FLAGS_FREEDATAPTR ) )
			{
				pDataToFree = request.pData;
			}
		}
	}

	if ( pDataToFree  )
	{
		Assert( !request.pfnAlloc );
#if defined( OSX ) || defined( _PS3 ) || defined( LINUX )
		// The ugly delete[] (void*) method generates a compile warning on osx, as it should.
		free( pDataToFree );
#else
		delete [] pDataToFree;
#endif
	}
}

