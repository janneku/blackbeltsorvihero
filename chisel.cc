/*
 * Black Belt Sorvi Hero
 *
 * Copyright 2011 Janne Kulmala <janne.t.kulmala@iki.fi>,
 * Antti Rajamäki <amikaze@gmail.com>
 *
 * Program code and resources are licensed with GNU LGPL 2.1. See
 * COPYING.LGPL file.
 *
 * Chisel-related stuff, different chisels
 */
#include "chisel.h"
#include "wood.h"
#include "sound.h"
#include <stdio.h>

namespace {

/*
	int x1;
	int x2;
	float depth1;
	float depth2;
*/
const Hole flat_blade[] = {
	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole curve_blade[] = {
	{HOLE_CURVE, 0, 15, 0, -0.1},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole sharp_blade[] = {
	{HOLE_LINEAR, 0, 8, 0, -0.1},
	{HOLE_LINEAR, 8, 15, -0.1, 0},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole luxsaber_blade[] = {
	{HOLE_CURVE, 0, 30, 0, -0.2},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

Vertex blade_vertex(const Blade *blade, const float *profile, int i)
{
	assert(i >= 0 && i < blade->width);

	int l = i - 1;
	if (l < 0)
		l = 0;
	int r = i + 1;
	if (r >= blade->width)
		r = blade->width - 1;

	Vertex v;
	v.norm = vec3((profile[l] - profile[r])*6, 0, r-l);
	v.pos = vec3(i * PROFILE_STEP, BLADE_LEN, profile[i]*6);
	return v;
}

Vertex blade_vertex2(const Blade *blade, const float *profile, int i)
{
	int l = i - 1;
	if (l < 0)
		l = 0;
	int r = i + 1;
	if (r >= blade->width)
		r = blade->width - 1;

	Vertex v;
	v.norm = vec3((profile[l] - profile[r])*-6, 0, l-r);
	v.pos = vec3(i * PROFILE_STEP, BLADE_LEN, profile[i]*6);
	return v;
}

}

float current_blade[MAX_BLADE_WIDTH];
int blade_num;
const Chisel *inventory[NUM_BLADES];
float stress[NUM_BLADES];
float blade_x = 0;
Animator<float> blade_raise;
Text chisel_name;
bool cutting;
Sound cutting_sound;
Sound luxsaber_hit;
Sound luxsaber_always;
Sound sanding;

const Blade blades[] = {
	{flat_blade, 15, false},
	{flat_blade, 15, false}, /* faster */
	{curve_blade, 15, false},
	{flat_blade, 30, false},
	{sharp_blade, 15, false},
	{flat_blade, 30, true}, /* sand */
	{luxsaber_blade, 30, false},
	{flat_blade, 5, false}, /* chainsaw */
};

/*
	int type;
	const char *name;
	int handle;
	float speed;
	float max_stress;
	int price;
	int level;
*/
const Chisel chisels[] = {
	{BLADE_FLAT,
	"Basic flat",
	"A basic chisel for amateurs.",
	HANDLE_BASIC, 0.0001, 5, 100, 0},

	{BLADE_CURVE,
	"Basic curve",
	"Curved chisel for rounded shapes.",
	HANDLE_BASIC, 0.0001, 5, 500, 0},

	{BLADE_WIDE,
	"Basic wide flat",
	"A wider version of the flat\n"
	"chisel.",
	HANDLE_BASIC, 0.0001, 5, 500, 0},

	{BLADE_FLAT,
	"Strong flat",
	"Stronger flat-headed chisel.",
	HANDLE_PRO, 0.0001, 10, 2000, 0},

	{BLADE_SAND,
	"Sanding hand",
	"A special hand to smooth out edges.\n"
	"Perfect for the small final touches.",
	HANDLE_HAND, 0.00003, 5, 1000, 0},

	{BLADE_SHARP,
	"Basic sharp",
	"Sharp-shaped chisel for\n"
	"making sharp grooves.",
	HANDLE_BASIC, 0.0001, 5, 1000, 1},

	{BLADE_FAST,
	"Ultimate flat",
	"The best flat-headed chisel\n"
	"there is.",
	HANDLE_ULTIMATE, 0.0002, 15, 2000, 2},

	{BLADE_CURVE,
	"Ultimate curve",
	"Improved version of the curved chisel\n"
	"that can last higher temperatures.",
	HANDLE_ULTIMATE, 0.0002, 15, 2000, 2},

	{BLADE_WIDE,
	"Samurai's revenge",
	"The carving tool of the ancient\n"
	"chinese. Extremely high quality.",
	HANDLE_SCIFI, 0.0003, 15, 3000, 3},

	{BLADE_SAND,
	"Jedi's hand",
	"The second best friend of a Jedi.\n"
	"Excellent for smoothing out sharp\n"
	"edges. Faster and more resistant.",
	HANDLE_HAND, 0.0001, 10, 3000, 3},

	{BLADE_LUXSABER,
	"Luxsaber",
	"The best friend of a Jedi!\n"
	"Suitable for making large holes\n"
	"fast! May the force be with you.",
	HANDLE_LUXSABER, 0.0005, 30, 5000, 3},

	{BLADE_CHAINSAW,
	"Chainsaw",
	"",
	HANDLE_CHAINSAW, 0.0005, 30, 5000, 3},

	{0, NULL, 0, 0, 0, 0, 0}
};

void create_hole(float *profile, int len, const Hole *hole)
{
	assert(hole->x2 > hole->x1);
	switch (hole->type) {
	case HOLE_CURVE:
		for (int i = hole->x1; i < hole->x2; ++i) {
			if (i >= 0 && i < len) {
				float depth = hole->depth1
					+ sinf((i - hole->x1) * M_PI / (hole->x2 - hole->x1))
					* (hole->depth2 - hole->depth1);
				if (depth < profile[i])
					profile[i] = depth;
			}
		}
		break;
	case HOLE_LINEAR:
		for (int i = hole->x1; i < hole->x2; ++i) {
			if (i >= 0 && i < len) {
				float depth = hole->depth1
					+ float(i - hole->x1) / (hole->x2 - hole->x1)
					* (hole->depth2 - hole->depth1);
				if (depth < profile[i])
					profile[i] = depth;
			}
		}
		break;
	default:
		assert(0);
		break;
	}
}

void begin_cutting()
{
	cutting = true;
	if (blade_num == BLADE_LUXSABER) {
		play_sound(&luxsaber_hit, LOOP_INFINITE);
	} else if (blade_num == BLADE_SAND) {
		play_sound(&sanding, LOOP_INFINITE);
	} else {
		play_sound(&cutting_sound, LOOP_INFINITE);
	}
}

void end_cutting()
{
	cutting = false;
	stop_sound(&luxsaber_hit);
	stop_sound(&cutting_sound);
	stop_sound(&sanding);
}

void switch_blade()
{
	assert(blade_num >= 0 && blade_num < NUM_BLADES);

	const Chisel *chisel = inventory[blade_num];
	const Blade *blade = &blades[blade_num];

	cutting = false;
	create_blade(blade, current_blade);
	blade_raise.set_value(10);
	chisel_name.init(font, chisel->name);

	if (blade_num == BLADE_LUXSABER) {
		play_sound(&luxsaber_always, LOOP_INFINITE);
	}
}

void buy(const Chisel *chisel)
{
	assert(chisel->blade >= 0 && chisel->blade < NUM_BLADES);

	if (chisel->price > score) {
		return;
	}
	score -= chisel->price;
	inventory[chisel->blade] = chisel;
	stress[chisel->blade] = 0;
}

void create_blade(const Blade *blade, float *profile)
{
	assert(blade->width > 0 && blade->width <= MAX_BLADE_WIDTH);
	memset(profile, 0, blade->width * sizeof(float));
	for (int i = 0; blade->holes[i].type >= 0; ++i) {
		create_hole(profile, blade->width, &blade->holes[i]);
	}
}

void draw_chisel(const Chisel *chisel, const float *profile)
{
	static bool ready = false;
	static Model handles[MAX_HANDLES];
	if (!ready) {
		ready = true;
		handles[HANDLE_BASIC].load("basic.obj");
		handles[HANDLE_PRO].load("pro.obj");
		handles[HANDLE_ULTIMATE].load("ultimate.obj");
		handles[HANDLE_SCIFI].load("scifi.obj");
		handles[HANDLE_LUXSABER].load("luxsaber.obj");
		handles[HANDLE_HAND].load("deadhand.obj");
		handles[HANDLE_CHAINSAW].load("chainsaw.obj");
	}

	const Blade *blade = &blades[chisel->blade];

	float color[] = {0.4, 0.4, 0.4, 1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);

	if (chisel->handle != HANDLE_LUXSABER &&
	    chisel->handle != HANDLE_HAND) {
		GLState state;
		state.enable(GL_LIGHTING);

		GLMatrixScope matrix;
		glTranslatef(blade->width * -PROFILE_STEP/2, 0, 0);

		const float diffuse[4] = {0.5, 0.5, 0.7, 1};
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);

		Vertex vert[MAX_BLADE_WIDTH * 4 * 3];
		int vi = 0;
		vert[vi].norm = vec3(-1, 0, 0);
		vert[vi++].pos = vec3(0, BLADE_LEN, profile[0]*6);
		vert[vi].norm = vec3(-1, 0, 0);
		vert[vi++].pos = vec3(0, 0, -0.2);
		vert[vi].norm = vec3(-1, 0, 0);
		vert[vi++].pos = vec3(0, 0, 0.2);

		vert[vi].norm = vec3(1, 0, 0);
		vert[vi++].pos = vec3((blade->width - 1) * PROFILE_STEP, BLADE_LEN,
				      profile[blade->width-1]*6);
		vert[vi].norm = vec3(1, 0, 0);
		vert[vi++].pos = vec3((blade->width - 1) * PROFILE_STEP, 0, 0.2);
		vert[vi].norm = vec3(1, 0, 0);
		vert[vi++].pos = vec3((blade->width - 1) * PROFILE_STEP, 0, -0.2);
		draw_array(GL_TRIANGLES, vert, 2 * 3);

		vi = 0;
		for (int i = 0; i < blade->width - 1; ++i) {
			vert[vi++] = blade_vertex(blade, profile, i + 1);
			vert[vi++] = blade_vertex(blade, profile, i);
			vert[vi].norm = vec3(0, 0, 1);
			vert[vi++].pos = vec3((i + 1) * PROFILE_STEP, 0, 0.2);

			vert[vi++] = blade_vertex(blade, profile, i);
			vert[vi].norm = vec3(0, 0, 1);
			vert[vi++].pos = vec3(i * PROFILE_STEP, 0, 0.2);
			vert[vi].norm = vec3(0, 0, 1);
			vert[vi++].pos = vec3((i + 1) * PROFILE_STEP, 0, 0.2);

			vert[vi++] = blade_vertex2(blade, profile, i);
			vert[vi++] = blade_vertex2(blade, profile, i + 1);
			vert[vi].norm = vec3(0, 0, -1);
			vert[vi++].pos = vec3(i * PROFILE_STEP, 0, -0.2);

			vert[vi++] = blade_vertex2(blade, profile, i + 1);
			vert[vi].norm = vec3(0, 0, -1);
			vert[vi++].pos = vec3((i + 1) * PROFILE_STEP, 0, -0.2);
			vert[vi].norm = vec3(0, 0, -1);
			vert[vi++].pos = vec3(i * PROFILE_STEP, 0, -0.2);
		}
		draw_array(GL_TRIANGLES, vert, (blade->width - 1) * 4 * 3);
	}

	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);

	assert(chisel->handle >= 0 && chisel->handle < MAX_HANDLES);

	GLMatrixScope matrix;
	if (chisel->handle == HANDLE_HAND) {
		glTranslatef(0, BLADE_LEN, 0);
	}
	handles[chisel->handle].draw();
}
