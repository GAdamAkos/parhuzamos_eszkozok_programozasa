#include <stdio.h>
#include <stdlib.h>

#include "cl_utils.h"
#include "kernel_loader.h"

int main(void)
{
    const int Width = 1920;
    const int M = 1080;

    cl_int error_code;
    cl_device_id device;
    cl_context context;
    cl_command_queue command_queue;

    char* kernel_source = NULL;
    const char* kernel_source_ptr = NULL;

    cl_program program;
    cl_kernel kernel;

    printf("=== Mandelbrot OpenCL Host Initialization ===\n");
    printf("Image dimensions: Width = %d, M = %d\n\n", Width, M);

    device = select_best_gpu_device();

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &error_code);
    check_cl_error(error_code, "clCreateContext");

    command_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error_code);
    check_cl_error(error_code, "clCreateCommandQueue");

    printf("OpenCL context created successfully.\n");
    printf("Command queue with profiling enabled created successfully.\n\n");

    kernel_source = load_kernel_source("kernels/mandelbrot.cl");
    if (kernel_source == NULL) {
        fprintf(stderr, "Kernel source loading failed.\n");
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        return EXIT_FAILURE;
    }

    kernel_source_ptr = kernel_source;

    program = clCreateProgramWithSource(context, 1, &kernel_source_ptr, NULL, &error_code);
    check_cl_error(error_code, "clCreateProgramWithSource");

    error_code = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (error_code != CL_SUCCESS) {
        fprintf(stderr, "OpenCL program build failed.\n");
        print_program_build_log(program, device);

        free(kernel_source);
        clReleaseProgram(program);
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        return EXIT_FAILURE;
    }

    printf("OpenCL program built successfully.\n");

    kernel = clCreateKernel(program, "mandelbrot_kernel", &error_code);
    check_cl_error(error_code, "clCreateKernel");

    printf("Kernel created successfully.\n");

    clReleaseKernel(kernel);
    clReleaseProgram(program);
    free(kernel_source);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);

    printf("Initialization and kernel setup finished successfully.\n");
    return 0;
}