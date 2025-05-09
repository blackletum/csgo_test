﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: XMA Decoding
//
//=====================================================================================//

#include "audio_pch.h"
#include "tier1/mempool.h"
#include "tier1/circularbuffer.h"
#include "tier1/utllinkedlist.h"
#include "tier1/utlmap.h"
#include "filesystem/iqueuedloader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define DEBUG_XMA

// XMA is supposed to decode at an ideal max of 512 mono samples every 4msec.
// XMA can only peel a max of 1984 stereo samples per poll request (if available).
// Max is not achievable and degrades based on quality settings, stereo, etc, but using these numbers for for calcs.
// 1984 stereo samples should be decoded by xma in 31 msec.
// 1984 stereo samples at 44.1Khz dictates a request every 45 msec.
// GetOutputData() must be clocked faster than 45 msec or samples will not be available.
// However, the XMA decoder must be serviced much faster. It was designed for 5 msec.
// 15 msec seems to be fast enough for XMA to decode enough to keep the smaller buffer sizes satisfied, and have slop for +/- 5 msec swings.

// Need at least this amount of decoded pcm samples before mixing can commence.
// This needs to be able to cover the initial mix request, while a new decode cycle is in flight.
#define MIN_READYTOMIX	( ( 2 * XMA_POLL_RATE ) * 0.001f )

// number of samples that xma decodes
// must be 128 aligned for mono (1984 is hw max for stereo)
#define XMA_MONO_OUTPUT_BUFFER_SAMPLES		2048
#define XMA_STEREO_OUTPUT_BUFFER_SAMPLES	1920

// for decoder input
// xma blocks are fetched from the datacache into one of these hw buffers for decoding
// must be in quantum units of XMA_BLOCK_SIZE
#define XMA_INPUT_BUFFER_SIZE		( 8 * XMA_BLOCK_SIZE )

// circular staging buffer to drain xma decoder and stage until mixer requests
// must be large enough to hold the slowest expected mixing frame worth of samples
#define PCM_STAGING_BUFFER_TIME		200

// xma physical heap, supplies xma input buffers for hw decoder
// each potential channel must be able to peel 2 buffers for driving xma decoder
#define XMA_PHYSICAL_HEAP_SIZE		( 2 * MAX_CHANNELS * XMA_INPUT_BUFFER_SIZE )

// in milliseconds
#define MIX_IO_DATA_TIMEOUT			10000	// async i/o from dvd could be very late
#define MIX_DECODER_TIMEOUT			3000	// decoder might be very busy
#define MIX_DECODER_POLLING_LATENCY	5		// not faster than 5ms, or decoder will sputter

// diagnostic errors
#define ERROR_IO_DATA_TIMEOUT		-1	// async i/o taking too long to deliver xma blocks
#define ERROR_IO_TRUNCATED_BLOCK	-2	// async i/o failed to deliver complete blocks
#define ERROR_IO_NO_XMA_DATA		-3	// async i/o failed to deliver any block
#define ERROR_DECODER_TIMEOUT		-4	// decoder taking too long to decode xma blocks
#define ERROR_OUT_OF_MEMORY			-5	// not enough physical memory for xma blocks
#define ERROR_XMA_PARSE				-6	// decoder barfed on xma blocks
#define ERROR_XMA_CANTLOCK			-7	// hw not acting as expected
#define ERROR_XMA_CANTSUBMIT		-8	// hw not acting as expected
#define ERROR_XMA_CANTRESUME		-9	// hw not acting as expected
#define ERROR_XMA_NO_PCM_DATA		-10	// no xma decoded pcm data ready
#define ERROR_NULL_BUFFER			-11	// logic flaw, expected buffer is null 
#define ERROR_XMA_NOPLAYBACKOBJECT	-12	// failed to create xma playback object

const char *g_XMAErrorStrings[] =
{
	"Unknown Error Code",
	"Async I/O Data Timeout",		// ERROR_IO_DATA_TIMEOUT
	"Async I/O Truncated Block",	// ERROR_IO_TRUNCATED_BLOCK
	"Async I/O Data Not Ready",		// ERROR_IO_NO_XMA_DATA
	"Decoder Timeout",				// ERROR_DECODER_TIMEOUT
	"Out Of Memory",				// ERROR_OUT_OF_MEMORY
	"XMA Parse",					// ERROR_XMA_PARSE
	"XMA Cannot Lock",				// ERROR_XMA_CANTLOCK
	"XMA Cannot Submit",			// ERROR_XMA_CANTSUBMIT
	"XMA Cannot Resume",			// ERROR_XMA_CANTRESUME
	"XMA No PCM Data Ready",		// ERROR_XMA_NO_PCM_DATA
	"NULL Buffer",					// ERROR_NULL_BUFFER
	"XMA Cannot Create Object",		// ERROR_XMA_NOPLAYBACKOBJECT
};

MEMALLOC_DEFINE_EXTERNAL_TRACKING(CXMAAllocator);
class CXMAAllocator
{
public:
	static void *Alloc( int bytes )
	{
		MEM_ALLOC_CREDIT();

		void *retval = XMemAlloc( bytes, 
			MAKE_XALLOC_ATTRIBUTES( 
			0, 
			false, 
			TRUE, 
			FALSE, 
			eXALLOCAllocatorId_XAUDIO,
			XALLOC_PHYSICAL_ALIGNMENT_4K, 
			XALLOC_MEMPROTECT_WRITECOMBINE_LARGE_PAGES, 
			FALSE,
			XALLOC_MEMTYPE_PHYSICAL ) );

		MemAlloc_RegisterExternalAllocation( CXMAAllocator, retval, XPhysicalSize( retval ) ); 
		return retval;
	}

	static void Free( void *p )
	{
		MemAlloc_RegisterExternalDeallocation( CXMAAllocator, p, XPhysicalSize( p ) ); 
		XMemFree( p, 
			MAKE_XALLOC_ATTRIBUTES( 
				0, 
				false, 
				TRUE, 
				FALSE, 
				eXALLOCAllocatorId_XAUDIO,
				XALLOC_PHYSICAL_ALIGNMENT_4K, 
				XALLOC_MEMPROTECT_WRITECOMBINE_LARGE_PAGES, 
				FALSE,
				XALLOC_MEMTYPE_PHYSICAL ) );
	}
};

// for XMA decoding, fixed size allocations aligned to 4K from a single physical heap
CAlignedMemPool< XMA_INPUT_BUFFER_SIZE, 4096, XMA_PHYSICAL_HEAP_SIZE, CXMAAllocator > g_XMAMemoryPool;

ConVar snd_xma_spew_warnings( "snd_xma_spew_warnings", "0" );
ConVar snd_xma_spew_startup( "snd_xma_spew_startup", "0" );
ConVar snd_xma_spew_mixers( "snd_xma_spew_mixers", "0" );
ConVar snd_xma_spew_decode( "snd_xma_spew_decode", "0" );
ConVar snd_xma_spew_drain( "snd_xma_spew_drain", "0" );
ConVar snd_xma_recover_from_exhausted_stream( "snd_xma_recover_from_exhausted_stream", "1", 0, "If 1, recovers when the stream is exhausted when playing XMA sounds (prevents music or ambiance sounds to stop if too many sounds are played). Set to 0, to stop the sound otherwise." );

#ifdef DEBUG_XMA
ConVar snd_xma_record( "snd_xma_record", "0" );
ConVar snd_xma_spew_errors( "snd_xma_spew_errors", "0" );
#endif

extern ConVar snd_report_loop_sound;
extern ConVar snd_delay_for_choreo_enabled;
extern ConVar snd_mixahead;
extern float g_fDelayForChoreo;
extern uint32 g_nDelayForChoreoLastCheckInMs;
extern int g_nDelayForChoreoNumberOfSoundsPlaying;
extern ConVar snd_async_stream_spew_delayed_start_time;
extern ConVar snd_async_stream_spew_delayed_start_filter;
extern ConVar snd_async_stream_spew_exhausted_buffer;
extern ConVar snd_async_stream_spew_exhausted_buffer_time;

//-----------------------------------------------------------------------------
// Purpose: Mixer for ADPCM encoded audio
//-----------------------------------------------------------------------------
class CAudioMixerWaveXMA : public CAudioMixerWave
{
public:
	typedef CAudioMixerWave BaseClass;

	CAudioMixerWaveXMA( IWaveData *data, int initialStreamPosition, int skipInitialSamples, bool bUpdateDelayForChoreo );
	virtual ~CAudioMixerWaveXMA( void );
	
	virtual void			Mix( IAudioDevice *pDevice, channel_t *pChannel, void *pData, int outputOffset, int inputOffset, fixedint fracRate, int outCount, int timecompress );

	virtual int				GetOutputData( void **pData, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] );

	virtual bool			IsSetSampleStartSupported() const;
	virtual void			SetSampleStart( int newPosition );
	virtual int				GetPositionForSave();
	virtual void			SetPositionFromSaved( int savedPosition );

	virtual int				GetMixSampleSize() { return CalcSampleSize( 16, m_NumChannels ); }

	virtual bool			IsReadyToMix();
	virtual bool			ShouldContinueMixing();

private:
	int						GetXMABlocksAndSubmitToDecoder( bool bDecoderLocked );
	int						UpdatePositionForLooping( int *pNumRequestedSamples );
	int						ServiceXMADecoder( bool bForceUpdate );
	int						GetPCMSamples( int numRequested, char *pData );
	bool					GetPlaybackInitialized();

	XMAPLAYBACK				*m_pXMAPlayback;

	// input buffers, encoded xma
	byte					*m_pXMABuffers[2];
	int						m_XMABufferIndex;

	// output buffer, decoded pcm samples, a staging circular buffer, waiting for mixer requests
	// due to staging nature, contains decoded samples from multiple input buffers
	CCircularBuffer			*m_pPCMSamples;

	int						m_SampleRate;
	int						m_NumChannels;
	// maximum possible decoded samples
	int						m_SampleCount;

	// decoded sample position
	int						m_SamplePosition;
	// current data marker
	int						m_LastDataOffset;
	int						m_DataOffset;
	// total bytes of data
	int						m_TotalBytes;

	// Numbers of samples to skip at first
	int						m_SkipInitialSamples;

	// timers
	unsigned int			m_StartTime;
	unsigned int			m_LastSuccessfulStreamTime;
	unsigned int			m_LastDecodingStartTime;
	unsigned int			m_LastDrainTime;
	unsigned int			m_LastPollTime;

	int						m_hMixerList;
	int						m_Error;

	int8					m_LastSample[4];

	bool					m_bStartedMixing : 1;
	bool					m_bFinished : 1;
	bool					m_bLooped : 1;
	bool					m_bUpdateDelayForChoreo : 1;
};

CUtlFixedLinkedList< CAudioMixerWaveXMA * >	g_XMAMixerList;

CON_COMMAND( snd_xma_info, "Spew XMA Info" )
{
	Msg( "XMA Memory:\n" );
	Msg( "  Blocks Allocated: %d\n", g_XMAMemoryPool.NumAllocated() );
	Msg( "  Blocks Free:      %d\n", g_XMAMemoryPool.NumFree() );
	Msg( "  Pool Size:        %.2f MB\n", g_XMAMemoryPool.BytesTotal() / ( 1024.0f * 1024.0f ) );
	Msg( "Active XMA Mixers:  %d\n", g_XMAMixerList.Count() );
	char nameBuf[MAX_PATH];
	for ( int hMixer = g_XMAMixerList.Head(); hMixer != g_XMAMixerList.InvalidIndex(); hMixer = g_XMAMixerList.Next( hMixer ) )
	{
		CAudioMixerWaveXMA *pXMAMixer = g_XMAMixerList[hMixer];
		Msg( "  rate:%5d ch:%1d '%s'\n", pXMAMixer->GetSource()->SampleRate(), pXMAMixer->GetSource()->IsStereoWav() ? 2 : 1, pXMAMixer->GetSource()->GetFileName(nameBuf, sizeof(nameBuf)) );
	}
}

CAudioMixerWaveXMA::CAudioMixerWaveXMA( IWaveData *data, int initialStreamPosition, int skipInitialSamples, bool bUpdateDelayForChoreo ) : CAudioMixerWave( data ) 
{
	Assert( dynamic_cast<CAudioSourceWave *>(&m_pData->Source()) != NULL );

	m_Error = 0;

	m_NumChannels = m_pData->Source().IsStereoWav() ? 2 : 1;
	m_SampleRate = m_pData->Source().SampleRate();
	m_bLooped = m_pData->Source().IsLooped();
	m_SampleCount = m_pData->Source().SampleCount();
	m_TotalBytes = m_pData->Source().DataSize();
	m_SkipInitialSamples = skipInitialSamples;

	m_LastDataOffset = initialStreamPosition;
	m_DataOffset = initialStreamPosition;
	m_SamplePosition = 0;
	if ( initialStreamPosition )
	{
		m_SamplePosition = m_pData->Source().StreamToSamplePosition( initialStreamPosition );

		// IMPORTANT: When initial stream position is requested we need
		// to make the XMA mixer obeys rules of how CAudioMixerWave::GetSampleLoadRequest
		// is using its members "m_fsample_index", "m_sample_loaded_index" and "m_sample_max_loaded"
		// If the implementation of CAudioMixerWave::GetSampleLoadRequest changes, then
		// XMA mixer should be changed accordingly. 

		CAudioMixerWave::m_fsample_index = m_SamplePosition;
		CAudioMixerWave::m_sample_loaded_index = m_SamplePosition;
		CAudioMixerWave::m_sample_max_loaded = m_SamplePosition + 1;
	}

	m_bStartedMixing = false;
	m_bFinished = false;
	m_bUpdateDelayForChoreo = bUpdateDelayForChoreo;

	m_StartTime = Plat_MSTime();
	m_LastSuccessfulStreamTime = 0;
	m_LastDecodingStartTime = 0;
	m_LastPollTime = 0;
	m_LastDrainTime = 0;

	if ( m_bUpdateDelayForChoreo )							// Do not test for enabled here as we want g_nDelayForChoreoNumberOfSoundsPlaying to be always accurate
	{
		++g_nDelayForChoreoNumberOfSoundsPlaying;
		g_nDelayForChoreoLastCheckInMs = m_StartTime;		// Not necessary as g_nDelayForChoreoNumberOfSoundsPlaying != 0 prevents the Reset, but does not hurt either
	}

	m_pXMAPlayback = NULL;
	m_pPCMSamples = NULL;

	m_pXMABuffers[0] = NULL;
	m_pXMABuffers[1] = NULL;
	m_XMABufferIndex = 0;

	V_memset( m_LastSample, 0, sizeof( m_LastSample ) );

	m_hMixerList = g_XMAMixerList.AddToTail( this );

#ifdef DEBUG_XMA
	if ( snd_xma_record.GetBool() )
	{
		WaveCreateTmpFile( "debug.wav", m_SampleRate, 16, m_NumChannels );
	}
#endif

	if ( snd_xma_spew_mixers.GetBool() )
	{
		char nameBuf[MAX_PATH];
		Msg( "XMA: 0x%8.8x (%2d), Mixer Alloc, '%s'\n", (unsigned int)this, g_XMAMixerList.Count(), m_pData->Source().GetFileName(nameBuf, sizeof(nameBuf)) );
	}
}

CAudioMixerWaveXMA::~CAudioMixerWaveXMA( void )
{
	if ( m_bUpdateDelayForChoreo )										// Do not test for enabled here as we want g_nDelayForChoreoNumberOfSoundsPlaying to be always accurate
	{
		--g_nDelayForChoreoNumberOfSoundsPlaying;
		Assert( g_nDelayForChoreoNumberOfSoundsPlaying >= 0 );
		g_nDelayForChoreoLastCheckInMs = Plat_MSTime();					// Critical to make sure that we are reseting the latency at least N ms after the last choreo sound
	}

	if ( m_pXMAPlayback )
	{
		XMAPlaybackDestroy( m_pXMAPlayback );

		g_XMAMemoryPool.Free( m_pXMABuffers[0] );
		if ( m_pXMABuffers[1] )
		{
			g_XMAMemoryPool.Free( m_pXMABuffers[1] );
		}
	}

	if ( m_pPCMSamples )
	{
		FreeCircularBuffer( m_pPCMSamples );
	}

	g_XMAMixerList.Remove( m_hMixerList );

	if ( snd_xma_spew_mixers.GetBool() )
	{
		char nameBuf[MAX_PATH];
		Msg( "XMA: 0x%8.8x (%2d), Mixer Freed, '%s'\n", (unsigned int)this, g_XMAMixerList.Count(), m_pData->Source().GetFileName(nameBuf, sizeof(nameBuf)) );
	}
}

void CAudioMixerWaveXMA::Mix( IAudioDevice *pDevice, channel_t *pChannel, void *pData, int outputOffset, int inputOffset, fixedint fracRate, int outCount, int timecompress )
{
	if ( m_NumChannels == 1 )
	{
		pDevice->Mix16Mono( pChannel, (short *)pData, outputOffset, inputOffset, fracRate, outCount, timecompress );
	}
	else
	{
		pDevice->Mix16Stereo( pChannel, (short *)pData, outputOffset, inputOffset, fracRate, outCount, timecompress );
	}
}

//-----------------------------------------------------------------------------
// Looping is achieved in two passes to provide a circular view of the linear data.
// Pass1: Clamps a sample request to the end of data.
// Pass2: Snaps to the loop start, and returns the number of samples to discard, could be 0,
// up to the expected loop sample position.
// Returns the number of samples to discard, or 0.
//-----------------------------------------------------------------------------
int CAudioMixerWaveXMA::UpdatePositionForLooping( int *pNumRequestedSamples )
{
	if ( !m_bLooped )
	{
		// not looping, no fixups
		return 0;
	}

	int numLeadingSamples;
	int numTrailingSamples;
	CAudioSourceWave &source = reinterpret_cast<CAudioSourceWave &>(m_pData->Source());
	int loopSampleStart = source.GetLoopingInfo( NULL, &numLeadingSamples, &numTrailingSamples );

	int numRemainingSamples = ( m_SampleCount - numTrailingSamples ) - m_SamplePosition;

	// possibly straddling the end of data (and thus about to loop)
	// want to split the straddle into two regions, due to loops possibly requiring a trailer and leader of discarded samples
	if ( numRemainingSamples > 0 )
	{
		// first region, all the remaining samples, clamped until end of desired data
		*pNumRequestedSamples = min( *pNumRequestedSamples, numRemainingSamples );

		// nothing to discard
		return 0;
	}
	else if ( numRemainingSamples == 0 )
	{
		if ( snd_report_loop_sound.GetBool() )
		{
			char nameBuf[MAX_PATH];
			Warning( "[Sound] Sound \"%s\" just looped.\n", m_pData->Source().GetFileName(nameBuf, sizeof( nameBuf ) ) );
		}

		// at exact end of desired data, snap the sample position back
		// the position will be correct AFTER discarding decoded trailing and leading samples
		m_SamplePosition = loopSampleStart;

		// clamp the request
		numRemainingSamples = ( m_SampleCount - numTrailingSamples ) - m_SamplePosition;
		*pNumRequestedSamples = min( *pNumRequestedSamples, numRemainingSamples );

		// flush these samples so the sample position is the real loop sample starting position
		return numTrailingSamples + numLeadingSamples;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Get and submit XMA block(s). The decoder must stay blocks ahead of mixer
// so the decoded samples are available for peeling.
// An XMA file is thus treated as a series of fixed size large buffers (multiple xma blocks),
// which are streamed in sequentially. The XMA buffers may be delayed from the 
// audio data cache due to async i/o latency.
// Returns < 0 if error, 0 if no decode started, 1 if decode submitted.
//-----------------------------------------------------------------------------
int CAudioMixerWaveXMA::GetXMABlocksAndSubmitToDecoder( bool bDecoderIsLocked )
{
	int status = 0;

	if ( m_DataOffset >= m_TotalBytes )
	{
		if ( !m_bLooped )
		{
			// end of file, no more data to decode
			// not an error, because decoder finishes long before samples drained
			return 0;
		}

		// start from beginning of loop
		CAudioSourceWave &source = reinterpret_cast<CAudioSourceWave &>(m_pData->Source());
		source.GetLoopingInfo( &m_DataOffset, NULL, NULL ); 
		m_DataOffset *= XMA_BLOCK_SIZE;
	}

	HRESULT hr;
	bool bLocked = false;
	if ( !bDecoderIsLocked )
	{
		// decoder must be locked before any access
		hr = XMAPlaybackRequestModifyLock( m_pXMAPlayback );
		if ( FAILED( hr ) )
		{
			status = ERROR_XMA_CANTLOCK;
			goto cleanUp;
		}

		hr = XMAPlaybackWaitUntilModifyLockObtained( m_pXMAPlayback );
		if ( FAILED( hr ) )
		{
			status = ERROR_XMA_CANTLOCK;
			goto cleanUp;
		}
		bLocked = true;
	}

	// the input buffer can never be less than a single xma block (buffer size is multiple blocks)
	int bufferSize = min( m_TotalBytes - m_DataOffset, XMA_INPUT_BUFFER_SIZE );
	if ( !bufferSize )
	{
		// EOF
		goto cleanUp;
	}
	Assert( !( bufferSize % XMA_BLOCK_SIZE ) );

	byte *pXMABuffer = m_pXMABuffers[m_XMABufferIndex & 0x01];
	if ( !pXMABuffer )
	{
		// shouldn't happen, buffer should have been allocated
		Assert( 0 );
		status = ERROR_NULL_BUFFER;
		goto cleanUp;
	}

	if ( !XMAPlaybackQueryReadyForMoreData( m_pXMAPlayback, 0 ) || XMAPlaybackQueryInputDataPending( m_pXMAPlayback, 0, pXMABuffer ) )
	{
		// decoder too saturated for more data or 
		// decoder still decoding from input hw buffer
		goto cleanUp;
	}

	// get xma block(s)
	// pump to get all of requested data
	char *pData;
	int total = 0;
	int nDataOffset = m_DataOffset;
	while ( total < bufferSize )
	{
		int available = m_pData->ReadSourceData( (void **)&pData, nDataOffset, bufferSize - total, NULL );
		if ( !available )
		{
			break;
		}

		// aggregate into hw buffer
		V_memcpy( pXMABuffer + total, pData, available );

		nDataOffset += available;
		total += available;
	}	
	if ( total != bufferSize )
	{
		if ( !total )
		{
			// failed to get any data, could be async latency or file error
			status = ERROR_IO_NO_XMA_DATA;
		}
		else
		{
			// failed to get complete xma block(s)
			// Do not set an error state, we will try later.
		}
		goto cleanUp;
	}

	m_DataOffset = nDataOffset;		// Now that it is successful, update m_DataOffset
									// It allows us to handle more gracefully partial xma blocks
	m_LastSuccessfulStreamTime = Plat_MSTime();

	// track the currently submitted offset
	// this is used as a cheap method for save/restore because an XMA seek table is not available
	m_LastDataOffset = m_DataOffset - total;

	// start decoding the block(s) in the hw buffer
	hr = XMAPlaybackSubmitData( m_pXMAPlayback, 0, pXMABuffer, bufferSize );
	if ( FAILED( hr ) )
	{
		// failed to start decoder
		status = ERROR_XMA_CANTSUBMIT;
		goto cleanUp;
	}

	m_LastDecodingStartTime = Plat_MSTime();

	// decode submitted
	status = 1;

	// advance to next buffer
	m_XMABufferIndex++;

	if ( snd_xma_spew_decode.GetBool() )
	{
		char nameBuf[MAX_PATH];
		Msg( "XMA: 0x%8.8x, XMABuffer: 0x%8.8x, BufferSize: %d, NextDataOffset: %d, %s\n", (unsigned int)this, pXMABuffer, bufferSize, m_DataOffset, m_pData->Source().GetFileName(nameBuf, sizeof(nameBuf)) );
	}

cleanUp:
	if ( bLocked )
	{
		// release the lock and let the decoder run
		hr = XMAPlaybackResumePlayback( m_pXMAPlayback );
 		if ( FAILED( hr ) )
		{
			status = ERROR_XMA_CANTRESUME;
		}
	}
	
	return status;
}

//-----------------------------------------------------------------------------
// Drain the XMA Decoder into the staging circular buffer of PCM for mixer.
// Fetch new XMA samples for the decoder.
//-----------------------------------------------------------------------------
int CAudioMixerWaveXMA::ServiceXMADecoder( bool bForceUpdate )
{
	uint32 nCurrentTime = Plat_MSTime();

	// allow decoder to work without being polled (lock causes a decoding stall)
	// decoder must be allowed minimum operating latency
	// the buffers are sized to compensate for the operating latency
	if ( !bForceUpdate && ( nCurrentTime - m_LastPollTime <= MIX_DECODER_POLLING_LATENCY ) )
	{
		return 0;
	}
	m_LastPollTime = nCurrentTime;

	if ( GetPlaybackInitialized() == false )
	{
		return -1;
	}

	// lock and pause the decoder to gain access
	HRESULT hr = XMAPlaybackRequestModifyLock( m_pXMAPlayback );
	if ( FAILED( hr ) )
	{
		m_Error = ERROR_XMA_CANTLOCK;
		return -1;
	}

	hr = XMAPlaybackWaitUntilModifyLockObtained( m_pXMAPlayback );
	if ( FAILED( hr ) )
	{
		m_Error = ERROR_XMA_CANTLOCK;
		return -1;
	}

	DWORD dwParseError = XMAPlaybackGetParseError( m_pXMAPlayback, 0 );
	if ( dwParseError )
	{
		if ( snd_xma_spew_warnings.GetBool() )
		{
			char nameBuf[MAX_PATH];
			Warning( "XMA: 0x%8.8x, Decoder Error, Parse: %d, '%s'\n", (unsigned int)this, dwParseError, m_pData->Source().GetFileName(nameBuf, sizeof(nameBuf)) );
		}
		m_Error = ERROR_XMA_PARSE;
		return -1;
	}

#ifdef DEBUG_XMA
	if ( snd_xma_spew_errors.GetBool() )
	{
		DWORD dwError = XMAPlaybackGetErrorBits( m_pXMAPlayback, 0 );
		if ( dwError )
		{
			char nameBuf[MAX_PATH];
			Warning( "XMA: 0x%8.8x, Playback Error: %d, '%s'\n", (unsigned int)this, dwError, m_pData->Source().GetFileName(nameBuf, sizeof(nameBuf)) );
		}
	}
#endif

	int numNewSamples = XMAPlaybackQueryAvailableData( m_pXMAPlayback, 0 );
	int numMaxSamples = m_pPCMSamples->GetWriteAvailable()/( m_NumChannels*sizeof( short ) );
	int numSamples = min( numNewSamples, numMaxSamples );
	while ( numSamples )
	{
		char *pPCMData = NULL;
		int numSamplesDecoded = XMAPlaybackConsumeDecodedData( m_pXMAPlayback, 0, numSamples, (void**)&pPCMData );

		if ( numSamplesDecoded > m_SkipInitialSamples )
		{
			pPCMData += m_SkipInitialSamples * m_NumChannels * sizeof( short );
			int numSamplesToCopy = numSamplesDecoded - m_SkipInitialSamples;

			// put into staging buffer, ready for mixer to drain
			m_pPCMSamples->Write( pPCMData, numSamplesToCopy*m_NumChannels*sizeof( short ) );

			m_SkipInitialSamples = 0;
		}
		else
		{
			m_SkipInitialSamples -= numSamplesDecoded;
		}
		
		numSamples -= numSamplesDecoded;
		numNewSamples -= numSamplesDecoded;
	}

	// queue up more blocks for the decoder
	// the decoder will always finish ahead of the mixer, submit nothing, and the mixer will still be draining
	int decodeStatus = GetXMABlocksAndSubmitToDecoder( true );
	if ( decodeStatus < 0 && decodeStatus != ERROR_IO_NO_XMA_DATA )
	{
		m_Error = decodeStatus;
		return -1;
	}

	m_bFinished = ( numNewSamples == 0 ) && ( decodeStatus == 0 ) && XMAPlaybackIsIdle( m_pXMAPlayback, 0 );

	// decoder was paused for access, let the decoder run
	hr = XMAPlaybackResumePlayback( m_pXMAPlayback );
 	if ( FAILED( hr ) )
	{
		m_Error = ERROR_XMA_CANTRESUME;
		return -1;
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Drain the PCM staging buffer.
// Copy samples (numSamplesToCopy && pData). Return actual copied.
// Flush Samples (numSamplesToCopy && !pData). Return actual flushed.
// Query available number of samples (!numSamplesToCopy && !pData). Returns available.
//-----------------------------------------------------------------------------
int CAudioMixerWaveXMA::GetPCMSamples( int numSamplesToCopy, char *pData )
{
	int numReadySamples = m_pPCMSamples->GetReadAvailable()/( m_NumChannels*sizeof( short ) );

	// peel sequential samples from the stream's staging buffer
	int numCopiedSamples = 0;
	int numRequestedSamples = min( numSamplesToCopy, numReadySamples );
	if ( numRequestedSamples )
	{
		if ( pData )
		{
			// copy to caller
			m_pPCMSamples->Read( pData, numRequestedSamples*m_NumChannels*sizeof( short ) );
			pData += numRequestedSamples*m_NumChannels*sizeof( short );
		}
		else
		{
			// flush
			m_pPCMSamples->Advance( numRequestedSamples*m_NumChannels*sizeof( short ) );
		}

		numCopiedSamples += numRequestedSamples;
	}

	if ( snd_xma_spew_drain.GetBool() )
	{
		char nameBuf[MAX_PATH];
		char *pOperation = ( numSamplesToCopy && !pData ) ? "Flushed" : "Copied";
		Msg( "XMA: 0x%8.8x, SamplePosition: %d, Ready: %d, Requested: %d, %s: %d, Elapsed: %d ms '%s'\n", 
			(unsigned int)this, m_SamplePosition, numReadySamples, numSamplesToCopy, pOperation, numCopiedSamples, Plat_MSTime() - m_LastDrainTime, m_pData->Source().GetFileName(nameBuf, sizeof(nameBuf)) );
	}
	m_LastDrainTime = Plat_MSTime();

	if ( numSamplesToCopy )
	{
		// could be actual flushed or actual copied
		return numCopiedSamples;
	}
	
	if ( !pData )
	{
		// satify query for available
		return numReadySamples;
	}

	return 0;
}

bool CAudioMixerWaveXMA::GetPlaybackInitialized()
{
	if ( !m_pXMAPlayback )
	{
		// first time, finish setup
		int numBuffers;
		if ( m_bLooped || m_TotalBytes > XMA_INPUT_BUFFER_SIZE )
		{
			// data will cascade through multiple buffers
			numBuffers = 2;
		}
		else
		{
			// data can fit into a single buffer
			numBuffers = 1;
		}

		// xma data must be decoded from a hw friendly buffer
		// pool should have buffers available
		if ( g_XMAMemoryPool.BytesAllocated() != numBuffers * g_XMAMemoryPool.ChunkSize() )
		{
			XMA_PLAYBACK_INIT xmaPlaybackInit = { 0 };
			xmaPlaybackInit.sampleRate = m_SampleRate;
			xmaPlaybackInit.channelCount = m_NumChannels;
			xmaPlaybackInit.subframesToDecode = 4;
			xmaPlaybackInit.outputBufferSizeInSamples = ( m_NumChannels == 2 ) ? XMA_STEREO_OUTPUT_BUFFER_SAMPLES : XMA_MONO_OUTPUT_BUFFER_SAMPLES;
			XMAPlaybackCreate( 1, &xmaPlaybackInit, 0, &m_pXMAPlayback );
			if ( !m_pXMAPlayback )
			{
				m_Error = ERROR_XMA_NOPLAYBACKOBJECT;
				Assert( 0 );
				return false;
			}

			for ( int i = 0; i < numBuffers; i++ )
			{
				m_pXMABuffers[i] = (byte*)g_XMAMemoryPool.Alloc();
			}

			int stagingSize = PCM_STAGING_BUFFER_TIME * m_SampleRate * m_NumChannels * sizeof( short ) * 0.001f;
			m_pPCMSamples = AllocateCircularBuffer( AlignValue( stagingSize, 4 ) );
		}
		else
		{
			// too many sounds playing, no xma buffers free
			m_Error = ERROR_OUT_OF_MEMORY;
			Assert( 0 );
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Stall mixing until initial buffer of decoded samples are available.
//-----------------------------------------------------------------------------
bool CAudioMixerWaveXMA::IsReadyToMix()
{
	// XMA mixing cannot be driven from the main thread
	Assert( ThreadInMainThread() == false );

	if ( m_Error )
	{
		// error has been set
		// let mixer try to get unavailable samples, which causes the real abort
		return true;
	}

	if ( m_bStartedMixing )
	{
		// decoding process has started
		return true;
	}

	if ( GetPlaybackInitialized() == false )
	{
		// An error has been set.
		return true;
	}

	// waiting for samples
	// allow decoder to work without being polled (lock causes a decoding stall)
	if ( Plat_MSTime() - m_LastPollTime <= MIX_DECODER_POLLING_LATENCY )
	{
		return false;
	}

	// must have buffers in flight before mixing can begin
	if ( m_DataOffset == m_LastDataOffset )
	{
		// keep trying to get data, async i/o has some allowable latency
		int decodeStatus = GetXMABlocksAndSubmitToDecoder( false );
		if ( decodeStatus < 0 && decodeStatus != ERROR_IO_NO_XMA_DATA )
		{
			m_Error = decodeStatus;
			return true;
		}
		else if ( !decodeStatus || decodeStatus == ERROR_IO_NO_XMA_DATA )
		{
			// async streaming latency could be to blame, check watchdog
			if ( m_LastSuccessfulStreamTime != 0 )
			{
				if ( Plat_MSTime() - m_LastSuccessfulStreamTime >= MIX_IO_DATA_TIMEOUT )
				{
					m_Error = ERROR_IO_DATA_TIMEOUT;
				}
			}
			return false;
		}
	}

	// get the available samples ready for immediate mixing
	if ( ServiceXMADecoder( true ) < 0 )
	{
		return true;
	}

	// can't mix until we have a minimum threshold of data or the decoder is finished
	int minSamplesNeeded = m_bFinished ? 0 : MIN_READYTOMIX * m_SampleRate;
	int numReadySamples = GetPCMSamples( 0, NULL );
	if ( numReadySamples >= minSamplesNeeded )
	{
		// decoder has samples ready for draining
		m_bStartedMixing = true;
		int nTimeSinceStart = Plat_MSTime() - m_StartTime;

		bool bDisplayStartupLatencyWarning = snd_xma_spew_startup.GetBool();

		char nameBuf[MAX_PATH];
		m_pData->Source().GetFileName( nameBuf, sizeof(nameBuf) );

		if ( nTimeSinceStart > snd_async_stream_spew_delayed_start_time.GetInt() )
		{
			const char * pFilter = snd_async_stream_spew_delayed_start_filter.GetString();
			if ( *pFilter != '\0' )
			{
				bDisplayStartupLatencyWarning |= ( V_stristr( nameBuf, pFilter ) != NULL );
			}
			else
			{
				bDisplayStartupLatencyWarning = true;
			}
		}
		if ( bDisplayStartupLatencyWarning )
		{
			Warning( "XMA: Startup Latency: %d ms, Samples Ready: %d, '%s'\n", nTimeSinceStart, numReadySamples, nameBuf );
		}

		if ( m_bUpdateDelayForChoreo && snd_delay_for_choreo_enabled.GetBool() )
		{
			// We are playing a VO, update the choreo system accordingly for any startup latency. That way future VO will be pushed back by the same amount, so cut off will not occur.

			float fNewValue = ( ( float )( nTimeSinceStart ) / 1000.0f );
			fNewValue -= snd_mixahead.GetFloat();							// Remove the mix-ahead latency so it does not accumulate over time.

			if ( fNewValue > 0.0f )
			{
				g_fDelayForChoreo += fNewValue;								// And we accumulate the error over time.
																			// If the 1st sound sound is late 1 second, we are going to play the 2nd sound with 1 second delay.
																			// However, if the 2nd sound is 2 seconds late, then the 3rd sound will have to be played with a delay of 3 seconds (or cut the 2nd delay too early).
																			// When choreo does not play sounds for a while (currently 0.5 seconds), this counter will be reset to zero.
				if ( bDisplayStartupLatencyWarning )
				{
					Msg( "XMA: Updated IO latency compensation for VCD to %f seconds, due to startup time for sound '%s' taking %d ms.\n", g_fDelayForChoreo, nameBuf, nTimeSinceStart );
				}
			}
			Assert( g_nDelayForChoreoNumberOfSoundsPlaying >= 1 );			// Let's make sure the counter is not zero, as it will prevent the reset of the latency.
		}
		return true;
	}

	if ( m_LastDecodingStartTime != 0 )
	{
		if ( Plat_MSTime() - m_LastDecodingStartTime >= MIX_DECODER_TIMEOUT )
		{
			m_Error = ERROR_DECODER_TIMEOUT;
		}
	}

	// on startup error, let mixer start and get unavailable samples, and abort
	// otherwise hold off mixing until samples arrive
	return ( m_Error != 0 );
}

//-----------------------------------------------------------------------------
// Returns true to mix, false to stop mixer completely. Called after
// mixer requests samples.
//-----------------------------------------------------------------------------
bool CAudioMixerWaveXMA::ShouldContinueMixing()
{
	if ( m_Error && snd_xma_spew_warnings.GetBool() )
	{
		char nameBuf[MAX_PATH];
		const char *pErrorString;
		if ( m_Error < 0 && -m_Error < ARRAYSIZE( g_XMAErrorStrings ) )
		{
			pErrorString = g_XMAErrorStrings[-m_Error];
		}
		else
		{
			pErrorString = g_XMAErrorStrings[0];
		}
		Warning( "XMA: 0x%8.8x, Mixer Aborted: %s, SamplePosition: %d/%d, DataOffset: %d/%d, '%s'\n", 
			(unsigned int)this, pErrorString, m_SamplePosition, m_SampleCount, m_DataOffset, m_TotalBytes, m_pData->Source().GetFileName(nameBuf, sizeof(nameBuf)) );
	}

	// an error condition is fatal to mixer
	return ( m_Error == 0 && BaseClass::ShouldContinueMixing() );
}

//-----------------------------------------------------------------------------
// Read existing buffer or decompress a new block when necessary.
// If no samples can be fetched, returns 0, which hints the mixer to a pending shutdown state.
// This routines operates in large buffer quantums, and nothing smaller.
// XMA decode performance severly degrades if the lock is too frequent.
//-----------------------------------------------------------------------------
int CAudioMixerWaveXMA::GetOutputData( void **pData, int numSamplesToCopy, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
{
	if ( m_Error )
	{
		// mixer will eventually shutdown
		return 0;
	}

	// This function can get called in some cases directly when the mixing did not start (when there is a delay but no seek table in the sound).

	// XMA mixing cannot be driven from the main thread
	Assert( ThreadInMainThread() == false );

	// needs to be clocked at regular intervals
	if ( ServiceXMADecoder( false ) < 0 )
	{
		return 0;
	}

	// loopback may require flushing some decoded samples
	int numRequestedSamples = numSamplesToCopy;
	int numDiscardSamples = UpdatePositionForLooping( &numRequestedSamples );
	if ( numDiscardSamples > 0 )
	{
		// loopback requires discarding samples to converge to expected looppoint
		numDiscardSamples -= GetPCMSamples( numDiscardSamples, NULL );
		if ( numDiscardSamples != 0 )
		{
			// not enough decoded data ready to flush
			// must flush these samples to achieve looping
			if ( snd_xma_recover_from_exhausted_stream.GetBool() == false )
			{
				m_Error = ERROR_XMA_NO_PCM_DATA;
				return 0;
			}
			// If we try to recover, the sound is not going to loop properly here...
		}
	}

	// can only drain as much as can be copied to caller		
	int numMaxSamples =  AUDIOSOURCE_COPYBUF_SIZE/( m_NumChannels * sizeof( short ) );
	numRequestedSamples = min( numRequestedSamples, numMaxSamples );

	int numCopiedSamples = GetPCMSamples( numRequestedSamples, copyBuf );
	if ( numCopiedSamples )
	{
		CAudioMixerWave::m_sample_max_loaded += numCopiedSamples;
		CAudioMixerWave::m_sample_loaded_index += numCopiedSamples;

		// advance position by valid samples
		m_SamplePosition += numCopiedSamples;

		*pData = (void*)copyBuf;

#ifdef DEBUG_XMA
		if ( snd_xma_record.GetBool() )
		{
			WaveAppendTmpFile( "debug.wav", copyBuf, 16, numCopiedSamples * m_NumChannels );
			WaveFixupTmpFile( "debug.wav" );
		}
#endif

		// We copy the last sample in case we have to recover later of an exhausted streamer
		V_memcpy( &m_LastSample, &copyBuf[ ( numCopiedSamples - 1) * m_NumChannels * sizeof(short) ], m_NumChannels * sizeof(short) );
	}
	else
	{
		// no samples copied
		if ( !m_bFinished && numRequestedSamples )
		{
			// XMA latency error occurs when decoder not finished (not at EOF) and caller wanted samples but can't get any
			if ( snd_xma_recover_from_exhausted_stream.GetBool() )
			{
				// Try to recover by using the last sample (to reduce potential pop).
				numCopiedSamples = numRequestedSamples;
				// Code not optimized, but hopefully is not needed in normal cases.
				for ( int i = 0 ; i < numCopiedSamples ; ++i )
				{
					V_memcpy( &copyBuf[ i * m_NumChannels * sizeof(short) ], &m_LastSample, m_NumChannels * sizeof(short) );
				}
				*pData = (void*)copyBuf;

				// Update some information used to calculate how much sound has been updated.
				// When there are some constant big latencies (testing extreme cases, like 500 ms), failure to update these will
				// create snowballing effect and force update of more and more samples (even if they are actually discarded at the end).
				// A visible consequence will be major slow down of the game, getting worse over time until the sound is finished / timed out.
				CAudioMixerWave::m_sample_max_loaded += numCopiedSamples;
				CAudioMixerWave::m_sample_loaded_index += numCopiedSamples;

				if ( snd_async_stream_spew_exhausted_buffer.GetBool()  && ( g_pQueuedLoader->IsMapLoading() == false ) )
				{
					static uint sOldTime = 0;
					uint nCurrentTime = Plat_MSTime();
					if ( nCurrentTime >= sOldTime + snd_async_stream_spew_exhausted_buffer_time.GetInt() )
					{
						char nameBuf[MAX_PATH];
						Warning( "XMA: The stream buffer is exhausted for sound '%s'. Except after loading, fill a bug to have the number of sounds played reduced.\n", m_pData->Source().GetFileName(nameBuf, sizeof(nameBuf)) );
						sOldTime = nCurrentTime;
					}
				}
			}
			else
			{
				// We do not want to recover from the late streamer, the sound is going to stop (very noticeable for music and background ambiance sound).
				m_Error = ERROR_XMA_NO_PCM_DATA;

				if ( snd_xma_spew_warnings.GetInt() )
				{
					char nameBuf[MAX_PATH];
					Warning( "XMA: 0x%8.8x, No Decoded Data Ready: %d samples needed, '%s'\n", (unsigned int)this, numSamplesToCopy, m_pData->Source().GetFileName(nameBuf, sizeof(nameBuf)) );
				}
			}
		}
	}

	return numCopiedSamples;
}

bool CAudioMixerWaveXMA::IsSetSampleStartSupported() const
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Seek to a new position in the file
//			NOTE: In most cases, only call this once, and call it before playing
//			any data.
// Input  : newPosition - new position in the sample clocks of this sample
//-----------------------------------------------------------------------------
void CAudioMixerWaveXMA::SetSampleStart( int newPosition )
{
	// Implementation not supported
	Assert( 0 );
}

int CAudioMixerWaveXMA::GetPositionForSave()
{
	if ( m_bLooped )
	{
		// A looped sample cannot be saved/restored because the decoded sample position,
		// which is needed for loop calc, cannot ever be correctly restored without
		// the XMA seek table. 
		return 0;
	}

	// This is silly and totally wrong, but doing it anyways.
	// The correct thing was to have the XMA seek table and use
	// that to determine the correct packet. This is just a hopeful
	// nearby approximation. Music did not have the seek table at
	// the time of this code. The Seek table was added for vo
	// restoration later.
	return m_LastDataOffset;
}

void CAudioMixerWaveXMA::SetPositionFromSaved( int savedPosition )
{
	// Not used here. The Mixer creation will be given the initial startup offset.
}

//-----------------------------------------------------------------------------
// Purpose: Abstract factory function for XMA mixers
//-----------------------------------------------------------------------------
CAudioMixer *CreateXMAMixer( IWaveData *data, int initialStreamPosition, int skipInitialSamples, bool bUpdateDelayForChoreo )
{
	return new CAudioMixerWaveXMA( data, initialStreamPosition, skipInitialSamples, bUpdateDelayForChoreo );
}
