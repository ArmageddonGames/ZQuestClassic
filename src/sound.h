#ifndef ZC_SOUND_H
#define ZC_SOUND_H

#include "zfix.h"

// XXX What should be done with sounds const-wise?
// Is playing a sound really const? Starting a loop isn't, is it?

class Sound
{
public:
	Sound();
	Sound(short id);
	Sound(const Sound& other);
	Sound& operator=(short newID);
	Sound& operator=(const Sound& other);
	void play() const;
	void play(zfix x) const;
	static void play(short id);
	static void play(short id, zfix x);

private:
	short id;
};

#endif
