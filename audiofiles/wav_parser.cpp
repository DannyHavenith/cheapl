//
//  Copyright (C) 2012 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <vector>
#include <iostream>
#include <iterator>
#include <iomanip>
#include <string>

#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi_binary.hpp>
#include <boost/spirit/home/support/iterators/istream_iterator.hpp>

#include "include/wav_parser.hpp"
#include "next_directive.hpp"
#include "wav_file_fusion.hpp"

#define DEBUG_NODE( node) BOOST_SPIRIT_DEBUG_NODE( node); debug( node);

using namespace std;
using namespace boost::phoenix;
using namespace boost::spirit;
using namespace boost::spirit::qi;

/// Grammar for wav files.
/// This grammar can be used to parse wav files. It generates a wav_file struct from the contents of the file.
template<typename Iterator>
struct wav_parser: grammar< Iterator, wav_file()>
{
    using base_type = grammar< Iterator, wav_file()>;
    wav_parser() :
        wav_parser::base_type( file)
    {
        using boost::spirit::ascii::char_;
        using boost::phoenix::ref;
        using boost::phoenix::at_c;
        using boost::phoenix::construct;
        using next_directive::next;

        file
            %= riff
            ;

        // a RIFF file consists of a RIFF header, followed by a sequence consisting of
        // format chunk(s) and data chunk(s).
        // Normally this is just a format chunk followed by a data chunk, but this parser
        // will take them in any order and stores only the last format chunk and the last data
        // chunk.
        riff
            = riff_header[_a = _1]
              >>
                      +(
                          fmt      [at_c<0>(_val) = _1]  // format chunk, or
                          |  data  [at_c<1>(_val) = _1]  // data chunk
                      )

            ;

        riff_header
            %= lit("RIFF") > little_dword > lit("WAVE")
            ;

        fmt
            %=
                lit("fmt ")
                >   omit[dword[_a = _1]] // chunk size
                >   next(_a + (_a%2))[
                        word  // compression
                        > word  // channels
                        > dword // sample rate
                        > dword // bytes per second
                        > word  // block align
                        > word  // bits per sample
                        > omit[*byte_]  // extra format bytes, ignored
                    ]
            ;

        dword %= little_dword;
        word %= little_word;
        data
            = lit("data")  > omit[ little_dword[_a = _1][ at_c<1>(_val) = _1] > repeat( _a + (_a%2))[ byte_]]
            ;
        on_error<fail>
         (
             file
           , std::cout
                 << val("Error! Expecting ")
                 << _4                               // what failed?
                 << std::endl
         );

        DEBUG_NODE( file);
        DEBUG_NODE(riff);
        DEBUG_NODE( riff_header);
        DEBUG_NODE( fmt);
        DEBUG_NODE( data);
        DEBUG_NODE(dword);
        DEBUG_NODE(word);
    }

    rule<Iterator, unsigned long()> riff_header, dword, word;
    rule<Iterator, wav_file(), locals<size_t>>      riff;
    rule<Iterator, wav_file() >      file;
    rule<Iterator, riff_fmt(), locals<size_t>>       fmt;
    rule<Iterator, riff_data(), locals<size_t>>      data;
};

/// parse the wav file that the istream 'in' refers to and return the result in a wav_file structure
/// Note that on some platforms the input file must have been opened as binary.
bool parse_wavfile( std::istream &in, wav_file &result)
{
    // don't skip whitespaces.
    in.unsetf( ios_base::skipws);
    using iterator = boost::spirit::istream_iterator;
    iterator first( in), last;
    // parse the stream
    wav_parser<iterator> parser;
    boost::spirit::qi::parse( first, last, parser, result);

    return first == last;
}
