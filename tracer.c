/* 
 * Ray-tracing core
 *
 * (C) Copyright Simon Frankau 1999-2014
 */

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <png.h>

#include "tracer.h"

/* ------------------------------------------------------------------
 * Macros
 */

/* #define DEBUG */

#define REFLECTSTOP 0.1

#define SHADE(c, i, k, p) { c.r += i.r * k.r * p; \
                           c.g += i.g * k.g * p; \
                           c.b += i.b * k.b * p; }

/* ------------------------------------------------------------
 * Global variables
 */

sphere *spheres;
int no_spheres;

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
int no_lights = 1;

/* These 2 are for convenience. */
colour white = {1.0, 1.0, 1.0};
colour black = {0.0, 0.0, 0.0};

/* ------------------------------------------------------------
 * Function prototypes.
 */

/* Trace a unit ray, to find an intersection */
static sphere *trace(vector from, vector direction, double *dist, int ignore);

/* Texture a point */
static void texture(sphere *s, vector from, vector dir, double dist, colour *colour);

/* ------------------------------------------------------------
 * Functions.
 */

/* Trace a unit ray, to find an intersection */
static sphere *trace(vector from, vector direction, double *dist, int ignore)
{
 double nearest_dist = HUGE_VAL;
 sphere *nearest_sphere = NULL;
 int i;

 /* Intermediate values */
 vector v;
 double b, d, s;

 for (i = 0; i < no_spheres; i++) {
   if (i == ignore)
     continue;
   v = spheres[i].center;
   SUB(v, from);
   b = DOT(direction, v);
   d = b*b - DOT(v,v) + spheres[i].radius*spheres[i].radius;
   if (d > 0) {
     /* Intersection! */
     s = b - sqrt(d);
     if ((s > 0) && (s < nearest_dist)) {
        /* We've hit it going forwards... */
        nearest_dist = s;
        nearest_sphere = spheres+i;
     }
   }
 }

 if (dist)
   *dist = nearest_dist;
 return nearest_sphere;
}


/* Texture a point */
static void texture(sphere *s, vector from, vector dir, double dist, colour *col)
{
 /* Texture by the nearest thing we hit. */
 vector w, n, l, r;
 double tmp;
 vector tmp2;
 double diffuse, specular;
 int i;

 colour c;

 /* Find point of intersection, w */
 w = dir;
 MULT(w, dist);
 ADD(w, from);

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
 for (i = 0; i < no_lights; i++) {
   /* Normalised vector pointing at the light source. */
   l = lights[i].loc;
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
   if (s != trace(lights[i].loc, tmp2, NULL, -1))
     continue;

   /* Diffuse colour */
   SHADE(c, lights[i].col, s->diffuse, diffuse);

   /* Specular */
   specular = DOT(r, l);
   if (specular >= 0.0) {
     specular = pow(specular, 10);
     SHADE(c, lights[i].col, s->specular, specular);
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
   sphere *intersect;
   double dist;

   intersect = trace(w, r, &dist, s - spheres);
   if (!intersect) {
     /* Missed! Send ray off to darkest infinity */
     *col = black;
   } else {
     texture(intersect, w, r, dist, col);
   }
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
void render(int width, int height, colour *image)
{
 vector origin = {0.0, 0.0, 0.0};
 vector ray;
 int x, y;
 sphere *intersect;
 double dist;
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
     intersect = trace(origin, ray, &dist, -1);
     if (!intersect) {
        /* Missed! Send ray off to darkest infinity */
        image[y*width+x] = black;
     } else {
        texture(intersect, origin, ray, dist,image + y*width + x);
     }
   }
 }
}

/* Convert a colour array into an image suitable for saving. */
void convert_image(int width, int height, colour const *im_in,
                   png_bytep im_out)
{
 int x, y;
 int r, g, b;
 double max = 0.0;

 for (x = 0; x < width; x++)
   for (y = 0; y < height; y++) {
     if (im_in[y*width+x].r > max)
        max = im_in[y*width+x].r;
     if (im_in[y*width+x].g > max)
        max = im_in[y*width+x].g;
     if (im_in[y*width+x].b > max)
        max = im_in[y*width+x].b;
   }

 max /= 256;

 for (x = 0; x < width; x++)
   for (y = 0; y < height; y++) {
     /* Red: */
     r = im_in[y*width + x].r /max;
     im_out[y*width*3 + x*3 + 0] = (r > 255) ? 255 : r;
     /* Green: */
     g = im_in[y*width + x].g / max;
     im_out[y*width*3 + x*3 + 1] = (g > 255) ? 255 : g;
     /* Blue: */
     b = im_in[y*width + x].b / max;
     im_out[y*width*3 + x*3 + 2] = (b > 255) ? 255 : b;
   }
}

/* Write an image from an array of R, G and B png_bytes. */
void write_image(int width, int height, png_bytep image,
                 char const *filename)
{
 FILE *fp;
 png_structp png_ptr;
 png_infop info_ptr;
 png_bytep *row_pointers;
 int i;

 fp = fopen(filename, "wb");
 if (!fp) {
   return;
 }
 png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
                                   NULL, NULL);
 if (!png_ptr)
   return;
 info_ptr = png_create_info_struct(png_ptr);
 if (!info_ptr) {
   png_destroy_write_struct(&png_ptr,
                            (png_infopp)NULL);
   return;
 }
 if (setjmp(png_jmpbuf(png_ptr))) {
   png_destroy_write_struct(&png_ptr, &info_ptr);
   fclose(fp);
   return;
 }
 png_init_io(png_ptr, fp);
 png_set_IHDR(png_ptr, info_ptr, width, height,
             8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
             PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
 png_write_info(png_ptr, info_ptr);
 row_pointers = (png_bytep *)malloc(height * sizeof(png_bytep));
 if (!row_pointers) {
   png_destroy_write_struct(&png_ptr, &info_ptr);
   fclose(fp);
   return;
 }
 for (i = 0; i < height; i++)
   row_pointers[i] = image + (i * width * 3);
 png_write_image(png_ptr, row_pointers);
 png_write_end(png_ptr, info_ptr);
 png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
 fclose(fp);
 printf("Saved file %s!\n", filename);
}
