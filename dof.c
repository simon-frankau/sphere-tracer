/*
 * dof.c: Depth of field demo
 *
 * (C) Copyright Simon Frankau 1999-2014
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "tracer.h"
#include "png_render.h"

#define WIDTH 512
#define HEIGHT 512

static light lights[] = {
 {{10.0, 10.0, 3.0}, {1.0, 1.0, 1.0}},
};
static int num_lights = 1;

static checkerboard checkerboards;

static void set_surface(surface *s, double r, double g, double b, double shine)
{
  s->diffuse.r = r;
  s->diffuse.g = g;
  s->diffuse.b = b;

  s->specular.r
    = s->specular.g
    = s->specular.b
    = shine;

  s->reflective.r
    = s->reflective.g
    = s->reflective.b
    = shine;
}

/* Find a colour n/m th of the way around the colour wheel. */
static colour colour_phase(int n, int m) {
  /* We construct a wheel from two unit vectors perpendicular to the
   * (1, 1, 1) vector (and each other) in colour-space.
   */

  colour c1;
  c1.r = sqrt(1.0 / 2.0); c1.g = -c1.r; c1.b = 0.0;
  colour c2;
  c2.r = sqrt(1.0 / 6.0); c2.g = c2.r; c2.b = -2.0 * c2.r;

  double phase = (n * 2 * M_PI) / m;
  double colour_cos = cos(phase);
  double colour_sin = sin(phase);

  /* Construct point and normalise back to unit cube. */
  colour c;
  c.r = (colour_cos * c1.r + colour_sin * c2.r + 1.0) / 2.0;
  c.g = (colour_cos * c1.g + colour_sin * c2.g + 1.0) / 2.0;
  c.b = (colour_cos * c1.b + colour_sin * c2.b + 1.0) / 2.0;

  return c;
}

/* Make a spherical shell filled with spheres. */
static scene *make_scene(double min, double max, int count)
{
  /* Place the checkerboard */
  checkerboards.normal.x = 0.0;
  checkerboards.normal.y = 1.0;
  checkerboards.normal.z = 0.0;
  checkerboards.distance = -2.0;
  set_surface(&checkerboards.p1, 0.9, 0.9, 0.9, 0.0);
  set_surface(&checkerboards.p2, 0.1, 0.1, 0.1, 0.0);
  int num_checkerboards = 1;

  /* Place a set of spheres. */
  int num_spheres = 5;
  sphere *spheres = (sphere *)malloc(num_spheres * sizeof(sphere));

  int i;
  vector pos = { -2.5, -0.5, 3.0 };
  double radius = pos.y - checkerboards.distance;

  for (i = 0; i < num_spheres; i++) {
    spheres[i].center = pos;
    spheres[i].radius = radius;

    colour c = colour_phase(i, num_spheres);
    set_surface(&spheres[i].props, c.r, c.g, c.b, 0.5);

    pos.x += 2.5;
    pos.z += 2.0;
  }

 scene *result = (scene *)malloc(sizeof(scene));
 result->spheres = spheres;
 result->num_spheres = num_spheres;
 result->checkerboards = &checkerboards;
 result->num_checkerboards = num_checkerboards;
 result->lights = lights;
 result->num_lights = num_lights;

 result->num_samples    = 100;
 result->blur_size      = 3.0;
 result->antialias_size = 0.5;
 result->focal_depth    = 5.0;

 return result;
}

int main(void) {
#ifdef DEBUG
 srand(0);
#else
 srand(time(NULL));
#endif

 scene *sc = make_scene(5, 10, 1000);
 png_render(sc, WIDTH, HEIGHT, "dof.png");
 return 0;
}
