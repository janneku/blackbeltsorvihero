#if !defined(_GL_H)
#define _GL_H

#if defined(CONFIG_GLES)
	#include <SDL.h>
	#include <GLES/gl.h>
	#include "gles_compat.h"
	#define GLEW_VERSION_1_5 1
	#define GLEW_VERSION_2_0 0
	#define GL_EXT_framebuffer_object 1
	#define glOrtho glOrthof
	#define glFrustum glFrustumf
	#define glUseProgram(x)	((void) (x))
	#define glColor4fv(x)	((void) (x))
	#define glewInit()
#else
#include <GL/glew.h>
#endif

#if defined(CONFIG_SDL_GLES)
	#include <SDL_gles.h>
	#define SDL_GL_SwapBuffers SDL_GLES_SwapBuffers
	#undef SDL_OPENGL
	#define SDL_OPENGL 0
#endif

#include <SDL_ttf.h>
#include <string>
#include <vector>
#include <assert.h>
#include "utils.h"

const GLuint INVALID_TEXTURE = -1;

class color {
public:
	float r, g, b, a;

	color() {}
	color(float i_r, float i_g, float i_b, float i_a = 1) :
		r(i_r), g(i_g), b(i_b), a(i_a)
	{
	}
};

struct Bitmap {
	int width;
	int height;
	int components;
	std::string pixels;
};

class Text {
	DISABLE_COPY_AND_ASSIGN(Text);

public:
	Text();
	~Text();

	bool ready() const { return m_texture != INVALID_TEXTURE; }

	void init(TTF_Font *font, const std::string &text);

	void draw() const;

private:
	GLuint m_texture;
	int m_width;
	int m_height;
};

class Image {
	DISABLE_COPY_AND_ASSIGN(Image);

public:
	Image();
	~Image();

	int width() const { return m_width; }
	int height() const { return m_height; }

	bool ready() const { return m_texture != INVALID_TEXTURE; }

	void load(const char *fname);

	void draw() const;

	void scale(float s);

private:
	GLuint m_texture;
	int m_components;
	int m_width;
	int m_height;
};

struct Vertex {
	vec3 pos;
	vec3 norm;
	vec2 tc;
};

struct VertexTexCoord {
	vec3 pos;
	vec2 tc;
};

struct VertexColor {
	vec3 pos;
	color c;
};

struct Face {
	Vertex vert[3];
};

struct Light {
	color diffuse;
	vec3 pos;
	int num_faces;
};

struct Group {
	color diffuse;
	GLuint buffer;
	GLuint texture;
	int num_faces;
	std::string name;
	float intensity;
	bool specular;
	std::vector<Face> faces;
	vec3 pos;
	std::list<Light> lights;
};

class Model {
	DISABLE_COPY_AND_ASSIGN(Model);

public:
	Model() {}
	~Model();

	void load(const char *fname);

	void set_lights(bool bloom = false) const;
	void draw() const;

	void set_alpha(float a);

	Group *find_material(const std::string &name);

private:
	std::map<std::string, Group> m_groups;

	void load_materials(const char *fname);
};

class GLState {
public:
	static const int MAX_STATES = 8;

	GLState() :
		m_num_states(0)
	{}
	~GLState()
	{
		for (int i = 0; i < m_num_states; ++i) {
			glDisable(m_states[i]);
		}
	}

	void enable(int state)
	{
		assert(m_num_states < MAX_STATES);
		glEnable(state);
		m_states[m_num_states++] = state;
	}

private:
	GLenum m_states[MAX_STATES];
	int m_num_states;
};

class GLClientState {
public:
	static const int MAX_STATES = 4;

	GLClientState() :
		m_num_states(0)
	{}
	~GLClientState()
	{
		for (int i = 0; i < m_num_states; ++i) {
			glDisableClientState(m_states[i]);
		}
	}

	void enable(int state)
	{
		assert(m_num_states < MAX_STATES);
		glEnableClientState(state);
		m_states[m_num_states++] = state;
	}

private:
	GLenum m_states[MAX_STATES];
	int m_num_states;
};

class GLMatrixScope {
public:
	GLMatrixScope()
	{
		glPushMatrix();
	}
	~GLMatrixScope()
	{
		glPopMatrix();
	}
};

extern const float white[];
extern const float black[];
extern const color white_color;
extern const color black_color;
extern bool shaders_enabled;

void set_light(const vec3 &pos, const color &diffuse, bool bloom = false);
void kill_lights();
void draw_faces(const Face *f, int len);
void draw_array(GLenum type, const Vertex *v, int len);
void draw_array(GLenum type, const VertexColor *v, int len);
void draw_array(GLenum type, const VertexTexCoord *v, int len);
GLuint draw_text(TTF_Font *font, const std::string &text);
GLuint create_fbo(GLuint *img, int width, int height, bool bilinear);
GLuint load_texture(const char *fname);
GLuint load_program(const char *vs_source, const char *fs_source);
void begin_bloom();
void end_bloom();
void draw_bloom();
void begin_blur();
void end_blur();
void draw_blur(float strength);
void flush_gl();
void init_gl();

#endif
