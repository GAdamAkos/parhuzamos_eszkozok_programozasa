#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "cl_utils.h"
#include "image_writer.h"
#include "kernel_loader.h"

/* Pozitív egész szám bekérése a futási paraméterekhez. */
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

/* Egyszerű fájllétezés-ellenőrzés a kimeneti név generálásához. */
static int file_exists(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file != NULL) {
        fclose(file);
        return 1;
    }
    return 0;
}

/* Egyedi kimeneti fájlnév készítése, hogy a korábbi futások megmaradjanak. */
static void build_unique_output_filename(
    char* buffer,
    size_t buffer_size,
    int width,
    int height,
    int max_iterations
)
{
    int suffix = 0;
    int written = snprintf(
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

/* Kimeneti puffer méretének számítása túlcsordulás-ellenőrzéssel. */
static size_t calculate_output_size_or_exit(int width, int height)
{
    size_t pixel_count;

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "A kep merete ervenytelen.\n");
        exit(EXIT_FAILURE);
    }

    if ((size_t)width > SIZE_MAX / (size_t)height) {
        fprintf(stderr, "A megadott kepmeret tul nagy.\n");
        exit(EXIT_FAILURE);
    }

    pixel_count = (size_t)width * (size_t)height;

    if (pixel_count > SIZE_MAX / sizeof(cl_int)) {
        fprintf(stderr, "A szukseges puffermeret tul nagy.\n");
        exit(EXIT_FAILURE);
    }

    return pixel_count * sizeof(cl_int);
}

/* GPU memóriahatárok ellenőrzése a pufferfoglalás előtt. */
static void validate_output_size_for_device(cl_device_id device, size_t output_size)
{
    cl_int error_code;
    cl_ulong global_mem_size = 0;
    cl_ulong max_mem_alloc_size = 0;

    error_code = clGetDeviceInfo(
        device,
        CL_DEVICE_GLOBAL_MEM_SIZE,
        sizeof(global_mem_size),
        &global_mem_size,
        NULL
    );
    check_cl_error(error_code, "clGetDeviceInfo(CL_DEVICE_GLOBAL_MEM_SIZE)");

    error_code = clGetDeviceInfo(
        device,
        CL_DEVICE_MAX_MEM_ALLOC_SIZE,
        sizeof(max_mem_alloc_size),
        &max_mem_alloc_size,
        NULL
    );
    check_cl_error(error_code, "clGetDeviceInfo(CL_DEVICE_MAX_MEM_ALLOC_SIZE)");

    if ((cl_ulong)output_size > max_mem_alloc_size) {
        fprintf(
            stderr,
            "A kert kep tul nagy: a szukseges puffermeret (%lu byte) meghaladja a GPU maximalis egyben foglalhato puffermeretet (%lu byte).\n",
            (unsigned long)output_size,
            (unsigned long)max_mem_alloc_size
        );
        exit(EXIT_FAILURE);
    }

    if ((cl_ulong)output_size > global_mem_size) {
        fprintf(
            stderr,
            "A kert kep tul nagy: a szukseges puffermeret (%lu byte) meghaladja a GPU globalis memoriajat (%lu byte).\n",
            (unsigned long)output_size,
            (unsigned long)global_mem_size
        );
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    int width;
    int height;
    int max_iterations;
    size_t pixel_count;
    size_t output_size;

    /* A komplex sík vizsgált tartománya. */
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

    /* A kernel pixelenként egy iterációszámot ír a kimeneti tömbbe. */
    output_size = calculate_output_size_or_exit(width, height);
    pixel_count = output_size / sizeof(cl_int);

    /* Eszköz kiválasztása és a szükséges puffer méretének ellenőrzése. */
    device = select_best_gpu_device();
    get_device_name(device, selected_device_name, sizeof(selected_device_name));
    validate_output_size_for_device(device, output_size);

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &error_code);
    check_cl_error(error_code, "clCreateContext");

    /* Profiling engedélyezve a kernelidő méréséhez. */
    command_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error_code);
    check_cl_error(error_code, "clCreateCommandQueue");

    kernel_source = load_kernel_source("kernels/mandelbrot.cl");
    if (kernel_source == NULL) {
        fprintf(stderr, "Nem sikerult betolteni a kernel forrasat.\n");
        goto cleanup;
    }

    program = clCreateProgramWithSource(context, 1, (const char**)&kernel_source, NULL, &error_code);
    check_cl_error(error_code, "clCreateProgramWithSource");

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
        fprintf(stderr, "Failed to allocate host memory for output buffer.\n");
        goto cleanup;
    }

    /* A kernel argumentumsorrendje megegyezik a .cl fájl definíciójával. */
    check_cl_error(clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer), "clSetKernelArg(0)");
    check_cl_error(clSetKernelArg(kernel, 1, sizeof(int), &width), "clSetKernelArg(1)");
    check_cl_error(clSetKernelArg(kernel, 2, sizeof(int), &height), "clSetKernelArg(2)");
    check_cl_error(clSetKernelArg(kernel, 3, sizeof(float), &min_re), "clSetKernelArg(3)");
    check_cl_error(clSetKernelArg(kernel, 4, sizeof(float), &max_re), "clSetKernelArg(4)");
    check_cl_error(clSetKernelArg(kernel, 5, sizeof(float), &min_im), "clSetKernelArg(5)");
    check_cl_error(clSetKernelArg(kernel, 6, sizeof(float), &max_im), "clSetKernelArg(6)");
    check_cl_error(clSetKernelArg(kernel, 7, sizeof(int), &max_iterations), "clSetKernelArg(7)");

    {
        size_t work_size[2] = {(size_t)width, (size_t)height};

        /* 2D NDRange: egy work-item egy pixel feldolgozását végzi. */
        error_code = clEnqueueNDRangeKernel(
            command_queue,
            kernel,
            2,
            NULL,
            work_size,
            NULL,
            0,
            NULL,
            &kernel_event
        );
        check_cl_error(error_code, "clEnqueueNDRangeKernel");
    }

    check_cl_error(clWaitForEvents(1, &kernel_event), "clWaitForEvents");

    {
        cl_ulong start_time = 0;
        cl_ulong end_time = 0;

        check_cl_error(
            clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(start_time), &start_time, NULL),
            "clGetEventProfilingInfo(START)"
        );
        check_cl_error(
            clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(end_time), &end_time, NULL),
            "clGetEventProfilingInfo(END)"
        );

        /* A kiírt idő a kernel végrehajtási ideje, ezredmásodpercben. */
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

    build_unique_output_filename(output_filename, sizeof(output_filename), width, height, max_iterations);

    printf("\n=== Eredmeny ===\n");
    printf("Selected device : %s\n", selected_device_name);
    printf("Pixels          : %lu\n", (unsigned long)pixel_count);
    printf("Resolution      : %dx%d\n", width, height);
    printf("Max iterations  : %d\n", max_iterations);
    printf("Kernel time     : %.3f ms\n", execution_time_ms);
    printf("Output file     : %s\n", output_filename);

    if (save_ppm_image(output_filename, host_output, width, height, max_iterations) != 0) {
        goto cleanup;
    }

    exit_status = EXIT_SUCCESS;

cleanup:
    if (host_output != NULL) free(host_output);
    if (kernel_source != NULL) free(kernel_source);
    if (kernel_event != NULL) clReleaseEvent(kernel_event);
    if (output_buffer != NULL) clReleaseMemObject(output_buffer);
    if (kernel != NULL) clReleaseKernel(kernel);
    if (program != NULL) clReleaseProgram(program);
    if (command_queue != NULL) clReleaseCommandQueue(command_queue);
    if (context != NULL) clReleaseContext(context);

    return exit_status;
}
