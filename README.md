CHEAPL, a $3 home automation system
=================================

CHEAPL is an xPL bridge. It acts as an UDP service that listens for xPL messages from a home automation system like, for instance, [domogik](http://www.domogik.org) and it reacts to X10 commands by sending waveforms to a [$3 USB 433Mhz RF device](http://rurandom.org/justintime/index.php?title=CheaplSystem).

Since the RF device is actually a USB sound card with an RF transmitter slammed on to it, what CHEAPL really does is to play sounds to that sound card. As it currently uses ALSA to do that, this will probably only work on linux.

Usage
-----

    cheapl [<wave file directory> [alsa sound device name]]
    
example:

    cheapl /home/me/data/wavs "Generic USB Audio Device"
 
 CHEAPL will scan the given directory for wav files with a file name that fits the
 following format:
 
 <pre>&lt;command&gt;&lt;devicename&gt;.wav</pre>
 
 Where *command* is either *on* or *off* and *devicename* can be any string you like. The device name will be interpreted as an X10 device name.
 
Creating wav files
------------------

 CHEAPL needs wav files, which have been recorded from an RF remote control. The [project page](http://rurandom.org/justintime/index.php?title=CheaplSystem) will tell you how to do that.
 
