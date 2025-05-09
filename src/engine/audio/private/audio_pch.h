﻿//===== Copyright © 1996-2007, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//

#include "platform.h"

#if !defined( _X360 ) && defined( WIN32 )
#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 1)
#pragma warning(disable: 4005)
#include <windows.h>
#include <mmsystem.h>
#pragma warning(pop)
#include <mmreg.h>
#endif

#ifdef _PS3
#include "ps3/ps3_core.h"
#endif

#include "basetypes.h"
#include "commonmacros.h"
#include "mathlib/mathlib.h"
#include "tier0/dbg.h"
#include "tier0/vprof.h"
#include "tier0/icommandline.h"
#include "tier1/strtools.h"

#include "tier2/riff.h"
#include "sound.h"
#include "Color.h"
#include "convar.h"
#include "soundservice.h"
#include "voice_sound_engine_interface.h"
#include "soundflags.h"
#include "filesystem.h"
#include "../filesystem_engine.h"

#include "snd_device.h"
#include "sound_private.h"
#include "snd_mix_buf.h"
#include "snd_env_fx.h"
#include "snd_channels.h"
#include "snd_audio_source.h"
#include "snd_convars.h"
#include "snd_dev_common.h"
#include "snd_dev_direct.h"
#ifdef _PS3
#include "snd_dev_ps3audio.h"
#else
#include "snd_dev_xaudio.h"
#endif
#include "snd_sfx.h"
#include "snd_stubs.h"
#include "snd_audio_source.h"
#include "snd_wave_source.h"
#include "snd_wave_temp.h"
#include "snd_wave_data.h"
#include "snd_wave_mixer_private.h"
#include "snd_wave_mixer_adpcm.h"
#include "snd_io.h"
#include "voice_mixer_controls.h"

#if IsX360()
# include "snd_wave_mixer_xma.h"
# include "xbox/xbox_win32stubs.h"
# include <xhv2.h>
#elif IsPS3()
# include "snd_wave_mixer_ps3_mp3.h"
# include "ps3/ps3_win32stubs.h"
#elif defined(POSIX)
# include "audio/private/posix_stubs.h"
#endif
