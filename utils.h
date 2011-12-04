#if !defined(_UTILS_H)
#define _UTILS_H

#include <list>
#include <map>
#include <math.h>
#include <string>
#include <assert.h>

#define warning(...)	fprintf(stderr, "WARNING: " __VA_ARGS__)

#define UNUSED(x)	((void) (x))

#define DISABLE_COPY_AND_ASSIGN(x)	\
	private: \
		x(const x &from); \
		void operator =(const x &from)

class vec3 {
public:
	float x, y, z;

	vec3() {}
	vec3(float i_x, float i_y, float i_z) :
		x(i_x), y(i_y), z(i_z)
	{
	}
};

class vec2 {
public:
	float x, y;

	vec2() {}
	vec2(float i_x, float i_y) :
		x(i_x), y(i_y)
	{
	}
};

extern inline vec3 operator + (const vec3 &a, const vec3 &b)
{
	return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

extern inline vec3 operator - (const vec3 &a, const vec3 &b)
{
	return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

extern inline vec3 operator * (const vec3 &v, float s)
{
	return vec3(v.x * s, v.y * s, v.z * s);
}

extern inline bool operator > (const vec3 &a, const vec3 &b)
{
	return a.x > b.x && a.y > b.y && a.z > b.z;
}

extern inline bool operator < (const vec3 &a, const vec3 &b)
{
	return a.x < b.x && a.y < b.y && a.z < b.z;
}

extern inline vec2 operator + (const vec2 &a, const vec2 &b)
{
	return vec2(a.x + b.x, a.y + b.y);
}

extern inline vec2 operator - (const vec2 &a, const vec2 &b)
{
	return vec2(a.x - b.x, a.y - b.y);
}

extern inline vec2 operator * (const vec2 &v, float s)
{
	return vec2(v.x * s, v.y * s);
}

extern inline bool operator > (const vec2 &a, const vec2 &b)
{
	return a.x > b.x && a.y > b.y;
}

extern inline bool operator < (const vec2 &a, const vec2 &b)
{
	return a.x < b.x && a.y < b.y;
}

extern inline float dot(const vec3 &a, const vec3 &b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

extern inline float dot(const vec2 &a, const vec2 &b)
{
	return a.x*b.x + a.y*b.y;
}

extern inline float length(const vec3 &v)
{
	return sqrtf(dot(v, v));
}

extern inline float length(const vec2 &v)
{
	return sqrtf(dot(v, v));
}

template<class T>
class list_iter {
public:
	list_iter(std::list<T> &l) :
		i(l.begin()),
		end(l.end())
	{
	}
	T *operator -> () const { return &*i; }
	T &operator *() const { return *i; }
	operator bool() const { return i != end; }
	void operator ++()
	{
		i++;
	}

private:
	typename std::list<T>::iterator i, end;
};

template<class T>
class safe_list_iter {
public:
	safe_list_iter(std::list<T> &l) :
		i(l.begin()),
		next(l.begin()),
		end(l.end())
	{
		if (next != end)
			next++;
	}
	typename std::list<T>::iterator iter() const { return i; }
	T *operator -> () const { return &*i; }
	T &operator *() const { return *i; }
	operator bool() const { return i != end; }
	void operator ++()
	{
		i = next;
		if (next != end)
			next++;
	}

private:
	typename std::list<T>::iterator i, next, end;
};

template<class T>
class const_list_iter {
public:
	const_list_iter(const std::list<T> &l) :
		i(l.begin()),
		end(l.end())
	{
	}
	const T *operator -> () const { return &*i; }
	T operator *() const { return *i; }
	operator bool() const { return i != end; }
	void operator ++()
	{
		i++;
	}

private:
	typename std::list<T>::const_iterator i, end;
};

template<class Key, class Value>
class map_iter {
public:
	map_iter(std::map<Key, Value> &l) :
		i(l.begin()),
		end(l.end())
	{
	}
	Key key() const { return i->first; }
	Value *operator -> () const { return &i->second; }
	Value &operator *() const { return i->second; }
	operator bool() const { return i != end; }
	void operator ++()
	{
		i++;
	}

private:
	typename std::map<Key, Value>::iterator i, end;
};

template<class Key, class Value>
class safe_map_iter {
public:
	safe_map_iter(std::map<Key, Value> &l) :
		i(l.begin()),
		next(l.begin()),
		end(l.end())
	{
		if (next != end)
			next++;
	}
	Key key() const { return i->first; }
	typename std::map<Key, Value>::iterator iter() const { return i; }
	Value *operator -> () const { return &i->second; }
	Value &operator *() const { return i->second; }
	operator bool() const { return i != end; }
	void operator ++()
	{
		i = next;
		if (next != end)
			next++;
	}

private:
	typename std::map<Key, Value>::iterator i, next, end;
};

template<class Key, class Value>
class const_map_iter {
public:
	const_map_iter(const std::map<Key, Value> &l) :
		i(l.begin()),
		end(l.end())
	{
	}
	Key key() const { return i->first; }
	const Value *operator -> () const { return &i->second; }
	Value operator *() const { return i->second; }
	operator bool() const { return i != end; }
	void operator ++()
	{
		i++;
	}

private:
	typename std::map<Key, Value>::const_iterator i, end;
};

template<class Key, class Value>
void ins(std::multimap<Key, Value> &map, const Key &key, const Value &value)
{
	map.insert(std::pair<Key, Value>(key, value));
}

template<class Key, class Value>
void del(std::multimap<Key, Value> &map, const Key &key, const Value &value)
{
	typename std::multimap<Key, Value>::iterator i = map.find(key);
	while (i != map.end()) {
		if (i->second == value) {
			map.erase(i);
			return;
		}
		i++;
	}
	assert(0);
}

template<class Key, class Value>
void ins(std::map<Key, Value> &map, const Key &key, const Value &value)
{
	map.insert(std::pair<Key, Value>(key, value));
}

template<class Key, class Value>
void del(std::map<Key, Value> &map, const Key &key)
{
	typename std::map<Key, Value>::iterator i = map.find(key);
	assert(i != map.end());
	map.erase(i);
}

template<class Key, class Value>
Value get(const std::map<Key, Value> &map, const Key &key)
{
	typename std::map<Key, Value>::const_iterator i = map.find(key);
	assert(i != map.end());
	return i->second;
}

template<class Key, class Value>
Value &get(std::map<Key, Value> &map, const Key &key)
{
	typename std::map<Key, Value>::iterator i = map.find(key);
	assert(i != map.end());
	return i->second;
}

std::string strf(const char *fmt, ...);
vec3 cross(const vec3 &a, const vec3 &b);

#endif
