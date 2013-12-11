/*
 * alsa_wrapper.hpp
 *
 *  Created on: Dec 11, 2013
 *      Author: danny
 */

#ifndef ALSA_WRAPPER_HPP_
#define ALSA_WRAPPER_HPP_
#include <alsa/asoundlib.h>
#include <string>
#include <boost/flyweight.hpp>
#include <boost/flyweight/key_value.hpp>
#include <exception>
#include <memory>
#include <boost/utility.hpp>
#include <iterator>

class alsa_exception: public std::exception
{
public:
	explicit alsa_exception(int error) :
			error(error)
	{
		// nop
	}

	virtual const char *what() const noexcept override
	{
		return snd_strerror(error);
	}
private:
	int error;
};

int throw_if_error(int returnvalue)
{
	if (returnvalue < 0)
	{
		throw alsa_exception(returnvalue);
	}

	return returnvalue;
}

struct pcm_device
{
	pcm_device(int index) :
			index(index)
	{
	}

private:
	int index;
};

class pcm_device_iterator: public std::iterator<std::forward_iterator_tag, pcm_device>
{
public:
	pcm_device_iterator(snd_ctl_t *handle) :
			card_handle
			{ handle }
	{
	}

	pcm_device_iterator &operator++()
	{
		snd_ctl_pcm_next_device(card_handle, &index);
		return *this;
	}

	bool operator<(const pcm_device_iterator &other) const
	{
		return index < other.index;
	}

	bool operator==(const pcm_device_iterator &other) const
	{
		return index == other.index;
	}

	bool operator!=(const pcm_device_iterator &other) const
	{
		return index != other.index;
	}

private:
	int index = -1;
	snd_ctl_t *card_handle;
};
class opened_soundcard: boost::noncopyable
{
public:
	opened_soundcard(const std::string &devicename)
	{
		throw_if_error(snd_ctl_open(&handle, devicename.c_str(), 0));
		snd_ctl_card_info_t *temp_info = 0;
		snd_ctl_card_info_malloc(&temp_info);
		info.reset(temp_info, snd_ctl_card_info_free);
		snd_ctl_card_info(handle, info.get());
	}

	snd_ctl_card_info_t *get_info() const
	{
		return info.get();
	}

	const std::string &get_devicename() const
	{
		return devicename;
	}

	~opened_soundcard()
	{
		snd_ctl_close(handle);
	}

private:
	std::string devicename;
	snd_ctl_t *handle;
	std::shared_ptr<snd_ctl_card_info_t> info;
};

struct soundcard_devicename_extractor
{
	const std::string &operator()(const opened_soundcard &card)
	{
		return card.get_devicename();
	}
};

class soundcard
{
public:

	explicit soundcard(int index) :
			index(index), card
			{ "hw:" + std::to_string(index) }
	{
	}

	std::string get_name() const
	{
		return snd_ctl_card_info_get_name(get_card().get_info());
	}

private:

	const opened_soundcard &get_card() const
	{
		return card;
	}

	int index;
	typedef boost::flyweights::flyweight<
			boost::flyweights::key_value<std::string, opened_soundcard,
					soundcard_devicename_extractor> > card_flyweight_type;
	card_flyweight_type card;
};

class soundcard_iterator: public std::iterator<std::forward_iterator_tag, soundcard>
{
public:
	// dummy argument int to fit the alsa container iterator concept
	soundcard_iterator( int){}

	soundcard_iterator &operator++()
	{
		snd_card_next(&index);
		return *this;
	}

	value_type operator*() const
	{
		return value_type
		{ index };
	}

	bool operator<(const soundcard_iterator &other) const
	{
		return index < other.index;
	}

	bool operator!=(const soundcard_iterator &other) const
	{
		return index != other.index;
	}
private:
	int index = -1;
};

template<typename iterator_t, typename handle_t = int>
class alsa_container
{
public:
	typedef iterator_t iterator_type;
	typedef typename iterator_type::value_type value_type;

	alsa_container( handle_t handle)
	:handle( handle) {}

	iterator_type begin()
	{
		return ++iterator_type(handle);
	}
	iterator_type end()
	{
		return iterator_type(handle);
	}
private:
	handle_t handle;
};



class alsalib
{
public:

	typedef alsa_container<soundcard_iterator> soundcard_container_type;

	soundcard_container_type get_cards() const
	{
		return soundcard_container_type( 0);
	}

	static alsalib & get_instance()
	{
		static alsalib the_instance;
		return the_instance;
	}
private:
	~alsalib()
	{
		snd_config_update_free_global();
	}
};

#endif /* ALSA_WRAPPER_HPP_ */
