##          Copyright Danny Havenith 2013.
## Distributed under the Boost Software License, Version 1.0.
##    (See accompanying file LICENSE_1_0.txt or copy at
##          http://www.boost.org/LICENSE_1_0.txt)

## This is the CMakeLists file for a small wav reader


file( GLOB local_headers *.hpp)
file( GLOB exported_headers include/*.hpp)

add_library( audiofiles
	wav_parser.cpp

# header files, just for VS' sake.
	${local_headers}
	${exported_headers}		
	)

