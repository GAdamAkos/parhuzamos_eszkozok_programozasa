#include "cl_utils.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_PLATFORMS 16
#define MAX_DEVICES   16

static void get_platform_string(cl_platform_id platform, cl_platform_info param, char* buffer, size_t buffer_size)
{
    cl_int error_code;
    size_t actual_size = 0;

    error_code = clGetPlatformInfo(platform, param, buffer_size, buffer, &actual_size);
    check_cl_error(error_code, "clGetPlatformInfo");

    if (actual_size > 0 && actual_size <= buffer_size) {
        buffer[actual_size - 1] = '\0';
    } else if (buffer_size > 0) {
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
    } else if (buffer_size > 0) {
        buffer[buffer_size - 1] = '\0';
    }
}

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
        case CL_INVALID_HOST_PTR: return "CL_INVALID_HOST_PTR";
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

void get_device_name(cl_device_id device, char* buffer, size_t buffer_size)
{
    get_device_string(device, CL_DEVICE_NAME, buffer, buffer_size);
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
    cl_uint clock_frequency = 0;
    size_t max_work_group_size = 0;
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
        clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL),
        "clGetDeviceInfo(CL_DEVICE_MAX_CLOCK_FREQUENCY)"
    );
    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL),
        "clGetDeviceInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE)"
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

static cl_ulong calculate_device_score(cl_device_id device)
{
    cl_uint compute_units = 0;
    cl_uint clock_frequency = 0;
    cl_ulong global_mem_size = 0;
    size_t max_work_group_size = 0;
    cl_device_type device_type = 0;
    cl_bool unified_memory = CL_FALSE;

    cl_ulong score;
    cl_ulong compute_score;
    cl_ulong memory_score;

    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL),
        "clGetDeviceInfo(CL_DEVICE_MAX_COMPUTE_UNITS)"
    );
    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL),
        "clGetDeviceInfo(CL_DEVICE_MAX_CLOCK_FREQUENCY)"
    );
    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL),
        "clGetDeviceInfo(CL_DEVICE_GLOBAL_MEM_SIZE)"
    );
    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL),
        "clGetDeviceInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE)"
    );
    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL),
        "clGetDeviceInfo(CL_DEVICE_TYPE)"
    );
    check_cl_error(
        clGetDeviceInfo(device, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(unified_memory), &unified_memory, NULL),
        "clGetDeviceInfo(CL_DEVICE_HOST_UNIFIED_MEMORY)"
    );

    compute_score = (cl_ulong)compute_units * (cl_ulong)clock_frequency;
    memory_score = global_mem_size / (1024ULL * 1024ULL);

    score = compute_score * 1000ULL;
    score += memory_score * 10ULL;
    score += (cl_ulong)max_work_group_size;

    if (device_type & CL_DEVICE_TYPE_GPU) {
        score += 1000000000ULL;
    }

    if (unified_memory == CL_FALSE) {
        score += 10000000ULL;
    }

    return score;
}

static cl_uint get_platforms(cl_platform_id* platforms)
{
    cl_int error_code;
    cl_uint platform_count = 0;

    error_code = clGetPlatformIDs(MAX_PLATFORMS, platforms, &platform_count);
    check_cl_error(error_code, "clGetPlatformIDs");

    return platform_count;
}

static cl_uint get_gpu_devices_for_platform(cl_platform_id platform, cl_device_id* devices)
{
    cl_int error_code;
    cl_uint device_count = 0;

    error_code = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, MAX_DEVICES, devices, &device_count);

    if (error_code == CL_DEVICE_NOT_FOUND) {
        return 0;
    }

    check_cl_error(error_code, "clGetDeviceIDs(CL_DEVICE_TYPE_GPU)");
    return device_count;
}

int get_gpu_device_count(void)
{
    cl_platform_id platforms[MAX_PLATFORMS];
    cl_device_id devices[MAX_DEVICES];
    cl_uint platform_count;
    cl_uint platform_index;
    int total_gpu_count = 0;

    platform_count = get_platforms(platforms);

    for (platform_index = 0; platform_index < platform_count; ++platform_index) {
        total_gpu_count += (int)get_gpu_devices_for_platform(platforms[platform_index], devices);
    }

    return total_gpu_count;
}

cl_device_id get_gpu_device_by_global_index(int global_device_index)
{
    cl_platform_id platforms[MAX_PLATFORMS];
    cl_device_id devices[MAX_DEVICES];
    cl_uint platform_count;
    cl_uint platform_index;
    int current_index = 0;

    platform_count = get_platforms(platforms);

    for (platform_index = 0; platform_index < platform_count; ++platform_index) {
        cl_uint device_count = get_gpu_devices_for_platform(platforms[platform_index], devices);
        cl_uint device_index;

        for (device_index = 0; device_index < device_count; ++device_index) {
            if (current_index == global_device_index) {
                return devices[device_index];
            }
            ++current_index;
        }
    }

    fprintf(stderr, "Invalid GPU index: %d\n", global_device_index);
    exit(EXIT_FAILURE);
}

void list_gpu_devices(void)
{
    cl_platform_id platforms[MAX_PLATFORMS];
    cl_device_id devices[MAX_DEVICES];
    cl_uint platform_count;
    cl_uint platform_index;
    int global_index = 0;

    platform_count = get_platforms(platforms);

    if (platform_count == 0) {
        printf("No OpenCL platform found.\n");
        return;
    }

    printf("Detected %u OpenCL platform(s).\n\n", platform_count);

    for (platform_index = 0; platform_index < platform_count; ++platform_index) {
        cl_uint device_count;
        cl_uint device_index;

        print_platform_info(platforms[platform_index]);
        device_count = get_gpu_devices_for_platform(platforms[platform_index], devices);

        if (device_count == 0) {
            printf("  No GPU devices found on this platform.\n\n");
            continue;
        }

        for (device_index = 0; device_index < device_count; ++device_index) {
            cl_ulong score = calculate_device_score(devices[device_index]);

            printf("GPU global index : %d\n", global_index);
            print_device_info(devices[device_index]);
            printf("  Score             : %.0f\n\n", (double)score);

            ++global_index;
        }
    }

    if (global_index == 0) {
        printf("No GPU device found.\n");
    }
}

cl_device_id select_best_gpu_device(void)
{
    cl_platform_id platforms[MAX_PLATFORMS];
    cl_device_id devices[MAX_DEVICES];
    cl_uint platform_count;
    cl_uint platform_index;

    cl_device_id best_device = NULL;
    cl_ulong best_score = 0;

    platform_count = get_platforms(platforms);

    if (platform_count == 0) {
        fprintf(stderr, "No OpenCL platforms found.\n");
        exit(EXIT_FAILURE);
    }

    for (platform_index = 0; platform_index < platform_count; ++platform_index) {
        cl_uint device_count = get_gpu_devices_for_platform(platforms[platform_index], devices);
        cl_uint device_index;

        for (device_index = 0; device_index < device_count; ++device_index) {
            cl_ulong score = calculate_device_score(devices[device_index]);

            if (best_device == NULL || score > best_score) {
                best_device = devices[device_index];
                best_score = score;
            }
        }
    }

    if (best_device == NULL) {
        fprintf(stderr, "No suitable GPU device found.\n");
        exit(EXIT_FAILURE);
    }

    return best_device;
}

void print_program_build_log(cl_program program, cl_device_id device)
{
    cl_int error_code;
    size_t log_size = 0;
    char* log_buffer;

    error_code = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    check_cl_error(error_code, "clGetProgramBuildInfo(size)");

    if (log_size == 0) {
        fprintf(stderr, "No build log available.\n");
        return;
    }

    log_buffer = (char*)malloc(log_size + 1);
    if (log_buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for build log.\n");
        return;
    }

    error_code = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log_buffer, NULL);
    check_cl_error(error_code, "clGetProgramBuildInfo(log)");

    log_buffer[log_size] = '\0';
    fprintf(stderr, "\n=== OpenCL Build Log ===\n%s\n", log_buffer);

    free(log_buffer);
}