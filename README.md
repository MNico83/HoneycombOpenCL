# HoneycombOpenCL
###Some graphic experiments on the [Honeycom theorem](https://en.wikipedia.org/wiki/Honeycomb_conjecture). 

The plane is restricted to a square, discretized into a 2D grid of pixels. A small iterative routine modifies the color of each pixel so that the boundary of each colored shape is "locally" minimal. The global minimum is reach for a regular tiling os hexagons, but the routine sometimes reaches a local minimum, which contains other kind of polygons...

The routine is written in **OpenCL** as pixel colors can be computed in parallel. Then, the resulting image is displayed by **OpenGL** (using CL/GL interop). The GUI relies on **Qt** (v5)

## Build instruction

It uses cmake, which requires

* Qt (> 5)
* OpenCL. It's OK if the CUDA SDK or the AMD APP is installed. Otherwise, the variables OpenCL_INCPATH and OpenCL_LIBPATH must be set to respectively the directory containing CL includes and library.
* OpenGL (>3)
