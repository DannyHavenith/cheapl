/*
 * xpl_application_service.cpp
 *
 *  Created on: Jan 9, 2014
 *      Author: danny
 *
 */
#include "xpl_application_service.h"
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>


#include <iostream>
#include <iomanip>

namespace ba = boost::asio;
namespace bs = boost::system;
namespace pt = boost::posix_time;
using ba::ip::udp;

namespace {
    /// initial heartbeat period, when we haven't been seen by our local hub.
    const pt::time_duration discovery_heartbeat_period =pt::seconds(3);

    /// heartbeat period to use if we haven't been seen any hub for two minutes.
    const pt::time_duration lonely_heartbeat_period = pt::seconds( 30);

    /// regular heartbeat period when we've been connected to a hub
    const pt::time_duration heartbeat_period = pt::minutes( 5);

    /// discovery period should be at most 120s, or 120/3 heartbeats.
    const int max_discovery_count = 120/discovery_heartbeat_period.seconds();
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
};

application_service::application_service(
        const std::string &application_id,
        const std::string &version_string)
:pimpl{new application_service::impl}, application_id{application_id}, version_string{version_string}
{
}

application_service::~application_service() = default;

void application_service::run()
{
    ba::deadline_timer &timer = get_impl().heartbeat_timer;
    send_heartbeat_message();
    using time_traits_t = ba::time_traits<boost::posix_time::ptime>;
    timer.expires_at( time_traits_t::now() + discovery_heartbeat_period);
    timer.async_wait( boost::bind(&application_service::discovery_heartbeat, this, ba::placeholders::error, 0));
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
 * After sending the heartbeat message a timer is set to call this function again. If we're not connected yet,
 * this timer will expire fairly quickly, if we're connected a longer timeout is chosen.
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
            "xpl-stat\n"                                                "\n"
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

const application_service::impl& application_service::get_impl() const
{
    return *pimpl;
}

} // end of namespace xpl


