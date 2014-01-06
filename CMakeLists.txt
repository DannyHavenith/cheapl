
##          Copyright Danny Havenith 2013.
## Distributed under the Boost Software License, Version 1.0.
##    (See accompanying file LICENSE_1_0.txt or copy at
##          http://www.boost.org/LICENSE_1_0.txt)


cmake_minimum_required(VERSION 2.6)

project(cheapl)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

IF (CMAKE_HOST_UNIX)
	FIND_PACKAGE( ALSA)
	set( libraries ${ALSA_LIBRARIES} pthread)
ENDIF (CMAKE_HOST_UNIX)

include_directories(${ALSA_INCLUDE_DIRS} ${cheapl_SOURCE_DIR})

add_subdirectory( audiofiles)
add_executable(
	cheapl
	
	cheapl_main.cpp
	)

target_link_libraries(cheapl audiofiles ${libraries})	

