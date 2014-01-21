/*
 * datagramparser.h
 *
 *  Created on: Jan 17, 2014
 *      Author: danny
 */

#ifndef DATAGRAMPARSER_H_
#define DATAGRAMPARSER_H_
#include <string>
#include <map>

namespace xpl
{

struct message
{
    using string = std::string;
    using map = std::map<string, string>;
    string  message_type;
    string  message_schema;
    map     headers;
    map     body;
};

class datagram_parser
{
public:
    void reset()
    {
        state = expect_message_type;
    }

    bool is_ready() const
    {
        return state == ready;
    }

    message get_message() const
    {
        return current_message;
    }
    void feed_line( std::string line);
private:
    enum state {
        expect_message_type,
        expect_header,
        expect_message_schema,
        expect_body,
        ready
    };

    state   state{expect_message_type};
    message current_message;
};

} /* namespace xpl */
#endif /* DATAGRAMPARSER_H_ */
