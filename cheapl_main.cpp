/*
 * cheapl_main.cpp
 *
 *  Created on: Dec 8, 2013
 *      Author: danny
 */
#include <alsa/asoundlib.h>

template< int (*next_func)(int*)>
class alsa_iterator
{
public:
	alsa_iterator(): index(-1){}
	alsa_iterator &operator++() { next_func( index);  return *this;}
private:
	int index;
};

int main()
{
	alsa_iterator<&snd_card_next> snd_card_iterator;
	return 0;
}



