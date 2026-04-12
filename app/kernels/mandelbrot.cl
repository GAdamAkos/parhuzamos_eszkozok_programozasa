__kernel void mandelbrot_kernel(__global int* output, const int Width, const int M)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int index;

    float min_real = -2.0f;
    float max_real = 1.0f;
    float min_imag = -1.2f;
    float max_imag = 1.2f;

    float real;
    float imag;

    float zr = 0.0f;
    float zi = 0.0f;
    float zr2 = 0.0f;
    float zi2 = 0.0f;

    int iteration = 0;
    int max_iterations = 1000;

    if (x >= Width || y >= M) {
        return;
    }

    index = y * Width + x;

    real = min_real + ((float)x / (float)(Width - 1)) * (max_real - min_real);
    imag = max_imag - ((float)y / (float)(M - 1)) * (max_imag - min_imag);

    while ((zr2 + zi2 <= 4.0f) && (iteration < max_iterations)) {
        zi = 2.0f * zr * zi + imag;
        zr = zr2 - zi2 + real;

        zr2 = zr * zr;
        zi2 = zi * zi;

        iteration++;
    }

    output[index] = iteration;
}