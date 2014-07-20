/* Sphere ray-tracer program
 * (C) Copyright Simon Frankau 1999
 *
 * This program should ray trace spheres.
 *
 * 22/04/1999 - sgf22 - Created
 * 23/04/1999 - sgf22 - Added sphere creation/multiple spheres
 * 24/04/1999 - sgf22 - Shadows and reflection
 */

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <png.h>

/* ------------------------------------------------------------
 * Macros
 */

/* #define DEBUG */

#define WIDTH 1280
#define HEIGHT 1024

#define REFLECTSTOP 0.1

#define NORMALISE(v) { double _len = sqrt(v.x*v.x + v.y*v.y + v.z*v.z); \
                      v.x /= _len; v.y /= _len; v.z /= _len; }

#define DOT(v1, v2) (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z)

#define ADD(v1, v2) {v1.x += v2.x; v1.y += v2.y; v1.z += v2.z;}

#define SUB(v1, v2) {v1.x -= v2.x; v1.y -= v2.y; v1.z -= v2.z;}

#define MULT(v, m) {v.x *= m; v.y *= m; v.z *= m;}

#define SHADE(c, i, k, p) { c.r += i.r * k.r * p; \
                           c.g += i.g * k.g * p; \
                           c.b += i.b * k.b * p; }

/* ------------------------------------------------------------
* Graphics structures.
*/

typedef struct {
 double x, y, z;
} vector;

typedef struct {
 double r, g, b;
} colour;

typedef struct {
 vector center;
 double radius;
 colour diffuse;
 colour specular;
 colour reflective;
} sphere;

typedef struct {
 vector loc;
 colour col;
} light;

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

/* Make a spherical shell filled with spheres. */
void make_spheres(double min, double max, int count);

/* Render a picture */
void render(colour *image);

/* Trace a unit ray, to find an intersection */
sphere *trace(vector from, vector direction, double *dist, int ignore);

/* Texture a point */
void texture(sphere *s, vector from, vector dir, double dist, colour *colour);

/* Convert a colour array into an image suitable for saving. */
void convert_image(int width, int height, colour *im_in, png_bytep im_out);

/* Write an image from an array of R, G and B png_bytes. */
void write_image(int width, int height, png_bytep image, char *filename);


/* ------------------------------------------------------------
* Functions.
*/

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
 make_spheres(5, 10, 1000);
 render(image);
 convert_image(WIDTH, HEIGHT, image, image2);
 write_image(WIDTH, HEIGHT, image2, "test.png");
 return 0;
}

/* Make a spherical shell filled with spheres. */
void make_spheres(double min, double max, int count)
{
 double radius, theta, phi, cosphi;
 double dist;
 int i, j;

 spheres = (sphere *)malloc(count * sizeof(sphere));
 if (!spheres) {
   printf("Couldn't allocated spheres storage.\n");
   exit(1);
 }
 no_spheres = count;

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
} 

/* Trace a unit ray, to find an intersection */
sphere *trace(vector from, vector direction, double *dist, int ignore)
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
void texture(sphere *s, vector from, vector dir, double dist, colour *col)
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
void render(colour *image)
{
 vector origin = {0.0, 0.0, 0.0};
 vector ray;
 int x, y;
 sphere *intersect;
 double dist;
 for (y = 0; y < HEIGHT; y++)
   for (x = 0; x < WIDTH; x++)
     image[y*WIDTH+x] = white;


 printf("Ray Tracing (%d):\n", HEIGHT);
 for (y = 0; y < HEIGHT; y++) {
   printf("%d\n", y);
   for (x = 0; x < WIDTH; x++) {
     ray.x = x - WIDTH/2;
     ray.y = y - HEIGHT/2;
     ray.z = WIDTH/2;
     NORMALISE(ray);
     intersect = trace(origin, ray, &dist, -1);
     if (!intersect) {
        /* Missed! Send ray off to darkest infinity */
        image[y*WIDTH+x] = black;
     } else {
        texture(intersect, origin, ray, dist,image + y*WIDTH + x);
     }
   }  
 }
}

/* Convert a colour array into an image suitable for saving. */
void convert_image(int width, int height, colour *im_in, png_bytep im_out)
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
void write_image(int width, int height, png_bytep image, char *filename)
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
 if (setjmp(png_ptr->jmpbuf)) {
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
