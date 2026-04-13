#include <stdio.h>
#include <stdlib.h>

#include "cl_utils.h"
#include "image_writer.h"
#include "kernel_loader.h"

static int read_positive_int(const char* prompt)
{
    int value;

    printf("%s", prompt);
    if (scanf("%d", &value) != 1 || value <= 0) {
        fprintf(stderr, "Hibas bemenet. Pozitiv egesz szam szukseges.\n");
        exit(EXIT_FAILURE);
    }

    return value;
}

static int file_exists(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file != NULL) {
        fclose(file);
        return 1;
    }
    return 0;
}

static void build_unique_output_filename(
    char* buffer,
    size_t buffer_size,
    int width,
    int height,
    int max_iterations
)
{
    int suffix = 0;
    int written;

    written = snprintf(
        buffer,
        buffer_size,
        "output/mandelbrot_%dx%d_iter%d.ppm",
        width,
        height,
        max_iterations
    );

    if (written < 0 || (size_t)written >= buffer_size) {
        fprintf(stderr, "Nem sikerult letrehozni a kimeneti fajlnevet.\n");
        exit(EXIT_FAILURE);
    }

    if (!file_exists(buffer)) {
        return;
    }

    suffix = 1;
    while (1) {
        written = snprintf(
            buffer,
            buffer_size,
            "output/mandelbrot_%dx%d_iter%d_%d.ppm",
            width,
            height,
            max_iterations,
            suffix
        );

        if (written < 0 || (size_t)written >= buffer_size) {
            fprintf(stderr, "Nem sikerult letrehozni az egyedi kimeneti fajlnevet.\n");
            exit(EXIT_FAILURE);
        }

        if (!file_exists(buffer)) {
            return;
        }

        ++suffix;
    }
}

int main(void)
{
    int width;
    int height;
    int max_iterations;
    long pixel_count;
    size_t output_size;

    const float min_re = -2.0f;
    const float max_re = 1.0f;
    const float min_im = -1.2f;
    const float max_im = 1.2f;

    cl_int error_code = CL_SUCCESS;
    cl_device_id device = NULL;
    cl_context context = NULL;
    cl_command_queue command_queue = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    cl_mem output_buffer = NULL;
    cl_int* host_output = NULL;
    cl_event kernel_event = NULL;
    char* kernel_source = NULL;
    double execution_time_ms = 0.0;
    int exit_status = EXIT_FAILURE;
    char selected_device_name[256];
    char output_filename[256];

    printf("=== Mandelbrot OpenCL Parameterek ===\n");
    width = read_positive_int("Kerem a szelesseget (pl. 1920): ");
    height = read_positive_int("Kerem a magassagot (pl. 1080): ");
    max_iterations = read_positive_int("Kerem a maximalis iteracioszamot (pl. 1000): ");

    pixel_count = (long)width * (long)height;
    output_size = (size_t)pixel_count * sizeof(cl_int);

    device = select_best_gpu_device();
    get_device_name(device, selected_device_name, sizeof(selected_device_name));

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &error_code);
    check_cl_error(error_code, "clCreateContext");

    command_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error_code);
    check_cl_error(error_code, "clCreateCommandQueue");

    kernel_source = load_kernel_source("kernels/mandelbrot.cl");

    {
        const char* kernel_source_ptr = kernel_source;
        program = clCreateProgramWithSource(context, 1, &kernel_source_ptr, NULL, &error_code);
        check_cl_error(error_code, "clCreateProgramWithSource");
    }

    error_code = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (error_code != CL_SUCCESS) {
        print_program_build_log(program, device);
        check_cl_error(error_code, "clBuildProgram");
    }

    kernel = clCreateKernel(program, "mandelbrot_kernel", &error_code);
    check_cl_error(error_code, "clCreateKernel");

    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, output_size, NULL, &error_code);
    check_cl_error(error_code, "clCreateBuffer");

    host_output = (cl_int*)malloc(output_size);
    if (host_output == NULL) {
        fprintf(stderr, "Nem sikerult memoriat foglalni a host oldali kimenethez.\n");
        goto cleanup;
    }

    error_code = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    check_cl_error(error_code, "clSetKernelArg(output_buffer)");
    error_code = clSetKernelArg(kernel, 1, sizeof(cl_int), &width);
    check_cl_error(error_code, "clSetKernelArg(width)");
    error_code = clSetKernelArg(kernel, 2, sizeof(cl_int), &height);
    check_cl_error(error_code, "clSetKernelArg(height)");
    error_code = clSetKernelArg(kernel, 3, sizeof(cl_float), &min_re);
    check_cl_error(error_code, "clSetKernelArg(min_re)");
    error_code = clSetKernelArg(kernel, 4, sizeof(cl_float), &max_re);
    check_cl_error(error_code, "clSetKernelArg(max_re)");
    error_code = clSetKernelArg(kernel, 5, sizeof(cl_float), &min_im);
    check_cl_error(error_code, "clSetKernelArg(min_im)");
    error_code = clSetKernelArg(kernel, 6, sizeof(cl_float), &max_im);
    check_cl_error(error_code, "clSetKernelArg(max_im)");
    error_code = clSetKernelArg(kernel, 7, sizeof(cl_int), &max_iterations);
    check_cl_error(error_code, "clSetKernelArg(max_iterations)");

    {
        size_t work_size[2] = {(size_t)width, (size_t)height};
        error_code = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, work_size, NULL, 0, NULL, &kernel_event);
        check_cl_error(error_code, "clEnqueueNDRangeKernel");
    }

    error_code = clWaitForEvents(1, &kernel_event);
    check_cl_error(error_code, "clWaitForEvents");

    {
        cl_ulong start_time = 0;
        cl_ulong end_time = 0;

        error_code = clGetEventProfilingInfo(
            kernel_event,
            CL_PROFILING_COMMAND_START,
            sizeof(cl_ulong),
            &start_time,
            NULL
        );
        check_cl_error(error_code, "clGetEventProfilingInfo(START)");

        error_code = clGetEventProfilingInfo(
            kernel_event,
            CL_PROFILING_COMMAND_END,
            sizeof(cl_ulong),
            &end_time,
            NULL
        );
        check_cl_error(error_code, "clGetEventProfilingInfo(END)");

        execution_time_ms = (double)(end_time - start_time) / 1000000.0;
    }

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

    build_unique_output_filename(
        output_filename,
        sizeof(output_filename),
        width,
        height,
        max_iterations
    );

    printf("\n--- Eredmenyek ---\n");
    printf("Selected GPU: %s\n", selected_device_name);
    printf("Pixelek szama: %ld\n", pixel_count);
    printf("Felbontas: %d x %d\n", width, height);
    printf("Maximalis iteracioszam: %d\n", max_iterations);
    printf("Kernel execution time: %.4f ms\n", execution_time_ms);
    printf("Output file: %s\n", output_filename);

    if (!save_ppm_image(output_filename, host_output, width, height, max_iterations)) {
        fprintf(stderr, "Nem sikerult elmenteni a kimeneti kepet.\n");
        goto cleanup;
    }

    exit_status = EXIT_SUCCESS;

cleanup:
    free(host_output);
    free(kernel_source);

    if (kernel_event != NULL) {
        clReleaseEvent(kernel_event);
    }
    if (output_buffer != NULL) {
        clReleaseMemObject(output_buffer);
    }
    if (kernel != NULL) {
        clReleaseKernel(kernel);
    }
    if (program != NULL) {
        clReleaseProgram(program);
    }
    if (command_queue != NULL) {
        clReleaseCommandQueue(command_queue);
    }
    if (context != NULL) {
        clReleaseContext(context);
    }

    return exit_status;
}