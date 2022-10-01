# gpuedit
An OpenGL 4.4+ text editor. Not done yet.


# Requirements
* Linux 2.6+
* A reasonably modern CPU.
* OpenGL 4.4+ capable hardware and drivers.
	* Note that some integrated Intel devices are capable of running gpuedit despite claiming to
	only support OpenGL 3.0, but with sub-optimal performance.


# Installing
## Automatically
The automatic setup script is out of date and should not be used.

## Manually

1. `https://github.com/yzziizzy/c3dlas` symlinked as `src/c3dlas`
1. `https://github.com/yzziizzy/c_json` symlinked as `src/c_json`
1. `https://github.com/yzziizzy/sti` symlinked as `src/sti`

1. `sudo apt-get install libx11-dev libglew-dev libfreetype6-dev libfontconfig1-dev libpng-dev libpcre3-dev`

  2. Optional Dependencies: `sudo apt-get install libjpeg-turbo8-dev alsa-source`

1. Compile the parser generator:
  2. `cd src/sti/parser/ && ./build.sh`

1. For each highlighter in `src/highlighters/`:
  2. `./build.sh`
  2. Copy or symlink the .so to /usr/local/lib/gpuedit/highlighters/

1. Build gpuedit
  2. `./debug.sh`

If you get build errors related to AVX or SSE4.1, try removing the appropriate compile flags in
`_build.c` and commenting out `C3DLAS_USE_SIMD` in `src/c3dlas/c3dlas.h`. I'll remove all that old garbage eventually.
