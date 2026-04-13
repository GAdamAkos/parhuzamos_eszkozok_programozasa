#include "image_writer.h"

#include <math.h>
#include <stdio.h>

static unsigned char clamp_to_byte(double value)
{
    if (value < 0.0) {
        return 0;
    }
    if (value > 255.0) {
        return 255;
    }
    return (unsigned char)(value + 0.5);
}

int save_ppm_image(const char* path, const int* data, int Width, int M, int max_iterations)
{
    FILE* file;
    int x;
    int y;

    file = fopen(path, "wb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open output image file: %s\n", path);
        return 0;
    }

    fprintf(file, "P6\n%d %d\n255\n", Width, M);

    for (y = 0; y < M; ++y) {
        for (x = 0; x < Width; ++x) {
            int index = y * Width + x;
            int value = data[index];
            unsigned char color[3];

            if (value >= max_iterations) {
                color[0] = 0;
                color[1] = 0;
                color[2] = 0;
            } else {
                double t = (double)value / (double)max_iterations;
                double r = 9.0 * (1.0 - t) * t * t * t * 255.0;
                double g = 15.0 * (1.0 - t) * (1.0 - t) * t * t * 255.0;
                double b = 8.5 * (1.0 - t) * (1.0 - t) * (1.0 - t) * t * 255.0;

                color[0] = clamp_to_byte(r);
                color[1] = clamp_to_byte(g);
                color[2] = clamp_to_byte(b);
            }

            fwrite(color, sizeof(unsigned char), 3, file);
        }
    }

    fclose(file);
    return 1;
}