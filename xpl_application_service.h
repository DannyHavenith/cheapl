//
// Copyright (c) 2013, 2014 Danny Havenith
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#ifndef XPL_APPLICATION_SERVICE_H_
#define XPL_APPLICATION_SERVICE_H_
#include <memory>
#include <functional>
#include <string>

#include <boost/system/error_code.hpp>

namespace xpl
{

class message;

/// This class implements a generic xpl application service.
/// This class will, when the run() member function is called automatically start to broadcast
/// UDP heartbeat messages and will listen for UDP xpl messages. Whenever an xpl
/// message (command, status or trigger) is received, this class will dispatch the message
/// to any registered handler for that message.
/// Message handlers may be registered by calling the register_command(), register_status() or
/// register_trigger() function.
/// The send() member function can be used to send xpl messages through this server.
class application_service
{
public:
    application_service( const std::string &application_id,
            const std::string &version_string);
    ~application_service();
    void run();

    /// returns whether this service has received messages from an xpl hub.
    bool is_connected() const {return connected;}

    using handler = std::function<void ( const message &)>;
    void register_command( const std::string &schema, handler h);
    void register_status(  const std::string &schema, handler h);
    void register_trigger( const std::string &schema, handler h);
    void send( message m);

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
