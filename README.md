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

* *soft* Demonstrates soft shadows, including a bit of coloured
  shadows.

* *fuzzy* Shows off fuzzy reflections. The five images are without
  fuzz, two different levels of fuzz, and then fuzz with only
  horizontal and vertical elements. To be honest, I'm a bit
  underwhelmed by this one.

* *moblur* Demonstrates motion blur. It's like 'dof', only with the
  balls moving away from/towards the viewer. I also turned off the
  depth-of-field effect in order to make sure the blurriness
  basically comes from the motion. I don't think the effect is
  super-impressive, but it's kind of nice. I prefer it to the fuzzy
  reflections.

* *trans* Deals with transparency and refractions. I don't
   particularly like it, since refraction at realistic levels is a
   very powerful effect which makes it difficult to make it looks
   nice.

* *dof2* This is really a combination of 'dof' and 'soft', applying
   soft shadows to the 'dof' image.

(If you want the pretty-much-original version, use the tag
"original-spheres")

## Instructions

Compile it with build.sh. For the original, run the produced
executable "spheres". For the depth-of-field demonstration, run
"dof". etc. You will need to have libpng installed. A PNG file should
appear in the current directory. You can alter the image produced by
modifying the source file, recompiling and rerunning. How high tech!

## Code quality disclaimer

I love writing these disclaimers. This is code I wrote 15 years ago,
and then hacked up to have new features added. If I were doing this
kind of thing now, I wouldn't write it in C, and I can't even remember
which features are in proper C and not C++. As I have limited spare
time, I'm just hacking the features in and moving on to the next one,
rather than creating a coherent and neat ray-tracer design. It's even
got special memory management (i.e. do a malloc and never bother with
free). Don't read too much into the style - this is not quality code!
