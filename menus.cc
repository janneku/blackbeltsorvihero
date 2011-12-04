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
 * Menus, high scores, mission
 */
#include "menus.h"
#include "wood.h"
#include "chisel.h"
#include "common.h"
#include "version.h"
#include "http.h"
#include "gl.h"
#include "sound.h"
#include "sha1.h"
#include <stdexcept>
#include <sstream>

namespace {

#if !defined(CONFIG_GLES)
const int ITEM_HEIGHT = 70;
#else
const int ITEM_HEIGHT = 40;
#endif

const char *HIGHSCORE_SERVER = "localhost";
const char *HIGHSCORE_SECRET_KEY = "XXXXXXXXXX";

const int NUM_SCORES = 5;
const int MAX_SETTINGS = 10;

Animator<float> ui_anim;
Animator<float> scroll;
std::string input_buf;
bool writing;
bool done;
Text largetext;
vec2 mouse_pos;
Text input;
Text shape_text[MAX_SHAPES];
Text highscore_names[NUM_SCORES];
int scores[NUM_SCORES];
Text settings_texts[MAX_SETTINGS];
bool credits;
bool help;
int level_score;

enum {
	MENU_START,
	MENU_HELP,
	MENU_HIGH_SCORES,
	MENU_SETTINGS,
	MENU_CREDITS,
	MENU_QUIT,
	NUM_MENU_ITEMS
};

void draw_mission()
{
	static Image logo;
	static Image contin;
	if (!logo.ready()) {
		logo.load("mission.png");
		contin.load("continue_button.png");
#if defined(CONFIG_GLES)
		logo.scale(0.5);
#endif
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, float(scr_width) / scr_height, 0.01, 500);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(80, 0, 50, -20, 0, 10, 0, 0, 1);

	set_light(vec3(60, 40, 60), white_color);
	set_light(vec3(100, -40, -60), color(0.2, 0.4, 0.6));

	int count = 0;
	while (level->shapes[count].holes != NULL) {
		count++;
	}

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	{
		GLState state;
		state.enable(GL_DEPTH_TEST);

		float x = (count - 1) * -10 - scroll * (count - 1) * 20;
		for (int i = 0; level->shapes[i].holes != NULL; ++i) {
			const Shape *shape = &level->shapes[i];

			GLMatrixScope matrix;
			glTranslatef(0, x, 0);

			float profile[PROFILE_LEN];
			create_wood_profile(shape, profile);
			draw_wood(profile, PROFILE_LEN);
			x += 20;
		}
	}

	kill_lights();

	hud_matrix();

	float x = (count - 1) * -scr_width/12 + scr_width/2 - scr_width/12
		- scroll * (count - 1) * scr_width/6;
	for (int i = 0; level->shapes[i].holes != NULL; ++i) {
		GLMatrixScope matrix;
		glTranslatef(x + scr_width/12 - 64, scr_height * 3/4, 0);
		shape_text[i].draw();

		x += scr_width / 6;
	}

	{
		GLMatrixScope matrix;
		glTranslatef(scr_width - 6*DIGIT_WIDTH - 10, 10, 0);
		draw_number(score, 6);
	}

	{
		GLMatrixScope matrix;
		glTranslatef(scr_width/2 - 200, scr_height/2 - 100, 0);
		largetext.draw();
	}

	{
		GLMatrixScope matrix;
		glTranslatef(scr_width - 160 - 10, scr_height - 64 - 10, 0);
		contin.draw();
	}

	GLMatrixScope matrix;
	glTranslatef(10, 10, 0);
	logo.draw();
}

void draw_finish()
{
	static Image logo;
	static Image contin;
	static Image failed;
	if (!logo.ready()) {
		logo.load("finished.png");
		contin.load("continue_button.png");
		failed.load("failed.png");
#if defined(CONFIG_GLES)
		logo.scale(0.5);
#endif
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, float(scr_width) / scr_height, 0.01, 500);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(80, 0, 50, -20, 0, 10, 0, 0, 1);

	set_light(vec3(60, 40, 60), white_color);
	set_light(vec3(100, -40, -60), color(0.2, 0.4, 0.6));

	int count = 0;
	while (level->shapes[count].holes != NULL) {
		count++;
	}

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	{
		GLState state;
		state.enable(GL_DEPTH_TEST);

		float x = (count - 1) * -10 - scroll * (count - 1) * 20;
		for (int i = 0; level->shapes[i].holes != NULL; ++i) {
			const Shape *shape = &level->shapes[i];

			GLMatrixScope matrix;
			glTranslatef(0, x, 0);

			float profile[PROFILE_LEN];
			create_wood_profile(shape, profile);
			draw_wood(profile, PROFILE_LEN);
			x += 20;
		}
	}

	kill_lights();

	hud_matrix();

	float x = (count - 1) * -scr_width/12 + scr_width/2 - scr_width/12
		- scroll * (count - 1) * scr_width/6;
	for (int i = 0; level->shapes[i].holes != NULL; ++i) {
		GLMatrixScope matrix;
		glTranslatef(x + scr_width/12 - 64, scr_height * 3/4, 0);
		shape_text[i].draw();

		x += scr_width / 6;
	}

	{
		GLMatrixScope matrix;
		glTranslatef(scr_width - 6*DIGIT_WIDTH - 10, 10 - ui_anim, 0);
		draw_number(score, 6);
	}

	{
		GLMatrixScope matrix;
		glTranslatef(scr_width/2 - 6*20, scr_height/2 - 64, 0);
		draw_number(level_score, 6);
	}

	if (level_score < level->min_score) {
		GLMatrixScope matrix;
		glTranslatef(scr_width/2, scr_height/2 + 40, 0);
		glScalef(1 + ui_anim, 1 + ui_anim, 1);
		glTranslatef(-150, -64, 0);
		failed.draw();
	}

	{
		GLMatrixScope matrix;
		glTranslatef(scr_width - 160 - 10,
			     scr_height - 64 - 10 + ui_anim, 0);
		contin.draw();
	}

	GLMatrixScope matrix;
	glTranslatef(10, 10, 0);
	logo.draw();
}

void draw_background_scene()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, float(scr_width) / scr_height, 0.01, 500);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(-50, -200, -90, -50, 0, -50, 0, 0, 1);

	set_light(vec3(-50, -100, 40), white_color);
	set_light(vec3(50, -100, -80), color(0.2, 0.4, 0.8));
	set_light(vec3(80, -100, -80), white_color);

	glRotatef(SDL_GetTicks() * 0.03, 0, 0, 1);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	GLState state;
	state.enable(GL_DEPTH_TEST);

	sorvi.draw();
	akseli.draw();
	support.draw();

	kill_lights();

}

void menu_background()
{
	if (shaders_enabled && GLEW_VERSION_2_0) {
		begin_blur();
		draw_background_scene();
		end_blur();
	}

	draw_background_scene();

	hud_matrix();

	if (shaders_enabled && GLEW_VERSION_2_0) {
		draw_blur(0.4);
	}
}

int get_selected(const vec2 &p)
{
	for (int i = 0; i < NUM_MENU_ITEMS; ++i) {
		vec2 place(scr_width - 256 - 30,
		           scr_height - ITEM_HEIGHT*NUM_MENU_ITEMS
			   + ITEM_HEIGHT*i);
		if (p > place && p < place + vec2(256, ITEM_HEIGHT)) {
			return i;
		}
	}
	return -1;
}

void draw_menu()
{
	static Image logo;
	static Image items[NUM_MENU_ITEMS];
	static Image back;
	static Image magix;
	static Image help_img;
	if (!logo.ready()) {
		logo.load("logo.png");
		magix.load("magix.png");
		items[MENU_START].load("menu_start.png");
		items[MENU_HELP].load("menu_help.png");
		items[MENU_HIGH_SCORES].load("menu_high.png");
		items[MENU_SETTINGS].load("menu_settings.png");
		items[MENU_CREDITS].load("menu_credits.png");
		items[MENU_QUIT].load("menu_quit.png");
#if defined(CONFIG_GLES)
		logo.scale(0.5);
		for (int i = 0; i < NUM_MENU_ITEMS; ++i) {
			items[i].scale(0.5);
		}
#endif
		back.load("back_button.png");
		help_img.load("help.png");
	}

	menu_background();

	hud_matrix();

	if (credits) {
		GLMatrixScope matrix;
		glTranslatef(scr_width/2 - 144/2, scr_height - 100, 0);
		magix.draw();
	}

	if (help) {
		GLMatrixScope matrix;
		glTranslatef(scr_width/2 - 450/2, scr_height/2 + 120, 0);
		help_img.draw();
	}

	if (credits || help) {
		GLMatrixScope matrix;
		glTranslatef(scr_width/2 - 200, scr_height/2 - 100, 0);
		largetext.draw();
	}

	if (credits || help) {
		GLMatrixScope matrix;
		glTranslatef(10, scr_height - 64 - 10, 0);

		back.draw();
	}

	{
		GLMatrixScope matrix;
		glTranslatef(10, 10, 0);
		logo.draw();
	}

	if (credits || help)
		return;

	const float passive[] = {0.5, 0.5, 0.5, 1};

	int selected = get_selected(mouse_pos);
	for (int i = 0; i < NUM_MENU_ITEMS; ++i) {
		GLMatrixScope matrix;
		glTranslatef(scr_width - 256 - 30,
			     scr_height - ITEM_HEIGHT*NUM_MENU_ITEMS
			     + ITEM_HEIGHT*i, 0);
		if (i != selected) {
			glColor4fv(passive);
		}
		items[i].draw();
		glColor4fv(white);
	}
}

void menu_event(const SDL_Event *e)
{
	switch (e->type) {
	case SDL_MOUSEMOTION: {
		mouse_pos = vec2(e->motion.x, e->motion.y);
		break;
	}

	case SDL_MOUSEBUTTONDOWN: {
		if (e->button.button == SDL_BUTTON_LEFT) {
			vec2 p(e->button.x, e->button.y);
			if (credits || help) {
				if (p > vec2(0, scr_height - 64 - 10) &&
				    p < vec2(128 + 10, scr_height)) {
					credits = false;
					help = false;
				}
				break;
			}
			switch (get_selected(p)) {
			case -1:
				break;
			case MENU_START:
				done = true;
				break;
			case MENU_HELP:
				help = true;
				largetext.init(font,
					"Your job is to carve a given shape out of wood with a\n"
					"chisel. The better you match the shape and more fine job\n"
					"you do, the better score you will get.\n\n"
					"With score you can buy new and better chisels. Watch out\n"
					"for going too fast, as you might overheat your chisel!\n\n"
					"You can try to beat the high scores of other users\n"
					"on Internet by playing throught all the levels!\n\n"
					"Change chisel with number keys.");
				break;
			case MENU_HIGH_SCORES:
				highscores();
				done = false;
				break;
			case MENU_SETTINGS:
				settings();
				done = false;
				break;
			case MENU_CREDITS: {
				credits = true;
				/* DO NOT CHANGE */
				std::string text = strf(NAME " (version %s)\n"
					"by Pizzalaatikko\n\n"
					"for Assembly Summer 2011\n\n"
					"Janne 'Japeq' Kulmala <janne.t.kulmala@iki.fi>\n"
					"Code, graphics, models\n\n"
					"Antti 'Amikaze' Rajamäki <amikaze@gmail.com>\n"
					"Models, graphics, sounds", version);
				largetext.init(font, text);
				break;
			}
			case MENU_QUIT:
				exit();
			default:
				assert(0);
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
			exit();
		default:
			break;
		}
		break;
	}
}

void draw_highscores()
{
	static Image logo;
	static Image enter;
	static Image back;
	if (!logo.ready()) {
		logo.load("highscores.png");
		enter.load("entername.png");
		back.load("back_button.png");
#if defined(CONFIG_GLES)
		logo.scale(0.5);
#endif
	}

	menu_background();

	hud_matrix();

	if (score > 0) {
		GLMatrixScope matrix;
		glTranslatef(scr_width - 6*DIGIT_WIDTH - 10, 10, 0);
		draw_number(score, 6);
	}

	{
		GLMatrixScope matrix;
		glTranslatef(10, scr_height - 64 - 10, 0);
		back.draw();
	}

	{
		GLMatrixScope matrix;
		glTranslatef(10, scr_height/2 - 50, 0);
		largetext.draw();
	}

	if (writing) {
		GLMatrixScope matrix;
		glTranslatef(10, scr_height/2, 0);
		enter.draw();
		glTranslatef(0, 70, 0);
		input.draw();
	}

	{
		GLMatrixScope matrix;
		glTranslatef(10, 10, 0);
		logo.draw();
	}

	for (int i = 0; i < NUM_SCORES; ++i) {
		GLMatrixScope matrix;
		glTranslatef(scr_width/2,
			     scr_height/2 + (DIGIT_HEIGHT + 10) * i, 0);
		draw_number(scores[i], 6);
		glTranslatef(10 + DIGIT_WIDTH*6, 10, 0);
		highscore_names[i].draw();
	}
}

void draw_settings()
{
	static Image logo;
	static Image back;
	static Image checked;
	static Image refused;
	if (!logo.ready()) {
		logo.load("logo.png");
		back.load("back_button.png");
		checked.load("checked.png");
		refused.load("refused.png");
#if defined(CONFIG_GLES)
		logo.scale(0.5);
#endif
	}

	menu_background();

	hud_matrix();

	{
		GLMatrixScope matrix;
		glTranslatef(10, scr_height - 64 - 10, 0);
		back.draw();
	}

	{
		GLMatrixScope matrix;
		glTranslatef(10, 10, 0);
		logo.draw();
	}

	for (int i = 0; setting_list[i].ptr != NULL; ++i) {
		GLMatrixScope matrix;
		glTranslatef(scr_width/2 - 150, scr_height/2 + i * 50, 0);
		if (*setting_list[i].ptr)
			checked.draw();
		else
			refused.draw();
		glTranslatef(40, 10, 0);
		settings_texts[i].draw();
	}
}

std::string urlencode(const std::string &s)
{
	std::string out;
	for (size_t i = 0; i < s.size(); ++i) {
		if (isalnum(s[i]))
			out += s[i];
		else
			out += strf("%%%02X", (unsigned char) s[i]);
	}
	return out;
}

std::string hexlify(const std::string &s)
{
	std::string out;
	for (size_t i = 0; i < s.size(); ++i) {
		out += strf("%02x", (unsigned char) s[i]);
	}
	return out;
}

void refresh_highscores()
{
	memset(scores, 0, sizeof scores);
	for (int i = 0; i < NUM_SCORES; ++i) {
		highscore_names[i].init(font, "?");
	}

	std::string page;
	try {
		page = get_page(HIGHSCORE_SERVER, DIR_NAME "/highscores.txt", "");
	} catch (const std::runtime_error &e) {
		largetext.init(font, e.what());
		return;
	}

	int num = 0;
	size_t i = 0;
	while (i < page.size() && num < NUM_SCORES) {
		size_t j = page.find('\n', i);
		if (j == std::string::npos)
			j = page.size();
		std::string line = page.substr(i, j - i);

		std::istringstream parser(line);
		std::string type;
		parser >> scores[num];

		std::string name = line.substr(parser.tellg());
		highscore_names[num].init(font, name);
		num++;

		i = j + 1;
	}
}

void submit_highscore()
{
	writing = false;
	SDL_EnableUNICODE(0);

	sha1_ctx_t sha1;
	sha1_begin(&sha1);
	std::string hash = strf("%s %d %s", HIGHSCORE_SECRET_KEY, score,
				input_buf.c_str());
	sha1_hash(hash.data(), hash.size(), &sha1);
	std::string digest(SHA1_DIGEST_SIZE, 0);
	sha1_end(&digest[0], &sha1);

	std::string payload = strf("name=%s&score=%d&sign=%s",
		urlencode(input_buf).c_str(), score,
		hexlify(digest).c_str());
	try {
		get_page(HIGHSCORE_SERVER, DIR_NAME "/submit.php", payload);
	} catch (const std::runtime_error &e) {
		largetext.init(font, e.what());
		return;
	}

	score = 0;
	refresh_highscores();
}

}

void mission()
{
	done = false;

	largetext.init(font,strf("%s\n\n"
		"Time limit: %d seconds\n"
		"Minimum required score: %d",
			level->descr, level->time_limit, level->min_score));
	for (int i = 0; level->shapes[i].holes != NULL; ++i) {
		const Shape *shape = &level->shapes[i];
		shape_text[i].init(font, strf("Max score: %d", shape->score));
	}

	while (!done) {
		SDL_Event e;
		while (get_event(&e)) {
			switch (e.type) {
			case SDL_MOUSEMOTION:
				scroll.set_target(float(e.motion.x) / scr_width - 0.5);
				break;

			case SDL_MOUSEBUTTONDOWN: {
				if (e.button.button == SDL_BUTTON_LEFT) {
					vec2 p(e.button.x, e.button.y);
					if (p > vec2(scr_width - 160 - 10, scr_height - 64 - 10) &&
					    p < vec2(scr_width, scr_height)) {
						done = true;
					}
				}
				break;
			}
			case SDL_KEYDOWN:
				switch (e.key.keysym.sym) {
				case SDLK_ESCAPE:
				case SDLK_q:
#if SDL_VERSION_ATLEAST(1, 3, 0)
				case SDLK_AC_BACK:
#endif
					done = true;
					break;
				default:
					break;
				}
				break;
			}
		}
		static Timer timer;
		float dt = timer.get_dt();
		scroll.update(dt, 300);
		draw_mission();
		flush_gl();
	}
}

bool finish()
{
	static Once once;
	static Sound hayjoo;
	if (once) {
		load_mpeg(&hayjoo, "hayjoo.ogg");
	}

	play_music("lobby.ogg");

	done = false;
	ui_anim.set_value(64 + 10);
	ui_anim.set_target(0);
	int max_score = 0;
	level_score = 0;
	for (int i = 0; level->shapes[i].holes != NULL; ++i) {
		const Shape *shape = &level->shapes[i];
		level_score += shape_score[i];
		max_score += shape->score;
		shape_text[i].init(font,
				strf("Score: %d\nMax score: %d",
				     shape_score[i], shape->score));
	}
	if (level_score >= max_score * 3/4) {
		play_sound(&hayjoo);
	}

	while (!done) {
		SDL_Event e;
		while (get_event(&e)) {
			switch (e.type) {
			case SDL_MOUSEMOTION:
				scroll.set_target(float(e.motion.x) / scr_width - 0.5);
				break;

			case SDL_MOUSEBUTTONDOWN: {
				if (e.button.button == SDL_BUTTON_LEFT) {
					vec2 p(e.button.x, e.button.y);
					if (p > vec2(scr_width - 160 - 10, scr_height - 64 - 10) &&
					    p < vec2(scr_width, scr_height)) {
						done = true;
					}
				}
				break;
			}
			case SDL_KEYDOWN:
				switch (e.key.keysym.sym) {
				case SDLK_ESCAPE:
				case SDLK_q:
#if SDL_VERSION_ATLEAST(1, 3, 0)
				case SDLK_AC_BACK:
#endif
					done = true;
					break;
				default:
					break;
				}
				break;
			}
		}
		static Timer timer;
		float dt = timer.get_dt();
		scroll.update(dt, 300);
		ui_anim.update(dt, 100);
		draw_finish();
		flush_gl();
	}
	kill_sounds();
	return level_score >= level->min_score;
}

void menu()
{
	play_music("lobby.ogg");
	done = false;
	help = false;
	credits = false;
	while (!done) {
		SDL_Event e;
		while (get_event(&e)) {
			menu_event(&e);
		}
		draw_menu();
		flush_gl();
	}
}

void highscores()
{
	done = false;
	writing = false;
	largetext.init(font, "");
	refresh_highscores();

	if (score > scores[NUM_SCORES - 1]) {
		writing = true;
		input_buf.clear();
		input.init(font, "_");
		SDL_EnableUNICODE(1);
	}

	while (!done) {
		SDL_Event e;
		while (get_event(&e)) {
			switch (e.type) {
			case SDL_MOUSEBUTTONDOWN: {
				if (e.button.button == SDL_BUTTON_LEFT) {
					vec2 p(e.button.x, e.button.y);
					if (p > vec2(0, scr_height - 64 - 10) &&
					    p < vec2(128 + 10, scr_height)) {
						done = true;
					}
				}
				break;
			}
			case SDL_KEYDOWN:
				switch (e.key.keysym.sym) {
				case SDLK_RETURN:
					if (writing) {
						submit_highscore();
					}
					break;
				case SDLK_ESCAPE:
					if (writing) {
						writing = false;
						SDL_EnableUNICODE(0);
					} else {
						done = true;
					}
					break;
				case SDLK_BACKSPACE:
					if (writing && !input_buf.empty()) {
						input_buf.resize(input_buf.size() - 1);
						input.init(font, input_buf + "_");
					}
					break;
				default:
					if (writing
					    && e.key.keysym.unicode >= ' '
					    && e.key.keysym.unicode < 256) {
						input_buf += char(e.key.keysym.unicode);
						input.init(font, input_buf + "_");
					}
					break;
				}
				break;
			}
		}
		draw_highscores();
		flush_gl();
	}
}

void settings()
{
	done = false;
	for (int i = 0; setting_list[i].ptr != NULL; ++i) {
		settings_texts[i].init(font, setting_list[i].text);
	}

	while (!done) {
		SDL_Event e;
		while (get_event(&e)) {
			switch (e.type) {
			case SDL_MOUSEBUTTONDOWN: {
				if (e.button.button == SDL_BUTTON_LEFT) {
					vec2 p(e.button.x, e.button.y);
					if (p > vec2(0, scr_height - 64 - 10) &&
					    p < vec2(128 + 10, scr_height)) {
						done = true;
					}
					for (int i = 0; setting_list[i].ptr != NULL; ++i) {
						vec2 place(scr_width/2 - 150,
							   scr_height/2 + i * 50);
						if (p > place && p < place + vec2(300, 32)) {
							set_setting(&setting_list[i],
								    !*setting_list[i].ptr);
							break;
						}
					}
				}
				break;
			}
			case SDL_KEYDOWN:
				switch (e.key.keysym.sym) {
				case SDLK_ESCAPE:
				case SDLK_q:
#if SDL_VERSION_ATLEAST(1, 3, 0)
				case SDLK_AC_BACK:
#endif
					done = true;
					break;
				default:
					break;
				}
				break;
			}
		}
		draw_settings();
		flush_gl();
	}
}
