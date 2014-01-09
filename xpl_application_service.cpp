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

namespace ba = boost::asio;
namespace bs = boost::system;
using ba::ip::udp;
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
    ba::io_service io_service;
    udp::endpoint send_endpoint{ba::ip::address_v4::broadcast(), hub_port};
    udp::endpoint receive_endpoint{udp::v4(), 0};
    udp::socket socket{ io_service, receive_endpoint};
};

application_service::application_service()
:pimpl{new application_service::impl}
{
}

application_service::~application_service() = default;

} // end of namespace xpl


