/*
 * Black Belt Sorvi Hero
 *
 * Copyright 2011 Janne Kulmala <janne.t.kulmala@iki.fi>,
 * Antti Rajamäki <amikaze@gmail.com>
 *
 * Program code and resources are licensed with GNU LGPL 2.1. See
 * COPYING.LGPL file.
 *
 *
 * The sound system.
 */
#include "sound.h"
#include "common.h"
#include "utils.h"
#include <SDL.h>
#include <limits.h>
#define fseeko64 fseek
#include <vorbis/vorbisfile.h>
#include <stdexcept>

const int INT16_MIN = -0x8000;
const int INT16_MAX = 0x7fff;
const int SAMPLERATE = 44100;

namespace {

struct Playing {
	const Sound *sound;
	size_t pos;
	int looping;
};

std::list<Playing> playing;
OggVorbis_File music;
PackFile music_file;

size_t read_cb(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	PackFile *f = (PackFile *) datasource;

	return f->read(ptr, size * nmemb) / size;
}

int seek_cb(void *datasource, ogg_int64_t off, int whence)
{
	PackFile *f = (PackFile *) datasource;

	switch (whence) {
	case SEEK_SET:
		f->seek(off);
		break;
	case SEEK_CUR:
		f->seek(off + f->offset());
		break;
	case SEEK_END:
		f->seek(off + f->size());
		break;
	default:
		assert(0);
	}
	return f->offset();
}

long tell_cb(void *datasource)
{
	PackFile *f = (PackFile *) datasource;

	return f->offset();
}

const ov_callbacks callbacks = {
	read_cb,
	seek_cb,
	NULL,
	tell_cb,
};

void fill_audio(void *userdata, Uint8 *buf, int fill_len)
try {
	UNUSED(userdata);

	if (music.datasource == NULL && playing.empty()) {
		memset(buf, 0, fill_len);
		return;
	}

	static int16_t music_buf[4096];
	static size_t music_pos = 0;
	static size_t music_len = 0;

	int16_t *ptr = (int16_t *) buf;
	for (size_t i = 0; i < size_t(fill_len) / 2; i += 2) {
		while (music.datasource != NULL && music_pos >= music_len) {
			int bitstream = 0;
			long got = ov_read(&music, (char *) music_buf, 4096 * 2,
					   0, 2, 1, &bitstream);
			if (got == OV_HOLE) {
				continue;
			}
			if (got <= 0) {
				ov_time_seek(&music, 0);
				continue;
			}
			vorbis_info *vi = ov_info(&music, -1);
			assert(vi->channels == 2);
			music_pos = 0;
			music_len = got / 2;
		}

		int l = 0, r = 0;
		if (music.datasource != NULL) {
			l = music_buf[music_pos];
			r = music_buf[music_pos + 1];
			music_pos += 2;
		}

		for (safe_list_iter<Playing> p(playing); p; ++p) {
			const Sound *sound = p->sound;
			l += sound->data[p->pos];
			if (sound->channels == 2) {
				r += sound->data[p->pos + 1];
			} else {
				r += sound->data[p->pos];
			}
			p->pos += sound->channels;
			if (p->pos >= sound->data.size()) {
				if (p->looping != 0) {
					if (p->looping != LOOP_INFINITE) {
						p->looping--;
					}
					p->pos = 0;
				} else {
					playing.erase(p.iter());
				}
			}
		}
		if (l < INT16_MIN) l = INT16_MIN;
		if (l > INT16_MAX) l = INT16_MAX;
		if (r < INT16_MIN) r = INT16_MIN;
		if (r > INT16_MAX) r = INT16_MAX;
		ptr[i] = l;
		ptr[i + 1] = r;
	}
} catch (const std::exception &e) {
	printf("ERROR in audio fill: %s\n", e.what());
}

}

bool music_enabled = true;

void load_mpeg(Sound *sound, const char *fname)
{
	PackFile f(fname);

	printf("loading %s\n", fname);

	OggVorbis_File vf;
	if (ov_open_callbacks(&f, &vf, NULL, 0, callbacks)) {
		throw std::runtime_error(std::string("Not a vorbis file ")
					 + fname);
	}

	sound->data.clear();
	sound->channels = 0;

	std::string buffer;
	while (true) {
		size_t pos = sound->data.size();
		sound->data.resize(pos + 4096, 0);
		int bitstream = 0;
		long got = ov_read(&vf, (char *) &sound->data[pos], 4096 * 2,
				   0, 2, 1, &bitstream);
		if (got == OV_HOLE) {
			sound->data.resize(pos);
			continue;
		}
		if (got <= 0) {
			sound->data.resize(pos);
			break;
		}
		sound->data.resize(pos + got / 2);

		vorbis_info *vi = ov_info(&vf, -1);
		if (sound->channels > 0) {
			assert(sound->channels == vi->channels);
		} else {
			sound->channels = vi->channels;
		}
	}
	ov_clear(&vf);

	printf("  %d channels\n", sound->channels);
	printf("  %d samples, %.1f sec\n", int(sound->data.size()),
		float(sound->data.size() / sound->channels) / SAMPLERATE);
}

void play_sound(const Sound *sound, int looping)
{
	SDL_LockAudio();
	Playing p;
	p.sound = sound;
	p.looping = looping;
	p.pos = 0;
	playing.push_back(p);
	SDL_UnlockAudio();
}

void stop_sound(Sound *sound)
{
	SDL_LockAudio();
	for (safe_list_iter<Playing> p(playing); p; ++p) {
		if (p->sound == sound) {
			playing.erase(p.iter());
		}
	}
	SDL_UnlockAudio();
}

void play_music(const char *fname)
{
	printf("playing: %s\n", fname);
	SDL_LockAudio();
	if (music.datasource != NULL) {
		ov_clear(&music);
		music.datasource = NULL;
	}
	if (fname != NULL && music_enabled) {
		PackFile f(fname);
		music_file = f;
		if (ov_open_callbacks(&music_file, &music, NULL, 0, callbacks)) {
			throw std::runtime_error(std::string("Not a vorbis file ")
						 + fname);
		}
	}
	SDL_UnlockAudio();
}

void kill_sounds()
{
	SDL_LockAudio();
	playing.clear();
	SDL_UnlockAudio();
}

void init_sound()
{
	printf("init_sound()\n");

	music.datasource = NULL;

	SDL_AudioSpec spec;
	SDL_AudioSpec obtained;
	spec.freq = SAMPLERATE;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples = 4410;
	spec.callback = fill_audio;
	if (SDL_OpenAudio(&spec, &obtained)) {
		warning("Unable to open audio: %s\n", SDL_GetError());
		return;
	}
	printf("  sound ready, %d Hz\n", obtained.freq);
}
