# gpuedit
An OpenGL 4.4+ text editor. Not done yet.


# Requirements
* Linux 2.6+
* A reasonably modern CPU.
* OpenGL 4.4+ capable hardware and drivers.
	* Note that some integrated Intel devices are capable of running gpuedit despite claiming to
	only support OpenGL 3.0, but with sub-optimal performance.


# Installing
## Automatically on Ubuntu

Run `setup.sh`

## Manually

* `https://github.com/yzziizzy/c3dlas` symlinked as `src/c3dlas`
* `https://github.com/yzziizzy/c_json` symlinked as `src/c_json`
* `https://github.com/yzziizzy/sti` symlinked as `src/sti`

* `sudo apt-get install libx11-dev libglew-dev libfreetype6-dev libfontconfig1-dev libpng-dev`

* Optional Dependencies: `sudo apt-get install libjpeg-turbo8-dev libvorbis-dev alsa-source`

* `./autogen.sh`
* `make && ./src/gpuedit`

If you get build errors related to AVX or SSE4.1, try removing the appropriate compile flags in
`src/Makefile.am` and commenting out `C3DLAS_USE_SIMD` in `src/c3dlas/c3dlas.h`.
