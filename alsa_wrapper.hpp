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

/// exception thrown when the alsa wrapper encounters an underlying alsa error.
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

/// internally used function to throw an exception whenever an alsa api function returns an error.
int throw_if_error(int returnvalue)
{
	if (returnvalue < 0)
	{
		throw alsa_exception(returnvalue);
	}

	return returnvalue;
}

/// generic container facade to alsa objects like sound cards, pcm devices, etc.
/// Alsa iteration normally follows a pattern of starting an index at 0 and then
/// letting some API function increase the index for every instance of an object type until
/// there are no objects left.
template<typename iterator_t, typename handle_t = int>
class alsa_container
{
public:
    typedef iterator_t iterator_type;
    typedef typename iterator_type::value_type value_type;

    /// Construct a container. The handle argument should be the initial handle value.
    /// Typically this is -1. The begin-iterator is created 'incrementing' that initial value (typically by calling some alsa function).
    /// The initial value is also the end-value of iterators (alsa will set the integer enumeration value to -1 to indicate there are no
    /// more objects to enumerate.
    alsa_container( handle_t handle = handle_t{-1})
    :handle( handle) {}

    iterator_type begin() const
    {
        return ++iterator_type(handle);
    }

    iterator_type end() const
    {
        return iterator_type(handle);
    }
private:
    const handle_t handle;
};

struct pcm_device
{
	pcm_device(int index) :
			index(index)
	{
	}

	int get_index() const
	{
	    return index;
	}

private:
	int index;
};

class pcm_device_iterator: public std::iterator<std::forward_iterator_tag, pcm_device>
{
public:
	pcm_device_iterator(snd_ctl_t *handle) :
			card_handle{ handle }
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

/// This class represents an opened alsa sound card.
/// This class is designed so that each sound card in the system is opened only once--if at all.
/// Applications typically use the soundcard class, which is a wrapper around a boost.flyweight object
/// for objects of this class.
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

    using  device_container_type = alsa_container<pcm_device_iterator> ;
    const device_container_type& pcm_devices() const
    {
        return devices;
    }
private:
	std::string                          devicename;
	snd_ctl_t                            *handle;
	std::shared_ptr<snd_ctl_card_info_t> info;
	device_container_type             devices;
};

/// This class is used to identify each opened_soundcard instance to
/// boost.flyweight.
struct soundcard_devicename_extractor
{
	const std::string &operator()(const opened_soundcard &card)
	{
		return card.get_devicename();
	}
};

/// This is a flyweight implementation for opened_soundcard instances (which are the
/// more heavy weight objects).
class soundcard
{
public:

	explicit soundcard(int index) :
			index(index), card{ "hw:" + std::to_string(index) }
	{
	}

	soundcard() : soundcard{0} {}

	std::string get_name() const
	{
		return snd_ctl_card_info_get_name(get_card().get_info());
	}

	int get_index() const
	{
	    return index;
	}

	using  soundcard_container_type = opened_soundcard::soundcard_container_type;
	const soundcard_container_type &get_
private:

	const opened_soundcard &get_card() const
	{
		return card;
	}

	int index;
	using card_flyweight_type =  boost::flyweights::flyweight<
			boost::flyweights::key_value<std::string, opened_soundcard,
					soundcard_devicename_extractor> >;

	card_flyweight_type card;
};

/// iterator over the soundcards that alsa offers.
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

class alsalib
{
public:

	using  soundcard_container_type = alsa_container<soundcard_iterator> ;

	const soundcard_container_type &get_cards() const
	{
		return cards;
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

	soundcard_container_type cards{0};
};

#endif /* ALSA_WRAPPER_HPP_ */
