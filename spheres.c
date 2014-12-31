/*
 * spheres.c: Sphere scene generator/renderer
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

/*
static light lights[] = {
 {{2.0, 1.0, 0.0}, {1.0, 0.3, 0.0}, 0.0},
 {{-3.0, 1.0, 0.0}, {0.0, 1.0, 0.3}, 0.0},
 {{0.0, -4.0, 0.0}, {0.3, 0.0, 1.0}, 0.0}
};
static int no_lights = 3;
*/

static light lights[] = {
  {{2.0, 1.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
};
static int num_lights = 1;

static checkerboard checkerboards;

static void set_surface(surface *s)
{
  /* Random colour, with some white specular part. */
   s->diffuse.r = 1.0*rand()/RAND_MAX;
   s->diffuse.g = 1.0*rand()/RAND_MAX;
   s->diffuse.b = 1.0*rand()/RAND_MAX;

   s->specular.r
     = s->specular.g
     = s->specular.b
     = 0.5;

   s->reflective.r
     = s->reflective.g
     = s->reflective.b
     = 0.5;

   s->transparency.r
     = s->transparency.g
     = s->transparency.b
     = 0.0;

   s->refractive_index = 1.0;
}

/* Make a spherical shell filled with spheres. */
static scene *make_scene(double min, double max, int count)
{
 double radius, theta, phi, cosphi;
 double dist;
 int i, j;

 sphere *spheres = (sphere *)malloc(count * sizeof(sphere));
 if (!spheres) {
   printf("Couldn't allocated spheres storage.\n");
   exit(1);
 }
 int num_spheres = count;

 printf("Making spheres (%d):\n", count);

 for (i = 0; i < count; i++) {
   do {
     radius = (max-min)*rand()/RAND_MAX + min;
     theta = 2.0*M_PI*rand()/RAND_MAX;
     phi = 2.0*M_PI*rand()/RAND_MAX - M_PI;

     cosphi = cos(phi);
     spheres[i].center.x = radius*cos(theta)*cosphi;
     spheres[i].center.y = radius*sin(theta)*cosphi;
     spheres[i].center.z = radius*sin(phi);

     spheres[i].radius = max-radius;
     if (radius-min < spheres[i].radius)
        spheres[i].radius = radius-min;

     /* Check the radius, etc. */
     j = 0;
     while (j < i) {
        vector v;
        v = spheres[i].center;
        SUB(v,spheres[j].center);
        dist = sqrt(DOT(v, v)) - spheres[j].radius;
        if (dist < spheres[i].radius)
          spheres[i].radius = dist;
        if (dist <= 0.0)
          break;
        j++;
     }
   } while (spheres[i].radius <= 0.0);
 
   set_surface(&(spheres[i].props));

   spheres[i].fuzz_size = 0.0;
   spheres[i].fuzz_style = none;

   printf("%d\n", i+1);
 }

 checkerboards.normal.x = 0.0;
 checkerboards.normal.y = 1.0;
 checkerboards.normal.z = 0.0;
 checkerboards.distance = -2.0;
 set_surface(&checkerboards.p1);
 set_surface(&checkerboards.p2);
 int num_checkerboards = 1;

 scene *result = (scene *)malloc(sizeof(scene));
 result->spheres = spheres;
 result->num_spheres = num_spheres;
 result->checkerboards = &checkerboards;
 result->num_checkerboards = num_checkerboards;
 result->lights = lights;
 result->num_lights = num_lights;
 result->num_samples = 1;
 result->blur_size = 0.0;
 result->antialias_size = 0.0;
 result->focal_depth = 0.0;
 result->callback = NULL;

 return result;
}

int main(void) {
#ifdef DEBUG
 srand(0);
#else
 srand(time(NULL));
#endif

 scene *sc = make_scene(5, 10, 1000);
 png_render(sc, WIDTH, HEIGHT, "spheres.png");
 return 0;
}
