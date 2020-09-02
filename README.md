# Rendering Bézier curves
This is a _very_ basic implementation of Eric Lengyel's work: [GPU-Centered Font Rendering Directly from Glyph Outlines](http://www.jcgt.org/published/0006/02/02/paper.pdf). It only implements the non-optimised version. The purpose of this is to have something that works.

At first I implemented a version where there was no anti-aliasing done, and the way I defined the quadratic Bézier curves was questionable, but at least it rendered something:

![Example](./img.png)

The I tried adding anti-aliasing:

![Example with anti-aliasing](./img_aa.png)

The difference can be seen either at lower resolutions or when zooming in:
![No-AA](./diff1.png)
![AA](./diff2.png)

# Building
```sh
${CXX} main.cpp stb.cpp
```
