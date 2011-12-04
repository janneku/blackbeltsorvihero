/*
 * Black Belt Sorvi Hero
 *
 * Copyright 2011 Janne Kulmala <janne.t.kulmala@iki.fi>,
 * Antti Rajamäki <amikaze@gmail.com>
 *
 * Program code and resources are licensed with GNU LGPL 2.1. See
 * COPYING.LGPL file.
 */
#include "gl.h"
#include "sound.h"
#include "chisel.h"
#include "version.h"
#include "wood.h"
#include "shop.h"
#include "menus.h"
#include <vector>
#include <stdexcept>
#include <time.h>
#if !defined(_WIN32)
	#include <dirent.h>
	#include <unistd.h>
#else
	/* windows */
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winsock.h>
	/* WTF? */
	#undef near
	#undef far
#endif

namespace {

const float CAMERA_SPEED = 0.002;
const float WALK_SPEED = 300;
const float LOOK_SPEED = 2;
const float SCROLL_SPEED = 5;

#if !defined(CONFIG_GLES)
const int FONT_SIZE = 16;
#else
const int FONT_SIZE = 12;
#endif

#if defined(CONFIG_N900)
#define DATA_PATH	"/opt/" DIR_NAME "/"
#elif defined(_WIN32)
#define DATA_PATH	".\\"
#else
#define DATA_PATH	"./"
#endif

enum {
	FORWARD,
	BACKWARD,
	STRAFE_LEFT,
	STRAFE_RIGHT,
	NUM_KEYS
};

const int CHEAT_LEN = 5;

int cheat[] = {SDLK_a, SDLK_s, SDLK_m, SDLK_i, SDLK_t};

float camera_x = -M_PI / 2;
float camera_y = M_PI / 6;
float camera_dist = 30;
vec2 mouse_pos(0, 0);
bool done;
int cheat_pos = 0;
float time_left;
Animator<float> ui_anim;
Animator<float> camera_force;
Animator<float> scroll_vel;
Animator<vec3> camera_anim;
Animator<vec3> walking_vel;
vec3 walk_pos;
bool walking;
bool overheated;
float scroll = 0;
float chisel_fly;
bool endgame;
bool keys[NUM_KEYS];
#if !defined(CONFIG_GLES)
GLdouble modelMatrix[16];
GLdouble projMatrix[16];
#else
GLfloat modelMatrix[16];
GLfloat projMatrix[16];
#endif

std::string hexlify(const std::string &s)
{
	std::string out;
	for (size_t i = 0; i < s.size(); ++i) {
		out += strf("%02x", (unsigned char) s[i]);
	}
	return out;
}

void draw_wood_profile(const float *profile, const color &c)
{
	static VertexColor vert[PROFILE_LEN];
	int vi = 0;
	float deriv = wood_derivate(profile, PROFILE_LEN, 0);
	vert[vi].pos = vec3(0, 0, profile[0]*6);
	vert[vi++].c = c;
	for (int i = 1; i < PROFILE_LEN; ++i) {
		float d = wood_derivate(profile, PROFILE_LEN, i);
		if (fabs(deriv - d) > 0.001) {
			vert[vi].pos = vec3(i * PROFILE_STEP, 0, profile[i]*6);
			vert[vi++].c = c;
			deriv = d;
		}
	}
	vert[vi].pos = vec3((PROFILE_LEN - 1) * PROFILE_STEP, 0,
			     profile[PROFILE_LEN - 1]*6);
	vert[vi++].c = c;
	draw_array(GL_LINE_STRIP, vert, vi);

}

void draw_profile()
{
	glLineWidth(2);
	GLMatrixScope matrix;
	glTranslatef(WOOD_ORIGIN_X, 0, 8);

	draw_wood_profile(cut_profile, color(1, 1, 1));
	draw_wood_profile(current_shape, color(0.2, 0.7, 1));

	glLineWidth(1);
}

void draw_ui()
{
	static Image start;
	static Image stop;
	static Image shop;
	static Image endg;
	static Image handles[MAX_HANDLES];
	static Text numbers[NUM_BLADES];
	if (!start.ready()) {
		start.load("start_button.png");
		stop.load("stop_button.png");
		shop.load("shop_button.png");
		endg.load("endgame.png");
		handles[HANDLE_BASIC].load("basic_icon.png");
		handles[HANDLE_PRO].load("pro_icon.png");
		handles[HANDLE_ULTIMATE].load("ultimate_icon.png");
		handles[HANDLE_SCIFI].load("scifi_icon.png");
		handles[HANDLE_HAND].load("hand_icon.png");
		handles[HANDLE_LIGHTSABER].load("lightsaber_icon.png");
		handles[HANDLE_CHAINSAW].load("lightsaber_icon.png");
		for (int i = 0; i < NUM_BLADES; ++i) {
			numbers[i].init(font, strf("%d", i + 1));
		}
	}

	GLState state;
	state.enable(GL_TEXTURE_2D);

	{
		GLMatrixScope matrix;
		glTranslatef(10, scr_height - 64 - 10 + ui_anim, 0);

		if (motor_on)
			stop.draw();
		else
			start.draw();
	}

	for (int i = 0; i < NUM_BLADES; ++i) {
		const Chisel *chisel = inventory[i];
		if (chisel == NULL)
			continue;

		GLMatrixScope matrix;
		glTranslatef(200 + i * 80, scr_height - 100 + ui_anim*2, 0);

		handles[chisel->handle].draw();
		numbers[i].draw();
	}

	if (endgame) {
		GLMatrixScope matrix;
		glTranslatef(scr_width/2 - 128, scr_height/2, 0);
		endg.draw();
	}

	{
		GLMatrixScope matrix;
		glTranslatef(200, 10 - ui_anim, 0);

		chisel_name.draw();

		glTranslatef(0, 30, 0);

		const Chisel *chisel = inventory[blade_num];

		VertexColor vert[3 * 2];
		int vi = 0;
		vert[vi].c = color(1, 1, 0);
		vert[vi++].pos = vec3(0, 0, 0);
		vert[vi].c = color(1, 1, 0);
		vert[vi++].pos = vec3(0, 30, 0);
		vert[vi].c = color(1, 1 - stress[blade_num] / chisel->max_stress, 0);
		vert[vi++].pos = vec3(stress[blade_num] * 30, 0, 0);

		vert[vi].c = color(1, 1, 0);
		vert[vi++].pos = vec3(0, 30, 0);
		vert[vi].c = color(1, 1 - stress[blade_num] / chisel->max_stress, 0);
		vert[vi++].pos = vec3(stress[blade_num] * 30, 30, 0);
		vert[vi].c = color(1, 1 - stress[blade_num] / chisel->max_stress, 0);
		vert[vi++].pos = vec3(stress[blade_num] * 30, 0, 0);
		draw_array(GL_TRIANGLES, vert, 3 * 2);

		Vertex line[5];
		vi = 0;
		line[vi++].pos = vec3(0, 0, 0);
		line[vi++].pos = vec3(0, 30, 0);
		line[vi++].pos = vec3(chisel->max_stress * 30, 30, 0);
		line[vi++].pos = vec3(chisel->max_stress * 30, 0, 0);
		line[vi++].pos = vec3(0, 0, 0);
		draw_array(GL_LINE_STRIP, line, 5);
	}

	{
		GLMatrixScope matrix;
		glTranslatef(scr_width - 128 - 10,
			     scr_height - 64 - 10 + ui_anim, 0);
		shop.draw();
	}

	{
		GLMatrixScope matrix;
		glTranslatef(10, 10 - ui_anim, 0);
		draw_number(int(time_left), 3);
	}

	{
		GLMatrixScope matrix;
		glTranslatef(scr_width - 5*DIGIT_WIDTH - 10,
			     20 + DIGIT_HEIGHT - ui_anim * 2, 0);
		draw_number(shape_score[shape_num], 5);
	}

	GLMatrixScope matrix;
	glTranslatef(scr_width - 6*DIGIT_WIDTH - 10, 10 - ui_anim, 0);
	draw_number(score, 6);
}

void draw_scene(bool bloom)
{
	static Model lightsaber;
	static Once once;
	if (once) {
		lightsaber.load("lightsaber.obj");
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50.0, float(scr_width) / scr_height, 0.01, 1000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	vec3 target(scroll, 0, 0);
	vec3 camera = camera_anim;
	if (walking) {
		vec3 d(cosf(camera_x) * cosf(camera_y),
		       sinf(camera_x) * cosf(camera_y), sinf(camera_y));
		camera = walk_pos;
		target = walk_pos - d;
	}
	gluLookAt(camera.x, camera.y, camera.z, target.x, target.y, target.z,
		  0, 0, 1);

#if !defined(CONFIG_GLES)
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
#else
	glGetFloatv(GL_PROJECTION_MATRIX, projMatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
#endif

	if (blade_num == BLADE_LIGHTSABER) {
		GLMatrixScope matrix;

		glTranslatef(blade_x, blade_raise * -0.7 - BLADE_LEN,
			blade_raise * 0.7 + chisel_fly * 10);
		lightsaber.set_lights(bloom);
	}
	set_particles_light(bloom);

	room.set_lights(bloom);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	GLState state;
	state.enable(GL_DEPTH_TEST);

	room.draw();
	sorvi.draw();

	if (explosion) {
		int cx = (blade_x - WOOD_ORIGIN_X) / PROFILE_STEP;

		{
			GLMatrixScope matrix;
			glTranslatef(cx * PROFILE_STEP/2 + WOOD_ORIGIN_X,
				wood_fly * -20, wood_fly * -10);
			glRotatef(wood_fly * 100, 0, 1, 0);
			glRotatef(wood_fly * -60, 1, 0, 0);
			glRotatef(rotation, 1, 0, 0);
			draw_wood(cut_profile, cx);
		}

		GLMatrixScope matrix;
		glTranslatef((PROFILE_LEN + cx) * PROFILE_STEP/2 + WOOD_ORIGIN_X,
			wood_fly * 20, wood_fly * 10);
		glRotatef(wood_fly * 70, 1, 0, 0);
		glRotatef(wood_fly * -100, 0, 1, 0);
		glRotatef(rotation, 1, 0, 0);
		draw_wood(cut_profile + cx, PROFILE_LEN - cx);
	} else {
		GLMatrixScope matrix;
		glTranslatef(0, 0, wood_raise);
		glRotatef(rotation, 1, 0, 0);
		draw_wood(cut_profile, PROFILE_LEN);
	}

	{
		GLMatrixScope matrix;
		glRotatef(rotation, 1, 0, 0);
		akseli.draw();
	}

	{
		GLMatrixScope matrix;

		const Chisel *chisel = inventory[blade_num];

		if (blade_num == BLADE_LIGHTSABER ||
		    blade_num == BLADE_CHAINSAW) {
			glTranslatef(blade_x, -BLADE_LEN,
				blade_raise + chisel_fly * 10);
		} else {
			glTranslatef(blade_x, blade_raise * -0.7 - BLADE_LEN,
				blade_raise * 0.7 + chisel_fly * 10);
		}
		glRotatef(chisel_fly * 100, 0, 1, 0);
		glRotatef(chisel_fly * -60, 1, 0, 0);
		draw_chisel(chisel, current_blade);
	}

	draw_particles();
	draw_profile();

	if (!bloom)
		draw_smoke(camera);

	state.enable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	support.draw();
	glDepthMask(GL_TRUE);

	kill_lights();
}

void draw_game()
{
	if (shaders_enabled && GLEW_VERSION_2_0) {
		begin_bloom();
		draw_scene(true);
		end_bloom();

		begin_blur();
		draw_scene(false);
		draw_bloom();
		end_blur();
	}

	draw_scene(false);

	if (shaders_enabled && GLEW_VERSION_2_0) {
		draw_bloom();
		draw_blur(0.2);
	}

	hud_matrix();

	draw_ui();
}

void visit_shop()
{
	memset(keys, 0, sizeof keys);
	kill_sounds();
	kill_particles();
	shop();
	if (motor_on) {
		play_sound(&motor_sound, LOOP_INFINITE);
	}
	switch_blade();
	play_music(level->music);
	ui_anim.set_value(64 + 10);
	ui_anim.set_target(0);
}

void begin_walking()
{
	walking = true;
	walk_pos = camera_anim;
	walk_pos.z = 0;
	walking_vel.set_value(vec3(0, 0, 0));
#if !defined(CONFIG_GLES)
	SDL_WarpMouse(scr_width/2, scr_height/2);
#endif
}

float calc_position(const vec2 &p, float *distance)
{
	int viewport[4] = {0, 0, scr_width, scr_height};
#if !defined(CONFIG_GLES)
	double near[3];
	double far[3];
#else
	float near[3];
	float far[3];
#endif
	gluUnProject(p.x, scr_height - p.y, 0.1,
		    modelMatrix, projMatrix, viewport,
		    &near[0], &near[1], &near[2]);
	gluUnProject(p.x, scr_height - p.y, 1,
		    modelMatrix, projMatrix, viewport,
		    &far[0], &far[1], &far[2]);
	float x, dist;
	if (fabs(vec3(camera_anim).z) > fabs(vec3(camera_anim).y)) {
		x = -near[2] * (far[0] - near[0]) /
			(far[2] - near[2]) + near[0];
		dist = -near[2] * (far[1] - near[1]) /
			(far[2] - near[2]) + near[1];
	} else {
		x = -near[1] * (far[0] - near[0]) /
			(far[1] - near[1]) + near[0];
		dist = -near[1] * (far[2] - near[2]) /
			(far[1] - near[1]) + near[2];
	}
	if (distance != NULL) {
		*distance = fabs(dist);
	}
	return x;
}

void game_event(const SDL_Event *e)
{
	switch (e->type) {
	case SDL_MOUSEMOTION: {
		mouse_pos = vec2(e->motion.x, e->motion.y);

		if ((e->motion.state & 1) && !cutting) {
			scroll_vel.set_value(-SCROLL_SPEED * e->motion.xrel);
                        scroll_vel.set_target(0);
		}

		vec2 move(0, 0);
#if !defined(CONFIG_GLES)
		move = vec2(e->motion.x - scr_width/2, e->motion.y - scr_height/2);
#else
		if (e->motion.state & 7) {
			move = vec2(e->motion.xrel, e->motion.yrel);
		}
#endif
		if (walking) {
			camera_x -= move.x * CAMERA_SPEED;
			camera_y += move.y * CAMERA_SPEED;
			if (camera_y < -M_PI / 2 + 0.1)
				camera_y = -M_PI / 2 + 0.1;
			if (camera_y > M_PI / 2 - 0.1)
				camera_y = M_PI / 2 - 0.1;
		} else if (e->motion.state & 4) {
			camera_x -= move.x * CAMERA_SPEED;
			camera_y += move.y * CAMERA_SPEED;
			if (camera_y < -M_PI / 2 + 0.1)
				camera_y = -M_PI / 2 + 0.1;
			if (camera_y > M_PI / 2 - 0.1)
				camera_y = M_PI / 2 - 0.1;
		} else
			break;
#if !defined(CONFIG_GLES)
		SDL_WarpMouse(scr_width/2, scr_height/2);
#endif
		break;
	}

	case SDL_MOUSEBUTTONDOWN: {
		if (e->button.button == SDL_BUTTON_LEFT) {
			vec2 p(e->button.x, e->button.y);
			float distance = 0;
#if defined(CONFIG_GLES)
			calc_position(p, &distance);
#endif
			if (p > vec2(0, scr_height - 64 - 10) &&
			    p < vec2(128 + 10, scr_height) &&
			    wood_raise < 0.1 && !explosion && !overheated &&
			    !cutting) {
				switch_motor();
			} else if (p > vec2(scr_width - 128 - 10, scr_height - 64 - 10) &&
				   p < vec2(scr_width, scr_height)) {
				visit_shop();
			} else if (motor_on && !explosion && !overheated &&
				   distance < 4) {
				begin_cutting();
			}
		} else if (e->button.button == SDL_BUTTON_RIGHT) {
#if !defined(CONFIG_GLES)
			SDL_WarpMouse(scr_width/2, scr_height/2);
#endif
		} else if (e->button.button == SDL_BUTTON_WHEELUP) {
			if (camera_dist > 20)
				camera_dist -= 10;
		} else if (e->button.button == SDL_BUTTON_WHEELDOWN) {
			if (camera_dist < 300)
				camera_dist += 10;
		}
		break;
	}

	case SDL_MOUSEBUTTONUP:
		if (e->button.button == SDL_BUTTON_LEFT) {
			end_cutting();
		}
		break;

	case SDL_KEYDOWN:
		if (cheat_pos < CHEAT_LEN &&
		    e->key.keysym.sym == cheat[cheat_pos]) {
			cheat_pos++;
		} else if (cheat_pos < CHEAT_LEN) {
			cheat_pos = 0;
		}
		switch (e->key.keysym.sym) {
		case SDLK_ESCAPE:
		case SDLK_q:
#if SDL_VERSION_ATLEAST(1, 3, 0)
		case SDLK_AC_BACK:
#endif
			endgame = true;
#if defined(CONFIG_GLES)
			done = true;
#endif
			break;
		case SDLK_w:
			keys[FORWARD] = true;
			break;
		case SDLK_s:
			if (!walking) {
				begin_walking();
			}
			keys[BACKWARD] = true;
			break;
		case SDLK_a:
			keys[STRAFE_LEFT] = true;
			break;
		case SDLK_d:
			keys[STRAFE_RIGHT] = true;
			break;
		case SDLK_y:
			if (endgame) {
				done = true;
			}
			break;
		case SDLK_n:
			endgame = false;
			break;
		case SDLK_SPACE:
			if (wood_raise < 0.1 && !explosion && !overheated &&
			    !cutting) {
				switch_motor();
			}
			break;
		case SDLK_UP:
		case SDLK_LEFT:
			for (int i = blade_num - 1; !overheated && i >= 0; --i) {
				if (inventory[i] != NULL) {
					end_cutting();
					blade_num = i;
					switch_blade();
					break;
				}
			}
			break;
		case SDLK_DOWN:
		case SDLK_RIGHT:
			for (int i = blade_num + 1; !overheated && i < NUM_BLADES; ++i) {
				if (inventory[i] != NULL) {
					end_cutting();
					blade_num = i;
					switch_blade();
					break;
				}
			}
			break;
		case SDLK_F1:
			if (cheat_pos >= CHEAT_LEN) {
				shape_score[0] = level->min_score;
				done = true;
			}
			break;
		case SDLK_F2:
			if (cheat_pos >= CHEAT_LEN) {
				score += 5000;
			}
			break;
		default:
			if (e->key.keysym.sym >= SDLK_1 &&
			    e->key.keysym.sym <= SDLK_9 && !overheated) {
				int i = e->key.keysym.sym - SDLK_1;
				if (i < NUM_BLADES && inventory[i] != NULL) {
					end_cutting();
					blade_num = i;
					switch_blade();
				}
			}
			break;
		}
		break;

	case SDL_KEYUP:
		switch (e->key.keysym.sym) {
		case SDLK_w:
			keys[FORWARD] = false;
			break;
		case SDLK_s:
			keys[BACKWARD] = false;
			break;
		case SDLK_a:
			keys[STRAFE_LEFT] = false;
			break;
		case SDLK_d:
			keys[STRAFE_RIGHT] = false;
			break;
		default:
			break;
		}
		break;
	}
}

void overheat()
{
}

void remove_blade()
{
	inventory[blade_num] = NULL;

	/* switch to a random blade */
	for (int i = 0; i < NUM_BLADES; ++i) {
		if (inventory[i] != NULL) {
			blade_num = i;
			switch_blade();
			return;
		}
	}

	/* give default blade and end the level */
	blade_num = 0;
	inventory[0] = &chisels[0];
	shape_score[shape_num] = 0;
	done = true;
}

void shape_complete()
{
	static Once once;
	static Sound coin;
	if (once) {
		load_mpeg(&coin, "coin.ogg");
	}

	score += shape_score[shape_num];
	if (shape_score[shape_num] > 0) {
		play_sound(&coin, shape_score[shape_num] / 200);
	}
}

void update_game()
{
	static Once once;
	static Sound overheated_sound;
	if (once) {
		load_mpeg(&overheated_sound, "overheated.ogg");
	}

	static Timer timer;
	float dt = timer.get_dt();
	update_particles(dt);

	if (!explosion && !overheated) {
		blade_x = calc_position(mouse_pos, NULL);
		if (blade_x < -PROFILE_LEN * PROFILE_STEP * 0.5)
			blade_x = -PROFILE_LEN * PROFILE_STEP * 0.5;
		if (blade_x > PROFILE_LEN * PROFILE_STEP * 0.5)
			blade_x = PROFILE_LEN * PROFILE_STEP * 0.5;
	}

	if (randf(0, 1 / dt) < 1) {
		Particle p;
		p.smoke = true;
		p.c = color(0.2, 0.2, 0.2);
		p.pos = room.find_material("smoke")->pos;
		p.vel = vec3(0, 0, 0);
		add_particle(p);
	}

	if (!walking) {
#if !defined(CONFIG_GLES)
		float vel = 0;
		if (mouse_pos.x < 50) {
			vel = (mouse_pos.x - 50) * LOOK_SPEED;
		} else if (mouse_pos.x > scr_width - 50) {
			vel = (mouse_pos.x - (scr_width - 50)) * LOOK_SPEED;
		}
		scroll_vel.set_target(vel);
#endif
		scroll_vel.update(dt, 100);
		if (vec3(camera_anim).y > 0) {
			scroll -= scroll_vel * dt;
		} else {
			scroll += scroll_vel * dt;
		}
		if (scroll < -PROFILE_LEN * PROFILE_STEP * 0.5)
			scroll = -PROFILE_LEN * PROFILE_STEP * 0.5;
		if (scroll > PROFILE_LEN * PROFILE_STEP * 0.5)
			scroll = PROFILE_LEN * PROFILE_STEP * 0.5;

		vec3 d(cosf(camera_x) * cosf(camera_y),
		       sinf(camera_x) * cosf(camera_y), sinf(camera_y));
		vec3 p = vec3(scroll, 0, 0) + d * camera_dist;
		if (p.x < -230) p.x = -230;
		if (p.x > 110) p.x = 110;
		if (p.y < -215) p.y = -215;
		if (p.y > 90) p.y = 90;
		camera_anim.set_target(p);
		camera_anim.update(dt, camera_force);

	} else {
		/* walking */
		vec3 vel(0, 0, 0);
		vec3 forward(cosf(camera_x), sinf(camera_x), 0);
		vec3 right(sinf(camera_x), -cosf(camera_x), 0);
		if (keys[FORWARD]) {
			vel = vel + forward * -WALK_SPEED;
		}
		if (keys[BACKWARD]) {
			vel = vel + forward * WALK_SPEED;
		}
		if (keys[STRAFE_LEFT]) {
			vel = vel + right * WALK_SPEED;
		}
		if (keys[STRAFE_RIGHT]) {
			vel = vel + right * -WALK_SPEED;
		}
		walking_vel.set_target(vel);
		walking_vel.update(dt, 100);
		walk_pos = walk_pos + walking_vel * dt;

		if (walk_pos.x < -230) walk_pos.x = -230;
		if (walk_pos.x > 110) walk_pos.x = 110;
		if (walk_pos.y < -215) walk_pos.y = -215;
		if (walk_pos.y > 90) walk_pos.y = 90;

		if (walk_pos.x > -50 && walk_pos.y > -10 && walk_pos.y < 10) {
			walking = false;
			camera_anim.set_value(walk_pos);
		}
		if (walk_pos.x < -225 && walk_pos.y < -60) {
			visit_shop();
			walk_pos.x = -220;
			walking_vel.set_value(vec3(0, 0, 0));
		}
	}

	if (explosion) {
		wood_fly += dt;
		if (wood_fly > 3) {
			shape_num++;
			const Shape *shape = &level->shapes[shape_num];
			if (shape->holes == NULL) {
				done = true;
				return;
			}
			stop_sound(&motor_sound);
			new_shape();
		}
	}

	if (overheated) {
		chisel_fly += dt;
		if (chisel_fly > 3) {
			/* not bought a new one? */
			const Chisel *chisel = inventory[blade_num];
			if (stress[blade_num] > chisel->max_stress) {
				remove_blade();
			}
			chisel_fly = 0;
			overheated = false;
		}
	}

	ui_anim.update(dt, 100);
	blade_raise.update(dt, 1000);
	wood_raise.update(dt, 10);
	camera_force.update(dt, 10);

	time_left -= dt;
	if (time_left < 0) {
		shape_complete();
		done = true;
		return;
	}

	const Blade *blade = &blades[blade_num];
	const Chisel *chisel = inventory[blade_num];

	int cx = (blade_x - WOOD_ORIGIN_X)
		/ PROFILE_STEP - blade->width*0.5 + 0.5;

	float depth = get_depth(cut_profile, cx);
	if (!cutting) {
		depth += 0.2;
	}
	blade_raise.set_target(depth * 6);

	speed.update(dt, 10);
	rotation += speed * dt;

	if (speed < 1 && !motor_on && motor_been_on) {
		/* motor stopped */
		wood_raise.set_target(20);
	}
	if (wood_raise > 20 - 1 && motor_been_on) {
		/* wood raised up */
		shape_complete();
		shape_num++;
		const Shape *shape = &level->shapes[shape_num];
		if (shape->holes == NULL) {
			done = true;
			return;
		}
		new_shape();
	}

	for (int i = 0; i < NUM_BLADES; ++i) {
		if (overheated && i == blade_num)
			continue;
		stress[i] -= dt * 2;
		if (stress[i] < 0) {
			stress[i] = 0;
		}
	}

	if (cutting) {
		cut(dt);

		if (stress[blade_num] > chisel->max_stress) {
			end_cutting();
			play_sound(&overheated_sound);
			overheated = true;
		}
	}
}

bool game()
{
	play_music(level->music);
	time_left = level->time_limit;
	endgame = false;
	walking = false;
	memset(keys, 0, sizeof keys);
	done = false;
	shape_num = 0;
	overheated = false;
	chisel_fly = 0;
	camera_anim.set_value(vec3(50, -150, 0));
	memset(stress, 0, sizeof(float) * NUM_BLADES);
	memset(shape_score, 0, sizeof(int) * MAX_SHAPES);
	new_shape();
	switch_blade();
	ui_anim.set_value(64 + 10);
	ui_anim.set_target(0);
	camera_force.set_value(0);
	camera_force.set_target(100);

	while (!done) {
		SDL_Event e;
		while (get_event(&e)) {
			game_event(&e);
		}
		update_game();
		draw_game();
		flush_gl();
	}
	kill_sounds();
	kill_particles();
	return endgame;
}

bool is_file(const char *fname)
{
	FILE *f = fopen(fname, "rb");
	if (f == NULL)
		return false;
	fclose(f);
	return true;
}

#if !defined(_WIN32)
std::string find_file(const std::string &path, const char *name, int depth = 0)
{
	if (depth >= 8)
		return "";
	std::string fname = path + "/" + name;
	if (is_file(fname.c_str())) {
		printf("found: %s\n", fname.c_str());
		return fname;
	}
	DIR *dir = opendir(path.c_str());
	if (dir == NULL)
		return "";
	while (true) {
		dirent *ent = readdir(dir);
		if (ent == NULL)
			break;
		if (ent->d_name[0] == '.')
			continue;
		fname = find_file(path + "/" + ent->d_name, name, depth + 1);
		if (!fname.empty()) {
			closedir(dir);
			return fname;
		}
	}
	closedir(dir);
	return "";
}
#endif

#if defined(__ANDROID__)
extern"C" void Android_JNI_GetLibraryDir(char *buf);
#endif

void init()
{
	printf(NAME " (version %s)\n", version);

	std::string fname;
#if defined(__ANDROID__)
	char data_path[256];
	Android_JNI_GetLibraryDir(data_path);
	fname = strf("%s/lib" DIR_NAME ".so", data_path);
#else
	fname = DATA_PATH DIR_NAME ".dat";
#endif
	open_pack(fname.c_str());

	load_settings();

	srand(time(NULL));

#if defined(_WIN32)
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2,2), &wsadata)) {
		throw std::runtime_error("Unable to initialize winsock");
	}
#endif
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) {
		throw std::runtime_error(
			std::string("Unable to initialize SDL: ")
			+ SDL_GetError());
	}
#if defined(CONFIG_SDL_GLES)
	if (SDL_GLES_Init(SDL_GLES_VERSION_1_1)) {
		throw std::runtime_error(
			std::string("Unable to initialize GLES: ")
			+ SDL_GetError());
	}
#endif

	open_window();
	init_sound();

	std::string title = strf(NAME " (%s)", version);
	SDL_WM_SetCaption(title.c_str(), NULL);

	glewInit();

	init_gl();

	glColor4fv(white);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_CULL_FACE);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 70);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, black);

	TTF_Init();
#if defined(_WIN32)
	fname = strf("%s\\fonts\\arial.ttf", getenv("WINDIR"));
#elif defined(__ANDROID__)
	fname = strf("%s/libVera.so", data_path);
#elif defined(CONFIG_GLES)
	fname = DATA_PATH "Vera.ttf";
#else
	fname = find_file("/usr/share/fonts", "Vera.ttf");
	if (fname.empty()) {
		fname = find_file("/usr/share/fonts", "DejaVuSans.ttf");
	}
#endif

	font = TTF_OpenFont(fname.c_str(), FONT_SIZE);
	if (font == NULL) {
		throw std::runtime_error("Unable to load " + fname);
	}

	room.load("room.obj");
	sorvi.load("sorvi.obj");
	akseli.load("akseli.obj");
	support.load("support.obj");
	support.set_alpha(0.6);
	load_mpeg(&motor_sound, "motor.ogg");
	load_mpeg(&cutting_sound, "cutting.ogg");
	load_mpeg(&lightsaber_hit, "lightsaberhit.ogg");
	load_mpeg(&lightsaber_always, "lightsaberalways.ogg");
	load_mpeg(&sanding, "sanding.ogg");
}

void playgame()
{
	level_num = 0;
	score = 0;
	memset(inventory, 0, sizeof(void *) * NUM_BLADES);
	inventory[0] = &chisels[0];
	blade_num = 0;

	while (levels[level_num].descr != NULL) {
		level = &levels[level_num];
		mission();
		if (game())
			return;
		if (!finish())
			break;
		level_num++;
	}
	highscores();
}

}

int main(int argc, char **argv)
try {
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "-noshaders") {
			shaders_enabled = false;
		} else if (arg == "-window") {
			fullscreen = false;
		}
	}

	init();
	SDL_PauseAudio(0);

	while (true) {
		menu();
		playgame();
	}
	return 0;

} catch (const std::exception &e) {
	SDL_Quit();
#if !defined(_WIN32)
	fprintf(stderr, "ERROR: %s\n", e.what());
#else
	/* windows */
	MessageBox(NULL, e.what(), NAME " ERROR", MB_ICONERROR|MB_OK);
#endif
	return 1;
}

