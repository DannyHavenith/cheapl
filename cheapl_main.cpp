//
//  Copyright (C) 2014 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include "cheaplservice.h"
#include <boost/asio/ip/host_name.hpp>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <string>
#include <cstdlib>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;

struct config
{
    string soundfile_directory  {"."};
    string usb_device           {"Generic USB Audio Device"};
    string application_id       {"rurandom-cheapl." + truncateto16( boost::asio::ip::host_name())};
    string application_version  {"0.1"};
private:
    static std::string truncateto16( const std::string &input)
    {
        if (input.size() <= 16) return input;
        return input.substr( 0, 16);
    }
};


/// Simple configuration retrieval function.
/// For now, let's stick with a _very_ simple command line parser.
config get_config( int argc, char *argv[])
{
    config result;

    // little known fact: argv ends with a nullptr
    ++argv; // get past the program name
    if (*argv)
    {
        result.soundfile_directory = *argv++;
    }

    if (*argv)
    {
        result.usb_device = *argv++;
    }

    return result;
}

std::unique_ptr< xpl::cheapl_service> service_ptr;

void exit_handler(void) noexcept
{
    try
    {
        if (service_ptr) service_ptr->signoff();
    }
    catch (...) // easiest way to guarantee noexcept
    {}
}
/// Main function of the cheapl server executable. This will start the cheapl xPL service and execute requests.
/// This program currently takes two optional arguments:
/// # the directory where wav-files can be found that should be played to the RF-connected sound card.
/// # the name of the alsa sound device to which the sequences should be sent.
int main( int argc, char *argv[])
{
    int result = 0;
    try
    {
        atexit(exit_handler);
        config conf = get_config( argc, argv);
        service_ptr.reset( new xpl::cheapl_service{ conf.soundfile_directory, conf.usb_device, conf.application_id, conf.application_version});
        service_ptr->run();
    }
    catch (std::exception &e)
    {
        std::cerr << "something went wrong: " << e.what() << std::endl;
        result = -1;
    }
    catch (boost::system::error_code &e)
    {
        std::cerr << "system error: " << e.message() << std::endl;
        result = -2;
    }

	return result;
}
