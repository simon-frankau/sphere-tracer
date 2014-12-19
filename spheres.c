/*
 * Sphere scene generator/renderer
 *
 * (C) Copyright Simon Frankau 1999-2014
 */

#include <math.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>

#include "tracer.h"

#define WIDTH 1280
#define HEIGHT 1024

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
   spheres[i].diffuse.r = 1.0*rand()/RAND_MAX;
   spheres[i].diffuse.g = 1.0*rand()/RAND_MAX;
   spheres[i].diffuse.b = 1.0*rand()/RAND_MAX;

   spheres[i].specular.r
     = spheres[i].specular.g
     = spheres[i].specular.b
     = 0.5;

   spheres[i].reflective.r
     = spheres[i].reflective.g
     = spheres[i].reflective.b
     = 0.5;

   printf("%d\n", i+1);
 }

 scene *result = (scene *)malloc(sizeof(scene));
 result->spheres = spheres;
 result->num_spheres = num_spheres;
 return result;
}


int main(void) {
 colour *image;
 png_byte *image2;

#ifdef DEBUG
 srand(0);
#else
 srand(time(NULL));
#endif

 image = (colour *)malloc(WIDTH*HEIGHT*sizeof(colour));
 image2 = (png_bytep)malloc(WIDTH*HEIGHT*3);
 scene *sc = make_scene(5, 10, 1000);
 render(sc, WIDTH, HEIGHT, image);
 convert_image(WIDTH, HEIGHT, image, image2);
 write_image(WIDTH, HEIGHT, image2, "test.png");
 return 0;
}
