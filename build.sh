#!/bin/sh

gcc spheres.c tracer.c png_render.c -lpng -O2 -Wall -std=c99 -o spheres
gcc dof.c tracer.c png_render.c -lpng -O2 -Wall -std=c99 -o dof
gcc soft.c tracer.c png_render.c -lpng -O2 -Wall -std=c99 -o soft
gcc fuzzy.c tracer.c png_render.c -lpng -O2 -Wall -std=c99 -o fuzzy
