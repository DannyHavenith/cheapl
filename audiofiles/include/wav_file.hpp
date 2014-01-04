//
//  Copyright (C) 2013 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

/// This file contains the definition of a few types that represent data that can be found in wav files.

#if !defined(WAV_FILE_HPP)
#define WAV_FILE_HPP
#include <cstdint>
struct riff_fmt
{
    uint16_t compression;
    uint16_t channels;
    uint32_t samplerate;
    uint32_t bytes_per_second;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct riff_data
{
    std::size_t pos; ///<position in file of samples
    uint32_t    size;///<size of sample data.
};

struct wav_file
{
    riff_fmt  fmt; ///< information about the format of the sound data
    riff_data data;///< the sound data itself, in the form of offsets into the original file.
};
#endif //WAV_FILE_HPP
