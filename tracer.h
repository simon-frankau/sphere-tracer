/*
 * tracer.h: Ray-tracing core
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
  colour diffuse;
  colour specular;
  colour reflective;
} surface;

typedef enum {
  none,
  horizontal,
  vertical,
  both
} fuzz_mode;

typedef struct {
  surface props;
  vector center;
  double radius;
  double fuzz_size;
  fuzz_mode fuzz_style;
} sphere;

/* Quite specialist and hacky! */
typedef struct {
  vector normal;
  double distance;
  surface p1;
  surface p2;
} checkerboard;

typedef struct {
  vector loc;
  colour col;
  vector area1;
  vector area2;
} light;

typedef struct {
  sphere *spheres;
  int num_spheres;
  checkerboard *checkerboards;
  int num_checkerboards;
  light *lights;
  int num_lights;
  int num_samples;
  double blur_size;
  double antialias_size;
  double focal_depth;
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

/* Find a colour x in [0, 1] of the way around the colour wheel. */
colour colour_phase(double x);

/* Render a picture */
void render(scene const *scene_in, int width, int height, colour *image);

#endif // TRACER_H_INCLUDED
