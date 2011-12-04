/*
 * Black Belt Sorvi Hero
 *
 * Copyright 2011 Janne Kulmala <janne.t.kulmala@iki.fi>,
 * Antti Rajamäki <amikaze@gmail.com>
 *
 * Program code and resources are licensed with GNU LGPL 2.1. See
 * COPYING.LGPL file.
 *
 * Shop
 */
#include "shop.h"
#include "gl.h"
#include "sound.h"
#include "chisel.h"

namespace {

const float SCROLL_SPEED = 0.4;

const vec3 SHOP_ORIG(-315, -100, -40);
bool done;
Animator<float> ui_anim;
Animator<float> scroll_anim;
Animator<float> scroll_vel;
float scroll = 0;
vec2 mouse_pos(0, 0);
Animator<vec3> camera;
Animator<float> sel_raise[MAX_SHOP_CHISELS];
bool leaving;
int num_chisels;

void draw_scene(bool bloom)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, float(scr_width) / scr_height, 0.01, 1000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	vec3 p = camera;
	gluLookAt(p.x, p.y, p.z, SHOP_ORIG.x, SHOP_ORIG.y, SHOP_ORIG.z + 5,
		  0, 0, 1);

	room.set_lights(bloom);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	GLState state;
	state.enable(GL_DEPTH_TEST);

	room.draw();

	float x = (num_chisels - 1) * 5 + scroll * 10;
	for (int i = 0; chisels[i].descr != NULL; ++i) {
		const Chisel *chisel = &chisels[i];
		const Blade *blade = &blades[chisel->blade];

		if (chisel->level > level_num)
			break;
		if (chisel <= inventory[chisel->blade])
			continue;

		GLMatrixScope matrix;
		glTranslatef(-315, -100, -40);
		glTranslatef(0, -x, sel_raise[i] - 1);
		glRotatef(-90, 0, 0, 1);

		float profile[MAX_BLADE_WIDTH] = {};
		create_blade(blade, profile);
		draw_chisel(chisel, profile);

		x -= 10;
	}

	kill_lights();

	draw_smoke(camera);
}

void draw_hud()
{
	static Image logo;
	static Image back;
	static Image buy;
	static Text names[MAX_SHOP_CHISELS];
	if (!logo.ready()) {
		logo.load("shop.png");
#if defined(CONFIG_GLES)
		logo.scale(0.5);
#endif
		for (int i = 0; chisels[i].descr != NULL; ++i) {
			const Chisel *chisel = &chisels[i];
			const Chisel *prev = inventory[chisel->blade];

			std::string text = strf("%s\n\n%s\n\nPrice: %d\nReplaces: %s",
				chisel->name, chisel->descr, chisel->price,
				prev ? prev->name : "(none)");
			names[i].init(font, text);
		}
		back.load("back_button.png");
		buy.load("buy_button.png");
	}

	hud_matrix();

	{
		GLMatrixScope matrix;
		glTranslatef(scr_width - 6*DIGIT_WIDTH - 10, 10 - ui_anim, 0);
		draw_number(score, 6);
	}

	{
		GLMatrixScope matrix;
		glTranslatef(10, 10, 0);
		logo.draw();
	}

	float x = (num_chisels - 1) * -scr_width/8
		- scroll * scr_width/4 + scr_width/2 - scr_width/8;
	for (int i = 0; chisels[i].descr != NULL; ++i) {
		const Chisel *chisel = &chisels[i];

		if (chisel->level > level_num)
			break;
		if (chisel <= inventory[chisel->blade])
			continue;

		vec2 place(x, scr_height/4);
		if (mouse_pos > place &&
		    mouse_pos < vec2(x + scr_width / 4, scr_height)) {
			sel_raise[i].set_target(1);
		} else {
			sel_raise[i].set_target(0);
		}

		{
			GLMatrixScope matrix;
			glTranslatef(x, scr_height/4, 0);

			names[i].draw();

			glTranslatef(0, 170, 0);

			VertexColor vert[2 * 3];
			int vi = 0;
			vert[vi].c = color(1, 1, 0);
			vert[vi++].pos = vec3(0, 0, 0);
			vert[vi].c = color(1, 1, 0);
			vert[vi++].pos = vec3(0, 20, 0);
			vert[vi].c = color(1, 1 - chisel->max_stress * 0.1, 0);
			vert[vi++].pos = vec3(chisel->max_stress * 10, 0, 0);

			vert[vi].c = color(1, 1, 0);
			vert[vi++].pos = vec3(0, 20, 0);
			vert[vi].c = color(1, 1 - chisel->max_stress * 0.1, 0);
			vert[vi++].pos = vec3(chisel->max_stress * 10, 20, 0);
			vert[vi].c = color(1, 1 - chisel->max_stress * 0.1, 0);
			vert[vi++].pos = vec3(chisel->max_stress * 10, 0, 0);
			draw_array(GL_TRIANGLES, vert, 2 * 3);

			Vertex line[5];
			vi = 0;
			line[vi++].pos = vec3(0, 0, 0);
			line[vi++].pos = vec3(0, 20, 0);
			line[vi++].pos = vec3(chisel->max_stress * 10, 20, 0);
			line[vi++].pos = vec3(chisel->max_stress * 10, 0, 0);
			line[vi++].pos = vec3(0, 0, 0);
			draw_array(GL_LINE_STRIP, line, 5);
		}

		if (chisel->price <= score) {
			GLMatrixScope matrix;
			glTranslatef(x + scr_width/8 - 64, scr_height * 3/4, 0);
			buy.draw();
		}

		x += scr_width / 4;
	}

	{
		GLMatrixScope matrix;
		glTranslatef(10, scr_height - 64 - 10 + ui_anim, 0);
		back.draw();
	}
}

void draw_shop()
{
	if (shaders_enabled && GLEW_VERSION_2_0) {
		begin_bloom();
		draw_scene(true);
		end_bloom();
	}

	draw_scene(false);

	if (shaders_enabled && GLEW_VERSION_2_0) {
		draw_bloom();
	}

	draw_hud();
}

const Chisel *get_selected(const vec2 &p)
{
	float x = (num_chisels - 1) * -scr_width/8
		- scroll * scr_width/4 + scr_width/2 - scr_width/8;
	for (int i = 0; chisels[i].descr != NULL; ++i) {
		const Chisel *chisel = &chisels[i];

		if (chisel->level > level_num)
			break;
		if (chisel <= inventory[chisel->blade])
			continue;

		vec2 buy_place(x + scr_width/8 - 64, scr_height * 3/4);
		if (p > buy_place &&
		    p < buy_place + vec2(128, 64) &&
		    chisel->price <= score) {
			return chisel;
		}

		x += scr_width / 4;
	}
	return NULL;
}

void shop_event(const SDL_Event *e)
{
	static Sound buy_sound;
	static Sound awwyeah;
	static Once once;
	if (once) {
		load_mpeg(&buy_sound, "buy.ogg");
		load_mpeg(&awwyeah, "awwyeah.ogg");
	}

	switch (e->type) {
	case SDL_MOUSEMOTION:
		mouse_pos = vec2(e->motion.x, e->motion.y);
#if !defined(CONFIG_GLES)
		scroll_anim.set_target((float(e->motion.x) / scr_width - 0.5)
					* (num_chisels - 1));
#else
		if (e->motion.state & 1) {
			scroll_vel.set_value(-SCROLL_SPEED * e->motion.xrel);
			scroll_vel.set_target(0);
		}
#endif
		break;

	case SDL_MOUSEBUTTONDOWN: {
		if (e->button.button == SDL_BUTTON_LEFT) {
			vec2 p(e->button.x, e->button.y);
			if (p > vec2(0, scr_height - 64 - 10) &&
			    p < vec2(128 + 10, scr_height)) {
				camera.set_target(SHOP_ORIG + vec3(200, 0, 20));
				ui_anim.set_target(64 + 10);
				leaving = true;
			} else {
				const Chisel *chisel = get_selected(p);
				if (chisel == NULL)
					break;
				if (chisel->blade == BLADE_LIGHTSABER) {
					play_sound(&awwyeah);
				} else {
					play_sound(&buy_sound);
				}
				buy(chisel);
			}
		}
		break;
	}
	case SDL_KEYDOWN:
		switch (e->key.keysym.sym) {
		case SDLK_ESCAPE:
		case SDLK_q:
#if SDL_VERSION_ATLEAST(1, 3, 0)
		case SDLK_AC_BACK:
#endif
			camera.set_target(SHOP_ORIG + vec3(200, 0, 20));
			ui_anim.set_target(64 + 10);
			leaving = true;
			break;
		default:
			break;
		}
		break;
	}
}

}

void shop()
{
	play_music("lobby.ogg");
	done = false;
	leaving = false;
	ui_anim.set_value(64 + 10);
	ui_anim.set_target(0);
	camera.set_value(SHOP_ORIG + vec3(200, 0, 20));
	camera.set_target(SHOP_ORIG + vec3(30, 0, 10));
	for (int i = 0; i < MAX_SHOP_CHISELS; ++i) {
		sel_raise[i].set_value(0);
	}
	num_chisels = 0;
	for (int i = 0; chisels[i].descr != NULL; ++i) {
		const Chisel *chisel = &chisels[i];

		if (chisel->level > level_num)
			break;
		if (chisel <= inventory[chisel->blade])
			continue;
		num_chisels++;
	}

	while (!done) {
		SDL_Event e;
		while (get_event(&e)) {
			shop_event(&e);
		}
		static Timer timer;
		float dt = timer.get_dt();
		ui_anim.update(dt, 100);
#if !defined(CONFIG_GLES)
		scroll_anim.update(dt, 300);
		scroll = scroll_anim;
#else
		scroll_vel.update(dt, 100);
		scroll += scroll_vel * dt;
		if (scroll < -(num_chisels - 1) * 0.5)
			scroll = -(num_chisels - 1) * 0.5;
		if (scroll > (num_chisels - 1) * 0.5)
			scroll = (num_chisels - 1) * 0.5;
#endif
		camera.update(dt, 10);
		update_particles(dt);

		if (randf(0, 1 / dt) < 0.5) {
			Particle p;
			p.smoke = true;
			p.c = color(0.2, 0.2, 0.2);
			p.pos = room.find_material("smoke")->pos;
			p.vel = vec3(0, 0, 0);
			add_particle(p);
		}

		if (vec3(camera).x > SHOP_ORIG.x+200-10 && leaving) {
			done = true;
		}
		for (int i = 0; i < 20; ++i) {
			sel_raise[i].update(dt, 100);
		}
		draw_shop();
		flush_gl();
	}
	kill_particles();
}
