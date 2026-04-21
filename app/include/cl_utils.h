#ifndef CL_UTILS_H
#define CL_UTILS_H

/* A projekt OpenCL 1.2 API-t használ. */
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

const char* cl_error_to_string(cl_int error_code);
void check_cl_error(cl_int error_code, const char* operation);

void print_platform_info(cl_platform_id platform);
void print_device_info(cl_device_id device);

cl_device_id select_best_gpu_device(void);
cl_device_id get_gpu_device_by_global_index(int global_device_index);
int get_gpu_device_count(void);
void list_gpu_devices(void);

void get_device_name(cl_device_id device, char* buffer, size_t buffer_size);

void print_program_build_log(cl_program program, cl_device_id device);

#endif