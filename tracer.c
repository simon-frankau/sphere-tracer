/*
 * tracer.c: Ray-tracing core
 *
 * (C) Copyright Simon Frankau 1999-2014
 */

#include <assert.h>
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
                          double *dist, vector *normal,
			  vector *trans_w, vector *trans_dir,
			  double *trans_dist);

/* Texture a point */
static void texture(scene const *sc, surface const *surf,
                    vector w, vector n, vector dir,
		    vector trans_w, vector trans_dir, double trans_dist,
		    colour *colour);

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
  vector trans_w;
  vector trans_dir;
  double trans_dist;
  surface *intersecting = intersect(sc, from, dir, &dist, &normal,
				    &trans_w, &trans_dir, &trans_dist);
  if (!intersecting) {
    /* Missed! Send ray off to darkest infinity */
    return black;
  } else {
    /* Find point of intersection, w */
    vector w = dir;
    MULT(w, dist);
    ADD(w, from);

    colour result = premul;
    texture(sc, intersecting, w, normal, dir,
	    trans_w, trans_dir, trans_dist, &result);
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

/* Perform refraction */
/* NB: 'normal' should be pointing in the direction we pass through the
 * material.
 */
static vector refract(vector dir, vector normal, double index)
{
  /* Assumes vectors are normalised */
  double normal_component = DOT(dir, normal);

  vector perp_component = normal;
  MULT(perp_component, -normal_component);
  ADD(perp_component, dir);

  double sin_in = sqrt(DOT(perp_component, perp_component));
  double sin_out = index * sin_in;

  if (sin_out >= 1) {
    assert(0);
    return dir; /* TODO! */
  } else if (sin_out < EPSILON) {
    /* Perpendicular component is negligible, pass straight through. */
    return dir;
  }

  double cos_out = sqrt(1.0 - sin_out * sin_out);
  double tan_out = sin_out / cos_out;

  vector result = perp_component;
  NORMALISE(result);
  MULT(result, tan_out);
  ADD(result, normal);
  NORMALISE(result);
  return result;
}

static void sphere_transmit(sphere const *sp, vector w, vector dir,
			    vector *trans_w, vector *trans_dir,
			    double *trans_dist)
{
  double refractive_index = sp->props.refractive_index;

  /* Vector to centre of sphere */
  vector to_centre = sp->center;
  SUB(to_centre, w);

  /* Make into normal vector, and use to refract. */
  vector normal = to_centre;
  NORMALISE(normal);
  dir = refract(dir, normal, refractive_index);

  /* Calculate distance to pass through. */
  double dist = 2.0 * DOT(to_centre, dir);

  /* And create a vector that passes through. */
  vector through = dir;
  MULT(through, dist);

  /* And now we have the position on the other side */
  vector other_side = w;
  ADD(other_side, through);

  /* Refract on exit. */
  normal = sp->center;
  SUB(normal, other_side);
  NORMALISE(normal);
  MULT(normal, -1.0);
  dir = refract(dir, normal, 1.0 / refractive_index);

#ifdef DEBUG
  vector dist = other_side;
  SUB(dist, sp->center);
  assert((DOT(dist, dist) - sp->radius * sp->radius) < 1e-7);
#endif /* DEBUG */

  if (trans_w) {
    *trans_w = other_side;
  }
  if (trans_dir) {
    *trans_dir = dir;
  }
  if (trans_dist) {
    *trans_dist = dist;
  }
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

static void plane_transmit(checkerboard const *pl, vector w, vector dir,
			   vector *trans_w, vector *trans_dir,
			   double *trans_dist)
{
  if (trans_w) {
    *trans_w = w;
  }
  if (trans_dir) {
    *trans_dir = dir;
  }
  if (trans_dist) {
    *trans_dist = 0.0;
  }
}

/* Trace a unit ray, to find an intersection */
static surface *intersect(scene const *sc,
                          vector from,
                          vector direction,
                          double *dist,
			  vector *normal,
			  vector *trans_w,
			  vector *trans_dir,
			  double *trans_dist)
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
    sphere_transmit(nearest_sphere, w, direction,
		    trans_w, trans_dir, trans_dist);
    if (normal != NULL) {
      *normal = sphere_normal(nearest_sphere, w);
    }
    return &(nearest_sphere->props);
  }

  if (nearest_checkerboard != NULL) {
    plane_transmit(nearest_checkerboard, w, direction,
		   trans_w, trans_dir, trans_dist);
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
		    vector trans_w, /* Place where we leave the surface
				       after taking into account refraction */
		    vector trans_dir, /* Direction after transmission */
		    double trans_dist, /* Distance to other side */
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
    intersect(sc, light_loc, tmp2, &dist, NULL, NULL, NULL, NULL);
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

  colour in = *col;

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

  /* Transparency */
  in.r *= pow(surf->transparency.r, trans_dist);
  in.g *= pow(surf->transparency.g, trans_dist);
  in.b *= pow(surf->transparency.b, trans_dist);

  if (in.r + in.g + in.b > REFLECTSTOP) {
    colour trans = trace(sc, trans_w, trans_dir, in);
    col->r += trans.r;
    col->g += trans.g;
    col->b += trans.b;
  }
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
