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

    /// Construct a container.
    alsa_container( handle_t handle)
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

	pcm_device operator*() const
	{
	    return pcm_device( index);
	}

private:
	int index{-1};
	snd_ctl_t *card_handle;
};

/// a utility object that will create an alsa-related object using the proper malloc function and that
/// will automatically free that same object using the proper free function, when destructed.
template< typename object_type, int (*alloc_func)( object_type **), void (*free_func)( object_type *)>
class alsa_object_wrapper
{
public:
    alsa_object_wrapper()
    :ptr( allocate(), free_func){}

    object_type *get() const
    {
        return ptr.get();
    }

    object_type &operator*() const
    {
        return *ptr;
    }

    object_type *operator->() const
    {
        return ptr.get();
    }

private:
    static object_type *allocate()
    {
        object_type *ptr = nullptr;
        throw_if_error( alloc_func( &ptr));
        return ptr;
    }
    std::shared_ptr<object_type> ptr;
};

#define ALSA_OBJECT_WRAPPER( objecttype) alsa_object_wrapper<objecttype##_t, objecttype##_malloc, objecttype##_free>;
using snd_ctl_card_info_wrapper = ALSA_OBJECT_WRAPPER(snd_ctl_card_info);
using snd_pcm_hw_params_wrapper = ALSA_OBJECT_WRAPPER( snd_pcm_hw_params);


#define IMPLEMENT_PARAM( name)    \
        void name( typename parameter_type<decltype(snd_pcm_hw_params_set_##name)>::type val) \
        {                               \
            set( snd_pcm_hw_params_set_##name, val);\
        }\
        typename parameter_type<decltype(snd_pcm_hw_params_set_##name)>::type name() const\
        {\
            return get( snd_pcm_hw_params_get_##name);\
        }\
        /**/


class opened_pcm_device
{
public:
    opened_pcm_device( int cardnumber, int devicenumber, snd_pcm_stream_t stream)
    :handle( open(cardnumber,devicenumber, stream), snd_pcm_close)
    {
        snd_pcm_hw_params_any( handle.get(), hw_params.get());
    }

    template<typename V>
    struct parameter_type {};

    template<typename P, typename H, typename V>
    struct parameter_type<int (H *, P *, V )>
    {
        using type = V;
    };

    template<typename P, typename H, typename V, typename W>
    struct parameter_type<int (H *, P *, V , W)>
    {
        using type = std::pair<V, W>;
    };

    IMPLEMENT_PARAM( format)
    IMPLEMENT_PARAM( channels)
    IMPLEMENT_PARAM( access)
    IMPLEMENT_PARAM( rate)
    IMPLEMENT_PARAM( period_size)
    IMPLEMENT_PARAM( period_time)

    void commit_parameters()
    {
        throw_if_error(snd_pcm_hw_params( get_handle(), get_params()));
    }

    void writei( char *buffer, size_t framecount)
    {
        snd_pcm_writei( get_handle(), buffer, framecount);
    }

    void drain()
    {
        snd_pcm_drain( get_handle());
    }
private:

    static snd_pcm_t *open( int cardnumber, int devicenumber, snd_pcm_stream_t stream)
    {
        snd_pcm_t *handle = nullptr;
        using std::to_string;
        const std::string devicename = "plughw:" + to_string(cardnumber) + ","+ to_string( devicenumber);
        throw_if_error(snd_pcm_open( &handle, devicename.c_str(), stream, 0));
        return handle;
    }

    template<typename T>
    void set( int (*set_func)(snd_pcm_t *, snd_pcm_hw_params_t *, T ), T value)
    {
        throw_if_error( set_func(get_handle(), get_params(), value));
    }

    template<typename T, typename U>
    void set( int (*set_func)(snd_pcm_t *, snd_pcm_hw_params_t *, T , U), std::pair<T, U> value)
    {
        throw_if_error( set_func(get_handle(), get_params(), value.first, value.second));
    }

    template< typename T>
    T get( int (*get_func)(const snd_pcm_hw_params_t *, T *)) const
    {
        T value;
        throw_if_error( get_func( get_params(), &value));
        return value;
    }

    template< typename T, typename U >
    std::pair< T, U> get( int (*get_func)(const snd_pcm_hw_params_t *, T *, U *)) const
    {
        std::pair<T, U> value;
        throw_if_error( get_func( get_params(), &value.first, &value.second));
        return value;
    }

    snd_pcm_t *get_handle() const
    {
        return handle.get();
    }

    snd_pcm_hw_params_t *get_params() const
    {
        return hw_params.get();
    }

    std::shared_ptr<snd_pcm_t>  handle;
    snd_pcm_hw_params_wrapper   hw_params;
};

/// This class represents an opened alsa sound card.
/// This class is designed so that each sound card in the system is opened only once--if at all.
/// Applications typically use the soundcard class, which is a wrapper around a boost.flyweight object
/// for objects of this class.
class opened_soundcard: boost::noncopyable
{
public:
	opened_soundcard( int cardnumber)
    :cardnumber{cardnumber}, handle{ open_snd_ctl( cardnumber)}, devices{ handle}
	{
		snd_ctl_card_info(handle, info.get());
	}

	snd_ctl_card_info_t *get_info() const
	{
		return info.get();
	}


	int get_cardnumber() const
	{
	    return cardnumber;
	}

	~opened_soundcard()
	{
		snd_ctl_close(handle);
	}

    using  device_container_type = alsa_container<pcm_device_iterator, snd_ctl_t *> ;
    const device_container_type& pcm_devices() const
    {
        return devices;
    }
private:

    /// Small function that returns a snd_ctl_t pointer. The alsa function
    /// doesn't return such a pointer, but takes it as an output parameter, which makes it
    /// tricky to use in member initializers.
    static snd_ctl_t *open_snd_ctl( int devicenumber)
    {
        snd_ctl_t *handle = nullptr;
        const std::string devicename = "hw:" + std::to_string( devicenumber);
        throw_if_error(snd_ctl_open(&handle, devicename.c_str(), 0));
        return handle;
    }

    int                   cardnumber;
	snd_ctl_t             *handle;
	snd_ctl_card_info_wrapper info;
	device_container_type devices;
};

/// This class is used to identify each opened_soundcard instance to
/// boost.flyweight.
struct soundcard_devicenumber_extractor
{
	int operator()(const opened_soundcard &card)
	{
		return card.get_cardnumber();
	}
};

/// This is a flyweight implementation for opened_soundcard instances (which are the
/// more heavy weight objects).
class soundcard
{
public:

	explicit soundcard(int index) :
			index(index), card{ index }
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

	using  device_container_type = opened_soundcard::device_container_type;
    const device_container_type& pcm_devices() const
    {
        return get_card().pcm_devices();
    }

private:

	const opened_soundcard &get_card() const
	{
		return card;
	}

	int index;
	using card_flyweight_type =  boost::flyweights::flyweight<
			boost::flyweights::key_value<int, opened_soundcard,
					soundcard_devicenumber_extractor> >;

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
