/*
 * fuzzy.c: Blurry reflections demo
 *
 * (C) Copyright Simon Frankau 1999-2014
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "tracer.h"
#include "png_render.h"

#define WIDTH 256
#define HEIGHT 256

static light lights[] = {
  {{10.0, 7.5, 0.5}, {1.0, 1.0, 1.0},
   {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
  {{-15.0, 10.0, -10.0}, {0.15, 0.15, 0.15},
   {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}
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

/* Make a spherical shell filled with spheres. */
static scene *make_scene(double fuzz_size, fuzz_mode style)
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
  int num_spheres = 1;
  sphere *spheres = (sphere *)malloc(num_spheres * sizeof(sphere));

  vector pos = { 0.0, 0.0, 5.0 };

  spheres->center = pos;
  spheres->radius = pos.y - checkerboards.distance;

  set_surface(&(spheres->props), 0.7, 0.7, 0.7, 0.5);

  spheres->fuzz_size = fuzz_size;
  spheres->fuzz_style = style;

  scene *result = (scene *)malloc(sizeof(scene));
  result->spheres = spheres;
  result->num_spheres = num_spheres;
  result->checkerboards = &checkerboards;
  result->num_checkerboards = num_checkerboards;
  result->lights = lights;
  result->num_lights = num_lights;

  result->num_samples    = 1000;
  result->blur_size      = 0.0;
  result->antialias_size = 0.5;
  result->focal_depth    = 0.0;
  result->callback       = NULL;

  return result;
}

int main(void) {
  scene scenes[5];
  scenes[0] = *make_scene(0.00, none);
  scenes[1] = *make_scene(0.05, both);
  scenes[2] = *make_scene(0.15, both);
  scenes[3] = *make_scene(0.15, horizontal);
  scenes[4] = *make_scene(0.15, vertical);
  png_render_ex(scenes, sizeof(scenes)/sizeof(scenes[0]), 3,
		WIDTH, HEIGHT, "fuzzy.png");
  return 0;
}
