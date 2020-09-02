# Rendering Bézier curves
This is a _very_ basic implementation of Eric Lengyel's work: [GPU-Centered Font Rendering Directly from Glyph Outlines](file:///tmp/mozilla_alex0/Lengyel2017FontRendering.pdf). It only implements the non-optimised version. The purpose of this is to have something that works; there is no antialiasing done, and the way I define the quadratic Bézier curves is questionable, but at least it renders something:

![Example](./img.png)

# Building
```sh
${CXX} main.cpp stb.cpp
```
