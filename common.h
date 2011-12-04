#if !defined(_COMMON_H)
#define _COMMON_H

#define NAME	"Black Belt Sorvi Hero"
#define DIR_NAME	"BlackBeltSorviHero"

#include <SDL.h>
#if !defined(M_PI)
#define M_PI 3.14159265358979323846
#endif
#include <string>
#include "utils.h"
#include "gl.h"

class Shape;

struct Level {
	const Shape *shapes;
	const char *descr;
	const char *music;
	int time_limit;
	int min_score;
};

struct Particle {
	bool smoke;
	color c;
	float time;
	vec3 pos;
	vec3 vel;
};

class Timer {
public:
	Timer();
	float get_dt();

private:
	Uint32 m_last_ticks;
};

#if !defined(CONFIG_GLES)
const int DIGIT_WIDTH = 40;
const int DIGIT_HEIGHT = 64;
#else
const int DIGIT_WIDTH = 20;
const int DIGIT_HEIGHT = 32;
#endif

template<class T>
T zero()
{
	return 0;
}

template<>
inline vec3 zero<vec3>()
{
	return vec3(0, 0, 0);
}

template<>
inline vec2 zero<vec2>()
{
	return vec2(0, 0);
}

class Once {
public:
	Once() :
		done(false)
	{}

	operator bool()
	{
		if (done)
			return false;
		done = true;
		return true;
	}

private:
	bool done;
};

template<class T>
class Animator {
public:
	Animator(const T &val = zero<T>()) :
		m_value(val), m_vel(zero<T>()), m_target(val)
	{}

	operator T() const { return m_value; }

	T value() const { return m_value; }

	void set_value(const T &val)
	{
		m_value = val;
		m_vel = zero<T>();
		m_target = val;
	}
	void set_target(const T &val)
	{
		m_target = val;
	}

	void update(float dt, float force)
	{
		const float MAX_STEP = 0.02;
		float friction = sqrt(force) * 2;

		while (dt > MAX_STEP) {
			T accel = (m_target - m_value) * force - m_vel * friction;
			m_value = m_value + m_vel * MAX_STEP;
			m_vel = m_vel + accel * MAX_STEP;
			dt -= MAX_STEP;
		}
		T accel = (m_target - m_value) * force - m_vel * friction;
		m_value = m_value + m_vel * dt;
		m_vel = m_vel + accel * dt;
	}

private:
	T m_value;
	T m_vel;
	T m_target;
};

class PackFile {
public:
	PackFile() {}
	PackFile(const char *fname);

	size_t offset() const { return m_offset; }
	size_t size() const { return m_size; }

	void seek(size_t offset);

	size_t read(void *buf, size_t len);

private:
	size_t m_begin;
	size_t m_offset;
	size_t m_size;
};

struct Setting {
	bool *ptr;
	const char *name;
	const char *text;
};

class Model;

extern int scr_width;
extern int scr_height;
extern bool fullscreen;
extern TTF_Font *font;
extern int score;
extern Model sorvi;
extern Model room;
extern Model akseli;
extern Model support;
extern const Level *level;
extern int level_num;
extern const Setting setting_list[];

float randf(float min, float max);
void exit();
void hud_matrix();
void open_window();
bool get_event(SDL_Event *e);
void set_particles_light(bool bloom);
void update_particles(float dt);
void draw_particles();
void draw_smoke(const vec3 &camera);
void add_particle(Particle p);
void kill_particles();
void draw_number(int v, int len);
void open_pack(const char *fname);
void load_settings();
void save_settings();
void set_setting(const Setting *setting, bool state);

#endif
