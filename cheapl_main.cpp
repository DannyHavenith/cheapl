/*
 * cheapl_main.cpp
 *
 *  Created on: Dec 8, 2013
 *      Author: danny
 */
#include "alsa_wrapper.hpp"
#include <iostream>

int main()
{
	alsalib &alsa( alsalib::get_instance());
	soundcard transmitter;
	for (auto card : alsa.get_cards())
	{
		if (card.get_name() == "Generic USB Audio Device")
		{
			transmitter = card;
		}
	}

    std::cout << transmitter.get_name() << '\n';

	return 0;
}
