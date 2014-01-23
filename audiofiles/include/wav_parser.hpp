//
//  Copyright (C) 2013 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#if !defined( WAV_PARSER_HPP)
#define WAV_PARSER_HPP
#include <iosfwd>
#include "wav_file.hpp"

/// parse the midi file represented by stream 'stream' and return the information of that file in output parameter 'result'.
/// This function returns true iff the file could be completely parsed as a midi file.
bool parse_wavfile( std::istream &stream, wav_file &result);

#endif //WAV_PARSER_HPP
