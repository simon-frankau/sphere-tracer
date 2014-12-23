/*
 * png_render.c: Call render and write the results to a PNG.
 *
 * (C) Copyright Simon Frankau 1999-2014
 */

#include <png.h>
#include <stdlib.h>
#include <string.h>

#include "tracer.h"

/* Convert a colour array into an image suitable for saving. */
static void convert_image(int width, int height, colour const *im_in,
                          int dest_width, png_bytep im_out)
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

 max /= 256.0;

 for (x = 0; x < width; x++)
   for (y = 0; y < height; y++) {
     /* Red: */
     r = im_in[y*width + x].r /max;
     im_out[y*dest_width*3 + x*3 + 0] = (r > 255) ? 255 : r;
     /* Green: */
     g = im_in[y*width + x].g / max;
     im_out[y*dest_width*3 + x*3 + 1] = (g > 255) ? 255 : g;
     /* Blue: */
     b = im_in[y*width + x].b / max;
     im_out[y*dest_width*3 + x*3 + 2] = (b > 255) ? 255 : b;
   }
}

/* Write an image from an array of R, G and B png_bytes. */
static void write_image(int width, int height, png_bytep image,
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

void png_render(scene *sc, int width, int height, char const *file)
{
 colour *image = (colour *)malloc(width*height*sizeof(colour));
 png_bytep image2 = (png_bytep)malloc(width*height*3);
 render(sc, width, height, image);
 convert_image(width, height, image, width, image2);
 write_image(width, height, image2, file);
}

/* Render a set of scenes into a big image. */
void png_render_ex(scene *sc, int num_scenes, int tiles_across,
		   int width, int height, char const *file)
{
  int tiles_down = (num_scenes - 1) / tiles_across + 1;
  colour *image = (colour *)malloc(width*height*sizeof(colour));
  int dest_size = width*height*tiles_across*tiles_down*3;
  png_bytep image2 = (png_bytep)malloc(dest_size);
  memset(image2, 0, dest_size);

  int i;
  for (i = 0; i < num_scenes; i++) {
    int tx = i % tiles_across;
    int ty = i / tiles_across;
    render(sc + i, width, height, image);
    convert_image(width, height, image,
		  width * tiles_across,
		  image2 + 3 * (ty * height * width * tiles_across
				+ tx * width));
  }
  write_image(width * tiles_across, height * tiles_down, image2, file);
}

