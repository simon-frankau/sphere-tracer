#!/bin/sh

gcc spheres.c tracer.c png_render.c -lpng -O2 -Wall -ansi -o spheres
gcc dof.c tracer.c png_render.c -lpng -O2 -Wall -ansi -o dof
gcc soft.c tracer.c png_render.c -lpng -O2 -Wall -ansi -o soft