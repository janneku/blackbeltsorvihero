#include "common.h"

const int PROFILE_LEN = 512;
const float PROFILE_STEP = 0.1;
const int MAX_SHAPES = 20;

const float WOOD_ORIGIN_X = -PROFILE_LEN * PROFILE_STEP / 2;

struct Hole;

struct Sound;

struct Shape {
	const Hole *holes;
	int score;
	float thickness;
	float stress;
};

extern int shape_score[];
extern float cut_profile[];
extern float current_shape[];
extern float initial_error;
extern int shape_num;
extern float rotation;
extern bool motor_on;
extern bool motor_been_on;
extern Animator<float> speed;
extern Animator<float> wood_raise;
extern const Level levels[];
extern bool explosion;
extern float wood_fly;
extern Sound motor_sound;

float wood_derivate(const float *profile, int length, int x);
void new_shape();
float get_depth(const float *profile, int cx);
void cut(float dt);
void create_wood_profile(const Shape *shape, float *profile);
void draw_wood(const float *profile, int length);
void switch_motor();
