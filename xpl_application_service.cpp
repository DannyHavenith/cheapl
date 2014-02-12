//
//  Copyright (C) 2014 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include "datagramparser.h"
#include "xpl_application_service.h"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>


#include <iostream>
#include <iomanip>
#include <string>

namespace ba = boost::asio;
namespace bs = boost::system;
namespace pt = boost::posix_time;
using ba::ip::udp;
using boost::bind;

namespace {
    /// initial heartbeat period, when we haven't been seen by our local hub.
    const pt::time_duration discovery_heartbeat_period =pt::seconds(3);

    /// heartbeat period to use if we haven't been seen any hub for two minutes.
    const pt::time_duration lonely_heartbeat_period = pt::seconds( 30);

    /// regular heartbeat period when we've been connected to a hub
    const pt::time_duration heartbeat_period = pt::minutes( 5);

    /// discovery period should be at most 120s, or 120/3 heartbeats.
    const int max_discovery_count = 120/discovery_heartbeat_period.seconds();

    // the three xpl message types. xpl-cmnd, xpl-stat or xpl-trig
    const std::string command_type{"xpl-cmnd"};
    const std::string status_type{"xpl-stat"};
    const std::string trigger_type{"xpl-trig"};


    void map_to_stream( const xpl::message::map &map, std::ostream &stream)
    {
        stream << "{\n";
        for (const auto &header_value : map)
        {
            stream << header_value.first << '=' << header_value.second << '\n';
        }
        stream << "}\n";
    }

    /// convert an xpl-message to a string that can be sent as an
    /// UDP packet.
    std::string to_string( const xpl::message &m)
    {
        std::ostringstream stream;
        stream << m.message_type << '\n';
        map_to_stream( m.headers, stream);
        stream << m.message_schema << '\n';
        map_to_stream( m.body, stream);
        return stream.str();
    }
}

namespace xpl
{

/// Implementatin of the pimpl (bridge) pattern.
/// This struct contains the private data for an xPL application service.
struct application_service::impl
{
    impl()
    {
        socket.set_option(udp::socket::reuse_address(true));
        socket.set_option(ba::socket_base::broadcast(true));
    }

    static const int hub_port = 3865;
    ba::io_service      io_service;
    udp::endpoint       send_endpoint{ba::ip::address_v4::broadcast(), hub_port};
    udp::endpoint       receive_endpoint{udp::v4(), 0};
    udp::socket         socket{ io_service, receive_endpoint};
    ba::deadline_timer  heartbeat_timer{ io_service};

    using handler = application_service::handler;
    using handler_map = std::map< std::string, handler>;
    using command_handler_map = std::map< std::string, handler_map>;

    command_handler_map handlers{{command_type, {}}, {status_type, {}},{trigger_type, {}}};;
};

/// Construct a service that will listen for xPL UDP messages.
/// Messages that are sent from this service will have the given application id and version string.
application_service::application_service(
        const std::string &application_id,
        const std::string &version_string)
:pimpl{new application_service::impl}, application_id{application_id}, version_string{version_string}
{
    // add a handler for heartbeat requests that just immediately sends a heartbeat.
    register_command("hbeat.request", bind( &application_service::send_heartbeat_message, this, false));
}

/// default constructor. explicitly defined in this translation unit so that
/// the impl destructor is in scope of this one.
application_service::~application_service() = default;

/// Run the actual xPL service.
/// While this run() function is executing, the xPL server will listen to incoming messages on its
/// UDP port and dispatch those messages to any registered handlers.
/// At the same time, this class will send regular heartbeat messages.
void application_service::run()
{
    ba::deadline_timer &timer = get_impl().heartbeat_timer;
    send_heartbeat_message();
    using time_traits_t = ba::time_traits<boost::posix_time::ptime>;
    timer.expires_at( time_traits_t::now() + discovery_heartbeat_period);
    timer.async_wait( boost::bind(&application_service::discovery_heartbeat, this, ba::placeholders::error, 0));
    start_read();
    std::cout << "starting service on port " << get_listening_port() << '\n';
    get_impl().io_service.run();
}

/**
 * Send a heartbeat while in "discovery" mode.
 * This function will send a heartbeat message and schedule another call of this function on short notice unless we're either connected
 * or we've sent more than max_discovery_count heartbeat messages already, in which case we'll schedule a call of the heartbeat()
 * member function.
 */
void application_service::discovery_heartbeat( const boost::system::error_code& e, unsigned int counter)
{
    // throw any error we encounter.
    if (e) throw e;

    send_heartbeat_message();
    ba::deadline_timer &timer = get_impl().heartbeat_timer;
    if (counter >= max_discovery_count || connected)
    {
        heartbeat( e);
    }
    else
    {
        timer.expires_at( timer.expires_at() + discovery_heartbeat_period);
        timer.async_wait( boost::bind(&application_service::discovery_heartbeat, this, ba::placeholders::error, counter + 1));
    }
}

/*
 * Send a heartbeat message.
 * After sending the heartbeat message a timer is set to call this function again. If we're not connected yet
 * this timer will expire fairly quickly, resulting in a high frequency heartbeat. If we're connected, a
 * longer timeout is chosen.
 */
void application_service::heartbeat( const boost::system::error_code& e)
{
    // throw any error we encounter.
    if (e) throw e;

    send_heartbeat_message();
    ba::deadline_timer &timer = get_impl().heartbeat_timer;
    if (connected)
    {
        timer.expires_at( timer.expires_at() + heartbeat_period);
    }
    else
    {
        timer.expires_at( timer.expires_at() + lonely_heartbeat_period);
    }

    timer.async_wait( boost::bind(&application_service::heartbeat, this, ba::placeholders::error));

}

/**
 * Send the actual xPL heartbeat message as a UDP broadcast.
 * If the boolean argument "final" is true the heartbeat message will be
 * use the hbeat.end schema, signalling that this service is about to end.
 */
void application_service::send_heartbeat_message( bool final)
{
    const std::string schema = final?"end":"app";
    const std::string message =
            "xpl-stat"                                                  "\n"
            "{"                                                         "\n"
            "hop=1"                                                     "\n"
            "source=" + application_id +                                "\n"
            "target=*"                                                  "\n"
            "}"                                                         "\n"
            "hbeat." + schema +                                         "\n"
            "{"                                                         "\n"
            "interval=" + std::to_string( heartbeat_period.minutes() ) +"\n"
            "port=" + std::to_string( get_listening_port()) +           "\n"
            "remote-ip=" + get_impl().socket.local_endpoint().address().to_string() + "\n"
            "version=" + version_string +                               "\n"
            "}"                                                         "\n";

    // send the heartbeat message synchronously. throws an error on failure.
    get_impl().socket.send_to( ba::buffer( message), get_impl().send_endpoint);
}

/// Get the UDP port number that this service is listening on.
unsigned int application_service::get_listening_port() const
{
    return get_impl().socket.local_endpoint().port();
}

/// pimpl implementation: get a reference to the implementation object.
application_service::impl& application_service::get_impl()
{
    return *pimpl;
}

void application_service::send_termination_message()
{
    // cancel the heartbeat timer
    get_impl().heartbeat_timer.cancel();

    // send a final heartbeat message.
    send_heartbeat_message( true);

}

/// get a reference to the implementation class
const application_service::impl& application_service::get_impl() const
{
    return *pimpl;
}

/// Register a handler for command messages.
/// The handler should have a signature compatible with:
/// @code
/// void handler( const xpl::message &m)
/// @endcode
/// Whenever a message with the given schema arrives at this server, the given handler will be
/// invoked. Any previously registered handler for the same message schema will be discarded.
void application_service::register_command(
        const std::string& schema,
        application_service::handler h)
{
    get_impl().handlers[command_type][schema] = h;
}

/// Register a handler for status messages
/// @see register_command()
void application_service::register_status( const std::string& schema, application_service::handler h)
{
    get_impl().handlers[status_type][schema] = h;
}

/// Register a handler for trigger messages.
/// @see register_command()
void application_service::register_trigger( const std::string& schema,
        application_service::handler h)
{
    get_impl().handlers[trigger_type][schema] = h;
}

/// Schedule a message to be sent and return immediately.
void application_service::send( message m)
{
    m.headers["source"] = application_id;

    // todo: officially, each xpl message should have its header and body attributes in a specific order. We should
    // really register this order for each schema type and make sure that they end up in that order in this string.
    // for now, the members will be ordered alphabetically.
    std::string as_string = to_string( m);

    get_impl().io_service.post( [as_string, this](){
        get_impl().socket.send_to( ba::buffer( as_string), get_impl().send_endpoint);
    });
}

/// Start an asynchronous read operation.
/// This starts an operation that will read a message and consequently parse and dispatch to any registered
/// handlers.
void application_service::start_read()
{
    get_impl().socket.async_receive( ba::buffer( receive_buffer),
            [this]( const bs::error_code &error, std::size_t bytes_received)
            {
                if (error) throw error;
                using separator_t = boost::char_separator<char>;
                using tokenizer_t = boost::tokenizer<separator_t, const char *>;
                tokenizer_t tokenizer( receive_buffer, receive_buffer + bytes_received, separator_t("\n"));
                datagram_parser parser;
                for( const auto &line: tokenizer)
                {
                    parser.feed_line( line);
                }
                if (parser.is_ready())
                {
                    handle_message( parser.get_message());
                }
                start_read();// start the next read
            });
}

/// Deal with an incoming message.
/// This function will dispatch the given message to any registered handlers for the message schema.
/// If the message is a heartbeat request, this function will immediately send a heartbeat before dispatching
/// the message to any registered handler.
void application_service::handle_message(const xpl::message &m)
{
    try
    {
        const auto &target = m.headers.at("target");
        // only act if this message was intended for us.
        if ( target == "*" || target == application_id)
        {
            // if we receive our own heartbeat, we know we've been connected by a hub.
            if (!connected && m.message_schema == "hbeat.app" && m.headers.at("source") == application_id)
            {
                connected = true;
            }

            // find a handler for the message and invoke it.
            auto handler_it = get_impl().handlers[m.message_type].find(m.message_schema);
            if (handler_it != get_impl().handlers[m.message_type].end() && handler_it->second)
            {
                handler_it->second( m);
            }
        }
    }
    catch (std::out_of_range &)
    {
        // if any of the 'at()' calls fails, we silently ignore the incoming message.
    }
}

} // end of namespace xpl


