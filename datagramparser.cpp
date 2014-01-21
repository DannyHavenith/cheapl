/*
 * datagramparser.cpp
 *
 *  Created on: Jan 17, 2014
 *      Author: danny
 */

#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp> // I'm having trouble with std::regex and brackets ("[" and "]").
#include "datagramparser.h"

namespace xpl
{

void datagram_parser::feed_line( std::string line)
{
    //static const std::string r(R"((.+)=(.+))");
    // make sure there's no lingering newlines or spaces.
    boost::algorithm::trim( line);
    const boost::regex namevalue{R"(^([^= ]+)\s*=\s*(.*)$)"};

    switch (state)
    {
    case expect_message_type:
        if (line == "{") state = expect_header;
        else current_message.message_type = line;
        break;
    case expect_header:
        if (line == "}") state = expect_message_schema;
        else
        {
            boost::smatch match;
            if (regex_match( line, match, namevalue))
            {
                current_message.headers[match[1]] = match[2];
            }
        }
        break;
    case expect_message_schema:
        if (line == "{") state = expect_body;
        else current_message.message_schema = line;
        break;
    case expect_body:
        if (line == "}") state = ready;
        else
        {
            boost::smatch match;
            if (regex_match( line, match, namevalue))
            {
                current_message.body[match[1]] = match[2];
            }
        }
        break;
    }
}

} /* namespace xpl */
