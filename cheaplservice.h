//
//  Copyright (C) 2014 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CHEAPLSERVICE_H_
#define CHEAPLSERVICE_H_
#include <memory> // for unique_ptr
#include <string>

namespace xpl
{

class message;

/// This class acts as an xPL service. It listens on an UDP port for xPL messages and when messages of the right type (x10 schema commands)
/// arrive, a corresponding wav-file will be played on the given soundcard device. The soundcard is supposed to be connected to an RF-transmitter.
/// This service will only run while the run() member function is being executed.
/// This class an object of type xpl::application_service class for all its xpl communication and uses the alsa_wrapper functions
/// to play "sounds".
class cheapl_service
{
public:
    cheapl_service( const std::string &directoryname, const std::string &soundcardname, const std::string &application_id, const std::string &application_version = "1.0");
    ~cheapl_service();
    void run();
    static void list_cards( std::ostream& output);

private:
    void handle_command( const message &m);
    void scan_files( const std::string &directoryname);

    struct impl;
    std::unique_ptr<impl> pimpl;

    impl& get_impl();
    const impl& get_impl() const;
};

} /* namespace xpl */
#endif /* CHEAPLSERVICE_H_ */
