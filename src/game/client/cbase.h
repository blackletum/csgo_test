﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CBASE_H
#define CBASE_H
#ifdef _WIN32
#pragma once
#endif

struct studiohdr_t;

#include <stdio.h>
#include <stdlib.h>

#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "vprof.h"

#include "tier1/strtools.h"
#include "vstdlib/random.h"
#include "tier1/utlvector.h"

#include "const.h"

#include "string_t.h"

// Steam uses C-runtime calls in headers, need to remap
#ifdef strncpy
#undef strncpy
#define strncpy Q_strncpy
#endif
#ifdef _snprintf
#undef _snprintf
#define _snprintf Q_snprintf
#endif

// These two have to be included very early
#include "predictableid.h"
#include "predictable_entity.h"

#include "cdll_util.h"
#include "util_shared.h"

#include "icvar.h"
#include "sharedvar.h"
#include "baseentity_shared.h"


// This is a precompiled header.  Include a bunch of common stuff.
// This is kind of ugly in that it adds a bunch of dependency where it isn't needed.
// But on balance, the compile time is much lower (even incrementally) once the precompiled
// headers contain these headers.
#include "precache_register.h"
#include "c_basecombatweapon.h"
#include "c_basecombatcharacter.h"
#include "gamerules.h"
#include "c_baseplayer.h"
#include "itempents.h"
#include "vphysics_interface.h"
//#include "vphysics2_interface.h"
#include "physics.h"
#include "c_recipientfilter.h"
#include "cdll_client_int.h"
#include "worldsize.h"
#include "engine/ivmodelinfo.h"
#include "npcevent.h"
#include "debugoverlay_shared.h"

#endif // CBASE_H
