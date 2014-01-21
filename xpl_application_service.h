/*
 * xpl_application_service.h
 *
 *  Created on: Jan 9, 2014
 *      Author: danny
 */

#ifndef XPL_APPLICATION_SERVICE_H_
#define XPL_APPLICATION_SERVICE_H_
#include <memory>
#include <functional>
#include <string>

#include <boost/system/error_code.hpp>
#include "datagramparser.h"

namespace xpl
{

class message;
class application_service
{
public:
    application_service( const std::string &application_id,
            const std::string &version_string);
    ~application_service();
    void run();
    bool is_connected() const {return connected;}

    using handler = std::function<void ( const message &)>;
    void register_command( const std::string &schema, handler h);
    void register_status(  const std::string &schema, handler h);
    void register_trigger( const std::string &schema, handler h);

private:
    void discovery_heartbeat( const boost::system::error_code& e, unsigned int counter);
    void heartbeat( const boost::system::error_code& e);
    void send_heartbeat_message();
    unsigned int get_listening_port() const;
    void start_read();
    void handle_message( const message &m);
    struct impl;
    impl& get_impl();
    const impl& get_impl() const;

    std::unique_ptr<impl>   pimpl;
    bool                    connected{ false};
    const std::string       application_id;
    const std::string       version_string;

    static const size_t buffer_size = 512;
    char receive_buffer[buffer_size];
};

}
#endif /* XPL_APPLICATION_SERVICE_H_ */
