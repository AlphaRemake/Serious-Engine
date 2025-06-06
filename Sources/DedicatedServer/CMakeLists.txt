cmake_minimum_required(VERSION 3.7.2)
project(DedicatedServer)

add_executable(DedicatedServer "DedicatedServer.cpp")
add_dependencies(DedicatedServer ${GAMELIB} Engine)

target_link_libraries(DedicatedServer Engine ${ENTITIESLIB} ${GAMELIB} ${SHADERSLIB})
#set_target_properties(DedicatedServer PROPERTIES ENABLE_EXPORTS ON)

if(LINUX)
  # For preserving global class registrars in static modules
  target_link_options(DedicatedServer PRIVATE -Wl,--whole-archive ../Mod/Entities/lib${ENTITIESLIB}.a ../Shaders/lib${SHADERSLIB}.a -Wl,--no-whole-archive)

  set_target_properties(DedicatedServer PROPERTIES LINK_FLAGS "-Wl,-rpath,$ORIGIN")
  target_link_libraries(DedicatedServer "m")
  target_link_libraries(DedicatedServer "dl")
  target_link_libraries(DedicatedServer "pthread")
  target_link_libraries(DedicatedServer SDL3::SDL3 ${ZLIB_LIBRARIES})

  if (SE1_OPENAL_SUPPORT)
    target_link_libraries(DedicatedServer ${OPENAL_LIBRARY})
  endif()
endif()
