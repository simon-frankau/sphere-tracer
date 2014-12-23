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
#ifndef EPSILON
#define EPSILON 1.0e-7
#endif

#define SHADE(c, i, k, p) { \
    c.r += i.r * k.r * p; \
    c.g += i.g * k.g * p; \
    c.b += i.b * k.b * p; \
  }

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
static surface *intersect(scene const *sc, vector from, vector direction,
                          double *dist, vector *normal);

/* Texture a point */
static void texture(scene const *sc, surface const *surf,
                    vector w, vector n, vector dir, colour *colour);

/* ------------------------------------------------------------
 * Functions.
 */

/* Find a colour x in [0, 1] of the way around the colour wheel. */
colour colour_phase(double x)
{
  /* We construct a wheel from two unit vectors perpendicular to the
   * (1, 1, 1) vector (and each other) in colour-space.
   */

  colour c1;
  c1.r = sqrt(1.0 / 2.0); c1.g = -c1.r; c1.b = 0.0;
  colour c2;
  c2.r = sqrt(1.0 / 6.0); c2.g = c2.r; c2.b = -2.0 * c2.r;

  double phase = x * 2 * M_PI;
  double colour_cos = cos(phase);
  double colour_sin = sin(phase);

  /* Construct point and normalise back to unit cube. */
  colour c;
  c.r = (colour_cos * c1.r + colour_sin * c2.r + 1.0) / 2.0;
  c.g = (colour_cos * c1.g + colour_sin * c2.g + 1.0) / 2.0;
  c.b = (colour_cos * c1.b + colour_sin * c2.b + 1.0) / 2.0;

  return c;
}

/* Find the colour for a given ray. Rather than post-multiply for
 * absorbtion further down the line, we pre-multiply, allowing us
 * to cut off at an appropriate point.
 */
static colour trace(scene const *sc, vector from, vector dir, colour premul)
{
  double dist;
  vector normal;
  surface *intersecting = intersect(sc, from, dir, &dist, &normal);
  if (!intersecting) {
    /* Missed! Send ray off to darkest infinity */
    return black;
  } else {
    /* Find point of intersection, w */
    vector w = dir;
    MULT(w, dist);
    ADD(w, from);

    colour result = premul;
    texture(sc, intersecting, w, normal, dir, &result);
    return result;
  }
}

static double sphere_intersect(sphere const *sp, vector from, vector dir)
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

/* Creates a random vector within the unit sphere */
static vector random_vector()
{
  vector v;
  do {
    v.x = 2.0 * ((double)rand() / RAND_MAX) - 1.0;
    v.y = 2.0 * ((double)rand() / RAND_MAX) - 1.0;
    v.z = 2.0 * ((double)rand() / RAND_MAX) - 1.0;
  } while (DOT(v, v) > 1);
  return v;
}

static vector sphere_normal(sphere const *sp, vector w)
{
  vector n = w;
  SUB(n, sp->center);
  NORMALISE(n);

  if (sp->fuzz_size > 0.0 && sp->fuzz_style != none) {
    vector r = random_vector();
    switch (sp->fuzz_style) {
    case horizontal:
      r.y = 0.0;
      break;
    case vertical:
      r.x = r.z = 0.0;
      break;
    case both:
      /* Already ok */
      break;
    case none:
      puts("Shouldn't happen!\n");
      exit(1);
      break;
    }
    MULT(r, sp->fuzz_size);
    ADD(n, r);
    NORMALISE(n);
  }
  return n;
}

static double plane_intersect(checkerboard const *pl, vector from, vector dir)
{
  double from_norm = DOT(from, pl->normal) - pl->distance;

  if (from_norm < 0) {
    return INFINITY;
  }

  double dir_norm = DOT(dir, pl->normal);

  return - from_norm / dir_norm;
}

static vector plane_normal(checkerboard const *pl, vector w)
{
  return pl->normal;
}

/* Trace a unit ray, to find an intersection */
static surface *intersect(scene const *sc,
                          vector from,
                          vector direction,
                          double *dist,
			  vector *normal)
{
  double nearest_dist = INFINITY;
  sphere *nearest_sphere = NULL;
  checkerboard *nearest_checkerboard = NULL;
  int i;

  for (i = 0; i < sc->num_spheres; i++) {
    double this_dist = sphere_intersect(sc->spheres + i, from, direction);
    if (EPSILON < this_dist && this_dist < nearest_dist) {
      nearest_dist = this_dist;
      nearest_sphere = sc->spheres + i;
    }
  }

  for (i = 0; i < sc->num_checkerboards; i++) {
    double this_dist = plane_intersect(sc->checkerboards + i, from, direction);
    if (EPSILON < this_dist && this_dist < nearest_dist) {
      nearest_dist = this_dist;
      nearest_sphere = NULL;
      nearest_checkerboard = sc->checkerboards + i;
    }
  }

  vector w = direction;
  MULT(w, nearest_dist);
  ADD(w, from);

  if (dist) {
    *dist = nearest_dist;
  }

  if (nearest_sphere != NULL) {
    if (normal != NULL) {
      *normal = sphere_normal(nearest_sphere, w);
    }
    return &(nearest_sphere->props);
  }

  if (nearest_checkerboard != NULL) {
    if (normal != NULL) {
      *normal = plane_normal(nearest_checkerboard, w);
    }
    /* Cheesy checkerboard hardwired... */
    int parity = (lrint(w.x) + lrint(w.z)) % 2;

    surface *surface = parity ?
           &(nearest_checkerboard->p1) :
           &(nearest_checkerboard->p2);

    return surface;
  }

  return NULL;
}

/* Texture a point */
static void texture(scene const *sc,
                    surface const *surf,
                    vector w, /* Point of intersection */
                    vector n, /* Surface normal */
                    vector dir,
                    colour *col)
{
  /* Texture by the nearest thing we hit. */
  vector l, r;
  double tmp;
  vector tmp2;
  double diffuse, specular;
  int i;

  colour c;

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
    vector light_loc = sc->lights[i].loc;
    colour light_col = sc->lights[i].col;

    double y_rand = ((double)rand())/RAND_MAX;
    double z_rand = ((double)rand())/RAND_MAX;

    vector lr1 = sc->lights[i].area1;
    MULT(lr1, z_rand);
    vector lr2 = sc->lights[i].area2;
    MULT(lr2, y_rand);
    ADD(light_loc, lr1);
    ADD(light_loc, lr2);

    if (light_col.r == 0.0 && light_col.g == 0.0 && light_col.b == 0.0) {
      light_col = colour_phase(z_rand);
    }

    /* Normalised vector pointing at the light source. */
    l = light_loc;
    SUB(l, w);
    NORMALISE(l);

    /* Dot product of the normal and vector to the light.
     * Used to calculate the amount of diffuse light received.
     * If negative, we are facing away from the light (no light).
     */
    if ((diffuse = DOT(n, l)) <= 0.0)
      continue;

    /* Light is on right side - check we can see it. */
    tmp2 = l;
    MULT(tmp2, -1.0);
    double dist;
    intersect(sc, light_loc, tmp2, &dist, NULL);
    vector to_l = w;
    SUB(to_l, light_loc);
    if (dist*dist + EPSILON < DOT(to_l, to_l)) {
      continue;
    }

    /* Diffuse colour */
    SHADE(c, light_col, surf->diffuse, diffuse);

    /* Specular */
    specular = DOT(r, l);
    if (specular >= 0.0) {
      specular = pow(specular, 10);
      SHADE(c, light_col, surf->specular, specular);
    }
  }

  /* Reflection */
  c.r *= col->r;
  c.g *= col->g;
  c.b *= col->b;

  col->r *= surf->reflective.r;
  col->g *= surf->reflective.g;
  col->b *= surf->reflective.b;

  if (col->r + col->g + col->b > REFLECTSTOP) {
    /* Enough light to make it worth tracing further */
    *col = trace(sc, w, r, *col);
  } else {
    /* Not enough light to bother tracing further. */
    *col = black;
  }

  /* Add reflection to light colour */
  col->r += c.r;
  col->g += c.g;
  col->b += c.b;
}

/* Create a normally distributed lump of noise in the X-Z plane */
static vector noise_xy(double std_var) {
  /* Box-Muller */
  double u1 = ((double)rand() + 1.0) / ((double)RAND_MAX + 1.0);
  double u2 = ((double)rand() + 1.0) / ((double)RAND_MAX + 1.0);
  double r  = sqrt(-2.0 * log(u1));
  double th = 2 * M_PI * u2;
  double z0 = r * cos(th);
  double z1 = r * sin(th);
  vector v;
  v.x = std_var * z0;
  v.y = std_var * z1;
  v.z = 0.0;
  return v;
}

/* Render a picture */
void render(scene *sc, int width, int height, colour *image)
{
  /* Make sure the noise we apply is reproduceable. */
  srand(42);

  vector origin;
  vector ray;
  int x, y;
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      image[y*width+x] = white;

  printf("Ray Tracing (%d):\n", height);
  for (y = 0; y < height; y++) {
    printf("%d\n", y);
    for (x = 0; x < width; x++) {
      colour c = { 0.0, 0.0, 0.0 };
      int i = 0;
      for (i = 0; i < sc->num_samples; i++) {
	if (sc->callback != NULL) {
	  /* Perform callback allowing the scene to be updated for e.g.
	   * motion blur. Downside is that the scene is no longer 'const'
	   * and we can't share the structure if we ever wanted to
	   * parallelise.
	   */
	  sc->callback(sc);
	}

	ray.x = x - width/2;
	ray.y = height/2 - y;
	ray.z = width/2;

	/* Add noise to the ray. */
	vector noise = noise_xy(sc->blur_size);
	ADD(ray, noise);

	/* And remove the noise at the focal distance. */
	origin.x = - sc->focal_depth * noise.x / ray.z;
	origin.y = - sc->focal_depth * noise.y / ray.z;
	origin.z = 0.0;

	/* And more noise to do antialiasing. */
	vector aa_noise = noise_xy(sc->antialias_size);
	ADD(ray, aa_noise);

	NORMALISE(ray);
	colour c2 = trace(sc, origin, ray, white);
	c.r += c2.r; c.g += c2.g; c.b += c2.b;
      }
      c.r /= sc->num_samples; c.g /= sc->num_samples; c.b /= sc->num_samples;
      image[y * width + x] = c;
    }
  }
}
