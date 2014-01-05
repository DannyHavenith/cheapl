/*
 * cheapl_main.cpp
 *
 *  Created on: Dec 8, 2013
 *      Author: danny
 */
#include "alsa_wrapper.hpp"
#include "audiofiles/include/wav_parser.hpp"
#include "audiofiles/include/wav_file.hpp"
#include <iostream>
#include <fstream>

void list_cards( std::ostream &output)
{
    alsalib &alsa( alsalib::get_instance());
    for (auto card : alsa.get_cards())
    {
        output << card.get_index() << '\t' << card.get_name() << '\n';
    }
}

soundcard find_card( const std::string &name)
{
    alsalib &alsa( alsalib::get_instance());
    for (auto card : alsa.get_cards())
    {
        if (card.get_name() == name) return card;
    }
    throw std::runtime_error("could not find sound card with name: " + name);
}

int main()
{
    list_cards( std::cout);
    wav_file wav;
    std::ifstream wavfile("/home/danny/Documents/433Mhz_experiments/off1.wav", std::ios::binary);
    if (parse_wavfile( wavfile,wav))
    {
        std::cout << "bits per sample:" << wav.fmt.bits_per_sample << " rate:" << wav.fmt.samplerate << '\n';
    }
    else
    {
        std::cerr << "parsing wav file failed\n";
    }

	return 0;
}
