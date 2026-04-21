#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "cl_utils.h"
#include "image_writer.h"
#include "kernel_loader.h"

typedef enum RunMode {
    MODE_GPU = 0,
    MODE_CPU = 1
} RunMode;

typedef struct AppConfig {
    int width;
    int height;
    int max_iterations;
    int use_interactive_input;
    int batch_mode;
    int list_devices_only;
    int save_image;
    int use_specific_device;
    int device_index;
    RunMode mode;
} AppConfig;

typedef struct RunResult {
    char device_name[256];
    double execution_time_ms;
    int width;
    int height;
    int max_iterations;
    int used_gpu;
    int device_index;
} RunResult;

typedef struct Resolution {
    int width;
    int height;
} Resolution;

/* Mandelbrot complex plane range. */
static const float MIN_RE = -2.0f;
static const float MAX_RE = 1.0f;
static const float MIN_IM = -1.2f;
static const float MAX_IM = 1.2f;

static int read_positive_int(const char* prompt)
{
    int value;
    printf("%s", prompt);

    if (scanf("%d", &value) != 1 || value <= 0) {
        fprintf(stderr, "Invalid input. A positive integer is required.\n");
        exit(EXIT_FAILURE);
    }

    return value;
}

static int read_menu_choice(const char* prompt, int min_value, int max_value)
{
    int value;
    printf("%s", prompt);

    if (scanf("%d", &value) != 1 || value < min_value || value > max_value) {
        fprintf(stderr, "Invalid menu choice.\n");
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

static void ensure_output_directory_exists(void)
{
#ifdef _WIN32
    _mkdir("output");
#else
    mkdir("output", 0777);
#endif
}

static void reset_measurements_csv(void)
{
    ensure_output_directory_exists();
    remove("output/measurements.csv");
}

static void build_unique_output_filename(
    char* buffer,
    size_t buffer_size,
    const char* mode_name,
    int device_index,
    int width,
    int height,
    int max_iterations
)
{
    int suffix = 0;
    int written;

    if (device_index >= 0) {
        written = snprintf(
            buffer,
            buffer_size,
            "output/mandelbrot_%s_dev%d_%dx%d_iter%d.ppm",
            mode_name, device_index, width, height, max_iterations
        );
    } else {
        written = snprintf(
            buffer,
            buffer_size,
            "output/mandelbrot_%s_%dx%d_iter%d.ppm",
            mode_name, width, height, max_iterations
        );
    }

    if (written < 0 || (size_t)written >= buffer_size) {
        fprintf(stderr, "Failed to build output filename.\n");
        exit(EXIT_FAILURE);
    }

    if (!file_exists(buffer)) {
        return;
    }

    for (suffix = 1; ; ++suffix) {
        if (device_index >= 0) {
            written = snprintf(
                buffer,
                buffer_size,
                "output/mandelbrot_%s_dev%d_%dx%d_iter%d_%d.ppm",
                mode_name, device_index, width, height, max_iterations, suffix
            );
        } else {
            written = snprintf(
                buffer,
                buffer_size,
                "output/mandelbrot_%s_%dx%d_iter%d_%d.ppm",
                mode_name, width, height, max_iterations, suffix
            );
        }

        if (written < 0 || (size_t)written >= buffer_size) {
            fprintf(stderr, "Failed to build unique output filename.\n");
            exit(EXIT_FAILURE);
        }

        if (!file_exists(buffer)) {
            return;
        }
    }
}

static size_t calculate_output_size_or_exit(int width, int height)
{
    size_t pixel_count;

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Invalid image size.\n");
        exit(EXIT_FAILURE);
    }

    if ((size_t)width > SIZE_MAX / (size_t)height) {
        fprintf(stderr, "Image size is too large.\n");
        exit(EXIT_FAILURE);
    }

    pixel_count = (size_t)width * (size_t)height;

    if (pixel_count > SIZE_MAX / sizeof(cl_int)) {
        fprintf(stderr, "Required buffer size is too large.\n");
        exit(EXIT_FAILURE);
    }

    return pixel_count * sizeof(cl_int);
}

static void validate_output_size_for_device(cl_device_id device, size_t output_size)
{
    cl_int error_code;
    cl_ulong global_mem_size = 0;
    cl_ulong max_mem_alloc_size = 0;

    error_code = clGetDeviceInfo(
        device, CL_DEVICE_GLOBAL_MEM_SIZE,
        sizeof(global_mem_size), &global_mem_size, NULL
    );
    check_cl_error(error_code, "clGetDeviceInfo(CL_DEVICE_GLOBAL_MEM_SIZE)");

    error_code = clGetDeviceInfo(
        device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,
        sizeof(max_mem_alloc_size), &max_mem_alloc_size, NULL
    );
    check_cl_error(error_code, "clGetDeviceInfo(CL_DEVICE_MAX_MEM_ALLOC_SIZE)");

    if ((cl_ulong)output_size > max_mem_alloc_size) {
        fprintf(
            stderr,
            "Requested image is too large: required buffer size (%lu bytes) exceeds the GPU maximum allocatable buffer size (%lu bytes).\n",
            (unsigned long)output_size, (unsigned long)max_mem_alloc_size
        );
        exit(EXIT_FAILURE);
    }

    if ((cl_ulong)output_size > global_mem_size) {
        fprintf(
            stderr,
            "Requested image is too large: required buffer size (%lu bytes) exceeds the GPU global memory (%lu bytes).\n",
            (unsigned long)output_size, (unsigned long)global_mem_size
        );
        exit(EXIT_FAILURE);
    }
}

static void print_usage(const char* program_name)
{
    printf("Usage:\n");
    printf("  %s\n", program_name);
    printf("  %s --list-devices\n", program_name);
    printf("  %s --batch\n", program_name);
    printf("  %s --width 1920 --height 1080 --iter 1000 --mode gpu --device-index 0 --no-image\n", program_name);
    printf("  %s --width 1920 --height 1080 --iter 1000 --mode cpu --no-image\n", program_name);
}

static void init_default_config(AppConfig* config)
{
    config->width = 0;
    config->height = 0;
    config->max_iterations = 0;
    config->use_interactive_input = 1;
    config->batch_mode = 0;
    config->list_devices_only = 0;
    config->save_image = 1;
    config->use_specific_device = 0;
    config->device_index = -1;
    config->mode = MODE_GPU;
}

static void setup_config(
    AppConfig* config,
    int width,
    int height,
    int max_iterations,
    RunMode mode,
    int save_image,
    int use_specific_device,
    int device_index
)
{
    init_default_config(config);
    config->width = width;
    config->height = height;
    config->max_iterations = max_iterations;
    config->use_interactive_input = 0;
    config->save_image = save_image;
    config->mode = mode;
    config->use_specific_device = use_specific_device;
    config->device_index = device_index;
}

static int parse_positive_int_arg(const char* value, const char* arg_name)
{
    long parsed;
    char* end_ptr = NULL;

    parsed = strtol(value, &end_ptr, 10);
    if (end_ptr == value || *end_ptr != '\0' || parsed <= 0 || parsed > INT_MAX) {
        fprintf(stderr, "Invalid value for %s: %s\n", arg_name, value);
        exit(EXIT_FAILURE);
    }

    return (int)parsed;
}

static void parse_arguments(int argc, char** argv, AppConfig* config)
{
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--list-devices") == 0) {
            config->list_devices_only = 1;
            config->use_interactive_input = 0;
        } else if (strcmp(argv[i], "--batch") == 0) {
            config->batch_mode = 1;
            config->use_interactive_input = 0;
            config->save_image = 0;
        } else if (strcmp(argv[i], "--width") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --width.\n");
                exit(EXIT_FAILURE);
            }
            config->width = parse_positive_int_arg(argv[++i], "--width");
            config->use_interactive_input = 0;
        } else if (strcmp(argv[i], "--height") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --height.\n");
                exit(EXIT_FAILURE);
            }
            config->height = parse_positive_int_arg(argv[++i], "--height");
            config->use_interactive_input = 0;
        } else if (strcmp(argv[i], "--iter") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --iter.\n");
                exit(EXIT_FAILURE);
            }
            config->max_iterations = parse_positive_int_arg(argv[++i], "--iter");
            config->use_interactive_input = 0;
        } else if (strcmp(argv[i], "--device-index") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --device-index.\n");
                exit(EXIT_FAILURE);
            }
            config->device_index = parse_positive_int_arg(argv[++i], "--device-index");
            config->use_specific_device = 1;
            config->use_interactive_input = 0;
        } else if (strcmp(argv[i], "--mode") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --mode.\n");
                exit(EXIT_FAILURE);
            }

            ++i;
            if (strcmp(argv[i], "gpu") == 0) {
                config->mode = MODE_GPU;
            } else if (strcmp(argv[i], "cpu") == 0) {
                config->mode = MODE_CPU;
            } else {
                fprintf(stderr, "Unknown mode: %s (expected gpu or cpu)\n", argv[i]);
                exit(EXIT_FAILURE);
            }

            config->use_interactive_input = 0;
        } else if (strcmp(argv[i], "--no-image") == 0) {
            config->save_image = 0;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        } else {
            fprintf(stderr, "Unknown parameter: %s\n", argv[i]);
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (!config->batch_mode && !config->list_devices_only && !config->use_interactive_input) {
        if (config->width <= 0 || config->height <= 0 || config->max_iterations <= 0) {
            fprintf(stderr, "For command-line execution you must provide: --width, --height, --iter\n");
            exit(EXIT_FAILURE);
        }
    }
}

static void append_result_to_csv(const RunResult* result)
{
    FILE* file = NULL;
    int write_header = 0;

    ensure_output_directory_exists();

    if (!file_exists("output/measurements.csv")) {
        write_header = 1;
    }

    file = fopen("output/measurements.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Failed to open output/measurements.csv.\n");
        return;
    }

    if (write_header) {
        fprintf(file, "mode,device_index,device_name,width,height,max_iterations,execution_time_ms\n");
    }

    fprintf(
        file,
        "%s,%d,\"%s\",%d,%d,%d,%.3f\n",
        result->used_gpu ? "gpu" : "cpu",
        result->device_index,
        result->device_name,
        result->width,
        result->height,
        result->max_iterations,
        result->execution_time_ms
    );

    fclose(file);
}

static int mandelbrot_iterations_at_pixel(
    int x,
    int y,
    int width,
    int height,
    float min_re,
    float max_re,
    float min_im,
    float max_im,
    int max_iterations
)
{
    float c_re = min_re + ((float)x / (float)width) * (max_re - min_re);
    float c_im = max_im - ((float)y / (float)height) * (max_im - min_im);
    float z_re = 0.0f;
    float z_im = 0.0f;
    int iter = 0;

    while ((z_re * z_re + z_im * z_im <= 4.0f) && iter < max_iterations) {
        float next_re = z_re * z_re - z_im * z_im + c_re;
        float next_im = 2.0f * z_re * z_im + c_im;
        z_re = next_re;
        z_im = next_im;
        ++iter;
    }

    return iter;
}

static void print_result(const RunResult* result)
{
    printf("\n=== Result ===\n");
    printf("Mode            : %s\n", result->used_gpu ? "GPU OpenCL" : "CPU sequential");
    printf("Selected device : %s\n", result->device_name);
    printf("Device index    : %d\n", result->device_index);
    printf("Resolution      : %dx%d\n", result->width, result->height);
    printf("Max iterations  : %d\n", result->max_iterations);
    printf("Execution time  : %.3f ms\n", result->execution_time_ms);
}

static int run_cpu_sequential(
    int width,
    int height,
    int max_iterations,
    int save_image,
    RunResult* result
)
{
    size_t output_size = calculate_output_size_or_exit(width, height);
    cl_int* host_output = (cl_int*)malloc(output_size);
    char output_filename[256];
    clock_t start_clock, end_clock;
    int x, y;

    if (host_output == NULL) {
        fprintf(stderr, "Failed to allocate memory for CPU output.\n");
        return EXIT_FAILURE;
    }

    start_clock = clock();

    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            host_output[y * width + x] = mandelbrot_iterations_at_pixel(
                x, y, width, height, MIN_RE, MAX_RE, MIN_IM, MAX_IM, max_iterations
            );
        }
    }

    end_clock = clock();

    snprintf(result->device_name, sizeof(result->device_name), "CPU sequential");
    result->execution_time_ms = 1000.0 * (double)(end_clock - start_clock) / (double)CLOCKS_PER_SEC;
    result->width = width;
    result->height = height;
    result->max_iterations = max_iterations;
    result->used_gpu = 0;
    result->device_index = -1;

    print_result(result);

    if (save_image) {
        ensure_output_directory_exists();

        build_unique_output_filename(
            output_filename, sizeof(output_filename),
            "cpu", -1, width, height, max_iterations
        );

        printf("Output file     : %s\n", output_filename);

        if (save_ppm_image(output_filename, host_output, width, height, max_iterations) != 0) {
            free(host_output);
            return EXIT_FAILURE;
        }
    }

    free(host_output);
    return EXIT_SUCCESS;
}

static int run_gpu_opencl(
    int width,
    int height,
    int max_iterations,
    int save_image,
    int use_specific_device,
    int device_index,
    RunResult* result
)
{
    size_t pixel_count;
    size_t output_size;

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
    int exit_status = EXIT_FAILURE;
    char selected_device_name[256];
    char output_filename[256];

    output_size = calculate_output_size_or_exit(width, height);
    pixel_count = output_size / sizeof(cl_int);

    device = use_specific_device
        ? get_gpu_device_by_global_index(device_index)
        : select_best_gpu_device();

    if (!use_specific_device) {
        device_index = -1;
    }

    get_device_name(device, selected_device_name, sizeof(selected_device_name));
    validate_output_size_for_device(device, output_size);

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &error_code);
    check_cl_error(error_code, "clCreateContext");

    command_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error_code);
    check_cl_error(error_code, "clCreateCommandQueue");

    kernel_source = load_kernel_source("kernels/mandelbrot.cl");
    if (kernel_source == NULL) {
        fprintf(stderr, "Failed to load kernel source.\n");
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

    check_cl_error(clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer), "clSetKernelArg(0)");
    check_cl_error(clSetKernelArg(kernel, 1, sizeof(int), &width), "clSetKernelArg(1)");
    check_cl_error(clSetKernelArg(kernel, 2, sizeof(int), &height), "clSetKernelArg(2)");
    check_cl_error(clSetKernelArg(kernel, 3, sizeof(float), &MIN_RE), "clSetKernelArg(3)");
    check_cl_error(clSetKernelArg(kernel, 4, sizeof(float), &MAX_RE), "clSetKernelArg(4)");
    check_cl_error(clSetKernelArg(kernel, 5, sizeof(float), &MIN_IM), "clSetKernelArg(5)");
    check_cl_error(clSetKernelArg(kernel, 6, sizeof(float), &MAX_IM), "clSetKernelArg(6)");
    check_cl_error(clSetKernelArg(kernel, 7, sizeof(int), &max_iterations), "clSetKernelArg(7)");

    {
        size_t work_size[2] = {(size_t)width, (size_t)height};
        cl_ulong start_time = 0;
        cl_ulong end_time = 0;

        error_code = clEnqueueNDRangeKernel(
            command_queue, kernel, 2, NULL, work_size, NULL, 0, NULL, &kernel_event
        );
        check_cl_error(error_code, "clEnqueueNDRangeKernel");

        check_cl_error(clWaitForEvents(1, &kernel_event), "clWaitForEvents");

        check_cl_error(
            clGetEventProfilingInfo(
                kernel_event, CL_PROFILING_COMMAND_START,
                sizeof(start_time), &start_time, NULL
            ),
            "clGetEventProfilingInfo(START)"
        );
        check_cl_error(
            clGetEventProfilingInfo(
                kernel_event, CL_PROFILING_COMMAND_END,
                sizeof(end_time), &end_time, NULL
            ),
            "clGetEventProfilingInfo(END)"
        );

        result->execution_time_ms = (double)(end_time - start_time) / 1000000.0;
    }

    error_code = clEnqueueReadBuffer(
        command_queue, output_buffer, CL_TRUE, 0, output_size, host_output, 0, NULL, NULL
    );
    check_cl_error(error_code, "clEnqueueReadBuffer");

    snprintf(result->device_name, sizeof(result->device_name), "%s", selected_device_name);
    result->width = width;
    result->height = height;
    result->max_iterations = max_iterations;
    result->used_gpu = 1;
    result->device_index = device_index;

    print_result(result);
    printf("Pixels          : %lu\n", (unsigned long)pixel_count);

    if (save_image) {
        ensure_output_directory_exists();

        build_unique_output_filename(
            output_filename, sizeof(output_filename),
            "gpu", device_index, width, height, max_iterations
        );

        printf("Output file     : %s\n", output_filename);

        if (save_ppm_image(output_filename, host_output, width, height, max_iterations) != 0) {
            goto cleanup;
        }
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

static int run_single_measurement(const AppConfig* config, RunResult* result)
{
    if (config->mode == MODE_CPU) {
        return run_cpu_sequential(
            config->width, config->height, config->max_iterations, config->save_image, result
        );
    }

    return run_gpu_opencl(
        config->width, config->height, config->max_iterations, config->save_image,
        config->use_specific_device, config->device_index, result
    );
}

static int run_and_store(const AppConfig* config)
{
    RunResult result;

    if (run_single_measurement(config, &result) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    append_result_to_csv(&result);
    return EXIT_SUCCESS;
}

static void run_batch_measurements(void)
{
    const Resolution resolutions[] = {
        {640, 480},
        {1280, 720},
        {1920, 1080}
    };
    const int iterations[] = {100, 1000, 5000};

    int gpu_count = get_gpu_device_count();
    int res_index, iter_index, gpu_index;
    AppConfig config;
    
    reset_measurements_csv();

    printf("=== Batch measurement started ===\n");
    printf("Detected GPU count: %d\n", gpu_count);
    printf("Images will not be saved in batch mode. Results will be written to CSV only.\n");

    for (res_index = 0; res_index < 3; ++res_index) {
        for (iter_index = 0; iter_index < 3; ++iter_index) {
            int width = resolutions[res_index].width;
            int height = resolutions[res_index].height;
            int max_iterations = iterations[iter_index];

            printf("\n----------------------------------------\n");
            printf("Measurement: %dx%d, iter=%d\n", width, height, max_iterations);

            for (gpu_index = 0; gpu_index < gpu_count; ++gpu_index) {
                setup_config(&config, width, height, max_iterations, MODE_GPU, 0, 1, gpu_index);

                if (run_and_store(&config) != EXIT_SUCCESS) {
                    fprintf(stderr, "GPU batch measurement failed: device-index=%d\n", gpu_index);
                }
            }

            setup_config(&config, width, height, max_iterations, MODE_CPU, 0, 0, -1);

            if (run_and_store(&config) != EXIT_SUCCESS) {
                fprintf(stderr, "CPU batch measurement failed.\n");
            }
        }
    }

    printf("\nBatch measurement finished. Results: output/measurements.csv\n");
}

static void run_interactive_image_generation(void)
{
    AppConfig config;
    int gpu_count = get_gpu_device_count();
    int selected_gpu;

    if (gpu_count <= 0) {
        fprintf(stderr, "No GPU device found. Image generation requires a GPU.\n");
        exit(EXIT_FAILURE);
    }

    printf("=== Available GPU devices ===\n");
    list_gpu_devices();
    printf("\n");

    selected_gpu = read_menu_choice("Select GPU index: ", 0, gpu_count - 1);

    setup_config(
        &config,
        read_positive_int("Enter image width: "),
        read_positive_int("Enter image height: "),
        read_positive_int("Enter maximum iterations: "),
        MODE_GPU,
        1,
        1,
        selected_gpu
    );

    if (run_and_store(&config) != EXIT_SUCCESS) {
        exit(EXIT_FAILURE);
    }
}

static void run_interactive_main_menu(void)
{
    int choice;

    printf("=== Mandelbrot Main Menu ===\n");
    printf("1 - Image generation\n");
    printf("2 - Measurement\n\n");

    choice = read_menu_choice("Select an option: ", 1, 2);
    printf("\n");

    if (choice == 1) {
        run_interactive_image_generation();
    } else {
        run_batch_measurements();
    }
}

int main(int argc, char** argv)
{
    AppConfig config;

    init_default_config(&config);

    if (argc == 1) {
        run_interactive_main_menu();
        return EXIT_SUCCESS;
    }

    parse_arguments(argc, argv, &config);

    if (config.list_devices_only) {
        list_gpu_devices();
        return EXIT_SUCCESS;
    }

    if (config.batch_mode) {
        run_batch_measurements();
        return EXIT_SUCCESS;
    }

    if (config.use_interactive_input) {
        config.width = read_positive_int("Enter image width: ");
        config.height = read_positive_int("Enter image height: ");
        config.max_iterations = read_positive_int("Enter maximum iterations: ");
    }

    return run_and_store(&config);
}