#ifndef IMAGE_WRITER_H
#define IMAGE_WRITER_H

/* Iterációszámok mentése PPM képként. */
int save_ppm_image(const char* path, const int* data, int Width, int M, int max_iterations);

#endif
