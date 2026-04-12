__kernel void mandelbrot_kernel(__global int* output)
{
    const int gid = get_global_id(0);
    output[gid] = gid;
}
