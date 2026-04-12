__kernel void mandelbrot_kernel(__global int* output, const int Width, const int M)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int index;

    if (x >= Width || y >= M) {
        return;
    }

    index = y * Width + x;
    output[index] = index;
}