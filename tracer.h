/* 
 * Ray-tracing core
 *
 * (C) Copyright Simon Frankau 1999-2014
 */

#ifndef TRACER_H_INCLUDED
#define TRACER_H_INCLUDED

/* ------------------------------------------------------------------
 * Data types
 */

typedef struct {
 double x, y, z;
} vector;

typedef struct {
 double r, g, b;
} colour;

typedef struct {
 vector center;
 double radius;
 colour diffuse;
 colour specular;
 colour reflective;
} sphere;

typedef struct {
 vector loc;
 colour col;
} light;

typedef struct {
 sphere *spheres;
 int num_spheres;
} scene;
  

/* ------------------------------------------------------------------
 * Macros
 */

#define NORMALISE(v) { double _len = sqrt(v.x*v.x + v.y*v.y + v.z*v.z); \
                      v.x /= _len; v.y /= _len; v.z /= _len; }

#define DOT(v1, v2) (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z)

#define ADD(v1, v2) {v1.x += v2.x; v1.y += v2.y; v1.z += v2.z;}

#define SUB(v1, v2) {v1.x -= v2.x; v1.y -= v2.y; v1.z -= v2.z;}

#define MULT(v, m) {v.x *= m; v.y *= m; v.z *= m;}

/* ------------------------------------------------------------------
 * Exported functions
 */

/* Render a picture */
void render(scene const *scene_in, int width, int height, colour *image);

/* Convert a colour array into an image suitable for saving. */
void convert_image(int width, int height, colour const *im_in,
                   png_bytep im_out);

/* Write an image from an array of R, G and B png_bytes. */
void write_image(int width, int height, png_bytep image,
                 char const *filename);

#endif // TRACER_H_INCLUDED
