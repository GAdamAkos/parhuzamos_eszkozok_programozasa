#include <stdio.h>
#include <stdlib.h>

#include "cl_utils.h"
#include "kernel_loader.h"
#include "image_writer.h"

int main(void)
{
    const int Width = 1920;
    const int M = 1080;
    const int max_iterations = 1000;
    const size_t element_count = (size_t)Width * (size_t)M;
    const size_t output_size = element_count * sizeof(cl_int);

    cl_int error_code;
    cl_device_id device;
    cl_context context;
    cl_command_queue command_queue;

    char* kernel_source = NULL;
    const char* kernel_source_ptr = NULL;

    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer = NULL;

    cl_int* host_output = NULL;

    size_t global_work_size[2];
    cl_event kernel_event;

    size_t i;
    cl_int min_value;
    cl_int max_value_found;
    long long sum_values = 0;

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

    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, output_size, NULL, &error_code);
    check_cl_error(error_code, "clCreateBuffer");

    host_output = (cl_int*)malloc(output_size);
    if (host_output == NULL) {
        fprintf(stderr, "Failed to allocate host output buffer.\n");

        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        free(kernel_source);
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        return EXIT_FAILURE;
    }

    error_code = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    check_cl_error(error_code, "clSetKernelArg(output_buffer)");

    error_code = clSetKernelArg(kernel, 1, sizeof(cl_int), &Width);
    check_cl_error(error_code, "clSetKernelArg(Width)");

    error_code = clSetKernelArg(kernel, 2, sizeof(cl_int), &M);
    check_cl_error(error_code, "clSetKernelArg(M)");

    global_work_size[0] = (size_t)Width;
    global_work_size[1] = (size_t)M;

    error_code = clEnqueueNDRangeKernel(
        command_queue,
        kernel,
        2,
        NULL,
        global_work_size,
        NULL,
        0,
        NULL,
        &kernel_event
    );
    check_cl_error(error_code, "clEnqueueNDRangeKernel");

    error_code = clWaitForEvents(1, &kernel_event);
    check_cl_error(error_code, "clWaitForEvents");

    error_code = clEnqueueReadBuffer(
        command_queue,
        output_buffer,
        CL_TRUE,
        0,
        output_size,
        host_output,
        0,
        NULL,
        NULL
    );
    check_cl_error(error_code, "clEnqueueReadBuffer");

    printf("Kernel executed successfully.\n");

    min_value = host_output[0];
    max_value_found = host_output[0];

    for (i = 0; i < element_count; ++i) {
        if (host_output[i] < min_value) {
            min_value = host_output[i];
        }
        if (host_output[i] > max_value_found) {
            max_value_found = host_output[i];
        }
        sum_values += host_output[i];
    }

    printf("First 10 output values:\n");
    for (i = 0; i < 10 && i < element_count; ++i) {
        printf("  output[%lu] = %d\n", (unsigned long)i, host_output[i]);
    }

    printf("\nOutput statistics:\n");
    printf("  Min iteration value : %d\n", min_value);
    printf("  Max iteration value : %d\n", max_value_found);
    printf("  Average value       : %.2f\n", (double)sum_values / (double)element_count);

    if (!save_ppm_image("output/mandelbrot.ppm", host_output, Width, M, max_iterations)) {
        fprintf(stderr, "Failed to save output image.\n");

        clReleaseEvent(kernel_event);
        free(host_output);
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        free(kernel_source);
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        return EXIT_FAILURE;
    }

    printf("Image saved successfully: output/mandelbrot.ppm\n");

    clReleaseEvent(kernel_event);
    free(host_output);
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    free(kernel_source);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);

    printf("\nMandelbrot computation and image export finished successfully.\n");
    return 0;
}