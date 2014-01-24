//
//  Copyright (C) 2014 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <map>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>
#include "cheaplservice.h"
#include "alsa_wrapper.hpp"
#include "audiofiles/include/wav_parser.hpp"
#include "audiofiles/include/wav_file.hpp"
#include "xpl_application_service.h"
#include "datagramparser.h"

#include <utility>
#include <fstream>

namespace bf = boost::filesystem;

namespace {

/// Find a PCM output device for the alsa sound card with the given name.
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

/// Find an alsa sound card with the given name.
soundcard find_card( const std::string &name)
{
    alsalib &alsa( alsalib::get_instance());
    for (auto card : alsa.get_cards())
    {
        if (card.get_name() == name) return card;
    }
    throw std::runtime_error("could not find sound card with name: " + name);
}

/// convert a size in bits into a SND_PCM enum value for use in alsa functions.
snd_pcm_format_t bitsize_to_pcm_format( unsigned int bitsize)
{
    if (bitsize <= 8) return SND_PCM_FORMAT_U8;
    if (bitsize <= 16) return SND_PCM_FORMAT_S16_LE;
    if (bitsize <= 24) return SND_PCM_FORMAT_S24_LE;
    if (bitsize <= 32) return SND_PCM_FORMAT_S32_LE;
    throw std::runtime_error( "don't know how to handle samples of bitsize " + std::to_string( bitsize));
}

/// Given a riff_fmt object that was the result of parsing a wav-file, push the wav-file attributes such
/// as sample rate and channel count to the given pcm device.
void set_parameters_from_wav( opened_pcm_device &device, const riff_fmt &format)
{
    device.format( bitsize_to_pcm_format(format.bits_per_sample));
    device.rate( {format.samplerate, 0});
    device.channels( format.channels);
    device.access( SND_PCM_ACCESS_RW_INTERLEAVED);
}

/// play the contents of the given wav-file to the given alsa pcm device.
void play_wav( opened_pcm_device &device, const std::string &wavfilename)
{
    wav_file wav;
    std::ifstream wavfile(wavfilename, std::ios::binary);
    if (!parse_wavfile( wavfile,wav)) throw std::runtime_error("parsing file " + wavfilename + " failed");

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

}

namespace xpl
{

/// Implementation of the pimpl (bridge)-pattern.
/// This struct contains the private members of the cheapl service.
struct cheapl_service::impl
{
    /// mapping from command names to wave files
    using onoffmap = std::map< std::string, bf::path>;

    /// mapping from device names to command maps
    using lightsmap = std::map< std::string, onoffmap>;

    application_service service; ///< xPl service object
    bf::path            directory;///< directory with wav-files
    lightsmap           lights;   ///< mapping of device names and command strings to wav-files
    opened_pcm_device   pcm_device;///< an opened alsa pcm device.
};

/// Construct an xPL service.
/// This constructor may throw an exception if the sound card cannot be found or opened.
cheapl_service::cheapl_service(
        const std::string& directoryname, ///< directory containing wav files to be played
        const std::string& soundcardname, ///< the alsa name of the soundcard device to which the wav files will be played
        const std::string& application_id,///< application id that will appear in xPL messages.
        const std::string& application_version ///< application version that will appear in xPL messages
        )
:pimpl{
    new impl{
        {application_id, application_version}, // application service
        directoryname,                          // directory
        {},                                     // lights map
        {find_card_pcm( soundcardname), SND_PCM_STREAM_PLAYBACK} // pcm device

    }
}
{
    // register our function that handles x10.basic commands
    get_impl().service.register_command( "x10.basic",
            [this]( const message &m){ handle_command( m);});

    scan_files(directoryname);
}

/// default implementation of the destructor.
cheapl_service::~cheapl_service() = default;

/// Start running the xPL service and playing sound files.
void xpl::cheapl_service::run()
{
    get_impl().service.run();
}

/// Utility function that lists all alsa sound cards to the given output stream.
void cheapl_service::list_cards( std::ostream& output)
{
    alsalib& alsa(alsalib::get_instance());
    for (auto card : alsa.get_cards())
    {
        output << card.get_index() << '\t' << card.get_name()
                << ", pcm devices: ";
        for (auto device : card.pcm_devices())
        {
            output << device.get_index() << ", ";
        }
        output << '\n';
    }
}

/// Pimpl pattern: return the internal impl object
cheapl_service::impl& cheapl_service::get_impl()
{
    return *pimpl;
}

/// Sign off from the network.
void cheapl_service::signoff()
{
    get_impl().service.send_termination_message();
}

/// Pimpl pattern: return the internal impl object
const cheapl_service::impl& cheapl_service::get_impl() const
{
    return *pimpl;
}

/// Handle x10 command messages.
/// This function is registered with the xPL service so that incoming
/// command messages of schema type x10.basic are handled.
/// This handler recognizes the "on" and "off" command and plays the appropriate
/// wav-file for the device that is specified in the command message.
void cheapl_service::handle_command( const message& m)
{
    try {
        const auto &command = m.body.at("command");
        const auto &device = m.body.at("device");
        if (command == "on" || command == "off")
        {
            const auto &device_wav = get_impl().lights.at(device).at(command);
            // todo: delegate wav playing to a separate thread queue.
            play_wav( get_impl().pcm_device, device_wav.string());
            message reply( m);
            reply.message_type = "xpl-trig";
            reply.headers["target"] = "*";
            get_impl().service.send( reply);

            // send both an x10.basic and an x10.confirm message, because domogik
            // wants an x10.basic (in error, I think).
            reply.message_schema = "x10.confirm";
            get_impl().service.send( reply);


        }
    }
    catch (std::range_error &)
    {
        // if any of our maps does not contain the entry we're looking for, we
        // silently ignore this message.
    }
}

/// Scan a single directory for wav-files and create a mapping from (device, command) to wav file.
/// This function scans all files with extension ".wav" in the given directory. If the name is either
/// "on<devicename>.wav" or "off<devicename>.wav" then the file will be stored as the file associated with
/// the "on" or "off" command for the given device. If only one of the two wav-files is present for a given
/// device name, the device will be ignored.
void cheapl_service::scan_files( const std::string& directoryname)
{
    using dirit = bf::directory_iterator;
    using boost::to_lower_copy;

    static const boost::regex onoff_regex{R"(^(on|off)([^.]+)\.wav)", boost::regex::icase};
    for (const auto &entry :
            boost::make_iterator_range( dirit(directoryname), dirit()))
    {
        boost::smatch match;
        std::string buffer = entry.path().filename().string();
        if (regex_match( buffer, match, onoff_regex))
        {
            get_impl().lights[match[2]][to_lower_copy(match[1].str())]
                                                             = entry.path();
        }
    }

    // now remove all entries for which there are not both an "on" and "off" wave file:
    auto lights_it = get_impl().lights.begin();
    while (lights_it != get_impl().lights.end())
    {
        if (lights_it->second.size() != 2)
        {
            lights_it = get_impl().lights.erase( lights_it);
        }
        else
        {
            ++lights_it;
        }
    }
}

} /* namespace xpl */
