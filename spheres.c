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
light lights[] = {
 {{2.0, 1.0, 0.0}, {1.0, 0.3, 0.0}},
 {{-3.0, 1.0, 0.0}, {0.0, 1.0, 0.3}},
 {{0.0, -4.0, 0.0}, {0.3, 0.0, 1.0}}
};
int no_lights = 3;
*/

light lights[] = {
 {{2.0, 1.0, 0.0}, {1.0, 1.0, 1.0}},
};
int num_lights = 1;

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
   /* Random colour, with some white specular part. */
   spheres[i].props.diffuse.r = 1.0*rand()/RAND_MAX;
   spheres[i].props.diffuse.g = 1.0*rand()/RAND_MAX;
   spheres[i].props.diffuse.b = 1.0*rand()/RAND_MAX;

   spheres[i].props.specular.r
     = spheres[i].props.specular.g
     = spheres[i].props.specular.b
     = 0.5;

   spheres[i].props.reflective.r
     = spheres[i].props.reflective.g
     = spheres[i].props.reflective.b
     = 0.5;

   printf("%d\n", i+1);
 }

 scene *result = (scene *)malloc(sizeof(scene));
 result->spheres = spheres;
 result->num_spheres = num_spheres;
 result->lights = lights;
 result->num_lights = num_lights;
 return result;
}


int main(void) {
#ifdef DEBUG
 srand(0);
#else
 srand(time(NULL));
#endif

 scene *sc = make_scene(5, 10, 1000);
 png_render(sc, WIDTH, HEIGHT, "test.png");
 return 0;
}
