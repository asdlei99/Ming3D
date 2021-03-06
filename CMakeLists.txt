cmake_minimum_required(VERSION 3.3)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "Runtime" )

project(Ming3D)


#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake/SDL2_image")
if(WIN32)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake/SDL2")
endif()

set(BUILD_ROOT "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

set(EXTERNAL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ExternalLibraries" CACHE PATH "External library directory")

set(SDL_SEARCH_PATH ${EXTERNAL_LIB_DIR})

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(BUILD_ROOT "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(ROOT_DIR "${CMAKE_SOURCE_DIR}")

# Cache variables
set(MING3D_BUILD_TESTS ON CACHE BOOL "Build test project")
set(MING3D_DEBUG_STATS OFF CACHE BOOL "Enable debug stats")
if(WIN32)
    set(MING3D_BUILD_EDITOR ON CACHE BOOL "Build editor project")
else()
    set(MING3D_BUILD_EDITOR OFF)
endif()

if(WIN32)
    # Graphics API
    set(MING3D_RENDERING_API "D3D11" CACHE STRING "Rendering API")
    set_property(CACHE MING3D_RENDERING_API PROPERTY STRINGS D3D11 OpenGL)
    # Physics API
    set(MING3D_PHYSICS_API "PhysX" CACHE STRING "Physics API")
    set_property(CACHE MING3D_PHYSICS_API PROPERTY STRINGS PhysX Null)
else()
    # Graphics API
    set(MING3D_RENDERING_API "OpenGL" CACHE STRING "Rendering API")
    set_property(CACHE MING3D_RENDERING_API PROPERTY STRINGS OpenGL)
    # Physics API
    set(MING3D_PHYSICS_API "Null" CACHE STRING "Physics API")
    set_property(CACHE MING3D_PHYSICS_API PROPERTY STRINGS Null)
endif()

# Set rendering API
if(MING3D_RENDERING_API STREQUAL "OpenGL")
    add_definitions(-DMING3D_OPENGL)
else()
    add_definitions(-DMING3D_D3D11)
endif()

# Set physics API
if(MING3D_PHYSICS_API STREQUAL "PhysX")
    add_definitions(-DMING3D_PHYSX)
endif()

if(MING3D_DEBUG_STATS)
	add_definitions(-DMING3D_DEBUG_STATS_ENABLED)
endif()

IF(WIN32)
    add_definitions(-DUNICODE)
endif()

# project
add_subdirectory(3rdParty)
add_subdirectory(Core)
add_subdirectory(Runtime)
add_subdirectory(Networking)
add_subdirectory(Rendering)
add_subdirectory(Engine)
add_subdirectory(Viewer)

if(MING3D_BUILD_EDITOR)
	add_subdirectory(NativeUI)
	add_subdirectory(Editor)
endif()

if(MING3D_BUILD_TESTS)
	add_subdirectory(Tests)
endif()
