
##          Copyright Danny Havenith 2013.
## Distributed under the Boost Software License, Version 1.0.
##    (See accompanying file LICENSE_1_0.txt or copy at
##          http://www.boost.org/LICENSE_1_0.txt)


cmake_minimum_required(VERSION 2.6)

project(cheapl)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

## boost
SET(Boost_USE_MULTITHREAD ON)
FIND_PACKAGE( Boost COMPONENTS system filesystem regex)
include_directories( ${Boost_INCLUDE_DIR})
link_directories( ${Boost_LIBRARY_DIRS})

set( libraries)

## alsa, which needs pthread as well
IF (CMAKE_HOST_UNIX)
	FIND_PACKAGE( ALSA)
	list(APPEND libraries ${ALSA_LIBRARIES} pthread)
ENDIF (CMAKE_HOST_UNIX)

## boost
SET(Boost_USE_MULTITHREAD ON)
FIND_PACKAGE( Boost COMPONENTS system filesystem regex)
include_directories( ${Boost_INCLUDE_DIR})
link_directories( ${Boost_LIBRARY_DIRS})
list( APPEND libraries ${Boost_LIBRARIES})

include_directories(${ALSA_INCLUDE_DIRS} ${cheapl_SOURCE_DIR})

add_subdirectory( audiofiles)
add_executable(
	cheapl
	
	cheapl_main.cpp
	xpl_application_service.cpp
	datagramparser.cpp
	)

target_link_libraries(cheapl audiofiles ${libraries})	

