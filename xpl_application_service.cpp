/*
 * xpl_application_service.cpp
 *
 *  Created on: Jan 9, 2014
 *      Author: danny
 *
 */
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


    /// convert an xpl-message to a string that can be sent as an
    /// UDP packet.
    std::string to_string( const xpl::message &m)
    {
        std::ostringstream stream;
        stream << m.message_type << "\n{\n";
        for (const auto &header_value : m.headers)
        {
            stream << header_value.first << '=' << header_value.second << '\n';
        }
        stream << "}\n";
        stream << m.message_schema << "\n{\n";
        for (const auto &body_value : m.body)
        {
            stream << body_value.first << '=' << body_value.second << '\n';
        }
        stream << "}\n";
        return stream.str();
    }
}

namespace xpl
{

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

application_service::application_service(
        const std::string &application_id,
        const std::string &version_string)
:pimpl{new application_service::impl}, application_id{application_id}, version_string{version_string}
{
    // add a handler for heartbeat requests that just immediately sends a heartbeat.
    register_command("hbeat.request", bind( &application_service::send_heartbeat_message, this));
}

application_service::~application_service() = default;

void application_service::run()
{
    ba::deadline_timer &timer = get_impl().heartbeat_timer;
    send_heartbeat_message();
    using time_traits_t = ba::time_traits<boost::posix_time::ptime>;
    timer.expires_at( time_traits_t::now() + discovery_heartbeat_period);
    timer.async_wait( boost::bind(&application_service::discovery_heartbeat, this, ba::placeholders::error, 0));
    start_read();
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
 * Send the actual xPL heartbeat message as a UDP broadcast
 */
void application_service::send_heartbeat_message()
{
    const std::string message =
            "xpl-stat"                                                 "\n"
            "{"                                                         "\n"
            "hop=1"                                                     "\n"
            "source=" + application_id +                                "\n"
            "target=*"                                                  "\n"
            "}"                                                         "\n"
            "hbeat.app"                                                 "\n"
            "{"                                                         "\n"
            "interval=" + std::to_string( heartbeat_period.minutes() ) +"\n"
            "port=" + std::to_string( get_listening_port()) +           "\n"
            "remote-ip=" + get_impl().socket.local_endpoint().address().to_string() + "\n"
            "version=" + version_string +                               "\n"
            "}"                                                         "\n";

    // send the heartbeat message synchronously. throws an error on failure.
    std::cout << "***\n" << message << "***" << std::endl;
    get_impl().socket.send_to( ba::buffer( message), get_impl().send_endpoint);
}

unsigned int application_service::get_listening_port() const
{
    return get_impl().socket.local_endpoint().port();
}

application_service::impl& application_service::get_impl()
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
    std::string as_string = to_string( m);
    get_impl().io_service.post( [as_string, this](){
        get_impl().socket.send_to( ba::buffer( as_string), get_impl().send_endpoint);
    });
}

/// get a reference to the implementation class
const application_service::impl& application_service::get_impl() const
{
    return *pimpl;
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
                    std::cout << "line:" << line;
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


