﻿//====== Copyright (c), Valve Corporation, All rights reserved. =======
//
// Purpose: includes all the headers required for the cliend side of the GC 
//			SDK GC SDK. Include this in your stdafx.h
//
//=============================================================================

#ifndef GCCLIENTSDK_H
#define GCCLIENTSDK_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "steam/steamtypes.h"

#include "tier0/dbg.h"
#include "tier0/vprof.h"
#include "tier0/fasttimer.h"
#include "tier0/t0constants.h"

#include "tier1/utlmap.h"
#include "tier1/utllinkedlist.h"
#include "tier1/utlpriorityqueue.h"
#include "tier1/utlstring.h"
#include "tier1/utlbuffer.h"
#include "tier1/mempool.h"
#include "tier1/tsmempool.h"
#include "tier1/tsmultimempool.h"
#include "tier1/checksum_crc.h"
#include "tier1/fmtstr.h"

#include "vstdlib/coroutine.h"

// public stuff
#include "steam/steamclientpublic.h"
#include "steam/isteamclient.h"
#include "steam/isteamgamecoordinator.h"
#include "steam/steam_api.h"

// stuff to include early because it is widely depended on
#include "netpacket.h"
#include "gcmsg.h"
#include "msgprotobuf.h"
#include "gcconstants.h"
#include "refcount.h"

#include "jobtime.h"
#include "messagelist.h"
#include "gclogger.h"
#include "job.h"
#include "jobmgr.h"
#include "netpacketpool.h"
#include "soid.h"
#include "sharedobject.h"
#include "protobufsharedobject.h"
#include "sharedobjectcache.h"
#include "gcclient_sharedobjectcache.h"
#include "gcclient.h"
#include "gcclientjob.h"

// this is a hack to make sure that the funny Valve override of 
// offsetof survives the STL includes that happened because of the
// protobuf includes above.
#ifdef LINUX
#undef offsetof
#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#endif

#endif // GCCLIENTSDK_H
