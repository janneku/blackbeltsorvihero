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
 * Common stuff: timer, events, window, settings
 */
#include "common.h"
#include "sound.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <fcntl.h>
#if !defined(_WIN32)
#define O_BINARY	0
#endif

const float PARTICLE_TIME = 2;
const float MAX_TIME_STEP = 0.2;

namespace {

struct Entry {
	size_t offset;
	size_t size;
};

#define SETTINGS_FILE "sorvi.cfg"

std::list<Particle> particles;
std::list<Particle> smoke;

int pack_fd;
std::map<std::string, Entry> entries;

}

const Setting setting_list[] = {
#if !defined(CONFIG_GLES)
	{&shaders_enabled, "shaders", "High quality (OpenGL 2.0 required)"},
	{&fullscreen, "fullscreen", "Fullscreen (takes effect after a restart)"},
#endif
	{&music_enabled, "music", "Music enabled"},

	/* terminator */
	{NULL, NULL, NULL}
};

bool fullscreen = true;
int scr_width = 1024;
int scr_height = 768;
TTF_Font *font = NULL;
Model sorvi;
Model room;
Model akseli;
Model support;
int score = 0;
const Level *level = NULL;
int level_num;

PackFile::PackFile(const char *fname)
{
	Entry entry = get(entries, std::string(fname));
	m_begin = entry.offset;
	m_offset = 0;
	m_size = entry.size;
}

void PackFile::seek(size_t offset)
{
	m_offset = offset;
	if (m_offset > m_size) {
		m_offset = m_size;
	}
}

size_t PackFile::read(void *buf, size_t len)
{
	if (m_offset + len > m_size) {
		len = m_size - m_offset;
	}
	lseek(pack_fd, m_begin + m_offset, SEEK_SET);
	if (::read(pack_fd, buf, len) < int(len)) {
		throw std::runtime_error("Unable to read from pack file");
	}
	m_offset += len;
	return len;
}

float randf(float min, float max)
{
	return rand() * (max - min) / RAND_MAX + min;
}

Timer::Timer() :
	m_last_ticks(0)
{
}

float Timer::get_dt()
{
	Uint32 ticks = SDL_GetTicks();
	float dt = (ticks - m_last_ticks) / 1000.0;
	m_last_ticks = ticks;
	if (dt > MAX_TIME_STEP) dt = MAX_TIME_STEP;
	if (dt < 0) dt = 0;
	return dt;
}

void exit()
{
	save_settings();
	SDL_Quit();
	exit(0);
}

void hud_matrix()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, scr_width, scr_height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void open_window()
{
	int width = scr_width;
	int height = scr_height;
	int flags = SDL_OPENGL|SDL_RESIZABLE;
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	if (fullscreen) {
		width = 0;
		height = 0;
		flags |= SDL_FULLSCREEN;
	}
	SDL_Surface *screen = SDL_SetVideoMode(width, height, 0, flags);
	if (screen == NULL) {
		throw std::runtime_error(
			std::string("Unable to create window: ")
			+ SDL_GetError());
	}

#if defined(CONFIG_SDL_GLES)
#if defined(CONFIG_N950)
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1);

	screen = SDL_SetVideoMode(0, 0, 0, SDL_OPENGLES | SDL_FULLSCREEN);
	assert(screen);

#else
	static SDL_GLES_Context *context = NULL;
	if (context == NULL) {
		SDL_GLES_SetAttribute(SDL_GLES_DEPTH_SIZE, 16);
		context = SDL_GLES_CreateContext();
		if (context == NULL) {
			throw std::runtime_error("Can not create GLES context");
		}
	}
	SDL_ShowCursor(SDL_DISABLE);

	SDL_GLES_MakeCurrent(context);
#endif /* CONFIG_N950 */
#endif

	scr_width = screen->w;
	scr_height = screen->h;
	glViewport(0, 0, scr_width, scr_height);
}

bool get_event(SDL_Event *e)
{
	while (SDL_PollEvent(e)) {
		switch (e->type) {
		case SDL_QUIT:
			exit();

		case SDL_VIDEORESIZE:
			scr_width = e->resize.w;
			scr_height = e->resize.h;
			if (!fullscreen)
#if !defined(_WIN32)
			open_window();
#else
			/* windows */
			glViewport(0, 0, scr_width, scr_height);
#endif
			break;

#if SDL_VERSION_ATLEAST(1, 3, 0)
		case SDL_WINDOWEVENT:
			switch (e->window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				scr_width = e->window.data1;
				scr_height = e->window.data2;
				glViewport(0, 0, scr_width, scr_height);
				break;
			}
			break;
#endif

		case SDL_KEYDOWN:
			switch (e->key.keysym.sym) {
#if defined(DEBUG)
			case SDLK_F5:
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				break;
			case SDLK_F6:
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			case SDLK_F7:
				shaders_enabled = !shaders_enabled;
				break;
#endif
			default:
				/* pass up */
				return true;
			}
			break;

		default:
			/* pass up */
			return true;
		}
	}
	return false;
}

void set_particles_light(bool bloom)
{
	vec3 pos(0, 0, 0);
	float sum = 0;
	color c;
	for (const_list_iter<Particle> i(particles); i; ++i) {
		float s = 1 - i->time / PARTICLE_TIME;
		pos = pos + i->pos * s;
		sum += s;
		c = i->c;
	}
	if (sum > 0.1) {
		c.r *= sum * 0.02;
		c.g *= sum * 0.02;
		c.b *= sum * 0.02;
		set_light(pos * (1 / sum), c, bloom);
	}
}

void update_particles(float dt)
{
	for (safe_list_iter<Particle> i(particles); i; ++i) {
		vec3 accel = i->vel * -0.1 + vec3(0, 0, -50);
		i->pos = i->pos + i->vel * dt + (accel * dt * dt * 0.5);
		i->vel = i->vel + accel * dt;
		i->time += dt;
		if (i->time > PARTICLE_TIME) {
			particles.erase(i.iter());
		}
	}

	for (safe_list_iter<Particle> i(smoke); i; ++i) {
		vec3 accel = i->vel * -0.1 + vec3(0, 0, 20)
			+ vec3(randf(-2, 2), randf(-2, 2), randf(-2, 2)) * dt;
		vec3 v = i->vel;
		i->pos = i->pos + v * dt;
		i->vel = i->vel + accel * dt;
		i->time += dt;
		if (i->time > PARTICLE_TIME) {
			smoke.erase(i.iter());
		}
	}
}

void draw_particles()
{
	std::vector<VertexColor> vert;
	for (const_list_iter<Particle> i(particles); i; ++i) {
		VertexColor v;
		v.c = i->c;
		v.c.a = 1 - i->time / PARTICLE_TIME;
		v.pos = i->pos;
		vert.push_back(v);
	}

	GLState state;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	state.enable(GL_BLEND);
	glPointSize(2);
	draw_array(GL_POINTS, &vert[0], vert.size());
	glPointSize(1);
}

void draw_smoke(const vec3 &camera)
{
	static GLuint smoke_tex = INVALID_TEXTURE;
	if (smoke_tex == INVALID_TEXTURE) {
		smoke_tex = load_texture("smoke.png");
	}

	GLState state;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	state.enable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBindTexture(GL_TEXTURE_2D, smoke_tex);
	state.enable(GL_TEXTURE_2D);

	for (const_list_iter<Particle> i(smoke); i; ++i) {
		vec3 n = camera - i->pos;
		n = n * (1 / length(n));
		vec3 a = cross(n, vec3(0, 0, 1));
		vec3 b = cross(a, n) * (i->time / PARTICLE_TIME * 8);
		a = a * (i->time / PARTICLE_TIME * 8);

		color c = i->c;
		c.a = 1 - i->time / PARTICLE_TIME;
		glColor4fv(&c.r);

		VertexTexCoord vert[2 * 3];
		int vi = 0;
		vert[vi].tc = vec2(0, 0);
		vert[vi++].pos = i->pos - a - b;
		vert[vi].tc = vec2(1, 0);
		vert[vi++].pos = i->pos - a + b;
		vert[vi].tc = vec2(0, 1);
		vert[vi++].pos = i->pos + a - b;

		vert[vi].tc = vec2(1, 0);
		vert[vi++].pos = i->pos - a + b;
		vert[vi].tc = vec2(1, 1);
		vert[vi++].pos = i->pos + a + b;
		vert[vi].tc = vec2(0, 1);
		vert[vi++].pos = i->pos + a - b;
		draw_array(GL_TRIANGLES, vert, 2 * 3);
	}
	glColor4fv(white);
	glDepthMask(GL_TRUE);
}

void add_particle(Particle p)
{
	p.time = 0;
	p.vel = p.vel + vec3(randf(-2, 2), randf(-2, 2), randf(-2, 2));
	if (p.smoke)
		smoke.push_back(p);
	else
		particles.push_back(p);
}

void kill_particles()
{
	smoke.clear();
	particles.clear();
}

void draw_number(int v, int len)
{
	static GLuint digits = INVALID_TEXTURE;
	if (digits == INVALID_TEXTURE) {
		digits = load_texture("digits.png");
	}

	assert(v >= 0);

	GLState state;
	state.enable(GL_BLEND);
	state.enable(GL_TEXTURE_2D);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, digits);
	Face faces[10 * 2];
	for (int i = 0; i < len; ++i) {
		vec3 o((len - i - 1) * DIGIT_WIDTH, 0, 0);

		float x = v % 10 * 40.0 / 512;
		faces[i * 2].vert[0].tc = vec2(x, 0);
		faces[i * 2].vert[0].pos = o + vec3(0, 0, 0);
		faces[i * 2].vert[1].tc = vec2(x, 1);
		faces[i * 2].vert[1].pos = o + vec3(0, DIGIT_HEIGHT, 0);
		faces[i * 2].vert[2].tc = vec2(x + 40.0 / 512, 0);
		faces[i * 2].vert[2].pos = o + vec3(DIGIT_WIDTH, 0, 0);

		faces[i * 2 + 1].vert[0].tc = vec2(x, 1);
		faces[i * 2 + 1].vert[0].pos = o + vec3(0, DIGIT_HEIGHT, 0);
		faces[i * 2 + 1].vert[1].tc = vec2(x + 40.0 / 512, 1);
		faces[i * 2 + 1].vert[1].pos = o + vec3(DIGIT_WIDTH, DIGIT_HEIGHT, 0);
		faces[i * 2 + 1].vert[2].tc = vec2(x + 40.0 / 512, 0);
		faces[i * 2 + 1].vert[2].pos = o + vec3(DIGIT_WIDTH, 0, 0);

		v /= 10;
	}
	draw_faces(faces, len * 2);
}

void open_pack(const char *fname)
{
	pack_fd = open(fname, O_RDONLY | O_BINARY);
	if (pack_fd < 0) {
		throw std::runtime_error(std::string("Unable to open: ")
					 + fname);
	}

	size_t i = 0;
	std::string buf;

	while (true) {
		size_t j = buf.find('\n', i);
		if (j == std::string::npos) {
			buf.erase(buf.begin(), buf.begin() + i);
			i = 0;

			size_t pos = buf.size();
			buf.resize(pos + 256, 0);
			size_t got = read(pack_fd, &buf[pos], 256);
			if (got <= 0)
				break;
			buf.resize(pos + got);
			continue;
		}

		std::istringstream parser(buf.substr(i, j - i));

		std::string name;
		parser >> name;
		if (name == "END")
			break;

		Entry entry;
		parser >> entry.offset >> entry.size;
		ins(entries, name, entry);

		i = j + 1;
	}
}

std::string get_settings_file()
{
#if defined(_WIN32)
	return strf("%s\\" SETTINGS_FILE, getenv("APPDATA"));
#elif defined(__ANDROID__)
	return "/sdcard/" SETTINGS_FILE;
#else
	return strf("%s/." SETTINGS_FILE, getenv("HOME"));
#endif
}

void load_settings()
{
	printf("load_settings()\n");

	std::string fname = get_settings_file();
	printf("  settings file: %s\n", fname.c_str());
	std::ifstream f(fname.c_str());
	if (!f) {
		return;
	}

	std::string line;
	Group group;
	while (std::getline(f, line)) {
		if (line.empty() || line[0] == '#')
			continue;

		std::istringstream parser(line);
		std::string name;
		std::string state;
		parser >> name;
		parser >> state;

		const Setting *setting = NULL;
		for (int i = 0; setting_list[i].ptr != NULL; ++i) {
			setting = &setting_list[i];
			if (setting->name == name) {
				if (state == "yes")
					*setting->ptr = true;
				else if (state == "no")
					*setting->ptr = false;
				break;
			}
		}
		if (setting == NULL) {
			warning("unknown setting: %s\n", name.c_str());
		}
	}
}

void save_settings()
{
	std::string fname = get_settings_file();
	FILE *f = fopen(fname.c_str(), "wb");
	if (f == NULL) {
		warning("Unable to save settings to %s\n", fname.c_str());
		return;
	}
	for (int i = 0; setting_list[i].ptr != NULL; ++i) {
		const Setting *setting = &setting_list[i];
		fprintf(f, "%s %s\n", setting->name,
			*setting->ptr ? "yes" : "no");
	}
	fclose(f);
}

void set_setting(const Setting *setting, bool state)
{
	*setting->ptr = state;
	if (setting->ptr == &music_enabled && !state) {
		play_music(NULL);
	} else if(setting->ptr == &fullscreen) {
#if !defined(_WIN32)
		open_window();
#endif
	}
}
