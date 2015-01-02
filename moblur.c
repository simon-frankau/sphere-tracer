/*
 * moblur.c: Motion blur demo
 *
 * (C) Copyright Simon Frankau 1999-2014
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "tracer.h"
#include "png_render.h"

#define WIDTH 1024
#define HEIGHT 512

static light lights[] = {
  {{10.0, 10.0, 3.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
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

  s->transparency.r
    = s->transparency.g
    = s->transparency.b
    = 0.0;

  s->refractive_index = 1.0;
}

static void do_motion_blur(scene *sc)
{
  assert(sc->num_spheres == 5);

  double time = (double)rand() / RAND_MAX;

  /* Constants duplicated from make_scene. Ick. */
  double z = 3.0;

  for (int i = 0; i < sc->num_spheres; ++i) {
    sc->spheres[i].center.z = z + time;
    z += 2.0;
  }
}

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
  int num_spheres = 5;
  sphere *spheres = (sphere *)malloc(num_spheres * sizeof(sphere));

  int i;
  vector pos = { -2.5, -0.5, 3.0 };
  double radius = pos.y - checkerboards.distance;

  for (i = 0; i < num_spheres; i++) {
    spheres[i].center = pos;
    spheres[i].radius = radius;

    colour c = colour_phase((double)i / num_spheres);
    set_surface(&spheres[i].props, c.r, c.g, c.b, 0.5);

    spheres[i].fuzz_size = 0.0;
    spheres[i].fuzz_style = none;

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

 result->num_samples    = 1000;
 result->blur_size      = 0.0;
 result->antialias_size = 0.5;
 result->focal_depth    = 5.0;
 result->callback       = do_motion_blur;

 return result;
}

int main(void) {
  scene *sc = make_scene();
  png_render(sc, WIDTH, HEIGHT, "moblur.png");
  return 0;
}
