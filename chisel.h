#include "common.h" /* for Animator<> */

const int MAX_BLADE_WIDTH = 64;
const float BLADE_LEN = 10;
const int MAX_SHOP_CHISELS = 20;

enum {
	HANDLE_BASIC,
	HANDLE_PRO,
	HANDLE_ULTIMATE,
	HANDLE_SCIFI,
	HANDLE_LIGHTSABER,
	HANDLE_HAND,
	HANDLE_CHAINSAW,
	MAX_HANDLES
};

enum {
	BLADE_FLAT,
	BLADE_FAST,
	BLADE_CURVE,
	BLADE_WIDE,
	BLADE_SHARP,
	BLADE_SAND,
	BLADE_LIGHTSABER,
	BLADE_CHAINSAW,
	NUM_BLADES
};

enum {
	HOLE_CURVE = 1,
	HOLE_LINEAR
};

struct Hole {
	int type;
	int x1;
	int x2;
	float depth1;
	float depth2;
};

struct Blade {
	const Hole *holes;
	int width;
	bool smooth;
};

struct Chisel {
	int blade;
	const char *name;
	const char *descr;
	int handle;
	float speed;
	float max_stress;
	int price;
	int level;
};

class Text;
struct Sound;

extern float current_blade[];
extern int blade_num;
extern const Chisel *inventory[];
extern float stress[];
extern float blade_x;
extern Animator<float> blade_raise;
extern Text chisel_name;
extern bool cutting;
extern Sound cutting_sound;
extern Sound lightsaber_hit;
extern Sound lightsaber_always;
extern Sound sanding;

extern const Blade blades[];
extern const Chisel chisels[];

void create_hole(float *profile, int len, const Hole *hole);
void begin_cutting();
void end_cutting();
void switch_blade();
void buy(const Chisel *chisel);
void create_blade(const Blade *blade, float *profile);
void draw_chisel(const Chisel *chisel, const float *profile);
