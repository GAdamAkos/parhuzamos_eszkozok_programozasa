__kernel void mandelbrot_kernel(
    __global int* output,
    const int Width,
    const int M,
    const float min_re,
    const float max_re,
    const float min_im,
    const float max_im,
    const int max_iter)
{
    /* Globális pozíció = aktuális pixel koordinátája. */
    int x = get_global_id(0);
    int y = get_global_id(1);

    /* Biztonsági ellenőrzés a kimeneti tartományhoz. */
    if (x >= Width || y >= M) {
        return;
    }

    /* Pixel koordináták leképezése a komplex síkra. */
    float real = min_re + ((float)x / (float)(Width - 1)) * (max_re - min_re);
    float imag = max_im - ((float)y / (float)(M - 1)) * (max_im - min_im);

    /* Kezdőérték: z = 0. */
    float zr = 0.0f;
    float zi = 0.0f;
    float zr2 = 0.0f;
    float zi2 = 0.0f;

    int iteration = 0;
    while ((zr2 + zi2 <= 4.0f) && (iteration < max_iter)) {
        /* z(n+1) = z(n)^2 + c valós és képzetes részre bontva. */
        zi = 2.0f * zr * zi + imag;
        zr = zr2 - zi2 + real;
        zr2 = zr * zr;
        zi2 = zi * zi;
        iteration++;
    }

    /* Kimenet: az adott pixelhez tartozó iterációszám. */
    output[y * Width + x] = iteration;
}
