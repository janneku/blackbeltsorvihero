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
 * Wood-related stuff, different wood shapes.
 */
#include "wood.h"
#include "chisel.h"
#include "sound.h"
#include <string.h>

const int WOOD_SIDES = 32;

namespace {

/*
	int x1;
	int x2;
	float depth1;
	float depth2;
*/

const Hole plain[] = {
	{HOLE_LINEAR, PROFILE_LEN * 1/10 - PROFILE_LEN/30, PROFILE_LEN * 1/10, 0.5, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 1/10, PROFILE_LEN * 2/10, 0.3, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 2/10, PROFILE_LEN * 2/10 + PROFILE_LEN/30, 0.3, 0.5},

	{HOLE_LINEAR, PROFILE_LEN * 4/10 - PROFILE_LEN/30, PROFILE_LEN * 4/10, 0.5, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 4/10, PROFILE_LEN * 6/10, 0.3, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 6/10, PROFILE_LEN * 6/10 + PROFILE_LEN/30, 0.3, 0.5},

	{HOLE_LINEAR, PROFILE_LEN * 8/10 - PROFILE_LEN/30, PROFILE_LEN * 8/10, 0.5, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 8/10, PROFILE_LEN * 9/10, 0.3, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 9/10, PROFILE_LEN * 9/10 + PROFILE_LEN/30, 0.3, 0.5},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole sawtooth[] = {
	{HOLE_LINEAR, PROFILE_LEN * 1/10 - PROFILE_LEN/30, PROFILE_LEN * 1/10, 0.5, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 1/10, PROFILE_LEN * 1/10 + PROFILE_LEN/30, 0.3, 0.5},

	{HOLE_LINEAR, PROFILE_LEN * 2/10 - PROFILE_LEN/30, PROFILE_LEN * 2/10, 0.5, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 2/10, PROFILE_LEN * 2/10 + PROFILE_LEN/30, 0.3, 0.5},

	{HOLE_LINEAR, PROFILE_LEN * 3/10 - PROFILE_LEN/30, PROFILE_LEN * 3/10, 0.5, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 3/10, PROFILE_LEN * 3/10 + PROFILE_LEN/30, 0.3, 0.5},

	{HOLE_LINEAR, PROFILE_LEN * 7/10 - PROFILE_LEN/30, PROFILE_LEN * 7/10, 0.5, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 7/10, PROFILE_LEN * 7/10 + PROFILE_LEN/30, 0.3, 0.5},

	{HOLE_LINEAR, PROFILE_LEN * 8/10 - PROFILE_LEN/30, PROFILE_LEN * 8/10, 0.5, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 8/10, PROFILE_LEN * 8/10 + PROFILE_LEN/30, 0.3, 0.5},

	{HOLE_LINEAR, PROFILE_LEN * 9/10 - PROFILE_LEN/30, PROFILE_LEN * 9/10, 0.5, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 9/10, PROFILE_LEN * 9/10 + PROFILE_LEN/30, 0.3, 0.5},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole round_middle[] = {
	{HOLE_LINEAR, PROFILE_LEN * 1/5, PROFILE_LEN * 2/5, 0.5, 0.1},
	{HOLE_CURVE, PROFILE_LEN * 2/5, PROFILE_LEN * 3/5, 0.5, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 3/5, PROFILE_LEN * 4/5, 0.1, 0.5},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole wide_hole[] = {
	{HOLE_CURVE, PROFILE_LEN * 1/4, PROFILE_LEN * 3/4, 0.7, 0.5},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole three_sharps[] = {
	{HOLE_LINEAR, PROFILE_LEN * 1/4 - PROFILE_LEN/12, PROFILE_LEN * 1/4, 0.5, 0.1},
	{HOLE_LINEAR, PROFILE_LEN * 1/4, PROFILE_LEN * 1/4 + PROFILE_LEN/12, 0.1, 0.5},

	{HOLE_LINEAR, PROFILE_LEN/2 - PROFILE_LEN/12, PROFILE_LEN/2, 0.5, 0.1},
	{HOLE_LINEAR, PROFILE_LEN/2, PROFILE_LEN/2 + PROFILE_LEN/12, 0.1, 0.5},

	{HOLE_LINEAR, PROFILE_LEN * 3/4 - PROFILE_LEN/12, PROFILE_LEN * 3/4, 0.5, 0.1},
	{HOLE_LINEAR, PROFILE_LEN * 3/4, PROFILE_LEN * 3/4 + PROFILE_LEN/12, 0.1, 0.5},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole three_curves[] = {
	{HOLE_CURVE, PROFILE_LEN * 1/5, PROFILE_LEN * 2/5, 0.5, 0.1},
	{HOLE_CURVE, PROFILE_LEN * 2/5, PROFILE_LEN * 3/5, 0.5, 0.1},
	{HOLE_CURVE, PROFILE_LEN * 3/5, PROFILE_LEN * 4/5, 0.5, 0.1},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole hair[] = {
	{HOLE_CURVE, PROFILE_LEN * 1/4 - PROFILE_LEN/12,
		PROFILE_LEN * 1/4 + PROFILE_LEN/12, 0.5, 0.2},
	{HOLE_LINEAR, PROFILE_LEN * 1/4, PROFILE_LEN * 3/4, 0.1, 0.1},
	{HOLE_CURVE, PROFILE_LEN * 3/4 - PROFILE_LEN/12,
		PROFILE_LEN * 3/4 + PROFILE_LEN/12, 0.5, 0.2},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole three_balls[] = {
	{HOLE_LINEAR, PROFILE_LEN * 1/5 - PROFILE_LEN/12, PROFILE_LEN * 1/5, 0.5, 0.1},
	{HOLE_CURVE, PROFILE_LEN * 1/5, PROFILE_LEN * 2/5, 0.1, 0.5},
	{HOLE_CURVE, PROFILE_LEN * 2/5, PROFILE_LEN * 3/5, 0.1, 0.5},
	{HOLE_CURVE, PROFILE_LEN * 3/5, PROFILE_LEN * 4/5, 0.1, 0.5},
	{HOLE_LINEAR, PROFILE_LEN * 4/5, PROFILE_LEN * 4/5 + PROFILE_LEN/12, 0.1, 0.5},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole cut_balls[] = {
	{HOLE_CURVE, PROFILE_LEN * 1/5, PROFILE_LEN * 2/5, 0.1, 0.5},
	{HOLE_CURVE, PROFILE_LEN * 2/5, PROFILE_LEN * 3/5, 0.1, 0.5},
	{HOLE_CURVE, PROFILE_LEN * 3/5, PROFILE_LEN * 4/5, 0.1, 0.5},
	{HOLE_LINEAR, PROFILE_LEN * 4/5, PROFILE_LEN * 4/5 + PROFILE_LEN/12, 0.1, 0.5},

	{HOLE_LINEAR, PROFILE_LEN * 1/5, PROFILE_LEN * 4/5, 0.2, 0.4},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole two_balls[] = {
	{HOLE_LINEAR, 0, PROFILE_LEN * 1/5, 0.5, 0.1},
	{HOLE_CURVE, PROFILE_LEN * 1/5, PROFILE_LEN * 2/5, 0.1, 0.5},
	{HOLE_LINEAR, PROFILE_LEN * 2/5, PROFILE_LEN * 3/5, 0.1, 0.1},
	{HOLE_CURVE, PROFILE_LEN * 3/5, PROFILE_LEN * 4/5, 0.1, 0.5},
	{HOLE_LINEAR, PROFILE_LEN * 4/5, PROFILE_LEN, 0.1, 0.5},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole nibble[] = {
	{HOLE_LINEAR, 0, PROFILE_LEN/2 - PROFILE_LEN / 30, 0.1, 0.1},
	{HOLE_CURVE, PROFILE_LEN/2 - PROFILE_LEN / 30,
		PROFILE_LEN/2 + PROFILE_LEN / 30, 0.1, 0.5},
	{HOLE_LINEAR, PROFILE_LEN/2 + PROFILE_LEN / 30, PROFILE_LEN, 0.1, 0.1},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole curveball[] = {
	{HOLE_LINEAR, 0, PROFILE_LEN * 1/8, 0.1, 0.5},
	{HOLE_LINEAR, PROFILE_LEN * 1/4, PROFILE_LEN * 9/10, 0.5, 0.1},
	{HOLE_CURVE, PROFILE_LEN * 9/10 - PROFILE_LEN / 30,
		PROFILE_LEN * 9/10 + PROFILE_LEN / 30, 0.4, 0.3},
	{HOLE_LINEAR, PROFILE_LEN * 9/10, PROFILE_LEN, 0.3, 0.3},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole worm[] = {
	{HOLE_CURVE, 0, PROFILE_LEN, 0.1, 1},
	{HOLE_CURVE, PROFILE_LEN * 1/5, PROFILE_LEN * 2/5, 1, 0.5},
	{HOLE_CURVE, PROFILE_LEN * 2/5, PROFILE_LEN * 3/5, 1, 0.5},
	{HOLE_CURVE, PROFILE_LEN * 3/5, PROFILE_LEN * 4/5, 1, 0.5},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

const Hole wavy[] = {
	{HOLE_CURVE, 0, PROFILE_LEN * 1/5, 0.8, 0.1},
	{HOLE_CURVE, PROFILE_LEN * 1/5, PROFILE_LEN * 2/5, 0.8, 1},
	{HOLE_CURVE, PROFILE_LEN * 2/5, PROFILE_LEN * 3/5, 0.8, 0.1},
	{HOLE_CURVE, PROFILE_LEN * 3/5, PROFILE_LEN * 4/5, 0.8, 1},
	{HOLE_CURVE, PROFILE_LEN * 4/5, PROFILE_LEN, 0.8, 0.1},

	/* terminator */
	{-1, 0, 0, 0, 0}
};

/*
	const Hole *holes;
	int score;
	float thickness;
	float stress;
*/
const Shape level_1[] = {
	{plain, 1000, 0.5, 0.003},
	{round_middle, 1000, 0.5, 0.003},
	{wide_hole, 2000, 1, 0.003},

	/* terminator */
	{NULL, 0, 0, 0}
};

const Shape level_2[] = {
	{three_sharps, 2000, 0.5, 0.003},
	{sawtooth, 1000, 0.5, 0.003},
	{round_middle, 1000, 0.5, 0.003},
	{three_curves, 3000, 0.5, 0.003},

	/* terminator */
	{NULL, 0, 0, 0}
};

const Shape level_3[] = {
	{cut_balls, 2000, 0.5, 0.003},
	{three_balls, 4000, 0.5, 0.003},
	{three_sharps, 2000, 0.5, 0.003},
	{three_curves, 3000, 0.5, 0.003},
	{hair, 5000, 0.5, 0.003},

	/* terminator */
	{NULL, 0, 0, 0}
};

const Shape level_4[] = {
	{cut_balls, 2000, 0.5, 0.003},
	{two_balls, 4000, 0.5, 0.003},
	{hair, 5000, 0.5, 0.003},
	{nibble, 6000, 0.5, 0.003},

	/* terminator */
	{NULL, 0, 0, 0}
};

const Shape level_5[] = {
	{two_balls, 4000, 0.5, 0.003},
	{nibble, 6000, 0.5, 0.003},
	{curveball, 8000, 0.5, 0.003},
	{worm, 8000, 1, 0.003},

	/* terminator */
	{NULL, 0, 0, 0}
};

const Shape level_6[] = {
	{wavy, 10000, 1, 0.003},

	/* terminator */
	{NULL, 0, 0, 0}
};

float wood_average(const float *profile, int cx)
{
	float sum = 0;
	float tot = 0;
	for (int i = -9; i < 10; ++i) {
		int x = cx + i;
		if (x >= 0 && x < PROFILE_LEN) {
			float s = 1 - (i * 0.1) * (i * 0.1);
			sum += s * profile[x];
			tot += s;
		}
	}
	if (tot < 0.0001)
		return 0;
	return sum / tot;
}

Vertex wood_vertex(const float *profile, int length, int x, int s)
{
	assert(x >= 0 && x < length);

	int l = x - 1;
	if (l < 0)
		l = 0;
	int r = x + 1;
	if (r >= length)
		r = length - 1;

	float a = s * 2*M_PI / WOOD_SIDES;
	vec3 av(0, cosf(a), sinf(a));
	vec3 p = av * (profile[x]*6) + vec3(x * PROFILE_STEP, 0, 0);

	Vertex v;
	v.norm = av * (r - l) + vec3((profile[l] - profile[r]) * 6, 0, 0);
	v.tc = vec2(p.y * 0.1, p.z * 0.1);
	v.pos = p;
	return v;
}

void calc_score()
{
	const Shape *shape = &level->shapes[shape_num];
	float sum = 0;
	float tot = 0;
	for (int i = 0; i < PROFILE_LEN; ++i) {
		float cur = shape->thickness - cut_profile[i];
		float target = shape->thickness - current_shape[i];
		if (cur < target) {
			sum += cur;
		} else {
			/* too deep */
			sum += target - (cur - target) * 2;
		}
		tot += target;

		cur = fabs(wood_average(cut_profile, i) - cut_profile[i]);
		target = fabs(wood_average(current_shape, i) - current_shape[i]);

		if (cur < target) {
			sum += cur * 3;
		} else {
			/* too steep */
			sum += (target - (cur - target) * 2) * 3;
		}
		tot += target * 3;
	}

	shape_score[shape_num] = sum * shape->score / tot;
	if (shape_score[shape_num] < 0)
		shape_score[shape_num] = 0;
}

}

int shape_score[MAX_SHAPES];
float cut_profile[PROFILE_LEN];
float current_shape[PROFILE_LEN];
int shape_num;
float rotation;
bool motor_on;
bool motor_been_on;
Animator<float> speed;
Animator<float> wood_raise;
bool explosion;
Sound motor_sound;
float wood_fly;

const Level levels[] = {
	{level_1,
	"Level 1: Introduction\n\n"
	"You are broke, bored and lonely, looking for a job.\n"
	"One day, you spot a job advertisement in a local magazine.\n"
	"A woodworks firm is looking for an addition to their\n"
	"staff that is able to handle the difficulties of turning.\n\n"
	"You are determined to spread fear to the world\n"
	"with your uber chisel handling skills!",
	"sorvipop.ogg",
	210, 1000},

	{level_2,
	"Level 2: The beginning\n\n"
	"So you made throught the first level without a hitch.\n"
	"But more is to come! Much more. You move on to new,\n"
	"harder assigments. Be sure to check the new tools\n"
	"available.\n\n"
	"Takaisin sorvin ääreen!",
	"talttapop.ogg",
	180, 2000},

	{level_3,
	"Level 3: Rising\n\n"
	"Seems that you are becoming the turning master!\n"
	"More challenging tasks are waiting for you and your\n"
	"skills. Let's see how the next level goes.\n\n"
	"Takaisin sorvin ääreen.",
	"sorvipop.ogg",
	180, 4000},

	{level_4,
	"Level 4: Getting harder\n\n"
	"The days and nights are blurring together as you work\n"
	"trought the given shapes. All the shapes are starting to\n"
	"look the same. All you can hear anymore is the humming of\n"
	"the motor.\n\n"
	"SORVIN ÄÄREEN! ASENTO! LEPO!",
	"luxsaberpop.ogg",
	120, 5000},

	{level_5,
	"Level 5: On to victory!\n\n"
	"Congratulations! You made it this far!\n"
	"sorvin ääreen >>",
	"talttapop.ogg",
	120, 7000},

	{level_6,
	"Level 6: Final showdown!\n\n"
	"Think fast and you will gain a place in the hall\n"
	"of fame and earn the respect of others!\n\n"
	"awwyeah!",
	"luxsaberpop.ogg",
	40, 1000},

	/* terminator */
	{NULL, NULL, NULL, 0, 0}
};

float wood_derivate(const float *profile, int length, int x)
{
	assert(x >= 0 && x < length);

	int l = x - 1;
	if (l < 0)
		l = 0;
	int r = x + 1;
	if (r >= length)
		r = length - 1;
	return (profile[l] - profile[r]) / (r - l);
}

void new_shape()
{
	motor_on = false;
	motor_been_on = false;
	rotation = 0;
	explosion = false;
	wood_fly = 0;
	speed.set_value(0);
	wood_raise.set_value(20);
	wood_raise.set_target(0);

	const Shape *shape = &level->shapes[shape_num];
	for (int i = 0; i < PROFILE_LEN; ++i) {
		cut_profile[i] = shape->thickness;
	}
	create_wood_profile(shape, current_shape);
}

void explode()
{
	static Once once;
	static Sound explosion_sound;
	if (once) {
		load_mpeg(&explosion_sound, "explosion.ogg");
	}

	for (int i = 0; i < 15; ++i) {
		Particle p;
		p.smoke = true;
		p.c = color(0.2, 0.2, 0.2);
		p.pos = vec3(blade_x, 0, 0);
		p.vel = vec3(0, 0, 0);
		add_particle(p);

		/* no smoke without fire */
		p.c = color(0.6, 0.3, 0.1);
		p.vel = vec3(randf(-3, 3), randf(-3, 3), randf(-3, 3));
		p.smoke = false;
		add_particle(p);
	}
	shape_score[shape_num] = 0;
	end_cutting();
	explosion = true;
	play_sound(&explosion_sound);
}

float get_depth(const float *profile, int cx)
{
	const Blade *blade = &blades[blade_num];

	float depth = -100;
	for (int i = 0; i < blade->width; ++i) {
		int x = cx + i;
		if (x >= 0 && x < PROFILE_LEN &&
		    profile[x] > depth + current_blade[i]) {
			depth = profile[x] - current_blade[i];
		}
	}
	return depth;
}

void cut(float dt)
{
	const Blade *blade = &blades[blade_num];
	const Chisel *chisel = inventory[blade_num];

	static float old_profile[PROFILE_LEN];
	memcpy(old_profile, cut_profile, sizeof cut_profile);

	int cx = (blade_x - WOOD_ORIGIN_X)
		/ PROFILE_STEP - blade->width*0.5 + 0.5;

	float depth = get_depth(cut_profile, cx) -
			speed * dt * chisel->speed;

	const Shape *shape = &level->shapes[shape_num];
	stress[blade_num] += speed * dt * shape->stress;

	for (int i = 0; i < blade->width; ++i) {
		int x = cx + i;
		float d = depth + current_blade[i];
		if (blade->smooth) {
			d = wood_average(old_profile, x);
			if (d < cut_profile[x] - speed * dt * chisel->speed) {
				d = cut_profile[x] -
					speed * dt * chisel->speed;
			}
		}
		if (x >= 0 && x < PROFILE_LEN && d < cut_profile[x]) {
			cut_profile[x] = d;
			if (cut_profile[x] < 0.05) {
				explode();
				return;
			}
			if (randf(0, 1 / dt) < 10) {
				Particle p;
				p.smoke = false;
				p.c = color(0.6, 0.3, 0.1);
				p.pos = vec3(WOOD_ORIGIN_X +
					x * PROFILE_STEP,
					cut_profile[x] * 6 * -0.7,
					cut_profile[x] * 6 * 0.7);
				if (rand() & 1) {
					p.vel = vec3(0, -20, 0);
				} else {
					p.vel = vec3(0, -10, 10);
				}
				add_particle(p);
			}
			if ((stress[blade_num] > chisel->max_stress - 1 ||
			     blade_num == BLADE_LUXSABER)
			    && randf(0, 1 / dt) < 2 - (chisel->max_stress -
					stress[blade_num])) {
				Particle p;
				p.c = color(0.2, 0.2, 0.2);
				p.smoke = true;
				p.pos = vec3(WOOD_ORIGIN_X +
					x * PROFILE_STEP,
					cut_profile[x] * 6 * -0.7,
					cut_profile[x] * 6 * 0.7);
				p.vel = vec3(0, 0, 0);
				add_particle(p);

				/* no smoke without fire */
				p.c = color(0.6, 0.3, 0.1);
				p.smoke = false;
				p.vel = vec3(0, -10, 0);
				add_particle(p);
			}
		}
	}

	calc_score();
}

void create_wood_profile(const Shape *shape, float *profile)
{
	for (int i = 0; i < PROFILE_LEN; ++i) {
		profile[i] = shape->thickness;
	}
	for (int i = 0; shape->holes[i].type >= 0; ++i) {
		create_hole(profile, PROFILE_LEN, &shape->holes[i]);
	}
}

void draw_wood(const float *profile, int length)
{
	static GLuint texture = INVALID_TEXTURE;
	if (texture == INVALID_TEXTURE) {
		texture = load_texture("wood.png");
	}

	if (length == 0)
		return;
	assert(length > 0);

	GLState state;
	state.enable(GL_TEXTURE_2D);
	state.enable(GL_LIGHTING);

	GLMatrixScope matrix;
	glTranslatef(-length * PROFILE_STEP/2, 0, 0);

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
	glBindTexture(GL_TEXTURE_2D, texture);

	/* ~3 megabytes */
	static Vertex vert[(PROFILE_LEN + 1) * WOOD_SIDES * 2 * 3];
	int vi = 0;
	for (int s = 0; s < WOOD_SIDES; ++s) {
		vert[vi++] = wood_vertex(profile, length, 0, s + 1);
		vert[vi++] = wood_vertex(profile, length, 0, s);
		vert[vi].norm = vec3(-1, 0, 0);
		vert[vi].tc = vec2(0, 0);
		vert[vi++].pos = vec3(0, 0, 0);

		vert[vi++] = wood_vertex(profile, length, length - 1, s);
		vert[vi++] = wood_vertex(profile, length, length - 1, s + 1);
		vert[vi].norm = vec3(1, 0, 0);
		vert[vi].tc = vec2(0, 0);
		vert[vi++].pos = vec3((length - 1) * PROFILE_STEP, 0, 0);
	}
	draw_array(GL_TRIANGLES, vert, WOOD_SIDES * 2 * 3);

	vi = 0;
	float deriv = wood_derivate(profile, length, 0);
	int j = 0;
	for (int i = 1; i < length; ++i) {
		float d = wood_derivate(profile, length, i);
		if (fabs(deriv - d) > 0.001) {
			if (j != i - 1) {
				for (int s = 0; s < WOOD_SIDES; ++s) {
					vert[vi++] = wood_vertex(profile, length, j, s);
					vert[vi++] = wood_vertex(profile, length, j, s + 1);
					vert[vi++] = wood_vertex(profile, length, i - 1, s);

					vert[vi++] = wood_vertex(profile, length, j, s + 1);
					vert[vi++] = wood_vertex(profile, length, i - 1, s + 1);
					vert[vi++] = wood_vertex(profile, length, i - 1, s);
				}
			}
			for (int s = 0; s < WOOD_SIDES; ++s) {
				vert[vi++] = wood_vertex(profile, length, i - 1, s);
				vert[vi++] = wood_vertex(profile, length, i - 1, s + 1);
				vert[vi++] = wood_vertex(profile, length, i, s);

				vert[vi++] = wood_vertex(profile, length, i - 1, s + 1);
				vert[vi++] = wood_vertex(profile, length, i, s + 1);
				vert[vi++] = wood_vertex(profile, length, i, s);
			}
			j = i;
			deriv = d;
		}
	}
	if (j != length - 1) {
		for (int s = 0; s < WOOD_SIDES; ++s) {
			vert[vi++] = wood_vertex(profile, length, j, s);
			vert[vi++] = wood_vertex(profile, length, j, s + 1);
			vert[vi++] = wood_vertex(profile, length, length - 1, s);

			vert[vi++] = wood_vertex(profile, length, j, s + 1);
			vert[vi++] = wood_vertex(profile, length, length - 1, s + 1);
			vert[vi++] = wood_vertex(profile, length, length - 1, s);
		}
	}
	draw_array(GL_TRIANGLES, vert, vi);
}

void switch_motor()
{
	motor_on = !motor_on;
	if (motor_on) {
		play_sound(&motor_sound, LOOP_INFINITE);
		speed.set_target(1000);
	} else {
		stop_sound(&motor_sound);
		speed.set_target(0);
		motor_been_on = true;
	}
}
