/* 
 * tracer.c: Ray-tracing core
 *
 * (C) Copyright Simon Frankau 1999-2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "tracer.h"

/* ------------------------------------------------------------------
 * Macros
 */

/* #define DEBUG */

#define REFLECTSTOP 0.1

#ifndef INFINITY
#define INFINITY (1.0 / 0.0)
#endif

#define SHADE(c, i, k, p) { c.r += i.r * k.r * p; \
                           c.g += i.g * k.g * p; \
                           c.b += i.b * k.b * p; }

/* ------------------------------------------------------------
 * Global variables
 */

/* These 2 are for convenience. */
colour white = {1.0, 1.0, 1.0};
colour black = {0.0, 0.0, 0.0};

/* ------------------------------------------------------------
 * Function prototypes.
 */

/* Trace a unit ray, to find an intersection */
static sphere *intersect(scene const *sc, vector from, vector direction, double *dist, sphere const *ignore);

/* Texture a point */
static void texture(scene const *sc, sphere *s, vector w, vector dir, colour *colour);

/* ------------------------------------------------------------
 * Functions.
 */

/* Find the colour for a given ray. Rather than post-multiply for
 * absorbtion further down the line, we pre-multiply, allowing us
 * to cut off at an appropriate point.
 */
static colour trace(scene const *sc, sphere const *skip,
	            vector from, vector dir, colour premul)
{
  double dist;
  sphere *intersecting = intersect(sc, from, dir, &dist, skip);
  if (!intersecting) {
    /* Missed! Send ray off to darkest infinity */
    return black;
  } else {
     /* Find point of intersection, w */
    vector w = dir;
    MULT(w, dist);
    ADD(w, from);

    colour result = premul;
    texture(sc, intersecting, w, dir, &result);
    return result;
  }
}

static double intersect_sphere(sphere const *sp, vector from, vector dir)
{
   vector v = sp->center;
   SUB(v, from);
   double b = DOT(dir, v);
   double d = b*b - DOT(v,v) + sp->radius * sp->radius;
   if (d > 0) {
     /* Intersection! */
     double s = b - sqrt(d);
     if (s > 0) {
       return s;
     }
   }
   return INFINITY;
}

/* Trace a unit ray, to find an intersection */
static sphere *intersect(scene const *sc,
			 vector from,
			 vector direction,
			 double *dist,
			 sphere const *ignore)
{
 double nearest_dist = HUGE_VAL;
 sphere *nearest_sphere = NULL;
 int i;

 for (i = 0; i < sc->num_spheres; i++) {
   if (sc->spheres + i == ignore)
     continue;
   double this_dist = intersect_sphere(sc->spheres + i, from, direction);
   if (this_dist < nearest_dist) {
        nearest_dist = this_dist;
        nearest_sphere = sc->spheres + i;
   }
 }

 if (dist)
   *dist = nearest_dist;
 return nearest_sphere;
}

/* Texture a point */
static void texture(scene const *sc,
		    sphere *s,
		    vector w, /* Point of intersection */
		    vector dir,
		    colour *col)
{
 /* Texture by the nearest thing we hit. */
 vector w, n, l, r;
 double tmp;
 vector tmp2;
 double diffuse, specular;
 int i;

 colour c;

 /* And the unit normal at that point, n */
 n = w;
 SUB(n, s->center);
 NORMALISE(n);

 /* And the normalised reflection vector, r */
 tmp = DOT(n, dir);
 tmp2 = n;
 MULT(tmp2, 2.0*tmp);
 r = dir;
 SUB(r, tmp2);

 /* FIXME: Ambient lighting? */
 c.r = c.g = c.b = 0.0;

 /* Diffuse and specular lighting. */
 for (i = 0; i < sc->num_lights; i++) {
   /* Normalised vector pointing at the light source. */
   l = sc->lights[i].loc;
   SUB(l, w);
   NORMALISE(l);

   /* Dot product of the number and vector to the light.
    * Used to calculate the amount of diffuse light received.
    * If negative, we are facing away from the light (no light).
    */
   if ((diffuse = DOT(n, l)) <= 0.0)
     continue;

   /* Light is on right side - check we can see it. */
   tmp2 = l;
   MULT(tmp2, -1.0);
   if (s != intersect(sc, sc->lights[i].loc, tmp2, NULL, NULL))
     continue;

   /* Diffuse colour */
   SHADE(c, sc->lights[i].col, s->diffuse, diffuse);

   /* Specular */
   specular = DOT(r, l);
   if (specular >= 0.0) {
     specular = pow(specular, 10);
     SHADE(c, sc->lights[i].col, s->specular, specular);
   }
 }

 /* Reflection */
 c.r *= col->r;
 c.g *= col->g;
 c.b *= col->b;

 col->r *= s->reflective.r;
 col->g *= s->reflective.g;
 col->b *= s->reflective.b;

 if (col->r + col->g + col->b > REFLECTSTOP) {
   /* Enough light to make it worth tracing further */
   *col = trace(sc, s, w, r, *col);
 } else {
   /* Not enough light to bother tracing further. */
   *col = black;
 }

 /* Add reflection to light colour */
 col->r += c.r;
 col->g += c.g;
 col->b += c.b;
}

/* Render a picture */
void render(scene const *sc, int width, int height, colour *image)
{
 vector origin = {0.0, 0.0, 0.0};
 vector ray;
 int x, y;
 for (y = 0; y < height; y++)
   for (x = 0; x < width; x++)
     image[y*width+x] = white;

 printf("Ray Tracing (%d):\n", height);
 for (y = 0; y < height; y++) {
   printf("%d\n", y);
   for (x = 0; x < width; x++) {
     ray.x = x - width/2;
     ray.y = y - height/2;
     ray.z = width/2;
     NORMALISE(ray);
     image[y * width + x] = trace(sc, NULL, origin, ray, white);
   }
 }
}
