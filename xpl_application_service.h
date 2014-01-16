/*
 * xpl_application_service.h
 *
 *  Created on: Jan 9, 2014
 *      Author: danny
 */

#ifndef XPL_APPLICATION_SERVICE_H_
#define XPL_APPLICATION_SERVICE_H_
#include <memory>
#include <stdexcept>
#include <boost/system/error_code.hpp>

namespace xpl
{

class no_hub_error : std::runtime_error
{
public:
    no_hub_error() : std::runtime_error("no connection to hub within time out period"){}
};

class application_service
{
public:
    application_service( const std::string &application_id,
            const std::string &version_string);
    ~application_service();
    void run();
    bool is_connected() const {return connected;}
private:
    void discovery_heartbeat( const boost::system::error_code& e, unsigned int counter);
    void heartbeat( const boost::system::error_code& e);
    void send_heartbeat_message();
    unsigned int get_listening_port() const;
    struct impl;
    impl& get_impl();
    const impl& get_impl() const;

    std::unique_ptr<impl>   pimpl;
    bool                    connected{ false};
    const std::string       application_id;
    const std::string       version_string;
};

}
#endif /* XPL_APPLICATION_SERVICE_H_ */
