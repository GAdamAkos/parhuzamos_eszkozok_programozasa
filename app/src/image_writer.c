#include "image_writer.h"

#include <stdio.h>

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
            int shade;

            if (value >= max_iterations) {
                color[0] = 0;
                color[1] = 0;
                color[2] = 0;
            } else {
                shade = (value * 255) / max_iterations;
                color[0] = (unsigned char)shade;
                color[1] = (unsigned char)shade;
                color[2] = (unsigned char)shade;
            }

            fwrite(color, sizeof(unsigned char), 3, file);
        }
    }

    fclose(file);
    return 1;
}