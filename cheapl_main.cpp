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

std::pair<int, int> find_card_pcm( const std::string &cardname)
{
    auto &alsa( alsalib::get_instance());
    for (auto card : alsa.get_cards())
    {
        if (card.get_name() == cardname)
        {
            auto &pcmdevices = card.pcm_devices();
            // get the first pcm device
            if (pcmdevices.begin() != pcmdevices.end())
            {
                return std::make_pair( card.get_index(), (*pcmdevices.begin()).get_index());
            }
            else
            {
                throw std::runtime_error("card '" + cardname +"' doesn't have any pcm devices");
            }
        }
    }
    throw std::runtime_error("could not find sound card with name: " + cardname);
}

void list_cards( std::ostream &output)
{
    alsalib &alsa( alsalib::get_instance());
    for (auto card : alsa.get_cards())
    {
        output << card.get_index() << '\t' << card.get_name() << ", pcm devices: ";

        for (auto device : card.pcm_devices())
        {
            output << device.get_index() << ", ";
        }
        output << '\n';
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

snd_pcm_format_t bitsize_to_pcm_format( unsigned int bitsize)
{
    if (bitsize <= 8) return SND_PCM_FORMAT_U8;
    if (bitsize <= 16) return SND_PCM_FORMAT_S16_LE;
    if (bitsize <= 24) return SND_PCM_FORMAT_S24_LE;
    if (bitsize <= 32) return SND_PCM_FORMAT_S32_LE;
    throw std::runtime_error( "don't know how to handle samples of bitsize " + std::to_string( bitsize));
}

void set_parameters_from_wav( opened_pcm_device &device, const riff_fmt &format)
{
    device.format( bitsize_to_pcm_format(format.bits_per_sample));
    device.rate( {format.samplerate, 0});
    device.channels( format.channels);
    device.access( SND_PCM_ACCESS_RW_INTERLEAVED);
}

void play_wav( opened_pcm_device &device, const std::string &wavfilename)
{
    wav_file wav;
    std::ifstream wavfile(wavfilename, std::ios::binary);
    if (!parse_wavfile( wavfile,wav)) throw std::runtime_error("parsing file " + wavfilename + " failed");

    std::cout << "bits per sample:" << wav.fmt.bits_per_sample << " rate:" << wav.fmt.samplerate << '\n';
    device.period_size( {128, 0});
    set_parameters_from_wav( device, wav.fmt);
    device.commit_parameters();

    const unsigned int actual_period_size = device.period_size().first;
    const auto framesize = wav.fmt.channels * wav.fmt.bits_per_sample/8 ;
    const auto buffersize = framesize * actual_period_size;

    auto framestogo = wav.data.size/ framesize;
    wavfile.seekg( wav.data.pos, std::ios::beg);
    std::vector<char> buffer(buffersize);
    while (framestogo)
    {
        const auto framecount = std::min( actual_period_size, framestogo);
        wavfile.read( &buffer.front(), framecount * framesize);
        device.writei( &buffer.front(), framecount);
        framestogo -= framecount;
    }

    device.drain();
}
int main()
{
    try
    {
        list_cards( std::cout);
        auto device = find_card_pcm( "Generic USB Audio Device");
        opened_pcm_device pcm( device.first, device.second, SND_PCM_STREAM_PLAYBACK);
        play_wav( pcm, "/home/danny/Documents/433Mhz_experiments/onA.wav");
    }
    catch (std::exception &e)
    {
        std::cerr << "something went wrong: " << e.what() << std::endl;
    }

	return 0;
}
