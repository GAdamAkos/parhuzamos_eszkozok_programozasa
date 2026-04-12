#include "cl_utils.h"

#include <stdio.h>
#include <stdlib.h>

const char* cl_error_to_string(cl_int error_code)
{
    switch (error_code) {
        case CL_SUCCESS: return "CL_SUCCESS";
        case CL_DEVICE_NOT_FOUND: return "CL_DEVICE_NOT_FOUND";
        case CL_DEVICE_NOT_AVAILABLE: return "CL_DEVICE_NOT_AVAILABLE";
        case CL_COMPILER_NOT_AVAILABLE: return "CL_COMPILER_NOT_AVAILABLE";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case CL_OUT_OF_RESOURCES: return "CL_OUT_OF_RESOURCES";
        case CL_OUT_OF_HOST_MEMORY: return "CL_OUT_OF_HOST_MEMORY";
        case CL_PROFILING_INFO_NOT_AVAILABLE: return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case CL_MEM_COPY_OVERLAP: return "CL_MEM_COPY_OVERLAP";
        case CL_IMAGE_FORMAT_MISMATCH: return "CL_IMAGE_FORMAT_MISMATCH";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case CL_BUILD_PROGRAM_FAILURE: return "CL_BUILD_PROGRAM_FAILURE";
        case CL_MAP_FAILURE: return "CL_MAP_FAILURE";
        case CL_INVALID_VALUE: return "CL_INVALID_VALUE";
        case CL_INVALID_DEVICE_TYPE: return "CL_INVALID_DEVICE_TYPE";
        case CL_INVALID_PLATFORM: return "CL_INVALID_PLATFORM";
        case CL_INVALID_DEVICE: return "CL_INVALID_DEVICE";
        case CL_INVALID_CONTEXT: return "CL_INVALID_CONTEXT";
        case CL_INVALID_QUEUE_PROPERTIES: return "CL_INVALID_QUEUE_PROPERTIES";
        case CL_INVALID_COMMAND_QUEUE: return "CL_INVALID_COMMAND_QUEUE";
        case CL_INVALID_MEM_OBJECT: return "CL_INVALID_MEM_OBJECT";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case CL_INVALID_IMAGE_SIZE: return "CL_INVALID_IMAGE_SIZE";
        case CL_INVALID_SAMPLER: return "CL_INVALID_SAMPLER";
        case CL_INVALID_BINARY: return "CL_INVALID_BINARY";
        case CL_INVALID_BUILD_OPTIONS: return "CL_INVALID_BUILD_OPTIONS";
        case CL_INVALID_PROGRAM: return "CL_INVALID_PROGRAM";
        case CL_INVALID_PROGRAM_EXECUTABLE: return "CL_INVALID_PROGRAM_EXECUTABLE";
        case CL_INVALID_KERNEL_NAME: return "CL_INVALID_KERNEL_NAME";
        case CL_INVALID_KERNEL_DEFINITION: return "CL_INVALID_KERNEL_DEFINITION";
        case CL_INVALID_KERNEL: return "CL_INVALID_KERNEL";
        case CL_INVALID_ARG_INDEX: return "CL_INVALID_ARG_INDEX";
        case CL_INVALID_ARG_VALUE: return "CL_INVALID_ARG_VALUE";
        case CL_INVALID_ARG_SIZE: return "CL_INVALID_ARG_SIZE";
        case CL_INVALID_KERNEL_ARGS: return "CL_INVALID_KERNEL_ARGS";
        case CL_INVALID_WORK_DIMENSION: return "CL_INVALID_WORK_DIMENSION";
        case CL_INVALID_WORK_GROUP_SIZE: return "CL_INVALID_WORK_GROUP_SIZE";
        case CL_INVALID_WORK_ITEM_SIZE: return "CL_INVALID_WORK_ITEM_SIZE";
        case CL_INVALID_GLOBAL_OFFSET: return "CL_INVALID_GLOBAL_OFFSET";
        case CL_INVALID_EVENT_WAIT_LIST: return "CL_INVALID_EVENT_WAIT_LIST";
        case CL_INVALID_EVENT: return "CL_INVALID_EVENT";
        case CL_INVALID_OPERATION: return "CL_INVALID_OPERATION";
        case CL_INVALID_GL_OBJECT: return "CL_INVALID_GL_OBJECT";
        case CL_INVALID_BUFFER_SIZE: return "CL_INVALID_BUFFER_SIZE";
        case CL_INVALID_MIP_LEVEL: return "CL_INVALID_MIP_LEVEL";
        default: return "UNKNOWN_OPENCL_ERROR";
    }
}

void check_cl_error(cl_int error_code, const char* operation)
{
    if (error_code != CL_SUCCESS) {
        fprintf(stderr, "[OpenCL ERROR] %s failed: %s (%d)\n",
                operation, cl_error_to_string(error_code), error_code);
        exit(EXIT_FAILURE);
    }
}

static void get_platform_string(cl_platform_id platform, cl_platform_info param, char* buffer, size_t buffer_size)
{
    cl_int error_code;
    size_t actual_size = 0;

    error_code = clGetPlatformInfo(platform, param, buffer_size, buffer, &actual_size);
    check_cl_error(error_code, "clGetPlatformInfo");

    if (actual_size > 0 && actual_size <= buffer_size) {
        buffer[actual_size - 1] = '\0';
    } else {
        buffer[buffer_size - 1] = '\0';
    }
}

static void get_device_string(cl_device_id device, cl_device_info param, char* buffer, size_t buffer_size)
{
    cl_int error_code;
    size_t actual_size = 0;

    error_code = clGetDeviceInfo(device, param, buffer_size, buffer, &actual_size);
    check_cl_error(error_code, "clGetDeviceInfo");

    if (actual_size > 0 && actual_size <= buffer_size) {
        buffer[actual_size - 1] = '\0';
    } else {
        buffer[buffer_size - 1] = '\0';
    }
}

void print_platform_info(cl_platform_id platform)
{
    char name[256];
    char vendor[256];
    char version[256];

    get_platform_string(platform, CL_PLATFORM_NAME, name, sizeof(name));
    get_platform_string(platform, CL_PLATFORM_VENDOR, vendor, sizeof(vendor));
    get_platform_string(platform, CL_PLATFORM_VERSION, version, sizeof(version));

    printf("Platform:\n");
    printf("  Name   : %s\n", name);
    printf("  Vendor : %s\n", vendor);
    printf("  Version: %s\n", version);
}

void print_device_info(cl_device_id device)
{
    char name[256];
    char vendor[256];
    char version[256];
    cl_ulong global_mem_size = 0;
    cl_uint compute_units = 0;
    size_t max_work_group_size = 0;
    cl_uint clock_frequency = 0;
    cl_device_type device_type = 0;

    get_device_string(device, CL_DEVICE_NAME, name, sizeof(name));
    get_device_string(device, CL_DEVICE_VENDOR, vendor, sizeof(vendor));
    get_device_string(device, CL_DEVICE_VERSION, version, sizeof(version));

    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL),
        "clGetDeviceInfo(CL_DEVICE_GLOBAL_MEM_SIZE)"
    );
    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL),
        "clGetDeviceInfo(CL_DEVICE_MAX_COMPUTE_UNITS)"
    );
    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL),
        "clGetDeviceInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE)"
    );
    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL),
        "clGetDeviceInfo(CL_DEVICE_MAX_CLOCK_FREQUENCY)"
    );
    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL),
        "clGetDeviceInfo(CL_DEVICE_TYPE)"
    );

    printf("Device:\n");
    printf("  Name              : %s\n", name);
    printf("  Vendor            : %s\n", vendor);
    printf("  Version           : %s\n", version);
    printf("  Type              : %s\n", (device_type & CL_DEVICE_TYPE_GPU) ? "GPU" : "Other");
    printf("  Compute Units     : %u\n", compute_units);
    printf("  Max Clock (MHz)   : %u\n", clock_frequency);
    printf("  Global Memory (MB): %.2f\n", (double)global_mem_size / (1024.0 * 1024.0));
    printf("  Max Work Group    : %lu\n", (unsigned long)max_work_group_size);
}

cl_device_id select_best_gpu_device(void)
{
    cl_int error_code;
    cl_uint platform_count = 0;
    cl_platform_id* platforms = NULL;

    cl_device_id best_device = NULL;
    cl_ulong best_score = 0;

    error_code = clGetPlatformIDs(0, NULL, &platform_count);
    check_cl_error(error_code, "clGetPlatformIDs(count)");

    if (platform_count == 0) {
        fprintf(stderr, "No OpenCL platforms found.\n");
        exit(EXIT_FAILURE);
    }

    platforms = (cl_platform_id*)malloc(platform_count * sizeof(cl_platform_id));
    if (platforms == NULL) {
        fprintf(stderr, "Host memory allocation failed for platform list.\n");
        exit(EXIT_FAILURE);
    }

    error_code = clGetPlatformIDs(platform_count, platforms, NULL);
    check_cl_error(error_code, "clGetPlatformIDs(list)");

    printf("Detected %u OpenCL platform(s).\n\n", platform_count);

    for (cl_uint p = 0; p < platform_count; ++p) {
        cl_uint device_count = 0;
        cl_device_id* devices = NULL;
        cl_int device_error;

        print_platform_info(platforms[p]);

        device_error = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, 0, NULL, &device_count);

        if (device_error == CL_DEVICE_NOT_FOUND || device_count == 0) {
            printf("  No GPU devices found on this platform.\n\n");
            continue;
        }
        check_cl_error(device_error, "clGetDeviceIDs(count)");

        devices = (cl_device_id*)malloc(device_count * sizeof(cl_device_id));
        if (devices == NULL) {
            fprintf(stderr, "Host memory allocation failed for device list.\n");
            free(platforms);
            exit(EXIT_FAILURE);
        }

        error_code = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, device_count, devices, NULL);
        check_cl_error(error_code, "clGetDeviceIDs(list)");

        for (cl_uint d = 0; d < device_count; ++d) {
            cl_uint compute_units = 0;
            cl_uint clock_frequency = 0;
            cl_ulong global_mem_size = 0;
            cl_ulong score = 0;

            print_device_info(devices[d]);

            check_cl_error(
                clGetDeviceInfo(devices[d], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL),
                "clGetDeviceInfo(CL_DEVICE_MAX_COMPUTE_UNITS)"
            );
            check_cl_error(
                clGetDeviceInfo(devices[d], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL),
                "clGetDeviceInfo(CL_DEVICE_MAX_CLOCK_FREQUENCY)"
            );
            check_cl_error(
                clGetDeviceInfo(devices[d], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL),
                "clGetDeviceInfo(CL_DEVICE_GLOBAL_MEM_SIZE)"
            );

            score = (cl_ulong)compute_units * (cl_ulong)clock_frequency + (global_mem_size / (1024ULL * 1024ULL));

            printf("  Selection score   : %lu\n\n", (unsigned long)score);
            
            if (score > best_score) {
                best_score = score;
                best_device = devices[d];
            }
        }

        free(devices);
    }

    free(platforms);

    if (best_device == NULL) {
        fprintf(stderr, "No suitable GPU device found.\n");
        exit(EXIT_FAILURE);
    }

    printf("Selected best available GPU device.\n\n");
    return best_device;
}
