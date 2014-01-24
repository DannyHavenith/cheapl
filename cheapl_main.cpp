//
//  Copyright (C) 2014 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include "cheaplservice.h"
#include <boost/system/error_code.hpp>
#include <iostream>
#include <string>

using namespace std;

struct config
{
    string soundfile_directory  {"."};
    string usb_device           {"Generic USB Audio Device"};
    string application_id       {"cheapl-b.home"};
    string application_version  {"0.1"};
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

/// Main function of the cheapl server executable. This will start the cheapl xPL service and execute requests.
/// This program currently takes two optional arguments:
/// # the directory where wav-files can be found that should be played to the RF-connected sound card.
/// # the name of the alsa sound device to which the sequences should be sent.
int main( int argc, char *argv[])
{
    int result = 0;
    try
    {
        config conf = get_config( argc, argv);
        xpl::cheapl_service service{ conf.soundfile_directory, conf.usb_device, conf.application_id, conf.application_version};
        service.run();
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
