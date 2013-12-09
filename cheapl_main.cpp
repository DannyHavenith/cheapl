/*
 * cheapl_main.cpp
 *
 *  Created on: Dec 8, 2013
 *      Author: danny
 */
#include <alsa/asoundlib.h>
#include <iostream>

template< int (*next_func)(int*)>
class alsa_iterator
{
public:
	typedef int value_type;
	alsa_iterator(): index(-1){}
	alsa_iterator &operator++() { next_func( &index);  return *this;}
	int operator*() const
	{
		return index;
	}

	bool operator<( const alsa_iterator &other) const
	{
		return index < other.index;
	}

	bool operator!=( const alsa_iterator &other) const
	{
		return index != other.index;
	}
private:
	int index;
};

template< int (*next_func)(int*)>
class alsa_container
{
public:
	typedef alsa_iterator<next_func> iterator_type;
	typedef typename iterator_type::value_type value_type;
	iterator_type begin()
	{
		return ++iterator_type();
	}
	iterator_type end()
	{
		return iterator_type();
	}
};

using snd_cards = alsa_container< &snd_card_next>;

int main()
{
	alsa_iterator<&snd_card_next> snd_card_iterator;
	for (int card: snd_cards())
	{
		std::cout << card << '\n';
	}
	return 0;
}



