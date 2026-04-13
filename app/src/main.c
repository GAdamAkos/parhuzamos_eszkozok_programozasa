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
    
    const float min_re = -2.0f;
    const float max_re = 1.0f;
    const float min_im = -1.2f;
    const float max_im = 1.2f;

    const size_t element_count = (size_t)Width * (size_t)M;
    const size_t output_size = element_count * sizeof(cl_int);

    cl_int error_code;
    cl_device_id device;
    cl_context context;
    cl_command_queue command_queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer = NULL;
    cl_int* host_output = NULL;
    cl_event kernel_event;
    cl_ulong start_time, end_time;

    device = select_best_gpu_device();
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &error_code);
    command_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error_code);

    char* kernel_source = load_kernel_source("kernels/mandelbrot.cl");
    const char* kernel_source_ptr = kernel_source;
    program = clCreateProgramWithSource(context, 1, &kernel_source_ptr, NULL, &error_code);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    kernel = clCreateKernel(program, "mandelbrot_kernel", &error_code);
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, output_size, NULL, &error_code);
    host_output = (cl_int*)malloc(output_size);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    clSetKernelArg(kernel, 1, sizeof(cl_int), &Width);
    clSetKernelArg(kernel, 2, sizeof(cl_int), &M);
    clSetKernelArg(kernel, 3, sizeof(cl_float), &min_re);
    clSetKernelArg(kernel, 4, sizeof(cl_float), &max_re);
    clSetKernelArg(kernel, 5, sizeof(cl_float), &min_im);
    clSetKernelArg(kernel, 6, sizeof(cl_float), &max_im);
    clSetKernelArg(kernel, 7, sizeof(cl_int), &max_iterations);

    size_t global_work_size[2] = {(size_t)Width, (size_t)M};

    clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, &kernel_event);
    clWaitForEvents(1, &kernel_event);

    clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start_time, NULL);
    clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end_time, NULL);
    double execution_time = (double)(end_time - start_time) / 1000000.0;

    clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0, output_size, host_output, 0, NULL, NULL);

    printf("Kernel execution time: %.4f ms\n", execution_time);

    save_ppm_image("output/mandelbrot.ppm", host_output, Width, M, max_iterations);

    free(host_output);
    free(kernel_source);
    clReleaseEvent(kernel_event);
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);

    return 0;
}