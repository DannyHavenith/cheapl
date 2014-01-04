//
//  Copyright (C) 2012 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

/// This file adapts the midi-file specific structs of midi_file.hpp to become first-class fusion tuple types.

#ifndef MIDI_FILE_FUSION_HPP
#define MIDI_FILE_FUSION_HPP
#include "include/wav_file.hpp"
#include <boost/fusion/include/adapt_struct.hpp>

BOOST_FUSION_ADAPT_STRUCT(
    riff_fmt,
    (unsigned short, compression)
    (unsigned short, channels)
    (unsigned long,  samplerate)
    (unsigned long,  bytes_per_second)
    (unsigned short, block_align)
    (unsigned short, bits_per_sample)
 )

BOOST_FUSION_ADAPT_STRUCT(
        riff_data,
        (std::size_t, pos)
        (std::size_t, size)
)

BOOST_FUSION_ADAPT_STRUCT(
        wav_file,
        (riff_fmt, fmt)
        (riff_data, data)
)

#endif //MIDI_FILE_FUSION_HPP
