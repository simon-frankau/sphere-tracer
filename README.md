# sphere-tracer - A simple ray-tracer

Way back in, er, apparently 1999, I wrote a very simple ray-tracer. It
traces spheres. Nothing but spheres. Shiny spheres.

If you've seen my excel-ray-tracer repository, you may have guessed
that I like ray-tracers. They're simple to write yet can produce
remarkably pretty results. They allow all kinds of extensions, from
highly-optimised code through support for complicated geometric
primitive, or fantastic textured, or whatever.

This tracer has none of that. On the other hand, it's simple and
straightforward, and (looking back) easy-to-read.

## Updated!

I have since decided that I would like to experiment with all the
techniques I didn't include in the very basic original renderer. We
now have:

* *spheres* Generates a whole pile of shiny spheres

* *dof* Generates a row of spheres on a checkerboard plane (how
  original!), in order to demonstrate depth-of-field, and a little
  bit of antialiasing.

(If you want the pretty-much-original version, use the tag
"original-spheres")

## Instructions

Compile it with build.sh. For the original, run the produced
executable "spheres". For the depth-of-field demonstration, run
"dof". You will need to have libpng installed. A PNG file should
appear in the current directory. You can alter the image produced by
modifying the source file, recompiling and rerunning. How high tech!
