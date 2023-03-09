/*
 *  Name: mathlib.h
 *  Description: Vector, matrix and quaternion math utilities
 *
 *  Copyright (C) 2012  Jason Hall
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

#ifndef _MATHLIB_H
#define _MATHLIB_H

#include <math.h>
#include <stdint.h>

#include <glib.h>

#define X	0
#define Y	1
#define Z	2
#define W	3

#define U	0
#define V	1

#define R	0
#define G	1
#define B	2
#define A	3

#define LEFT	0
#define RIGHT	1
#define TOP	2
#define BOT	3

#define LO	0
#define HI	1

#define HORIZONTAL	0
#define VERTICAL	1

#define	WIDTH	0
#define HEIGHT	1

#define M(x, y)  m[x + y * 4]


#define DEG2RAD (M_PI/180.0)
#define RAD2DEG (180.0/M_PI)

#define EQ_EPSILON 0.005
#define REAL_EQ(x,v) (((v) - EQ_EPSILON) < (x) && (x) < ((v) + EQ_EPSILON))      

#define square(a) ((a) * (a))

#define quat_sub_quick(a,b,c) {	(c)[X] = (a)[X] - (b)[X]; \
				(c)[Y] = (a)[Y] - (b)[Y]; \
				(c)[Z] = (a)[Z] - (b)[Z]; \
				(c)[W] = (a)[W] - (b)[W]; \
			}

#define quat_sub(a,b,c) {	(c)[X] = (a)[X] - (b)[X]; \
				(c)[Y] = (a)[Y] - (b)[Y]; \
				(c)[Z] = (a)[Z] - (b)[Z]; \
				(c)[W] = (a)[W] - (b)[W]; \
				quat_norm((c)); }

#define vec3_inv(a) {(a)[X] = -(a)[X]; (a)[Y] = -(a)[Y]; (a)[Z] = -(a)[Z];}
#define vec4_zero(a) {(a)[X] = 0; (a)[Y] = 0; (a)[Z] = 0; (a)[W] = 0;}
#define quat_zero vec4_zero
#define vec3_zero(a) {(a)[X] = 0; (a)[Y] = 0; (a)[Z] = 0;}
#define vec2_zero(a) {(a)[X] = 0; (a)[Y] = 0; }
#define vec2_set(a,b,c) {(a)[X] = b; (a)[Y] = c; }
#define vec3_set(a,b,c,d) {(a)[X] = b; (a)[Y] = c; (a)[Z] = d;}
#define vec4_set(a,b,c,d,e) {(a)[X] = b; (a)[Y] = c; (a)[Z] = d; (a)[W] = e;}
#define vec3_len(v) sqrt((v)[X]*(v)[X] + (v)[Y]*(v)[Y] + (v)[Z]*(v)[Z])
#define vec3_dot(x,y)   ((x)[X]*(y)[X] + (x)[Y]*(y)[Y] + (x)[Z]*(y)[Z])

#define vec2_sub(a,b,c)   {	(c)[X] = (a)[X] - (b)[X]; \
				(c)[Y] = (a)[Y] - (b)[Y];}
#define vec3_sub(a,b,c)   {	(c)[X] = (a)[X] - (b)[X]; \
				(c)[Y] = (a)[Y] - (b)[Y]; \
				(c)[Z] = (a)[Z] - (b)[Z];}
#define vec3_add(a,b,c) {c[X]=a[X]+b[X];c[Y]=a[Y]+b[Y];c[Z]=a[Z]+b[Z];}
#define vec3_div(b,a)  {b[X]/=a; b[Y]/=a; b[Z]/=a;}
#define vec3_scale(a,b) {a[X]*=b; a[Y]*=b; a[Z]*=b;}
#define vec3_mult(a,b,c) {c[X]=a[X]*b[X]; c[Y]=a[Y]*b[Y]; c[Z]=a[Z]*b[Z];}
#define vec3_eq(a,b) ((fabs(a[X]-b[X]) < EQ_EPSILON &&\
				fabs(a[Y]-b[Y]) < EQ_EPSILON &&\
				fabs(a[Z]-b[Z]) < EQ_EPSILON) ? 1 : 0)

#define vec3_cp(a,b) do { (b)[X]=(a)[X]; (b)[Y]=(a)[Y]; (b)[Z]=(a)[Z]; } while (0)
#define vec2_cp(a,b) { (b)[X]=(a)[X]; (b)[Y]=(a)[Y]; }
#define vec4_cp(a,b) do { (b)[X]=(a)[X]; (b)[Y]=(a)[Y]; (b)[Z]=(a)[Z]; (b)[W]=(a)[W]; } while (0)

#define MATH64

#ifdef MATH64
typedef double real;	/* 64 bit */
#else
typedef float real;	/* 32 bit */
#endif

/* all kinds of vector defs */
typedef real	vec2_t[2];
typedef real	vec3_t[3];
typedef real	vec4_t[4];

typedef int32_t int32_vec2_t[2];
typedef int32_t int32_vec3_t[3];
typedef int32_t int32_vec4_t[4];

typedef int16_t int16_vec2_t[2];
typedef int16_t int16_vec3_t[3];
typedef int16_t int16_vec4_t[4];

typedef int8_t int8_vec2_t[2];
typedef int8_t int8_vec3_t[3];
typedef int8_t int8_vec4_t[4];

typedef uint32_t uint32_vec2_t[2];
typedef uint32_t uint32_vec3_t[3];
typedef uint32_t uint32_vec4_t[4];

typedef uint16_t uint16_vec2_t[2];
typedef uint16_t uint16_vec3_t[3];
typedef uint16_t uint16_vec4_t[4];

typedef uint8_t uint8_vec2_t[2];
typedef uint8_t uint8_vec3_t[3];
typedef uint8_t uint8_vec4_t[4];

typedef vec4_t quat_t;

typedef real mat4x4_t[16];
typedef real mat3x3_t[9];

quat_t *quat_new(real x, real y, real z, real w);
void quat_add(quat_t q1, quat_t q2, quat_t dest);
void quat_id(quat_t q);
void quat_to_mat(quat_t q, mat4x4_t m);
void quat_to_mat_inv(quat_t q, mat4x4_t m);
void quat_to_mat_transpose(quat_t q, mat4x4_t m);
void quat_cp(quat_t src, quat_t dst);
void quat_set3(quat_t q, real x, real y, real z);
void quat_set4(quat_t q, real x, real y, real z, real angle);
void quat_mult(quat_t q1, quat_t q2);
void quat_to_vecs(quat_t q, vec3_t dir, vec3_t up, vec3_t right);
void quat_print(quat_t q);
void quat_norm(quat_t q);
void quat_inv(quat_t q);

vec3_t *vec3_new(real, real, real);
void vec3_destroy(vec3_t *v);
gboolean vec3_angle(vec3_t a, vec3_t b, real *angle);
void vec3_cross(vec3_t v1, vec3_t v2, vec3_t res);
void vec3_norm(vec3_t v);
void vec3_print(vec3_t v);
void vec3_transform(vec3_t *v, int32_t n, mat4x4_t m);
void transform_point(real out[4], real m[16], real in[4]);

/* matrix api */
mat4x4_t *mat4x4_new(void);
void mat4x4_zero(mat4x4_t m);
void mat4x4_scale(mat4x4_t m, real s);
void mat4x4_translate(mat4x4_t m, real x, real y, real z);
void mat4x4_id(mat4x4_t m);
void mat4x4_mult(mat4x4_t m1, mat4x4_t m2, mat4x4_t dest);
void mat4x4_copy(mat4x4_t src, mat4x4_t dest);


#endif
