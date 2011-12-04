#include "utils.h"
#include <stdarg.h>
#include <stdio.h>

std::string strf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buffer[1024];
	vsprintf(buffer, fmt, args);
	va_end(args);
	return buffer;
}

vec3 cross(const vec3 &a, const vec3 &b)
{
	return vec3(a.y*b.z - a.z*b.y,
		    a.z*b.x - a.x*b.z,
		    a.x*b.y - a.y*b.x);
}
