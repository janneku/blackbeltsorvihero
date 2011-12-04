#include <stdint.h>
#include <vector>

struct Sound {
	std::vector<int16_t> data;
	int channels;
};

#define LOOP_INFINITE -1

extern bool music_enabled;

void load_mpeg(Sound *sound, const char *fname);
void play_sound(const Sound *sound, int looping = 0);
void stop_sound(Sound *sound);
void play_music(const char *fname);
void kill_sounds();
void init_sound();
