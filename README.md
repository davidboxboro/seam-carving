# Seam Carving

Content-aware image resizing in C++. Uses gradient-based image energy and dynamic programming to find low-energy seams, supporting both shrinking by seam removal and enlargement by seam insertion.

## Build and run

Requires a C++11 compiler; image I/O and Eigen are bundled.

```bash
make run
```

The default demo writes energy maps and seam visualizations to `Output/`. Enable the shrinking or enlargement examples in `a9_main.cpp` to run them on images in `Input/`. The implementation is in `a9.cpp`; a [write-up](write-up/write-up.pdf) is included.
