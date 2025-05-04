# Welcome to the Kisak-Strike options file
# Options here can be freely changed, however it is recommended to set the options via command line.
# Example: cmake .. -DUSE_ROCKETUI=1 -DUSE_IPHYS=1 -DDEDICATED=0

# UI Options
option(USE_ROCKETUI "Use New Open Source Kisak-Strike RmlUI" OFF)
option(USE_SCALEFORM "Use In-Complete Proprietary Flash UI with blob ( Not Recommended )" OFF)
# There is also a 3rd option, which is nothing. so it'll be defaulted to RocketUI

# Physics Options
option(USE_IPHYS "Use the Open Source Physics Re-Build made for linux-strike from various leaked sources" OFF)
# 2nd option is to have this OFF, the closed source blob from Valve will be used instead.

# Sound Options
option(USE_VALVE_HRTF "Use the Closed Source Valve Sound System" OFF)
# 2nd option is to have this OFF, the closed source blob from Valve will be used instead.

# DEDICATED Server
option(DEDICATED "Build as DEDICATED server. This is Separate from the main build and they are not in-tree compatible.
Make sure to build with -DDEDICATED=0 once you want a regular client again." OFF)
# 2nd option is to have this OFF, the client build will be built.

# Kisak-Strike Developer Options
# (Gamer Tip: use gdb command `b __asan::ReportGenericError` to break on ASAN errors)
option(USE_ASAN "Enable the Address Sanitizer GCC plugin, used for finding memory errors/bugs" OFF)
option(USE_TRACY "Enable Tracy Profiler support" OFF)
option(TRACY_STORE_LOGS "Turn off Tracy's On-Demand mode. With this flag the profiler will store logs and send them later when the UI connects. Consumes RAM quickly! Mainly useful for profiling the application startup." OFF)
option(RELEASE_ASSERTS "Enable asserts in release build" OFF)

#CMAKE_BUILD_TYPE is supported: RELEASE, DEBUG -- See source_posix_base.cmake for more compiler flags.
