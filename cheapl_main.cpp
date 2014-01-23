/*
 * cheapl_main.cpp
 *
 *  Created on: Dec 8, 2013
 *      Author: danny
 */
#include "cheaplservice.h"
#include <boost/system/error_code.hpp>
#include <iostream>
#include <fstream>


int main( int argc, char *argv[])
{
    try
    {
        xpl::cheapl_service service{ "/home/danny/Documents/433Mhz_experiments", "Generic USB Audio Device", "cheapl-b.home", "0.1"};
        xpl::cheapl_service::list_cards( std::cout);
        service.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "something went wrong: " << e.what() << std::endl;
    }
    catch (boost::system::error_code &e)
    {
        std::cerr << "system error: " << e.message() << std::endl;
    }

	return 0;
}
