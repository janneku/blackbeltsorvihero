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
 * OpenGL related stuff: models, lights, bloom
 */
#include "gl.h"
#include "common.h"
#include <fstream>
#include <assert.h>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#define BLOOM_SIZE	1024
#define BLUR_SIZE	256

#if !defined(CONFIG_GLES)
const float MAX_LIGHT_DIST = 100;
#else
const float MAX_LIGHT_DIST = 1000;
#endif

const float white[] = {1, 1, 1, 1};
const float black[] = {0, 0, 0, 1};
const color white_color(1, 1, 1);
const color black_color(0, 0, 0);

namespace {

int num_lights = 0;

#if !defined(CONFIG_GLES)
GLuint bump;
GLuint bloom1;
GLuint bloom2;
GLuint blur;
GLuint bloom1_fbo;
GLuint bloom2_fbo;
GLuint bloom_out_fbo;
GLuint bloom1_tex;
GLuint bloom2_tex;
GLuint bloom_out_tex;
GLuint blur_fbo;
GLuint blur_tex;
GLuint blur_out_fbo;
GLuint blur_out_tex;
#endif

}

namespace {

int round_power2(int val)
{
	int i = 1;
	while (i < val)
		i <<= 1;
	return i;
}

void png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	PackFile *f = (PackFile *) png_get_io_ptr(png_ptr);
	if (f->read(data, length) < length) {
		throw std::runtime_error("Partial PNG");
	}
}

void load_png(Bitmap *bitmap, const char *fname)
{
	PackFile f(fname);

	printf("loading %s\n", fname);

	png_byte header[8];
	f.read(header, 8);
	if (png_sig_cmp(header, 0, 8)) {
		throw std::runtime_error(std::string("not a PNG: ") + fname);
	}

	png_structp png_ptr =
		png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	png_set_read_fn(png_ptr, &f, png_read_data);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);
	bitmap->width = png_get_image_width(png_ptr, info_ptr);
	bitmap->height = png_get_image_height(png_ptr, info_ptr);

	switch (png_get_color_type(png_ptr, info_ptr)) {
	case PNG_COLOR_TYPE_GRAY:
		bitmap->components = 1;
		break;
	case PNG_COLOR_TYPE_RGB:
		bitmap->components = 3;
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		bitmap->components = 4;
		break;
	default:
		throw std::runtime_error(std::string("invalid color type: ")
					+ fname);
	}

	printf("  %d x %d x %d\n", bitmap->width, bitmap->height,
		bitmap->components);

	bitmap->pixels.resize(bitmap->width * bitmap->height
			      * bitmap->components, 0);
	std::vector<png_bytep> rows(bitmap->height);
	for (int i = 0; i < bitmap->height; ++i) {
		rows[i] = png_bytep(
			&bitmap->pixels[bitmap->width * bitmap->components * i]);
	}
	png_read_image(png_ptr, &rows[0]);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

}

bool shaders_enabled = true;

void set_light(const vec3 &pos, const color &diffuse, bool bloom)
{
	if (num_lights >= 8)
		return;
	GLenum light = GL_LIGHT0 + num_lights;
	num_lights++;

	const float ambient[] = {0.3, 0.3, 0.3, 1};

	float p[] = {0, 0, 0, 1};
	memcpy(p, &pos.x, sizeof(float) * 3);
	if (!bloom) {
		glLightfv(light, GL_DIFFUSE, &diffuse.r);
		glLightfv(light, GL_AMBIENT, ambient);
	} else {
		glLightfv(light, GL_DIFFUSE, black);
		glLightfv(light, GL_AMBIENT, black);
	}
	glLightf(light, GL_QUADRATIC_ATTENUATION, 0.00001);
	glLightfv(light, GL_SPECULAR, &diffuse.r);
	glLightfv(light, GL_POSITION, p);
	glEnable(light);
}

void kill_lights()
{
	for (int i = 1; i < num_lights; ++i) {
		glDisable(GL_LIGHT0 + i);
	}
	num_lights = 0;
}

void draw_faces(const Face *f, int len)
{
	GLClientState state;
	state.enable(GL_VERTEX_ARRAY);
	state.enable(GL_NORMAL_ARRAY);
	state.enable(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &f->vert[0].pos.x);
	glNormalPointer(GL_FLOAT, sizeof(Vertex), &f->vert[0].norm.x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &f->vert[0].tc.x);

	glDrawArrays(GL_TRIANGLES, 0, len * 3);
}

void draw_array(GLenum type, const Vertex *v, int len)
{
	if (type == GL_TRIANGLES) {
		assert(len % 3 == 0);
	}

	GLClientState state;
	state.enable(GL_VERTEX_ARRAY);
	state.enable(GL_NORMAL_ARRAY);
	state.enable(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(*v), &v->pos.x);
	glNormalPointer(GL_FLOAT, sizeof(*v), &v->norm.x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(*v), &v->tc.x);

	glDrawArrays(type, 0, len);
}

void draw_array(GLenum type, const VertexColor *v, int len)
{
	if (type == GL_TRIANGLES) {
		assert(len % 3 == 0);
	}

	GLClientState state;
	state.enable(GL_VERTEX_ARRAY);
	state.enable(GL_COLOR_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(*v), &v->pos.x);
	glColorPointer(4, GL_FLOAT, sizeof(*v), &v->c.r);

	glDrawArrays(type, 0, len);
	glColor4fv(white);
}

void draw_array(GLenum type, const VertexTexCoord *v, int len)
{
	if (type == GL_TRIANGLES) {
		assert(len % 3 == 0);
	}

	GLClientState state;
	state.enable(GL_VERTEX_ARRAY);
	state.enable(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(*v), &v->pos.x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(*v), &v->tc.x);

	glDrawArrays(type, 0, len);
	glColor4fv(white);
}

Text::Text() :
	m_texture(INVALID_TEXTURE)
{
}

Text::~Text()
{
	if (m_texture != INVALID_TEXTURE) {
		glDeleteTextures(1, &m_texture);
	}
}

void Text::init(TTF_Font *font, const std::string &text)
{
	std::list<SDL_Surface *> rows;
	SDL_Color fg = {255, 255, 255, 0};
	SDL_Color bg = {0, 0, 0, 0};
	m_height = 0;
	m_width = 0;
	size_t i = 0;
	while (i < text.size()) {
		size_t j = text.find('\n', i);
		if (j == std::string::npos)
			j = text.size();
		std::string row = text.substr(i, j - i);
		if (row.empty()) {
			row = " ";
		}

		SDL_Surface *s =
			TTF_RenderText_Shaded(font, row.c_str(), fg, bg);
		if (s == NULL)
			break;
		rows.push_back(s);
		if (s->w > m_width)
			m_width = s->w;
		m_height += s->h;
		i = j + 1;
	}
	m_width = round_power2(m_width);
	m_height = round_power2(m_height);
	std::string image(m_width * m_height, 0);

	/* build one big texture */
	int y = 0;
	for (const_list_iter<SDL_Surface *> j(rows); j; ++j) {
		SDL_Surface *s = *j;

		for (int i = 0; i < s->h; ++i) {
			char *dst = &image[y * m_width];
			Uint8 *src = (Uint8 *)s->pixels + i * s->pitch;
			memcpy(dst, src, s->w);
			y++;
		}
		SDL_FreeSurface(s);
	}

	if (m_texture == INVALID_TEXTURE) {
		glGenTextures(1, &m_texture);
	}
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_width, m_height, 0,
			  GL_ALPHA, GL_UNSIGNED_BYTE, image.data());
}

void Text::draw() const
{
	assert(m_texture != INVALID_TEXTURE);

	GLState state;
	state.enable(GL_TEXTURE_2D);
	state.enable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	Vertex vert[2 * 3];
	int vi = 0;
	vert[vi].tc = vec2(0, 0);
	vert[vi++].pos = vec3(0, 0, 0);
	vert[vi].tc = vec2(0, 1);
	vert[vi++].pos = vec3(0, m_height, 0);
	vert[vi].tc = vec2(1, 0);
	vert[vi++].pos = vec3(m_width, 0, 0);

	vert[vi].tc = vec2(0, 1);
	vert[vi++].pos = vec3(0, m_height, 0);
	vert[vi].tc = vec2(1, 1);
	vert[vi++].pos = vec3(m_width, m_height, 0);
	vert[vi].tc = vec2(1, 0);
	vert[vi++].pos = vec3(m_width, 0, 0);
	draw_array(GL_TRIANGLES, vert, 2 * 3);
}

Image::Image() :
	m_texture(INVALID_TEXTURE)
{
}

Image::~Image()
{
	if (m_texture != INVALID_TEXTURE) {
		glDeleteTextures(1, &m_texture);
	}
}

void Image::load(const char *fname)
{
	Bitmap bitmap;
	load_png(&bitmap, fname);

	GLenum mode;
	switch (bitmap.components) {
	case 1:
		mode = GL_ALPHA;
		break;
	case 3:
		mode = GL_RGB;
		break;
	case 4:
		mode = GL_RGBA;
		break;
	default:
		assert(0);
	}

	m_width = round_power2(bitmap.width);
	m_height = round_power2(bitmap.height);
	m_components = bitmap.components;

	std::string image(m_width * m_height * m_components, 0);
	for (int i = 0; i < bitmap.height; ++i) {
		memcpy(&image[m_width * m_components * i],
			&bitmap.pixels[bitmap.width * m_components * i],
			bitmap.width * m_components);
	}

	if (m_texture == INVALID_TEXTURE) {
		glGenTextures(1, &m_texture);
	}
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, mode, m_width, m_height, 0,
			  mode, GL_UNSIGNED_BYTE, image.data());
}

void Image::draw() const
{
	assert(m_texture != INVALID_TEXTURE);

	GLState state;
	state.enable(GL_TEXTURE_2D);
	if (m_components == 1 || m_components == 4) {
		state.enable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glBindTexture(GL_TEXTURE_2D, m_texture);
	Vertex vert[2 * 3];
	int vi = 0;
	vert[vi].tc = vec2(0, 0);
	vert[vi++].pos = vec3(0, 0, 0);
	vert[vi].tc = vec2(0, 1);
	vert[vi++].pos = vec3(0, m_height, 0);
	vert[vi].tc = vec2(1, 0);
	vert[vi++].pos = vec3(m_width, 0, 0);

	vert[vi].tc = vec2(0, 1);
	vert[vi++].pos = vec3(0, m_height, 0);
	vert[vi].tc = vec2(1, 1);
	vert[vi++].pos = vec3(m_width, m_height, 0);
	vert[vi].tc = vec2(1, 0);
	vert[vi++].pos = vec3(m_width, 0, 0);
	draw_array(GL_TRIANGLES, vert, 2 * 3);
}

void Image::scale(float s)
{
	m_width *= s;
	m_height *= s;
}

namespace {

#if !defined(CONFIG_GLES)
void print_shader_log(GLuint obj)
{
	int maxLength;
	if (glIsShader(obj)) {
		glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &maxLength);
	} else {
		glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &maxLength);
	}

	std::string infoLog(maxLength, 0);

	int infologLength = 0;
	if (glIsShader(obj)) {
		glGetShaderInfoLog(obj, maxLength, &infologLength,
				   &infoLog[0]);
	} else {
		glGetProgramInfoLog(obj, maxLength, &infologLength,
				    &infoLog[0]);
	}
	infoLog.resize(infologLength);

	if (infologLength > 0) {
		printf("---- SHADER LOG ----\n%s----------------\n",
			infoLog.c_str());
	}
}
#endif

}

#if !defined(CONFIG_GLES)
GLuint create_fbo(GLuint *img, int width, int height, bool bilinear)
{
	printf("creating FBO %d x %d\n", width, height);

	GLuint fbo;
	glGenFramebuffersEXT(1, &fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

	GLuint depthbuffer;
	glGenRenderbuffersEXT(1, &depthbuffer);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthbuffer);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
				 width, height);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
				GL_RENDERBUFFER_EXT, depthbuffer);

	GLenum filter = GL_NEAREST;
	if (bilinear) {
		filter = GL_LINEAR;
	}

	glGenTextures(1, img);
	glBindTexture(GL_TEXTURE_2D, *img);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB,
		     GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
				GL_TEXTURE_2D, *img, 0);

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)
	    != GL_FRAMEBUFFER_COMPLETE_EXT) {
		throw std::runtime_error("Unable to create FBO");
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	return fbo;
}
#endif

GLuint load_texture(const char *fname)
{
	Bitmap bitmap;
	load_png(&bitmap, fname);

	GLenum mode;
	switch (bitmap.components) {
	case 1:
		mode = GL_ALPHA;
		break;
	case 3:
		mode = GL_RGB;
		break;
	case 4:
		mode = GL_RGBA;
		break;
	default:
		assert(0);
	}

	if (round_power2(bitmap.width) != bitmap.width
	    || round_power2(bitmap.height) != bitmap.height) {
		throw std::runtime_error(
			std::string("Non power-of-two texture: ")
			+ fname);
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D, 0, mode, bitmap.width, bitmap.height, 0,
			  mode, GL_UNSIGNED_BYTE, bitmap.pixels.data());
	return texture;
}

#if !defined(CONFIG_GLES)
GLuint load_program(const char *vs_source, const char *fs_source)
{
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_source, NULL);
	glCompileShader(vs);
	print_shader_log(vs);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_source, NULL);
	glCompileShader(fs);
	print_shader_log(fs);

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	print_shader_log(program);
	return program;
}
#endif

Model::~Model()
{
	for (map_iter<std::string, Group> g(m_groups); g; ++g) {
		if (g->num_faces > 0) {
			glDeleteBuffers(1, &g->buffer);
		}
	}
}

Group *Model::find_material(const std::string &name)
{
	return &get(m_groups, name);
}

void Model::load_materials(const char *fname)
{
	PackFile f(fname);

	size_t i = 0;
	std::string buf;
	Group group;

	while (true) {
		size_t j = buf.find('\n', i);
		if (j == std::string::npos) {
			buf.erase(buf.begin(), buf.begin() + i);
			i = 0;

			size_t pos = buf.size();
			buf.resize(pos + 256, 0);
			size_t got = f.read(&buf[pos], 256);
			if (got <= 0)
				break;
			buf.resize(pos + got);
			continue;
		}

		std::istringstream parser(buf.substr(i, j - i));

		std::string type;
		parser >> type;
		if (!parser || type[0] == '#') {
			i = j + 1;
			continue;
		}

		if (type == "newmtl") {
			if (!group.name.empty()) {
				ins(m_groups, group.name, group);
			}
			parser >> group.name;
			group.buffer = 0;
			group.texture = INVALID_TEXTURE;
			group.intensity = 0;
			group.specular = false;
			group.pos = vec3(0, 0, 0);
			group.diffuse = white_color;
		} else if (type == "Kd") {
			color c;
			parser >> c.r >> c.g >> c.b;
			c.a = 1;
			group.diffuse = c;
		} else if (type == "map_Kd") {
			std::string fname;
			parser >> fname;
			group.texture = load_texture(fname.c_str());
		} else if (type == "spec") {
			group.specular = true;
		} else if (type == "bloom") {
			parser >> group.intensity;
			assert(group.intensity > 0);
		}
		i = j + 1;
	}
	if (!group.name.empty()) {
		ins(m_groups, group.name, group);
	}
}

void Model::load(const char *fname)
{
	PackFile f(fname);

	for (map_iter<std::string, Group> g(m_groups); g; ++g) {
		if (g->num_faces > 0) {
			glDeleteBuffers(1, &g->buffer);
		}
	}
	m_groups.clear();

	std::vector<vec3> vertices;
	std::vector<vec3> normals;
	std::vector<vec2> texcoords;
	size_t i = 0;
	std::string buf;
	Group *group = NULL;

	while (true) {
		size_t j = buf.find('\n', i);
		if (j == std::string::npos) {
			buf.erase(buf.begin(), buf.begin() + i);
			i = 0;

			size_t pos = buf.size();
			buf.resize(pos + 256, 0);
			size_t got = f.read(&buf[pos], 256);
			if (got <= 0)
				break;
			buf.resize(pos + got);
			continue;
		}

		std::istringstream parser(buf.substr(i, j - i));

		std::string type;
		parser >> type;
		if (!parser || type[0] == '#') {
			i = j + 1;
			continue;
		}

		if (type == "v" || type == "vn") {
			vec3 v;
			parser >> v.x >> v.y >> v.z;
			if (type == "v")
				vertices.push_back(v);
			else
				normals.push_back(v);
		} else if (type == "vt") {
			vec2 v;
			parser >> v.x >> v.y;
			v.y = -v.y;
			texcoords.push_back(v);
		} else if (type == "mtllib") {
			std::string fname;
			parser >> fname;
			load_materials(fname.c_str());
		} else if (type == "usemtl") {
			std::string name;
			parser >> name;
			group = find_material(name);
		} else if (type == "f") {
			Face f;
			memset(&f, 0, sizeof f);
			for (int i = 0; i < 3; ++i) {
				int idx;
				parser >> idx;
				assert(idx > 0 && idx <= int(vertices.size()));
				f.vert[i].pos = vertices[idx - 1];
				if (parser.peek() == '/') {
					char c;
					parser >> c;
					if (isdigit(parser.peek())) {
						parser >> idx;
						assert(idx > 0 && idx <= int(texcoords.size()));
						f.vert[i].tc = texcoords[idx - 1];
					}
					if (parser.peek() == '/') {
						parser >> c;
						parser >> idx;
						assert(idx > 0 && idx <= int(normals.size()));
						f.vert[i].norm = normals[idx - 1];
					}
				}
				if (group != NULL) {
					group->pos = group->pos + f.vert[i].pos;
				}
			}
			if (group != NULL) {
				group->faces.push_back(f);
			}
		}
		i = j + 1;
	}

	for (map_iter<std::string, Group> g(m_groups); g; ++g) {
		g->num_faces = g->faces.size();
		if (g->num_faces == 0) {
			continue;
		}

		g->pos = g->pos * (0.3333 / g->num_faces);
		glGenBuffers(1, &g->buffer);
		glBindBuffer(GL_ARRAY_BUFFER, g->buffer);
		glBufferData(GL_ARRAY_BUFFER,
				sizeof(Face) * g->num_faces,
				&g->faces[0], GL_STATIC_DRAW);

		if (g->intensity < 0.001)
			continue;
		for (size_t j = 0; j < g->faces.size(); ++j) {
			const Face *f = &g->faces[j];
			vec3 pos(0, 0, 0);
			for (int i = 0; i < 3; ++i) {
				pos = pos + f->vert[i].pos * 0.3333;
			}
			Light *light = NULL;
			float nearest_dist = MAX_LIGHT_DIST * MAX_LIGHT_DIST;
			for (list_iter<Light> i(g->lights); i; ++i) {
				vec3 d = i->pos * (1.0 / i->num_faces) - pos;
				float dist = dot(d, d);
				if (dist < nearest_dist) {
					nearest_dist = dist;
					light = &*i;
				}
			}
			if (light == NULL) {
				Light light;
				light.pos = pos;
				light.num_faces = 1;
				g->lights.push_back(light);
			} else {
				light->pos = light->pos + pos;
				light->num_faces++;
			}
		}
		g->faces.clear();
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	printf("loaded %s\n", fname);
	printf("  %d groups\n", int(m_groups.size()));

	for (map_iter<std::string, Group> g(m_groups); g; ++g) {
		if (g->lights.empty())
			continue;
		g->intensity /= g->lights.size();
		printf("  %s: %d lights, %.2f\n", g.key().c_str(),
			int(g->lights.size()), g->intensity);
		color c = g->diffuse;
		c.r *= g->intensity;
		c.g *= g->intensity;
		c.b *= g->intensity;
		for (list_iter<Light> i(g->lights); i; ++i) {
			i->diffuse = c;
			i->pos = i->pos * (1.0 / i->num_faces);
		}
	}
}

void Model::set_alpha(float a)
{
	for (map_iter<std::string, Group> g(m_groups); g; ++g) {
		g->diffuse.a = a;
	}
}

void Model::set_lights(bool bloom) const
{
	for (const_map_iter<std::string, Group> g(m_groups); g; ++g) {
		for (const_list_iter<Light> i(g->lights); i; ++i) {
			set_light(i->pos, i->diffuse, bloom);
		}
	}
}

void Model::draw() const
{
	static int numlights_loc;
	static GLuint noise = INVALID_TEXTURE;
	if (noise == INVALID_TEXTURE) {
		noise = load_texture("noise.png");
#if !defined(CONFIG_GLES)
		if (GLEW_VERSION_2_0) {
			numlights_loc = glGetUniformLocation(bump, "numlights");
			assert(numlights_loc != -1);
		}
#endif
	}

	for (const_map_iter<std::string, Group> g(m_groups); g; ++g) {
		if (g->num_faces == 0) {
			continue;
		}

		GLState state;

		if (g->specular) {
			float color[] = {0.4, 0.4, 0.4, 1};
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
		}

		glBindBuffer(GL_ARRAY_BUFFER, g->buffer);

		if (g->texture != INVALID_TEXTURE) {
			state.enable(GL_LIGHTING);
			state.enable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, g->texture);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
		} else if (g->intensity > 0.001) {
			/* render without lighting */
			glColor4fv(&g->diffuse.r);
		} else {
			if (g->specular && shaders_enabled && GLEW_VERSION_2_0) {
#if !defined(CONFIG_GLES)
				/* the shader assumes lighting */
				glBindTexture(GL_TEXTURE_2D, noise);
				glUseProgram(bump);
				glUniform1i(numlights_loc, num_lights);
#endif
			} else {
				state.enable(GL_LIGHTING);
			}
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,
				     &g->diffuse.r);
		}

		draw_faces(NULL, g->num_faces);

		if (g->intensity > 0.001) {
			glColor4fv(white);
		} else if (g->texture == INVALID_TEXTURE &&
			   g->specular && shaders_enabled && GLEW_VERSION_2_0) {
#if !defined(CONFIG_GLES)
			glUseProgram(0);
#endif
		}
		if (g->specular) {
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#if !defined(CONFIG_GLES)
void begin_bloom()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, bloom1_fbo);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, BLOOM_SIZE, BLOOM_SIZE);
}

void end_bloom()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, bloom2_fbo);

	glUseProgram(bloom1);
	glBindTexture(GL_TEXTURE_2D, bloom1_tex);
	VertexTexCoord vert[4];
	int vi = 0;
	vert[vi].tc = vec2(0, 1);
	vert[vi++].pos = vec3(0, 0, 0);
	vert[vi].tc = vec2(0, 0);
	vert[vi++].pos = vec3(0, 1, 0);
	vert[vi].tc = vec2(1, 0);
	vert[vi++].pos = vec3(1, 1, 0);
	vert[vi].tc = vec2(1, 1);
	vert[vi++].pos = vec3(1, 0, 0);
	draw_array(GL_QUADS, vert, 4);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, bloom_out_fbo);

	glUseProgram(bloom2);
	glBindTexture(GL_TEXTURE_2D, bloom2_tex);
	vi = 0;
	vert[vi].tc = vec2(0, 1);
	vert[vi++].pos = vec3(0, 0, 0);
	vert[vi].tc = vec2(0, 0);
	vert[vi++].pos = vec3(0, 1, 0);
	vert[vi].tc = vec2(1, 0);
	vert[vi++].pos = vec3(1, 1, 0);
	vert[vi].tc = vec2(1, 1);
	vert[vi++].pos = vec3(1, 0, 0);
	draw_array(GL_QUADS, vert, 4);

	glUseProgram(0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glPopAttrib();
}

void draw_bloom()
{
	GLState state;
	state.enable(GL_BLEND);
	state.enable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, bloom_out_tex);

	glBlendFunc(GL_ONE, GL_ONE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Vertex vert[4];
	int vi = 0;
	vert[vi].tc = vec2(0, 1);
	vert[vi++].pos = vec3(0, 0, 0);
	vert[vi].tc = vec2(0, 0);
	vert[vi++].pos = vec3(0, 1, 0);
	vert[vi].tc = vec2(1, 0);
	vert[vi++].pos = vec3(1, 1, 0);
	vert[vi].tc = vec2(1, 1);
	vert[vi++].pos = vec3(1, 0, 0);
	draw_array(GL_QUADS, vert, 4);
}

void begin_blur()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, blur_fbo);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, BLUR_SIZE, BLUR_SIZE);
}

void end_blur()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, blur_out_fbo);

	glUseProgram(blur);
	glBindTexture(GL_TEXTURE_2D, blur_tex);
	VertexTexCoord vert[4];
	int vi = 0;
	vert[vi].tc = vec2(0, 1);
	vert[vi++].pos = vec3(0, 0, 0);
	vert[vi].tc = vec2(0, 0);
	vert[vi++].pos = vec3(0, 1, 0);
	vert[vi].tc = vec2(1, 0);
	vert[vi++].pos = vec3(1, 1, 0);
	vert[vi].tc = vec2(1, 1);
	vert[vi++].pos = vec3(1, 0, 0);
	draw_array(GL_QUADS, vert, 4);

	glUseProgram(0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glPopAttrib();
}

void draw_blur(float strength)
{
	GLState state;
	state.enable(GL_BLEND);
	state.enable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, blur_out_tex);

	float color[] = {1, 1, 1, strength};
	glColor4fv(color);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Vertex vert[4];
	int vi = 0;
	vert[vi].tc = vec2(0, 1);
	vert[vi++].pos = vec3(0, 0, 0);
	vert[vi].tc = vec2(0, 0);
	vert[vi++].pos = vec3(0, 1, 0);
	vert[vi].tc = vec2(1, 0);
	vert[vi++].pos = vec3(1, 1, 0);
	vert[vi].tc = vec2(1, 1);
	vert[vi++].pos = vec3(1, 0, 0);
	draw_array(GL_QUADS, vert, 4);

	glColor4fv(white);
}
#else
void begin_bloom() {}
void end_bloom() {}
void draw_bloom() {}
void begin_blur() {}
void end_blur() {}
void draw_blur(float strength) {}
#endif

void flush_gl()
{
#if defined(DEBUG)
	static Uint32 next_fps = 0;
	static int frames = 0;

	frames++;
	if (SDL_GetTicks() >= next_fps) {
		printf("%d fps\n", frames);
		next_fps = SDL_GetTicks() + 1000;
		frames = 0;
	}

	std::string errors;
	while (true) {
		GLenum error = glGetError();
		if (error == GL_NO_ERROR) {
			break;
		}
		errors += strf("0x%X ", error);
	}
	if (!errors.empty()) {
		throw std::runtime_error("GL errors occured: " + errors);
	}
#endif

	SDL_GL_SwapBuffers();
	SDL_Delay(5);
}

namespace {

#define stringify(s) str(s)
#define str(s) #s

const char *bloom1_source =
"uniform sampler2D tex;\
varying vec2 tc;\
void main(void)\
{\
	vec4 sum = vec4(0.0);\
	int i;\
	float s;\
	for (i = -9; i < 10; i++) {\
		s = 1.0 - abs(float(i) * 0.1);\
		sum = max(sum, texture2D(tex, tc + vec2(float(i)/" stringify(BLOOM_SIZE) ".0, 0.0)) * vec4(s));\
	}\
	gl_FragColor = sum;\
}";

const char *bloom2_source =
"uniform sampler2D tex;\
varying vec2 tc;\
void main(void)\
{\
	vec4 sum = vec4(0.0);\
	int i;\
	float s;\
	for (i = -9; i < 10; i++) {\
		s = 1.0 - abs(float(i) * 0.1);\
		sum = max(sum, texture2D(tex, tc + vec2(0.0, float(i)/" stringify(BLOOM_SIZE) ".0)) * vec4(s));\
	}\
	gl_FragColor = sum * 0.7;\
}";

const char *blur_source =
"uniform sampler2D tex;\
varying vec2 tc;\
void main(void)\
{\
	int x;\
	int y;\
	float s;\
	vec4 sum = vec4(0.0);\
	for (y = -2; y < 3; y++) {\
		for (x = -2; x < 3; x++) {\
			s = (0.26 - abs(float(x) * 0.05)) * (0.26 - abs(float(y) * 0.05));\
			sum = sum + texture2D(tex, tc + vec2(float(x)/" stringify(BLUR_SIZE) ".0, float(y)/" stringify(BLUR_SIZE) ".0)) * vec4(s);\
		}\
	}\
	gl_FragColor = sum;\
	gl_FragColor.a = 1.0;\
}";

const char *simple_vs =
"varying vec2 tc;\
void main(void)\
{\
	gl_Position = ftransform();\
	tc = gl_MultiTexCoord0.xy;\
}";

const char *bump_vs =
"varying vec3 N;\
varying vec3 v;\
varying vec2 tc;\
void main(void)\
{\
	v = vec3(gl_ModelViewMatrix * gl_Vertex);\
	N = normalize(gl_NormalMatrix * gl_Normal);\
	tc = (gl_Vertex.xy + gl_Vertex.zz) * 0.15;\
	gl_Position = ftransform();\
}";

const char *bump_fs =
"varying vec3 N;\
varying vec3 v;\
uniform sampler2D tex;\
uniform int numlights;\
varying vec2 tc;\
void main(void)\
{\
	int i;\
	gl_FragColor = vec4(0.0);\
	vec3 NN = N + (texture2D(tex, tc).xyz - vec3(0.5, 0.5, 0.5)) * 0.3;\
	for (i = 0; i < 6; i++) {\
	if (i < numlights) {\
	float d = length(gl_LightSource[i].position.xyz - v);\
	vec3 L = (gl_LightSource[i].position.xyz - v) * (1.0 / d);\
	vec3 E = normalize(-v);\
	float atten = 1.0 / (gl_LightSource[i].constantAttenuation +\
		d * (gl_LightSource[i].linearAttenuation +\
		d * gl_LightSource[i].quadraticAttenuation));\
	vec3 R = normalize(-reflect(L,NN));\
	vec4 Iamb = gl_FrontLightProduct[i].ambient;\
	vec4 Idiff = gl_FrontLightProduct[i].diffuse * max(dot(NN,L), 0.0);\
	vec4 Ispec = gl_FrontLightProduct[i].specular\
		* pow(max(dot(R,E),0.0),0.3*gl_FrontMaterial.shininess);\
	gl_FragColor += (Iamb + Idiff + Ispec) * atten;\
	}}\
	gl_FragColor.a = gl_FrontLightProduct[0].diffuse.a;\
}";

}

void init_gl()
{
	printf("init_gl()\n");

	if (!GLEW_VERSION_1_5) {
		throw std::runtime_error("You need a better graphics card! (OpenGL 1.5 required)");
	}
	if (!GL_EXT_framebuffer_object) {
		throw std::runtime_error("GL_EXT_framebuffer_object required");
	}

	if (GLEW_VERSION_2_0) {
		printf("  OpenGL 2.0 found\n");

#if !defined(CONFIG_GLES)
		bloom1 = load_program(simple_vs, bloom1_source);
		bloom2 = load_program(simple_vs, bloom2_source);
		bloom1_fbo = create_fbo(&bloom1_tex, BLOOM_SIZE, BLOOM_SIZE, false);
		bloom2_fbo = create_fbo(&bloom2_tex, BLOOM_SIZE, BLOOM_SIZE, false);
		bloom_out_fbo = create_fbo(&bloom_out_tex, BLOOM_SIZE, BLOOM_SIZE, true);
		blur_fbo = create_fbo(&blur_tex, BLUR_SIZE, BLUR_SIZE, false);
		blur_out_fbo = create_fbo(&blur_out_tex, BLUR_SIZE, BLUR_SIZE, true);
		bump = load_program(bump_vs, bump_fs);
		blur = load_program(simple_vs, blur_source);
#endif
	} else {
		warning("OpenGL 2.0 not found - shaders not available!\n");
	}
}

