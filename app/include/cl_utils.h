#ifndef CL_UTILS_H
#define CL_UTILS_H

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

const char* cl_error_to_string(cl_int error_code);
void check_cl_error(cl_int error_code, const char* operation);
void print_platform_info(cl_platform_id platform);
void print_device_info(cl_device_id device);
cl_device_id select_best_gpu_device(void);

#endif
