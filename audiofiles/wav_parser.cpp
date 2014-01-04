//
//  Copyright (C) 2013 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include "include/wav_parser.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
#include <deque>

/// simple wav-file reader class. objects of this class can be used to read a wav file and parse it for information about the
/// sample format and the location in the file of the sample data.
class wav_reader
{
public:
    class wav_file_exception : public std::runtime_error
    {
    public:
        wav_file_exception( const std::string reason)
                :runtime_error( "error reading wav file:" + reason){}
    };

    wav_reader(std::istream &input, wav_file &result)
    :in(input), result(result) {};

    bool riff()
    {
        expect( "RIFF");
        uint32_t filesize;
        read( filesize);
        expect( "WAVE");

        while( (in.peek(), !in.eof()) && expect(fmt() || data(), "expected either a fmt- or a data chunk"))
        {
            // do nothing
        }

        return in;
}

private:
    /// Expect a 4-character sequence in the file.
    /// The first argument is a 5-characters sequence because the string literal "abcd" also contains the terminating zero.
    bool expect( const char (&keyword)[5], bool throw_if_fail = true)
    {
        auto begin = &keyword[0];
        auto end = &keyword[4];
        uint8_t byte;
        while (begin != end && (byte=tentative_next()) == *begin++) /*nop*/;
        if (begin != end)
        {
            rewind_tentative();
            if (throw_if_fail)
            {
                throw wav_file_exception( "couldn't find sequence \"" + std::string(keyword, 4) + "\"");
            }
        }
        else
        {
            clear_tentative();
        }
        return begin == end;
    }

    /// Tentatively read a format chunk. Return false if no format chunk was found.
    bool fmt()
    {
        if (!expect("fmt ", false)) return false;

        // we're commited now. Any failure from this point is a file format error.
        uint32_t chunksize = 0;
        read( chunksize);
        expect( chunksize >= 16, "fmt chunk is unexpectedly small");

        read( result.fmt.compression);
        read( result.fmt.channels);
        read( result.fmt.samplerate);
        read( result.fmt.bytes_per_second);
        read( result.fmt.block_align);
        read( result.fmt.bits_per_sample);

        // add 1 if chunksize is odd.
        chunksize += chunksize%2;
        chunksize -= 16;
        while (chunksize) next();

        return expect(in, "format error while reading format chunk");
    }

    /// Tentatively read a data chunk. Return false if no data chunk was found.
    bool data()
    {
        if (!expect("data", false)) return false;
        uint32_t chunksize = 0;
        read( result.data.size);
        result.data.pos = in.tellg();
        in.seekg( result.data.size + result.data.size%2, std::ios_base::cur);
        return in;
    }

    /// read a 4-byte little endian integer
    bool read( uint32_t &value)
    {
        // not the fastest implementation, but it's short and portable.
        value = next();
        value |= next() << 8;
        value |= next() << 16;
        value |= next() << 24;
        return in;
    }

    /// read a 2-byte little endian integer
    bool read( uint16_t &value)
    {
        value = next();
        value |= next() << 8;
        return in;
    }

    uint8_t next()
    {
        if (!buffer.empty())
        {
            auto value = buffer.front();
            buffer.pop_front();
            buffer_pos = buffer.begin();
            return value;
        }
        else
        {
            return in.rdbuf()->sbumpc(); // narrow to byte.
        }
    }

    uint8_t tentative_next()
    {
        if (buffer_pos == buffer.end())
        {
            auto value = in.rdbuf()->sbumpc();
            buffer.push_back( value);
            buffer_pos = buffer.end();
            return value;
        }
        else
        {
            return *buffer_pos++;
        }
    }

    void rewind_tentative()
    {
        buffer_pos = buffer.begin();
    }

    void clear_tentative()
    {
        buffer.clear();
        buffer_pos = buffer.end();
    }

    /// throw an exception if the input value is false, return the input value otherwise
    static bool expect( bool input, const std::string &expected_as_string)
    {
        if (!input) throw wav_file_exception( expected_as_string);
        return input;
    }

    std::istream    &in;
    wav_file        &result;
    using deque = std::deque<uint8_t>;
    deque buffer;
    deque::const_iterator buffer_pos = buffer.begin();
};

/// parse the wav file that the istream 'in' refers to and return the result in a wav_file structure
/// Note that on some platforms the input file must have been opened as binary.
bool parse_wavfile( std::istream &in, wav_file &result)
{
    // don't skip whitespaces.
    in.unsetf( std::ios_base::skipws);
    wav_reader reader(in, result);
    return reader.riff();
}
