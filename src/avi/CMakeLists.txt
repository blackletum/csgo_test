#//-----------------------------------------------------------------------------
#//	VALVE_AVI.VPC
#//
#//	Project Script
#//
#// MGP 4/010 Updated to support Quicktime on OSX.  Win32 builds can optionally
#//   be enabled for cross platform work if quickTime is installed on the Win32
#//   PC.
#//-----------------------------------------------------------------------------

#TODO: Not Done. Moving to the tier libraries first and following the build-order -lwss

cmake_minimum_required(VERSION 3.16)
project(valve_avi_client)
set(SRCDIR "${CMAKE_SOURCE_DIR}")
set(OUTBINNAME "valve_avi_client")
set(CMAKE_MODULE_PATH ${SRCDIR}/cmake)
set(OUTBINDIR "${SRCDIR}/../game/bin")

include(${CMAKE_MODULE_PATH}/detect_platform.cmake)
include(${CMAKE_MODULE_PATH}/source_dll_base.cmake)
#include("${CMAKE_MODULE_PATH}/source_video_base.cmake") already included in source_bases.

#if( QUICKTIME_WINDOWS )
    #$AdditionalIncludeDirectories	"$BASE,..\common\quicktime_win32\"		[$QUICKTIME_WINDOWS]
#endif()
#if( WINDOWS )
    #$AdditionalIncludeDirectories	"$BASE;$SRCDIR\dx9sdk\include"			[$WINDOWS]
#endif()

#Compiler
#if( LINUXALL )
    #these dont exist... Seems to work... Commented out
    #include_directories(${SRCDIR}/thirdparty/sdl/include)
    #include_directories(${SRCDIR}/thirdparty/sdl_mixer)
#endif()

#linker
if( LINUXALL )
    #target_link_options(${OUTBINNAME} PRIVATE "-z muldefs -L${SRCDIR}/thirdparty/sdl/build/.libs")
    target_link_libraries(${OUTBINNAME} SDL2)
endif()

#source files
if( WIN32 )
    target_sources(${OUTBINNAME} PRIVATE "avi.cpp")
endif()
if( OSXALL )
    target_sources(${OUTBINNAME} PRIVATE "avi_osx.cpp")
else()
    target_sources(${OUTBINNAME} PRIVATE "bink.cpp")
endif()

if( QUICKTIME_VIDEO OR QUICKTIME_WINDOWS )
    target_sources(${OUTBINNAME} PRIVATE "quicktime.cpp")
endif()

if( NOT OSXALL )
    target_link_libraries(${OUTBINNAME} vstdlib_client tier2_client tier3_client)
    if( LINUXALL )
        #target_link_libraries(${OUTBINNAME} ${SRCDIR}/lib/common/libSDL2_mixer.a)
        target_link_libraries(${OUTBINNAME} SDL2_mixer)
    endif()
	if( WIN32 )
		target_link_libraries(${OUTBINNAME} Vfw32.lib)
	endif()
endif()
