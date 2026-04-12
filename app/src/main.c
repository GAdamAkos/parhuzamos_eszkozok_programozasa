#include <stdio.h>

#include "cl_utils.h"

int main(void)
{
    const int Width = 1920;
    const int M = 1080;

    cl_int error_code;
    cl_device_id device;
    cl_context context;
    cl_command_queue command_queue;

    printf("=== Mandelbrot OpenCL Host Initialization ===\n");
    printf("Image dimensions: Width = %d, M = %d\n\n", Width, M);

    device = select_best_gpu_device();

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &error_code);
    check_cl_error(error_code, "clCreateContext");

    command_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error_code);
    check_cl_error(error_code, "clCreateCommandQueue");

    printf("OpenCL context created successfully.\n");
    printf("Command queue with profiling enabled created successfully.\n\n");

    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);

    printf("Initialization finished successfully.\n");
    return 0;
}
