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
 * OpenGL compability for GLES
 */
#include "gl.h"
#include "utils.h"
#include "gles_compat.h"
#include <math.h>

void gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
	float xmin, xmax, ymin, ymax;
	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;
	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
	       GLfloat centerx, GLfloat centery, GLfloat centerz,
	       GLfloat upx, GLfloat upy, GLfloat upz)
{
	GLfloat m[16];

	/* Make rotation matrix */

	/* Z vector */
	vec3 z;
	z.x = eyex - centerx;
	z.y = eyey - centery;
	z.z = eyez - centerz;
	z = z * (1 / length(z));

	/* Y vector */
	vec3 y;
	y.x = upx;
	y.y = upy;
	y.z = upz;

	vec3 x = cross(y, z);
	y = cross(z, x);

	x = x * (1 / length(x));
	y = y * (1 / length(y));

#define M(row,col)  m[col*4+row]
	M(0, 0) = x.x;
	M(0, 1) = x.y;
	M(0, 2) = x.z;
	M(0, 3) = 0.0;
	M(1, 0) = y.x;
	M(1, 1) = y.y;
	M(1, 2) = y.z;
	M(1, 3) = 0.0;
	M(2, 0) = z.x;
	M(2, 1) = z.y;
	M(2, 2) = z.z;
	M(2, 3) = 0.0;
	M(3, 0) = 0.0;
	M(3, 1) = 0.0;
	M(3, 2) = 0.0;
	M(3, 3) = 1.0;
#undef M
	glMultMatrixf(m);

	/* Translate Eye to Origin */
	glTranslatef(-eyex, -eyey, -eyez);

}

static int __gluInvertMatrixd(const GLfloat m[16], GLfloat invOut[16])
{
	float inv[16], det;
	int i;

	inv[0] =
	    m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15]
	    + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
	inv[4] =
	    -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15]
	    - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
	inv[8] =
	    m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15]
	    + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
	inv[12] =
	    -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14]
	    - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
	inv[1] =
	    -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15]
	    - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
	inv[5] =
	    m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15]
	    + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
	inv[9] =
	    -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15]
	    - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
	inv[13] =
	    m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14]
	    + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
	inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15]
	    + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
	inv[6] =
	    -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15]
	    - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
	inv[10] =
	    m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15]
	    + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
	inv[14] =
	    -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14]
	    - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
	inv[3] =
	    -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11]
	    - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
	inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11]
	    + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
	inv[11] =
	    -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11]
	    - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
	inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10]
	    + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	if (det == 0)
		return GL_FALSE;

	det = 1.0 / det;

	for (i = 0; i < 16; i++)
		invOut[i] = inv[i] * det;

	return GL_TRUE;
}

static void __gluMultMatrixVecd(const GLfloat matrix[16], const GLfloat in[4],
				GLfloat out[4])
{
	int i;

	for (i = 0; i < 4; i++) {
		out[i] =
		    in[0] * matrix[0 * 4 + i] +
		    in[1] * matrix[1 * 4 + i] +
		    in[2] * matrix[2 * 4 + i] + in[3] * matrix[3 * 4 + i];
	}
}

static void __gluMultMatricesd(const GLfloat a[16], const GLfloat b[16],
			       GLfloat r[16])
{
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			r[i * 4 + j] =
			    a[i * 4 + 0] * b[0 * 4 + j] +
			    a[i * 4 + 1] * b[1 * 4 + j] +
			    a[i * 4 + 2] * b[2 * 4 + j] +
			    a[i * 4 + 3] * b[3 * 4 + j];
		}
	}
}

int gluUnProject(GLfloat winx, GLfloat winy, GLfloat winz,
		  const GLfloat modelMatrix[16],
		  const GLfloat projMatrix[16],
		  const GLint viewport[4],
		  GLfloat * objx, GLfloat * objy, GLfloat * objz)
{
	float finalMatrix[16];
	float in[4];
	float out[4];

	__gluMultMatricesd(modelMatrix, projMatrix, finalMatrix);
	if (!__gluInvertMatrixd(finalMatrix, finalMatrix))
		return (GL_FALSE);

	in[0] = winx;
	in[1] = winy;
	in[2] = winz;
	in[3] = 1.0;

	/* Map x and y from window coordinates */
	in[0] = (in[0] - viewport[0]) / viewport[2];
	in[1] = (in[1] - viewport[1]) / viewport[3];

	/* Map to range -1 to 1 */
	in[0] = in[0] * 2 - 1;
	in[1] = in[1] * 2 - 1;
	in[2] = in[2] * 2 - 1;

	__gluMultMatrixVecd(finalMatrix, in, out);
	if (out[3] == 0.0)
		return (GL_FALSE);
	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];
	*objx = out[0];
	*objy = out[1];
	*objz = out[2];
	return (GL_TRUE);
}
