## Ditherer:
### A tool for dithering images using Bayer matricies, blue-noise convolution, and white-noise convolution.

---

### Build:

Grab the `stb_image.h` and `stb_image_write.h` headers from the stb library [here](https://github.com/nothings/stb). 
Compile with gcc using `-lm` flag.

### Usage:

Run with the options -i and -t (minimum). These options require a filepath argument and an integer by default. 
*e.g. ./ditherer -i example.jpg -t 1* for bayer dithering on *example.jpg*.

Run the built executable with the `-h` flag for more information.
