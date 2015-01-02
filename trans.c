/*
 * trans.c: Transparency/refraction demo
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
#define HEIGHT 384

static light lights[] = {
  {{20.0, 12.5, 2.5}, {1.0, 1.0, 1.0}, {0.0, 0.0, 5.0}, {0.0, 5.0, 0.0}},
};
static int num_lights = 1;

static checkerboard checkerboards;

static void set_surface(surface *s, double r, double g, double b, double shine)
{
  /* Limit the diffuse component */
  s->diffuse.r = r * 0.2;
  s->diffuse.g = g * 0.2;
  s->diffuse.b = b * 0.2;

  s->specular.r
    = s->specular.g
    = s->specular.b
    = shine;

  s->reflective.r
    = s->reflective.g
    = s->reflective.b
    = 0.0; /* shine; */

  /* Make it so that you get the colour if traversing 5 units */
  s->transparency.r = pow(r, 1.0 / 5.0);
  s->transparency.g = pow(g, 1.0 / 5.0);
  s->transparency.b = pow(b, 1.0 / 5.0);

  s->refractive_index = 1;
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
  vector pos = { -6.0, 0.0, 8.0 };
  double radius = 1.3;

  for (i = 0; i < num_spheres; i++) {
    spheres[i].center = pos;
    spheres[i].radius = radius;

    colour c = colour_phase((double)i / num_spheres);
    set_surface(&spheres[i].props, c.r, c.g, c.b, 0.1);

    /* Create a range of refractive indices. */
    spheres[i].props.refractive_index = 1.0 - 0.333 * pow(0.5, 4 - i);

    spheres[i].fuzz_size = 0.0;
    spheres[i].fuzz_style = none;

    pos.x += 3.0;
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
 result->focal_depth    = 0.0;
 result->callback       = NULL;

 return result;
}

int main(void) {
  scene *sc = make_scene();
  png_render(sc, WIDTH, HEIGHT, "trans.png");
  return 0;
}
