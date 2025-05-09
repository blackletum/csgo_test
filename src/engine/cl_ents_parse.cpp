﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Parsing of entity network packets.
//
// $NoKeywords: $
//=============================================================================//


#include "client_pch.h"
#include "con_nprint.h"
#include "iprediction.h"
#include "cl_entityreport.h"
#include "dt_recv_eng.h"
#include "net_synctags.h"
#include "ispatialpartitioninternal.h"
#include "LocalNetworkBackdoor.h"
#include "basehandle.h"
#include "dt_localtransfer.h"
#include "iprediction.h"
#include "netmessages.h"
#include "ents_shared.h"
#include "cl_ents_parse.h"
#include "serializedentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar cl_flushentitypacket("cl_flushentitypacket", "0", FCVAR_CHEAT, "For debugging. Force the engine to flush an entity packet.");
extern ConVar replay_debug;
// Prints important entity creation/deletion events to console
#if defined( _DEBUG )
static ConVar cl_deltatrace( "cl_deltatrace", "0", 0, "For debugging, print entity creation/deletion info to console." );
#define TRACE_DELTA( text ) if ( cl_deltatrace.GetInt() ) { ConMsg( "%s", text ); };
#else
#define TRACE_DELTA( funcs )
#endif



//-----------------------------------------------------------------------------
// Debug networking stuff.
//-----------------------------------------------------------------------------


// #define DEBUG_NETWORKING 1

#if defined( DEBUG_NETWORKING )
void SpewToFile( char const* pFmt, ... );
static ConVar cl_packettrace( "cl_packettrace", "1", 0, "For debugging, massive spew to file." );
#define TRACE_PACKET( text ) if ( cl_packettrace.GetInt() ) { SpewToFile text ; };
#else
#define TRACE_PACKET( text )
#endif


#if defined( DEBUG_NETWORKING )

//-----------------------------------------------------------------------------
// Opens the recording file
//-----------------------------------------------------------------------------

static FileHandle_t OpenRecordingFile()
{
	FileHandle_t fp = 0;
	static bool s_CantOpenFile = false;
	static bool s_NeverOpened = true;
	if (!s_CantOpenFile)
	{
		fp = g_pFileSystem->Open( "cltrace.txt", s_NeverOpened ? "wt" : "at" );
		if (!fp)
		{
			s_CantOpenFile = true;			
		}
		s_NeverOpened = false;
	}
	return fp;
}

//-----------------------------------------------------------------------------
// Records an argument for a command, flushes when the command is done
//-----------------------------------------------------------------------------

void SpewToFile( char const* pFmt, ... )
{
	static CUtlVector<unsigned char> s_RecordingBuffer;

	char temp[2048];
	va_list args;

	va_start( args, pFmt );
	int len = Q_vsnprintf( temp, sizeof( temp ), pFmt, args );
	va_end( args );
	assert( len < 2048 );

	int idx = s_RecordingBuffer.AddMultipleToTail( len );
	memcpy( &s_RecordingBuffer[idx], temp, len );
	if ( 1 ) //s_RecordingBuffer.Size() > 8192)
	{
		FileHandle_t fp = OpenRecordingFile();
		g_pFileSystem->Write( s_RecordingBuffer.Base(), s_RecordingBuffer.Size(), fp );
		g_pFileSystem->Close( fp );

		s_RecordingBuffer.RemoveAll();
	}
}

#endif // DEBUG_NETWORKING

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Frees the client DLL's binding to the object.
// Input  : iEnt - 
//-----------------------------------------------------------------------------
void CL_DeleteDLLEntity( int iEnt, char *reason, bool bOnRecreatingAllEntities )
{
	IClientNetworkable *pNet = entitylist->GetClientNetworkable( iEnt );

	if ( pNet )
	{
		ClientClass *pClientClass = pNet->GetClientClass();
		TRACE_DELTA( va( "Trace %i (%s): delete (%s)\n", iEnt, pClientClass ? pClientClass->m_pNetworkName : "unknown", reason ) );

		CL_RecordDeleteEntity( iEnt, pClientClass );

		if ( bOnRecreatingAllEntities )
		{
			pNet->SetDestroyedOnRecreateEntities();
		}

		pNet->Release();
		Assert( !entitylist->GetClientNetworkable( iEnt ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Has the client DLL allocate its data for the object.
// Input  : iEnt - 
//			iClass - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
IClientNetworkable* CL_CreateDLLEntity( int iEnt, int iClass, int iSerialNum )
{
#if defined( _DEBUG )
	IClientNetworkable *pOldNetworkable = entitylist->GetClientNetworkable( iEnt );
	Assert( !pOldNetworkable );
#endif

	ClientClass *pClientClass;
	if ( ( pClientClass = GetBaseLocalClient().m_pServerClasses[iClass].m_pClientClass ) != NULL )
	{
		TRACE_DELTA( va( "Trace %i (%s): create\n", iEnt, pClientClass->m_pNetworkName ) );

		CL_RecordAddEntity( iEnt );

		if ( !GetBaseLocalClient().IsActive() )
		{
			COM_TimestampedLog( "cl:  create '%s'", pClientClass->m_pNetworkName );
			EngineVGui()->UpdateProgressBar( PROGRESS_CREATEENTITIES ); 				
		}

		// Create the entity.
		return pClientClass->m_pCreateFn( iEnt, iSerialNum );
	}

	Assert(false);
	return NULL;
}

void	SpewBitStream( unsigned char* pMem, int bit, int lastbit )
{
	int val = 0;
	char buf[1024];
	char* pTemp = buf;
	int bitcount = 0;
	int charIdx = 1;
	while( bit < lastbit )
	{
		int byte = bit >> 3;
		int bytebit = bit & 0x7;

		val |= ((pMem[byte] & bytebit) != 0) << bitcount;

		++bit;
		++bitcount;

		if (bitcount == 4)
		{
			if ((val >= 0) && (val <= 9))
				pTemp[charIdx] = '0' + val;
			else
				pTemp[charIdx] = 'A' + val - 0xA;
			if (charIdx == 1)
				charIdx = 0;
			else
			{
				charIdx = 1;
				pTemp += 2;
			}
			bitcount = 0;
			val = 0;
		}
	}
	if ((bitcount != 0) || (charIdx != 0))
	{
		if (bitcount > 0)
		{
			if ((val >= 0) && (val <= 9))
				pTemp[charIdx] = '0' + val;
			else
				pTemp[charIdx] = 'A' + val - 0xA;
		}
		if (charIdx == 1)
		{
			pTemp[0] = '0';
		}
		pTemp += 2;
	}
	pTemp[0] = '\0';

	TRACE_PACKET(( "    CL Bitstream %s\n", buf ));
}


inline static void CL_AddPostDataUpdateCall( CEntityReadInfo &u, int iEnt, DataUpdateType_t updateType )
{
	ErrorIfNot( u.m_nPostDataUpdateCalls < MAX_EDICTS,
		("CL_AddPostDataUpdateCall: overflowed u.m_PostDataUpdateCalls") );

	u.m_PostDataUpdateCalls[u.m_nPostDataUpdateCalls].m_iEnt = iEnt;
	u.m_PostDataUpdateCalls[u.m_nPostDataUpdateCalls].m_UpdateType = updateType;
	++u.m_nPostDataUpdateCalls;
}




//-----------------------------------------------------------------------------
// Purpose: Get the receive table for the specified entity
// Input  : *pEnt - 
// Output : RecvTable*
//-----------------------------------------------------------------------------
static inline RecvTable* GetEntRecvTable( int entnum )
{
	IClientNetworkable *pNet = entitylist->GetClientNetworkable( entnum );
	if ( pNet )
		return pNet->GetClientClass()->m_pRecvTable;
	else
		return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the entity index corresponds to a player slot 
// Input  : index - 
// Output : bool
//-----------------------------------------------------------------------------
static inline bool CL_IsPlayerIndex( int index )
{
	return ( index >= 1 && index <= GetBaseLocalClient().m_nMaxClients );
}


//-----------------------------------------------------------------------------
// Purpose: Bad data was received, just discards incoming delta data.
//-----------------------------------------------------------------------------
void CL_FlushEntityPacket( CClientFrame *packet, char const *errorString, ... )
{
	con_nprint_t np;
	char str[2048];
	va_list marker;

	// Spit out an error.
	va_start(marker, errorString);
	Q_vsnprintf(str, sizeof(str), errorString, marker);
	va_end(marker);
	
	ConMsg("%s", str);

	np.fixed_width_font = false;
	np.time_to_live = 1.0;
	np.index = 0;
	np.color[ 0 ] = 1.0;
	np.color[ 1 ] = 0.2;
	np.color[ 2 ] = 0.0;
	Con_NXPrintf( &np, "WARNING:  CL_FlushEntityPacket, %s", str );

	// Free packet memory.
	GetBaseLocalClient().DeleteUnusedClientFrame( packet );
}


// ----------------------------------------------------------------------------- //
// Regular handles for ReadPacketEntities.
// ----------------------------------------------------------------------------- //

void CL_CopyNewEntity( 
	CEntityReadInfo &u,
	int iClass,
	int iSerialNum
	)
{
	if ( u.m_nNewEntity < 0 || u.m_nNewEntity >= MAX_EDICTS )
	{
		Host_Error ("CL_CopyNewEntity: m_nNewEntity >= MAX_EDICTS");
		return;
	}

	// If it's new, make sure we have a slot for it.
	IClientNetworkable *ent = entitylist->GetClientNetworkable( u.m_nNewEntity );

	if( iClass >= GetBaseLocalClient().m_nServerClasses )
	{
		Host_Error("CL_CopyNewEntity: invalid class index (%d).\n", iClass);
		return;
	}

	// Delete the entity.
	ClientClass *pClass = GetBaseLocalClient().m_pServerClasses[iClass].m_pClientClass;
	bool bNew = false;
	if ( ent )
	{
		// if serial number is different, destory old entity
		if ( ent->GetIClientUnknown()->GetRefEHandle().GetSerialNumber() != iSerialNum )
		{
			CL_DeleteDLLEntity( u.m_nNewEntity, "CopyNewEntity" );
			ent = NULL; // force a recreate
		}
	}

	if ( !ent )
	{	
		// Ok, it doesn't exist yet, therefore this is not an "entered PVS" message.
		ent = CL_CreateDLLEntity( u.m_nNewEntity, iClass, iSerialNum );
		if( !ent )
		{
			Host_Error( "CL_ParsePacketEntities:  Error creating entity %s(%i)\n", GetBaseLocalClient().m_pServerClasses[iClass].m_pClientClass->m_pNetworkName, u.m_nNewEntity );
			return;
		}

		bNew = true;
	}

	int start_bit = u.m_pBuf->GetNumBitsRead();

	DataUpdateType_t updateType = bNew ? DATA_UPDATE_CREATED : DATA_UPDATE_DATATABLE_CHANGED;
	ent->PreDataUpdate( updateType );

	SerializedEntityHandle_t oldbaseline = SERIALIZED_ENTITY_HANDLE_INVALID;

	// Get either the static or instance baseline.
	PackedEntity *baseline = u.m_bAsDelta ? GetBaseLocalClient().GetEntityBaseline( u.m_nBaseline, u.m_nNewEntity ) : NULL;
	if ( baseline && baseline->m_pClientClass == pClass )
	{
		oldbaseline = baseline->GetPackedData();
	}
	else
	{
		// Every entity must have a static or an instance baseline when we get here.
		ErrorIfNot(
			GetBaseLocalClient().GetClassBaseline( iClass, &oldbaseline ),
			("CL_CopyNewEntity: GetClassBaseline(%d) failed.", iClass)
		);
	}

	RecvTable *pRecvTable = GetEntRecvTable( u.m_nNewEntity );
	if( !pRecvTable )
	{
		Host_Error( "CL_ParseDelta: invalid recv table for ent %d.\n", u.m_nNewEntity );
	}
	else
	{
		RecvTable_ReadFieldList( pRecvTable, *u.m_pBuf, u.m_DecodeEntity, -1, true );

		if ( u.m_bUpdateBaselines )
		{
			// store this baseline in u.m_pUpdateBaselines
			SerializedEntityHandle_t newbaseline = g_pSerializedEntities->AllocateSerializedEntity( __FILE__, __LINE__ );

			RecvTable_MergeDeltas( pRecvTable, oldbaseline, u.m_DecodeEntity, newbaseline, -1, NULL );

			// set the other baseline
			GetBaseLocalClient().SetEntityBaseline( (u.m_nBaseline==0)?1:0, pClass, u.m_nNewEntity, newbaseline );


			RecvTable_Decode( pRecvTable, ent->GetDataTableBasePtr(), newbaseline, u.m_nNewEntity );

		}
		else
		{
			// write data from baseline into entity
			RecvTable_Decode( pRecvTable, ent->GetDataTableBasePtr(), oldbaseline, u.m_nNewEntity );

			// Now parse in the contents of the network stream.
			RecvTable_Decode( pRecvTable, ent->GetDataTableBasePtr(), u.m_DecodeEntity, u.m_nNewEntity );
		}
	}

	CL_AddPostDataUpdateCall( u, u.m_nNewEntity, updateType );

	// If ent doesn't think it's in PVS, signal that it is
	Assert( u.m_pTo->last_entity <= u.m_nNewEntity );
	u.m_pTo->last_entity = u.m_nNewEntity;
	Assert( !u.m_pTo->transmit_entity.Get(u.m_nNewEntity) );
	u.m_pTo->transmit_entity.Set( u.m_nNewEntity );

	//
	// Net stats..
	//
	int bit_count = u.m_pBuf->GetNumBitsRead() - start_bit;

	if ( cl_entityreport.GetBool() )
		CL_RecordEntityBits( u.m_nNewEntity, bit_count );

	if ( CL_IsPlayerIndex( u.m_nNewEntity ) )
	{
		if ( u.m_nNewEntity == GetBaseLocalClient().m_nPlayerSlot + 1 )
		{
			u.m_nLocalPlayerBits += bit_count;
		}
		else
		{
			u.m_nOtherPlayerBits += bit_count;
		}
	}
}

void CL_PreserveExistingEntity( int nOldEntity )
{
	IClientNetworkable *pEnt = entitylist->GetClientNetworkable( nOldEntity );
	if ( !pEnt )
	{
		Host_Error( "CL_PreserveExistingEntity: missing client entity %d.\n", nOldEntity );
		return;
	}

	pEnt->OnDataUnchangedInPVS();
}

void CL_CopyExistingEntity( CEntityReadInfo &u )
{
	int start_bit = u.m_pBuf->GetNumBitsRead();

	IClientNetworkable *pEnt = entitylist->GetClientNetworkable( u.m_nNewEntity );
	if ( !pEnt )
	{
		Host_Error( "CL_CopyExistingEntity: missing client entity %d.\n", u.m_nNewEntity );
		return;
	}

	Assert( u.m_pFrom->transmit_entity.Get(u.m_nNewEntity) );

	// Read raw data from the network stream
	pEnt->PreDataUpdate( DATA_UPDATE_DATATABLE_CHANGED );

	RecvTable *pRecvTable = GetEntRecvTable( u.m_nNewEntity );

	if( !pRecvTable )
	{
		Host_Error( "CL_ParseDelta: invalid recv table for ent %d.\n", u.m_nNewEntity );
		return;
	}

	RecvTable_ReadFieldList( pRecvTable, *u.m_pBuf, u.m_DecodeEntity, u.m_nNewEntity, true );

	// BUG: sometimes we get DeltaEnt packets with zero fields in them, for C_WeaponCSBaseGun-derived entities...
	//      [CSGO, July 2 2012 - repro is to run a classic/hostage online listen server on X360]
	CSerializedEntity *pEntity; pEntity = reinterpret_cast< CSerializedEntity * >( u.m_DecodeEntity );
	//Assert( pEntity->GetFieldCount() > 0 );

	RecvTable_Decode( pRecvTable, pEnt->GetDataTableBasePtr(), u.m_DecodeEntity, u.m_nNewEntity );

	CL_AddPostDataUpdateCall( u, u.m_nNewEntity, DATA_UPDATE_DATATABLE_CHANGED );

	u.m_pTo->last_entity = u.m_nNewEntity;
	Assert( !u.m_pTo->transmit_entity.Get(u.m_nNewEntity) );
	u.m_pTo->transmit_entity.Set( u.m_nNewEntity );

	int bit_count = u.m_pBuf->GetNumBitsRead() - start_bit;

	if ( cl_entityreport.GetBool() )
		CL_RecordEntityBits( u.m_nNewEntity,  bit_count );

	if ( CL_IsPlayerIndex( u.m_nNewEntity ) )
	{
		if ( u.m_nNewEntity == GetBaseLocalClient().m_nPlayerSlot + 1 )
		{
			u.m_nLocalPlayerBits += bit_count;
		}
		else
		{
			u.m_nOtherPlayerBits += bit_count;
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_MarkEntitiesOutOfPVS( CBitVec<MAX_EDICTS> *pvs_flags )
{
	PREFETCH360(pvs_flags, 0);
	EntityCacheInfo_t *pInfo = entitylist->GetClientNetworkableArray();
	int entityMax = entitylist->GetHighestEntityIndex() + 1;
	// Note that we go up to and including the highest_index
	bool bReport = cl_entityreport.GetBool();
	for ( int i = 0; i < entityMax; i++ )
	{
#if defined( _X360 ) || defined( _PS3 )
		if ( !(i & 0xF) )
		{
			PREFETCH360(&pInfo[i], 128);
		}
#endif
		if ( !pInfo[i].m_pNetworkable )
			continue;

		// FIXME: We can remove IClientEntity here if we keep track of the
		// last frame's entity_in_pvs
		bool curstate = !pInfo[i].m_bDormant;
		bool newstate = pvs_flags->Get( i )  ? true : false;

		if ( !curstate && newstate )
		{
			// Inform the client entity list that the entity entered the PVS
			pInfo[i].m_pNetworkable->NotifyShouldTransmit( SHOULDTRANSMIT_START );
		}
		else if ( curstate && !newstate )
		{
			// Inform the client entity list that the entity left the PVS
			pInfo[i].m_pNetworkable->NotifyShouldTransmit( SHOULDTRANSMIT_END );
			if ( bReport )
			{
				CL_RecordLeavePVS( i );
			}
		}
	}
}

static void CL_CallPostDataUpdates( CEntityReadInfo &u )
{
	MDLCACHE_CRITICAL_SECTION_(g_pMDLCache);
	int saveSlot = splitscreen->SetActiveSplitScreenPlayerSlot( 0 );

	bool bSaveResolvable = splitscreen->SetLocalPlayerIsResolvable( __FILE__, __LINE__, false );
	for ( int i=0; i < u.m_nPostDataUpdateCalls; i++ )
	{
		CPostDataUpdateCall *pCall = &u.m_PostDataUpdateCalls[i];
	
		IClientNetworkable *pEnt = entitylist->GetClientNetworkable( pCall->m_iEnt );
		ErrorIfNot( pEnt, 
			("CL_CallPostDataUpdates: missing ent %d", pCall->m_iEnt) );

		pEnt->PostDataUpdate( pCall->m_UpdateType );
	}

	splitscreen->SetActiveSplitScreenPlayerSlot( saveSlot );
	splitscreen->SetLocalPlayerIsResolvable( __FILE__, __LINE__, bSaveResolvable );
}


//-----------------------------------------------------------------------------
// Purpose: An svc_packetentities has just been parsed, deal with the
//  rest of the data stream.  This can be a delta from the baseline or from a previous
//  client frame for this client.
// Input  : delta - 
//			*playerbits - 
// Output : void CL_ParsePacketEntities
//-----------------------------------------------------------------------------
bool CL_ProcessPacketEntities( const CSVCMsg_PacketEntities &msg )
{
	VPROF( "_CL_ParsePacketEntities" );

	// Packed entities for that frame
	// Allocate space for new packet info.
	CClientFrame *newFrame = GetBaseLocalClient().AllocateAndInitFrame( GetBaseLocalClient().GetServerTickCount() );
	CClientFrame *oldFrame = NULL;

	// if cl_flushentitypacket is set to N, the next N entity updates will be flushed
	if ( cl_flushentitypacket.GetInt() )
	{	
		// we can't use this, it is too old
		CL_FlushEntityPacket( newFrame, "Forced by cvar\n" );
		cl_flushentitypacket.SetValue( cl_flushentitypacket.GetInt() - 1 );	// Reduce the cvar.
		return false;
	}

	if ( msg.is_delta() )
	{
		if ( replay_debug.GetInt() > 10 && GetBaseLocalClient().m_nHltvReplayDelay )
			Msg( "Replay delta-%d update at tick %d, %s bytes\n", GetBaseLocalClient().GetServerTickCount() - msg.delta_from(), GetBaseLocalClient().GetServerTickCount(), V_pretifynum( msg.ByteSize() ) );

		if ( GetBaseLocalClient().GetServerTickCount() == msg.delta_from() )
		{
			Host_Error( "Update self-referencing, connection dropped.\n" );
			return false;
		}

		// Otherwise, mark where we are valid to and point to the packet entities we'll be updating from.
		oldFrame = GetBaseLocalClient().GetClientFrame( msg.delta_from() );

		if ( !oldFrame )
		{
			CL_FlushEntityPacket( newFrame, "Update delta not found.\n" );
			return false;
		}
	}
	else
	{	
		if ( replay_debug.GetBool() )
			Msg( "Full tick %d update\n", GetBaseLocalClient().GetServerTickCount() );
		if ( developer.GetInt() != 0 )
			ConColorMsg( Color( 255, 100, 255 ), "Receiving uncompressed update from server, baseline %d, byte size %d \n", msg.baseline(), msg.ByteSize() );

		// Clear out the client's entity states..
		for ( int i=0; i <= entitylist->GetHighestEntityIndex(); i++ )
		{
			CL_DeleteDLLEntity( i, "ProcessPacketEntities", true );
		}

		GetBaseLocalClient().FreeEntityBaselines();
		
		ClientDLL_FrameStageNotify( FRAME_NET_FULL_FRAME_UPDATE_ON_REMOVE );
	}

	// signal client DLL that we have started updating entities
	ClientDLL_FrameStageNotify( FRAME_NET_UPDATE_START );

	Assert( msg.baseline() >= 0 && msg.baseline() < 2 );

	if ( msg.update_baseline() )
	{
		// server requested to use this snapshot as baseline update
		int nUpdateBaseline = ( msg.baseline() == 0) ? 1 : 0;
		GetBaseLocalClient().CopyEntityBaseline( msg.baseline(), nUpdateBaseline );

		// send new baseline acknowledgement(as reliable)
		// Used a (brilliantly named) named temporary because SendNetMsg
		// takes a non-const reference because INetMessage::WriteToBuffer
		// is non-const.
		CCLCMsg_BaselineAck_t namedTemporary;
		namedTemporary.set_baseline_tick( GetBaseLocalClient().GetServerTickCount() );
		namedTemporary.set_baseline_nr( msg.baseline() );
		GetBaseLocalClient().m_NetChannel->SendNetMsg( namedTemporary, true );		
	}

	CEntityReadInfo u;
	bf_read entityBuf( &msg.entity_data()[0], msg.entity_data().size() );
	u.m_pBuf = &entityBuf;
	u.m_pFrom = oldFrame;
	u.m_pTo = newFrame;
	u.m_bAsDelta = msg.is_delta();
	u.m_nHeaderCount = msg.updated_entries();
	u.m_nBaseline = msg.baseline();
	u.m_bUpdateBaselines = msg.update_baseline();
	
	// update the entities
	{
		int saveSlot = splitscreen->SetActiveSplitScreenPlayerSlot( 0 );
		bool bSaveResolvable = splitscreen->SetLocalPlayerIsResolvable( __FILE__, __LINE__, false );

		GetBaseLocalClient().ReadPacketEntities( u );

		splitscreen->SetActiveSplitScreenPlayerSlot( saveSlot );
		splitscreen->SetLocalPlayerIsResolvable( __FILE__, __LINE__, bSaveResolvable );
	}

	ClientDLL_FrameStageNotify( FRAME_NET_UPDATE_POSTDATAUPDATE_START );

	// call PostDataUpdate() for each entity
	CL_CallPostDataUpdates( u );

	ClientDLL_FrameStageNotify( FRAME_NET_UPDATE_POSTDATAUPDATE_END );

	// call NotifyShouldTransmit() for entities that entered or left the PVS
	CL_MarkEntitiesOutOfPVS( &newFrame->transmit_entity );

	// adjust net channel stats

	GetBaseLocalClient().m_NetChannel->UpdateMessageStats( INetChannelInfo::LOCALPLAYER, u.m_nLocalPlayerBits );
	GetBaseLocalClient().m_NetChannel->UpdateMessageStats( INetChannelInfo::OTHERPLAYERS, u.m_nOtherPlayerBits );
	GetBaseLocalClient().m_NetChannel->UpdateMessageStats( INetChannelInfo::ENTITIES, -(u.m_nLocalPlayerBits+u.m_nOtherPlayerBits) );

 	GetBaseLocalClient().DeleteClientFrames( msg.delta_from() );

	// If the client has more than 64 frames, the host will start to eat too much memory.
	// TODO: We should enforce this somehow.
	if ( MAX_CLIENT_FRAMES < GetBaseLocalClient().AddClientFrame( newFrame ) )
	{
		DevMsg( 1, "CL_ProcessPacketEntities: frame window too big (>%i)\n", MAX_CLIENT_FRAMES );	
	}

	// all update activities are finished
	ClientDLL_FrameStageNotify( FRAME_NET_UPDATE_END );

	return true;
}

/*
==================
CL_PreprocessEntities

Server information pertaining to this client only
==================
*/
namespace CDebugOverlay
{
	extern void PurgeServerOverlays( void );
}

void CL_PreprocessEntities( void )
{
	// Zero latency!!! (single player or listen server?)
	bool bIsUsingMultiplayerNetworking = NET_IsMultiplayer();
	bool bLastOutgoingCommandEqualsLastAcknowledgedCommand = GetBaseLocalClient().lastoutgoingcommand == GetBaseLocalClient().command_ack;

	// We always want to re-run prediction when using the multiplayer networking, or if we're the listen server and we get a packet
	//  before any frames have run
	if ( bIsUsingMultiplayerNetworking ||
		bLastOutgoingCommandEqualsLastAcknowledgedCommand )
	{
		//Msg( "%i/%i CL_ParseClientdata:  no latency server ack %i\n", 
		//	host_framecount, GetBaseLocalClient1().tickcount,
		//	command_ack );
		CL_RunPrediction( PREDICTION_SIMULATION_RESULTS_ARRIVING_ON_SEND_FRAME );
	}

	// Copy some results from prediction back into right spot
	// Anything not sent over the network from server to client must be specified here.
	//if ( GetBaseLocalClient().last_command_ack  )
	{
		int number_of_commands_executed = ( GetBaseLocalClient().command_ack - GetBaseLocalClient().last_command_ack );
		int number_of_simulation_ticks = ( GetBaseLocalClient().GetServerTickCount() - GetBaseLocalClient().last_server_tick );

#if 0
		COM_Log( "GetBaseLocalClient().log", "Receiving frame acknowledging %i commands\n",
			number_of_commands_executed );

		COM_Log( "GetBaseLocalClient().log", "  last command number executed %i\n",
			GetBaseLocalClient().command_ack );

		COM_Log( "GetBaseLocalClient().log", "  previous last command number executed %i\n",
			GetBaseLocalClient().last_command_ack );

		COM_Log( "GetBaseLocalClient().log", "  current world frame %i\n",
			GetBaseLocalClient().m_nCurrentSequence );
#endif

		// Copy last set of changes right into current frame.
		g_pClientSidePrediction->PreEntityPacketReceived( number_of_commands_executed, GetBaseLocalClient().m_nCurrentSequence, number_of_simulation_ticks );
	}

	CDebugOverlay::PurgeServerOverlays();
}





