/*
 * png_render.h: Call render and write the results to a PNG.
 *
 * (C) Copyright Simon Frankau 1999-2014
 */

#ifndef PNG_RENDER_H_INCLUDED
#define PNG_RENDER_H_INCLUDED

#include "tracer.h"

void png_render(scene *sc, int width, int height, char const *file);

void png_render_ex(scene *sc, int num_scenes, int tiles_across,
		   int width, int height, char const *file);

#endif // PNG_RENDER_H_INCLUDED
