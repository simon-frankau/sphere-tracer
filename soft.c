/*
 * soft.c: Soft shadow demo
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
  {{10.0, 7.5, 0.5}, {0.0, 0.0, 0.0},
   {0.0, 0.0, 5.0}, {0.0, 5.0, 0.0}},
  {{-15.0, 10.0, -10.0}, {0.15, 0.15, 0.15},
   {0.0, 0.0, 30.0}, {30.0, 0.0, 0.0}}
};
static int num_lights = 2;

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

  s->transparency.r
    = s->transparency.g
    = s->transparency.b
    = 0.0;
}

/* Make a spherical shell filled with spheres. */
static scene *make_scene()
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

  spheres->fuzz_size = 0.0;
  spheres->fuzz_style = none;

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
  scene *sc = make_scene();
  png_render(sc, WIDTH, HEIGHT, "soft.png");
  return 0;
}
