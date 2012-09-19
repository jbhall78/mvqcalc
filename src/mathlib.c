/*
 *  Name: mathlib.c
 *  Description: Vector, matrix and quaternion math utilities
 *
 *  Copyright (C) 2012  Jason Hall <gplexcalc@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mathlib.h"

#undef QUAT_ABORT_ON_NAN

/******************************************************************************
 *
 * Quaternion Functions
 *
 *****************************************************************************/
quat_t *
quat_new(real x, real y, real z, real w)
{
    quat_t *q;

    q = malloc(sizeof(quat_t));
    memset(q, 0, sizeof(quat_t));

    *q[X] = x;
    *q[Y] = y;
    *q[Z] = z;
    *q[W] = w;

    return q;
}

void
quat_id(quat_t q)
{
    q[X] = 0.0;
    q[Y] = 0.0;
    q[Z] = 0.0;
    q[W] = 1.0;
}

void 
quat_cp(quat_t src, quat_t dst)
{
    dst[X] = src[X];
    dst[Y] = src[Y];
    dst[Z] = src[Z];
    dst[W] = src[W];
}

void
quat_set3(quat_t q, real x, real y, real z)
{
    quat_t xq, yq, zq;

    quat_id(q);

    if (x) {
	quat_set4(xq, 1.0, 0.0, 0.0, x);
	quat_mult(q, xq);
    }
    if (y) {
	quat_set4(yq, 0.0, 1.0, 0.0, y);
	quat_mult(q, yq);
    }
    if (z) {
	quat_set4(zq, 0.0, 0.0, 1.0, z);
	quat_mult(q, zq);
    }
}

#define quat_set4_optimized  quat_set4
void
quat_set4_optimized(quat_t q, real x, real y, real z, real angle)
{
    real r;
    // this function is only used for rotation around an already
    // unitized axis so we can skip a lot of multiplication and
    // a square root operation

    // convert to radians
    angle *= ( G_PI / 180.0 );

    /* NOTE(2div) i think the divide by two can be removed since it is 
                  remultiplied by two later, although the location of the
                  two would be a convenient place to magnify/shrink the 
                  value of the angles */

    // apparently half angle is the correct thing to use here
    r = sin(angle / 2.0);
    //r = sin(angle);
    q[X] = x * r;
    q[Y] = y * r;
    q[Z] = z * r;
    q[W] = cos(angle / 2.0);
    //q[W] = cos(angle);
}

void
quat_set4_correct(quat_t q, real x, real y, real z, real angle)
{
    real factor = x*x + y*y + z*z;
    real r;

    g_assert(factor != 0.0);

    angle *= ( G_PI / 180.0 );

    r = 1.0 / sqrt(factor);
    x = x * r;
    y = y * r;
    z = z * r;

    r = sin(angle / 2.0);
    q[X] = x * r;
    q[Y] = y * r;
    q[Z] = z * r;
    q[W] = cos(angle / 2.0);
}
#undef quat_set4_optimized

void
quat_mult(quat_t q1, quat_t q2)
{
    quat_t tmp;

    quat_cp(q1, tmp);

    q1[W] = q2[W] * tmp[W] - q2[X] * tmp[X] - q2[Y] * tmp[Y] - q2[Z] * tmp[Z];
    q1[X] = q2[W] * tmp[X] + q2[X] * tmp[W] + q2[Y] * tmp[Z] - q2[Z] * tmp[Y];
    q1[Y] = q2[W] * tmp[Y] - q2[X] * tmp[Z] + q2[Y] * tmp[W] + q2[Z] * tmp[X];
    q1[Z] = q2[W] * tmp[Z] + q2[X] * tmp[Y] - q2[Y] * tmp[X] + q2[Z] * tmp[W];
}

/* remember to normalize every so often!  quat_norm(dest); */
void
quat_add(quat_t q1, quat_t q2, quat_t dest)
{
        vec3_t t1, t2, t3;
        quat_t tf;

        vec3_cp(q1,t1);
        vec3_scale(t1,q2[W]);

        vec3_cp(q2,t2);
        vec3_scale(t2,q1[W]);

        vec3_cross(q2,q1,t3);
        vec3_add(t1,t2,tf);
        vec3_add(t3,tf,tf);
        tf[W] = q1[W] * q2[W] - vec3_dot(q1,q2);

        dest[X] = tf[X];
        dest[Y] = tf[Y];
        dest[Z] = tf[Z];
        dest[W] = tf[W];

}

void
quat_norm(quat_t q)
{
    real factor = q[W]*q[W] + q[X]*q[X] + q[Y]*q[Y] + q[Z]*q[Z];
    real scale;


#ifdef QUAT_ABORT_ON_NAN
    g_assert(factor != 0.0);
    scale = 1.0 / sqrt(factor);
#else
    if (factor == 0.0)
	scale = NAN;
    else
    	scale = 1.0 / sqrt(factor);
#endif

    q[X] *= scale;
    q[Y] *= scale;
    q[Z] *= scale;
    q[W] *= scale;
}

void
quat_inv(quat_t q)
{
    real inv;

    inv = 1.0 / (q[X]*q[X] + q[Y]*q[Y] + q[Z]*q[Z] + q[W]*q[W]);

    q[X] = -q[X] * inv;
    q[Y] = -q[Y] * inv;
    q[Z] = -q[Z] * inv;
    q[W] =  q[W] * inv;

    quat_norm(q); /* normalize */
}

void
quat_to_mat(quat_t q, mat4x4_t m)
{
    real x, y, z, w;
  
    x = q[X];
    y = q[Y];
    z = q[Z];
    w = q[W];

    // x vec
    M(0, 0) = 1.0 - 2.0 * (y*y + z*z);
    M(1, 0) = 2.0       * (x*y + z*w);
    M(2, 0) = 2.0       * (x*z - y*w);
    M(3, 0) = 0.0;

    // y vec
    M(0, 1) = 2.0       * (y*x - z*w);
    M(1, 1) = 1.0 - 2.0 * (x*x + z*z);
    M(2, 1) = 2.0       * (y*z + x*w);
    M(3, 1) = 0.0;

    // z vec
    M(0, 2) = 2.0       * (z*x + y*w);
    M(1, 2) = 2.0       * (z*y - x*w);
    M(2, 2) = 1.0 - 2.0 * (x*x + y*y);
    M(3, 2) = 0.0;

    M(0, 3) = 0.0;
    M(1, 3) = 0.0;
    M(2, 3) = 0.0;
    M(3, 3) = 1.0;
}

void
quat_to_mat_transpose(quat_t q, mat4x4_t m)
{
    real x, y, z, w;

    x = q[X];
    y = q[Y];
    z = q[Z];
    w = q[W];

    // x vec
    M(0, 0) = 1.0 - 2.0 * (y*y + z*z);
    M(0, 1) = 2.0       * (x*y + z*w);
    M(0, 2) = 2.0       * (x*z - y*w);
    M(0, 3) = 0.0;

    // y vec
    M(1, 0) = 2.0       * (y*x - z*w);
    M(1, 1) = 1.0 - 2.0 * (x*x + z*z);
    M(1, 2) = 2.0       * (y*z + x*w);
    M(1, 3) = 0.0;

    // z vec
    M(2, 0) = 2.0       * (z*x + y*w);
    M(2, 1) = 2.0       * (z*y - x*w);
    M(2, 2) = 1.0 - 2.0 * (x*x + y*y);
    M(2, 3) = 0.0;

    M(3, 0) = 0.0;
    M(3, 1) = 0.0;
    M(3, 2) = 0.0;
    M(3, 3) = 1.0;
}

void
quat_to_vecs(quat_t q, vec3_t dir, vec3_t up, vec3_t right)
{
    real x, y, z, w;

    x = q[X];
    y = q[Y];
    z = q[Z];
    w = q[W];

    // z vec
    if (dir != NULL) {
	dir[X] = 2.0       * (z*x - y*w);
	dir[Y] = 2.0       * (z*y + x*w);
	dir[Z] = 1.0 - 2.0 * (x*x + y*y);
	vec3_norm(dir);
    }

    // y vec
    if (up != NULL) {
	up[X] = 2.0       * (y*x + w*z);
	up[Y] = 1.0 - 2.0 * (x*x + z*z);
	up[Z] = 2.0       * (y*z - w*x);
	vec3_norm(up);
    }

    // x vec
    if (right != NULL) {
	right[X] = 1.0 - 2.0 * (y*y + z*z);
	right[Y] = 2.0       * (x*y - w*z);
	right[Z] = 2.0       * (x*z + w*y);
	vec3_norm(right);
    }
}


/******************************************************************************
 *
 * Vector Functions
 *
 *****************************************************************************/
vec3_t *
vec3_new(real x, real y, real z)
{
    vec3_t *v = g_malloc(sizeof(vec3_t));

    vec3_set(*v, x, y, z);

    return v;
}

void
vec3_destroy(vec3_t *v)
{
    g_free(v);
}

void
vec3_cross(vec3_t v1, vec3_t v2, vec3_t res)
{
    res[X] = v1[Y]*v2[Z] - v1[Z]*v2[Y];
    res[Y] = v1[Z]*v2[X] - v1[X]*v2[Z];
    res[Z] = v1[X]*v2[Y] - v1[Y]*v2[X];
}

void
vec3_norm(vec3_t v)
{
    real mag;
    
    mag = vec3_len(v);
    vec3_div(v, mag);
}

/* transform vectors by a matrix */
void
vec3_transform(vec3_t *v, int n, mat4x4_t m)
{
    real rx, ry, rz;
    int i;

    for (i = 0; i < n; i++) {
	real vx = v[i][X];
	real vy = v[i][Y];
	real vz = v[i][Z];

	rx = vx * M(0, 0) + vy * M(0, 1) + vz * M(0, 2) + M(0, 3);
	ry = vx * M(1, 0) + vy * M(1, 1) + vz * M(1, 2) + M(1, 3);
	rz = vx * M(2, 0) + vy * M(2, 1) + vz * M(2, 2) + M(2, 3);

	v[i][X] = rx;
	v[i][Y] = ry;
	v[i][Z] = rz;
    }
}

/*
 * NOTE: This method does not appear to be producing the desired results
 */
// return 0 if the angle is very small and probably shouldnt be used
gboolean
vec3_angle(vec3_t v1, vec3_t v2, real *angle)
{
    real dot;

    // obtain the lengths of the 2 vectors
    dot = vec3_dot(v1, v2);

    // now calculate the angle
    *angle = acos(dot);

    // check to see if the angle should be used or not
    if ((dot < 0.99990) && (dot > -0.9999))
	return 1;
    else 
	return 0;
}

/* taken from mesa */
// does not apply translation
void
transform_point(real out[4], real m[16], real in[4])
{
#define M1(row,col)  m[col*4+row]
   out[0] = M(0,0) * in[0] + M(0,1) * in[1] + M(0,2) * in[2] + M(0,3) * in[3];
   out[1] = M(1,0) * in[0] + M(1,1) * in[1] + M(1,2) * in[2] + M(1,3) * in[3];
   out[2] = M(2,0) * in[0] + M(2,1) * in[1] + M(2,2) * in[2] + M(2,3) * in[3];
   out[3] = M(3,0) * in[0] + M(3,1) * in[1] + M(3,2) * in[2] + M(3,3) * in[3];
#undef M1
}

gboolean
project(vec3_t obj, mat4x4_t model, mat4x4_t proj, int32_t viewport[4],
	vec3_t win)
{
    real in[4], out[4];
    
    in[X] = obj[X];
    in[Y] = obj[Y];
    in[Z] = obj[Z];
    in[W] = 1.0;

    transform_point(out, model, in);
    transform_point(in, proj, out);
   
    if (in[W] == 0.0)
	return FALSE;
    
    vec3_div(in, in[3]);
    
    win[X] = viewport[0] + (1 + in[0]) * viewport[2] / 2;
    win[Y] = viewport[1] + (1 + in[1]) * viewport[3] / 2;
    win[Z] = (1 + in[2]) / 2;

    return TRUE;
}


/******************************************************************************
 *
 * Matrix Functions
 *
 *****************************************************************************/
mat4x4_t *
mat4x4_new(void)
{
    mat4x4_t *m;
    m = malloc(sizeof(mat4x4_t));
    memset(m, 0, sizeof(mat4x4_t));
    mat4x4_id(*m);
    return m;
}

void
mat4x4_zero(mat4x4_t m)
{
    int i;

    for (i = 0; i < 16; i++)
	m[i] = 0.0;
}

void
mat4x4_scale(mat4x4_t m, real s)
{
    int i;

    for (i = 0; i < 16; i += 5)
	m[i] *= s;
}

void
mat4x4_id(mat4x4_t m)
{
    m[ 0] = 1.0; m[ 1] = 0.0; m[ 2] = 0.0; m[ 3] = 0.0;
    m[ 4] = 0.0; m[ 5] = 1.0; m[ 6] = 0.0; m[ 7] = 0.0;
    m[ 8] = 0.0; m[ 9] = 0.0; m[10] = 1.0; m[11] = 0.0;
    m[12] = 0.0; m[13] = 0.0; m[14] = 0.0; m[15] = 1.0;
}

#define M1(row,col)  m1[(col<<2)+row]
#define M2(row,col)  m2[(col<<2)+row]
#define M3(row,col)  dest[(col<<2)+row]
void
mat4x4_mult(mat4x4_t m1, mat4x4_t m2, mat4x4_t dest)
{
    int i;

    for (i = 0; i < 4; i++) {
	const real ai0=M1(i,0),  ai1=M1(i,1),  ai2=M1(i,2),  ai3=M1(i,3);
	M3(i,0) = ai0 * M2(0,0) + ai1 * M2(1,0) + ai2 * M2(2,0) + ai3 * M2(3,0);
	M3(i,1) = ai0 * M2(0,1) + ai1 * M2(1,1) + ai2 * M2(2,1) + ai3 * M2(3,1);
	M3(i,2) = ai0 * M2(0,2) + ai1 * M2(1,2) + ai2 * M2(2,2) + ai3 * M2(3,2);
	M3(i,3) = ai0 * M2(0,3) + ai1 * M2(1,3) + ai2 * M2(2,3) + ai3 * M2(3,3);
   }
}
#undef M1
#undef M2
#undef M3

void
mat4x4_translate(mat4x4_t m, real x, real y, real z)
{
   m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
   m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
   m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
   m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

void
mat4x4_copy(mat4x4_t src, mat4x4_t dest)
{
    int i;

    for (i = 0; i < 16; i++)
	dest[i] = src[i];
}
