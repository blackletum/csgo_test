#Generated by VPC

NAME=FileSystem_Stdio
TARGET_PLATFORM=linux64
TARGET_PLATFORM_EXT=_client
VPC_HOST=linux

SRCROOT=..
LINUX_MAKEFILE=1
USE_VALVE_BINDIR=0
# If no configuration is specified, "release" will be used.
CFG ?= release




















################################################################################
#
# CFG=debug
#
################################################################################

ifeq "$(CFG)" "debug"

GCC_ExtraCompilerFlags=
GCC_ExtraCxxCompilerFlags=
GCC_ExtraLinkerFlags=
DisableLinkerDeadCodeElimination=
SymbolVisibility=hidden
TreatWarningsAsErrors=false
OptimizerLevel=-gdwarf-4 -g2 $(OptimizerLevel_CompilerSpecific)
PrecompiledHeaderInclude=
SystemLibraries=
DLL_EXT=_client.so
SYM_EXT=.dbg
DEVTOOLS := ../devtools
MAKEFILE_BASE := $(DEVTOOLS)/makefile_base_posix.mak
COMPILE_DEPENDANT_MAKEFILES := filesystem_stdio_linux64.mak $(MAKEFILE_BASE)
FORCEINCLUDES= 
DEFINES += -DVPC -DCSTRIKE_REL_BUILD=1 -DRAD_TELEMETRY_DISABLED -DDEBUG -D_DEBUG -DGNUC -DPOSIX -DCOMPILER_GCC -DMEMOVERRIDE_MODULE=filesystem_stdio -D_DLL_EXT=_client.so -D_LINUX -DLINUX -DPOSIX -D_POSIX -DDLLNAME=filesystem_stdio -DGL_GLEXT_PROTOTYPES -DDX_TO_GL_ABSTRACTION -DUSE_SDL -DCSTRIKE15 -DFILESYSTEM_STDIO_EXPORTS -DDONT_PROTECT_FILEIO_FUNCTIONS -DPROTECTED_THINGS_ENABLE -D_USE_32BIT_TIME_T -DSUPPORT_PACKED_STORE -D_DLL_EXT=_client.so -D_DLL_PREFIX=lib -D_EXTERNAL_DLL_EXT=.so -D_LINUX=1 -D_LINUXSTEAMRT64=1 -D_POSIX=1 -DLINUX=1 -DLINUX64=1 -DLINUXSTEAMRT64=1 -DPOSIX=1 -DPROJECTDIR=/home/csgo-src/src/filesystem -DVPCGAME=csgo -DVPCGAMECAPS=CSGO 
INCLUDEDIRS += ../common ../public ../public/tier0 ../public/tier1 ../thirdparty/SDL2 
CONFTYPE=dll
IMPORTLIBRARY=
OUTPUTFILE=$(OBJ_DIR)/filesystem_stdio_client.so
GAMEOUTPUTFILE=../../game/bin/linux64/filesystem_stdio_client.so

CPPFILES= \
    ../public/tier0/memoverride.cpp \
    basefilesystem.cpp \
    packfile.cpp \
    filetracker.cpp \
    filesystem_async.cpp \
    filesystem_stdio.cpp \
    ../public/kevvaluescompiler.cpp \
    ../public/zip_utils.cpp \
    QueuedLoader.cpp \
    linux_support.cpp

unexport CPPFILES

LIBFILES = \
    ../lib/public/linux64/tier1_client.a \
    ../lib/public/linux64/interfaces_client.a \
    ../lib/public/linux64/tier2_client.a \
    ../lib/public/linux64/vpklib_client.a \
    -L../lib/public/linux64 -ltier0_client \
    -L../lib/public/linux64 -lvstdlib_client

unexport LIBFILES

LIBFILENAMES = \
    ../lib/public/linux64/libtier0_client.so \
    ../lib/public/linux64/tier1_client.a \
    ../lib/public/linux64/interfaces_client.a \
    ../lib/public/linux64/libvstdlib_client.so \
    ../lib/public/linux64/tier2_client.a \
    ../lib/public/linux64/vpklib_client.a

unexport LIBFILENAMES

ALL_CUSTOM_BUILD_TOOLS =

unexport ALL_CUSTOM_BUILD_TOOLS

ALL_CUSTOM_BUILD_TOOL_DEPENDENCIES =

unexport ALL_CUSTOM_BUILD_TOOL_DEPENDENCIES

ALL_CUSTOM_BUILD_TOOL_SCRIPTS =

unexport ALL_CUSTOM_BUILD_TOOL_SCRIPTS

ALL_CUSTOM_BUILD_TOOL_OUTPUTS =

unexport ALL_CUSTOM_BUILD_TOOL_OUTPUTS

ALL_DEPGEN_FILES = \
    $(OBJ_DIR)/memoverride.P \
    $(OBJ_DIR)/basefilesystem.P \
    $(OBJ_DIR)/packfile.P \
    $(OBJ_DIR)/filetracker.P \
    $(OBJ_DIR)/filesystem_async.P \
    $(OBJ_DIR)/filesystem_stdio.P \
    $(OBJ_DIR)/kevvaluescompiler.P \
    $(OBJ_DIR)/zip_utils.P \
    $(OBJ_DIR)/QueuedLoader.P \
    $(OBJ_DIR)/linux_support.P

unexport ALL_DEPGEN_FILES


#
# Include the base makefile now.
#
include $(MAKEFILE_BASE)


#
# Create sentinel directories
#
$(OBJ_DIR)/_vpc_generated_scripts/custom_build_tools/_vpc_create_dirs: filesystem_stdio_linux64.mak | $(OBJ_DIR)/_create_dir
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) create custom build tools dir start
	$(QUIET_PREFIX) $(MKDIR) -p $(OBJ_DIR)/_vpc_generated_scripts/custom_build_tools/locks $(QUIET_ECHO_POSTFIX)
	$(QUIET_PREFIX) $(MKDIR) -p $(OBJ_DIR)/_vpc_generated_scripts/custom_build_tools/ranthisbuild $(QUIET_ECHO_POSTFIX)
	$(QUIET_PREFIX) $(MKDIR) -p $(OBJ_DIR)/_vpc_generated_scripts/custom_build_tools/sentinels $(QUIET_ECHO_POSTFIX)
	$(QUIET_PREFIX) $(TOUCH) $(OBJ_DIR)/_vpc_generated_scripts/custom_build_tools/_vpc_create_dirs


#
# include all the depgen .P files which will include dependency information.
#
include $(wildcard $(OBJ_DIR)/memoverride.P)
include $(wildcard $(OBJ_DIR)/basefilesystem.P)
include $(wildcard $(OBJ_DIR)/packfile.P)
include $(wildcard $(OBJ_DIR)/filetracker.P)
include $(wildcard $(OBJ_DIR)/filesystem_async.P)
include $(wildcard $(OBJ_DIR)/filesystem_stdio.P)
include $(wildcard $(OBJ_DIR)/kevvaluescompiler.P)
include $(wildcard $(OBJ_DIR)/zip_utils.P)
include $(wildcard $(OBJ_DIR)/QueuedLoader.P)
include $(wildcard $(OBJ_DIR)/linux_support.P)


#
# Compare/cache a string of compile settings to see if we need to recompile .o's
#
$(eval $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS,COMPILE_O_GLOBAL_SETTINGS,$$(call COMPILE_O_SETTINGS,$$(CFG_CPPFLAGS))))


#
# Compile rules
#
$(OBJ_DIR)/memoverride.o: ../public/tier0/memoverride.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/memoverride.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/memoverride.o

$(OBJ_DIR)/basefilesystem.o: basefilesystem.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/basefilesystem.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/basefilesystem.o

$(OBJ_DIR)/packfile.o: packfile.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/packfile.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/packfile.o

$(OBJ_DIR)/filetracker.o: filetracker.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/filetracker.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/filetracker.o

$(OBJ_DIR)/filesystem_async.o: filesystem_async.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/filesystem_async.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/filesystem_async.o

$(OBJ_DIR)/filesystem_stdio.o: filesystem_stdio.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/filesystem_stdio.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/filesystem_stdio.o

$(OBJ_DIR)/kevvaluescompiler.o: ../public/kevvaluescompiler.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/kevvaluescompiler.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/kevvaluescompiler.o

$(OBJ_DIR)/zip_utils.o: ../public/zip_utils.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/zip_utils.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/zip_utils.o

$(OBJ_DIR)/QueuedLoader.o: QueuedLoader.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/QueuedLoader.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/QueuedLoader.o

$(OBJ_DIR)/linux_support.o: linux_support.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/linux_support.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/linux_support.o

endif # (CFG=debug)




















################################################################################
#
# CFG=release
#
################################################################################

ifeq "$(CFG)" "release"

GCC_ExtraCompilerFlags=
GCC_ExtraCxxCompilerFlags=
GCC_ExtraLinkerFlags=
DisableLinkerDeadCodeElimination=
SymbolVisibility=hidden
TreatWarningsAsErrors=false
OptimizerLevel=-gdwarf-4 -g2 $(OptimizerLevel_CompilerSpecific)
PrecompiledHeaderInclude=
SystemLibraries=
DLL_EXT=_client.so
SYM_EXT=.dbg
DEVTOOLS := ../devtools
MAKEFILE_BASE := $(DEVTOOLS)/makefile_base_posix.mak
COMPILE_DEPENDANT_MAKEFILES := filesystem_stdio_linux64.mak $(MAKEFILE_BASE)
FORCEINCLUDES= 
DEFINES += -DVPC -DCSTRIKE_REL_BUILD=1 -DRAD_TELEMETRY_DISABLED -DNDEBUG -DGNUC -DPOSIX -DCOMPILER_GCC -DMEMOVERRIDE_MODULE=filesystem_stdio -D_DLL_EXT=_client.so -D_LINUX -DLINUX -DPOSIX -D_POSIX -DDLLNAME=filesystem_stdio -DGL_GLEXT_PROTOTYPES -DDX_TO_GL_ABSTRACTION -DUSE_SDL -DCSTRIKE15 -DFILESYSTEM_STDIO_EXPORTS -DDONT_PROTECT_FILEIO_FUNCTIONS -DPROTECTED_THINGS_ENABLE -D_USE_32BIT_TIME_T -DSUPPORT_PACKED_STORE -D_DLL_EXT=_client.so -D_DLL_PREFIX=lib -D_EXTERNAL_DLL_EXT=.so -D_LINUX=1 -D_LINUXSTEAMRT64=1 -D_POSIX=1 -DLINUX=1 -DLINUX64=1 -DLINUXSTEAMRT64=1 -DPOSIX=1 -DPROJECTDIR=/home/csgo-src/src/filesystem -DVPCGAME=csgo -DVPCGAMECAPS=CSGO 
INCLUDEDIRS += ../common ../public ../public/tier0 ../public/tier1 ../thirdparty/SDL2 
CONFTYPE=dll
IMPORTLIBRARY=
OUTPUTFILE=$(OBJ_DIR)/filesystem_stdio_client.so
GAMEOUTPUTFILE=../../game/bin/linux64/filesystem_stdio_client.so

CPPFILES= \
    ../public/tier0/memoverride.cpp \
    basefilesystem.cpp \
    packfile.cpp \
    filetracker.cpp \
    filesystem_async.cpp \
    filesystem_stdio.cpp \
    ../public/kevvaluescompiler.cpp \
    ../public/zip_utils.cpp \
    QueuedLoader.cpp \
    linux_support.cpp

unexport CPPFILES

LIBFILES = \
    ../lib/public/linux64/tier1_client.a \
    ../lib/public/linux64/interfaces_client.a \
    ../lib/public/linux64/tier2_client.a \
    ../lib/public/linux64/vpklib_client.a \
    -L../lib/public/linux64 -ltier0_client \
    -L../lib/public/linux64 -lvstdlib_client

unexport LIBFILES

LIBFILENAMES = \
    ../lib/public/linux64/libtier0_client.so \
    ../lib/public/linux64/tier1_client.a \
    ../lib/public/linux64/interfaces_client.a \
    ../lib/public/linux64/libvstdlib_client.so \
    ../lib/public/linux64/tier2_client.a \
    ../lib/public/linux64/vpklib_client.a

unexport LIBFILENAMES

ALL_CUSTOM_BUILD_TOOLS =

unexport ALL_CUSTOM_BUILD_TOOLS

ALL_CUSTOM_BUILD_TOOL_DEPENDENCIES =

unexport ALL_CUSTOM_BUILD_TOOL_DEPENDENCIES

ALL_CUSTOM_BUILD_TOOL_SCRIPTS =

unexport ALL_CUSTOM_BUILD_TOOL_SCRIPTS

ALL_CUSTOM_BUILD_TOOL_OUTPUTS =

unexport ALL_CUSTOM_BUILD_TOOL_OUTPUTS

ALL_DEPGEN_FILES = \
    $(OBJ_DIR)/memoverride.P \
    $(OBJ_DIR)/basefilesystem.P \
    $(OBJ_DIR)/packfile.P \
    $(OBJ_DIR)/filetracker.P \
    $(OBJ_DIR)/filesystem_async.P \
    $(OBJ_DIR)/filesystem_stdio.P \
    $(OBJ_DIR)/kevvaluescompiler.P \
    $(OBJ_DIR)/zip_utils.P \
    $(OBJ_DIR)/QueuedLoader.P \
    $(OBJ_DIR)/linux_support.P

unexport ALL_DEPGEN_FILES


#
# Include the base makefile now.
#
include $(MAKEFILE_BASE)


#
# Create sentinel directories
#
$(OBJ_DIR)/_vpc_generated_scripts/custom_build_tools/_vpc_create_dirs: filesystem_stdio_linux64.mak | $(OBJ_DIR)/_create_dir
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) create custom build tools dir start
	$(QUIET_PREFIX) $(MKDIR) -p $(OBJ_DIR)/_vpc_generated_scripts/custom_build_tools/locks $(QUIET_ECHO_POSTFIX)
	$(QUIET_PREFIX) $(MKDIR) -p $(OBJ_DIR)/_vpc_generated_scripts/custom_build_tools/ranthisbuild $(QUIET_ECHO_POSTFIX)
	$(QUIET_PREFIX) $(MKDIR) -p $(OBJ_DIR)/_vpc_generated_scripts/custom_build_tools/sentinels $(QUIET_ECHO_POSTFIX)
	$(QUIET_PREFIX) $(TOUCH) $(OBJ_DIR)/_vpc_generated_scripts/custom_build_tools/_vpc_create_dirs


#
# include all the depgen .P files which will include dependency information.
#
include $(wildcard $(OBJ_DIR)/memoverride.P)
include $(wildcard $(OBJ_DIR)/basefilesystem.P)
include $(wildcard $(OBJ_DIR)/packfile.P)
include $(wildcard $(OBJ_DIR)/filetracker.P)
include $(wildcard $(OBJ_DIR)/filesystem_async.P)
include $(wildcard $(OBJ_DIR)/filesystem_stdio.P)
include $(wildcard $(OBJ_DIR)/kevvaluescompiler.P)
include $(wildcard $(OBJ_DIR)/zip_utils.P)
include $(wildcard $(OBJ_DIR)/QueuedLoader.P)
include $(wildcard $(OBJ_DIR)/linux_support.P)


#
# Compare/cache a string of compile settings to see if we need to recompile .o's
#
$(eval $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS,COMPILE_O_GLOBAL_SETTINGS,$$(call COMPILE_O_SETTINGS,$$(CFG_CPPFLAGS))))


#
# Compile rules
#
$(OBJ_DIR)/memoverride.o: ../public/tier0/memoverride.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/memoverride.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/memoverride.o

$(OBJ_DIR)/basefilesystem.o: basefilesystem.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/basefilesystem.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/basefilesystem.o

$(OBJ_DIR)/packfile.o: packfile.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/packfile.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/packfile.o

$(OBJ_DIR)/filetracker.o: filetracker.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/filetracker.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/filetracker.o

$(OBJ_DIR)/filesystem_async.o: filesystem_async.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/filesystem_async.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/filesystem_async.o

$(OBJ_DIR)/filesystem_stdio.o: filesystem_stdio.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/filesystem_stdio.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/filesystem_stdio.o

$(OBJ_DIR)/kevvaluescompiler.o: ../public/kevvaluescompiler.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/kevvaluescompiler.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/kevvaluescompiler.o

$(OBJ_DIR)/zip_utils.o: ../public/zip_utils.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/zip_utils.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/zip_utils.o

$(OBJ_DIR)/QueuedLoader.o: QueuedLoader.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/QueuedLoader.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/QueuedLoader.o

$(OBJ_DIR)/linux_support.o: linux_support.cpp $(call DETECT_STRING_CHANGE_BETWEEN_BUILDS_TIMESTAMP_FILE,COMPILE_O_GLOBAL_SETTINGS) | _precompile_steps
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile start for file $(OBJ_DIR)/linux_support.o
	$(QUIET_PREFIX) $(call SPEW_UPDATED_DEPENDENCIES)
	$(QUIET_PREFIX) $(call RUN_PREBUILD_EVENT_ONCE)
	$(PRE_COMPILE_FILE)
	$(QUIET_PREFIX) $(MKDIR) -p "$(dir $@)"
	$(call COMPILE_FILE, $(CFG_CPPFLAGS)) $(POST_COMPILE_FILE)
	$(QUIET_PREFIX) $(BUILD_DEBUG_ECHO) COMPILE_FILE compile end for file $(OBJ_DIR)/linux_support.o

endif # (CFG=release)
