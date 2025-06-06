cmake_minimum_required(VERSION 3.7.2)
project(SeriousEngine)

# Enable in order to build under the x86 platform
option(SE1_BUILD_X86 "Build 32-bit libraries and executables" OFF)

# Disable in order to reuse already compiled entity sources
option(SE1_COMPILE_ES "Compile entity source files (.es)" ON)

# Replace generated info about the current engine build with dummy values
option(SE1_DUMMY_BUILD_INFO "Generate dummy info about an engine build" OFF)

if(SE1_BUILD_X86)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif()

# Disable in order to remove OpenAL support from the engine
option(SE1_OPENAL_SUPPORT "Implement OpenAL sound API" ON)

# Don't use system libraries by default
option(USE_SYSTEM_SDL3 "Use systems SDL3 development files" OFF)
option(USE_SYSTEM_ZLIB "Use systems zlib development files" OFF)

option(USE_CCACHE "Set to ON to use ccache if present in the system" ${USE_CCACHE})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CheckCXXCompilerFlag)
include(ParserAndScanner)
include(SE1Setup)

# Start setting up the projects
if(DEBUG)
  set(DEBUGSUFFIX "D")
else()
  set(DEBUGSUFFIX "")
endif()

include_directories(
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/Mod"
  "${CMAKE_SOURCE_DIR}/ThirdParty"
  "${CMAKE_SOURCE_DIR}/ThirdParty/libogg/include"
  "${CMAKE_SOURCE_DIR}/ThirdParty/libvorbis/include"
  "${CMAKE_SOURCE_DIR}/ThirdParty/SDL/include"
)

# Build ECC from source if there isn't a prebuilt one specified on the command line
# Normally it is built here, but we might need a prebuilt native binary if we're cross-compiling the rest of the game
if(ECC)
  # Use given ECC
elseif(CMAKE_CROSSCOMPILING)
  message(FATAL_ERROR "ECC variable must point to native ecc binary when cross-compiling.")
else()
  add_subdirectory(ThirdParty/ECC)
  set(ECC ecc)
endif()

macro(entity _LIST _NAME)
  if(SE1_COMPILE_ES)
    add_custom_command(
      OUTPUT "${CMAKE_SOURCE_DIR}/${_NAME}.cpp" "${CMAKE_SOURCE_DIR}/${_NAME}.h" "${CMAKE_SOURCE_DIR}/${_NAME}_tables.h"
      MAIN_DEPENDENCY "${CMAKE_SOURCE_DIR}/${_NAME}.es"
      DEPENDS ${ECC}
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      COMMAND ${ECC}
      ARGS "${_NAME}.es" "-cfg" "ecc-common.inl"
    )
  endif()

  list(APPEND ${_LIST}_CPP "${CMAKE_SOURCE_DIR}/${_NAME}.cpp")
  list(APPEND ${_LIST}_H   "${CMAKE_SOURCE_DIR}/${_NAME}.h")
  list(APPEND ${_LIST}_H   "${CMAKE_SOURCE_DIR}/${_NAME}_tables.h")
endmacro()

# Projects
add_subdirectory(ThirdParty)
add_subdirectory(Engine)

set(ENTITIESLIB "Entities${DEBUGSUFFIX}")
add_subdirectory(Mod/Entities)

set(GAMELIB "Game${DEBUGSUFFIX}")
add_subdirectory(Mod/Game)

set(SHADERSLIB "Shaders${DEBUGSUFFIX}")
add_subdirectory(Shaders)

add_subdirectory(DedicatedServer)
add_subdirectory(SeriousSam)

# Install executable files
if(DEBUG)
  install(TARGETS DedicatedServer SeriousSam
          RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin/Debug"
          LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin/Debug"
          ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin/Debug"
          PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
else()
  install(TARGETS DedicatedServer SeriousSam
          RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
          LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
          ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/Bin"
          PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

# If CMAKE_INSTALL_PREFIX is set to another directory, SE1_10.gro needs to be copied there
if(NOT LOCAL_INSTALL)
  install(FILES "${CMAKE_SOURCE_DIR}/../SE1_10.gro"
          DESTINATION "${CMAKE_INSTALL_PREFIX}"
          PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()
