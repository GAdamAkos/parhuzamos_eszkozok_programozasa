#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <CL/cl.h>

#define ARRAY_SIZE 1024
#define LARGE_FILE_SIZE (1024 * 1024 * 10) // 10 MB a teszthez

// 1. Feladat: Callback fuggveny
void CL_CALLBACK on_read_complete(cl_event e, cl_int status, void* data) {
    printf("\n>>> [Callback]: Adatvisszaolvasas kesz, a statusz: %d\n", status);
}

int main() {
    cl_int err;
    srand(time(NULL));

    // 1. Setup
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    cl_command_queue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);

    // Kernel betoltes es FORDITAS ELLENORZESSEL
    FILE* fp = fopen("kernels.cl", "rb");
    if(!fp) { printf("Hiba: kernels.cl nem talalhato!\n"); return 1; }
    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp);
    rewind(fp);
    char* src = malloc(sz + 1);
    fread(src, 1, sz, fp);
    src[sz] = '\0';
    fclose(fp);

    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&src, NULL, &err);
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    
    // FONTOS: Ha hiba van a kernelben, itt kiirjuk!
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("!!! KERNEL FORDITASI HIBA !!!\n%s\n", log);
        return 1;
    }

    // --- 2. FELADAT: GYAKORISAG ---
    int h_data[ARRAY_SIZE];
    int h_counts[101] = {0};
    for(int i=0; i<ARRAY_SIZE; i++) h_data[i] = rand() % 101;

    cl_mem d_data = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(h_data), h_data, NULL);
    cl_mem d_counts = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(h_counts), h_counts, NULL);

    cl_kernel k_freq = clCreateKernel(program, "count_frequencies", NULL);
    clSetKernelArg(k_freq, 0, sizeof(cl_mem), &d_data);
    clSetKernelArg(k_freq, 1, sizeof(cl_mem), &d_counts);

    size_t global = ARRAY_SIZE;
    clEnqueueNDRangeKernel(queue, k_freq, 1, NULL, &global, NULL, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_counts, CL_TRUE, 0, sizeof(h_counts), h_counts, 0, NULL, NULL);

    printf("2. Feladat: Gyakorisag (par pelda): [0]:%d, [50]:%d, [100]:%d\n", h_counts[0], h_counts[50], h_counts[100]);

    // --- 3. FELADAT: SZORAS ---
    float h_floats[ARRAY_SIZE], h_sq_sums[ARRAY_SIZE];
    double sum = 0;
    for(int i=0; i<ARRAY_SIZE; i++) {
        h_floats[i] = (float)rand() / RAND_MAX;
        sum += h_floats[i];
    }
    double mean = sum / ARRAY_SIZE;

    cl_mem d_floats = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(h_floats), h_floats, NULL);
    cl_mem d_sq_sums = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(h_sq_sums), NULL, NULL);

    cl_kernel k_std = clCreateKernel(program, "std_dev_helper", NULL);
    clSetKernelArg(k_std, 0, sizeof(cl_mem), &d_floats);
    clSetKernelArg(k_std, 1, sizeof(cl_mem), &d_sq_sums);

    clEnqueueNDRangeKernel(queue, k_std, 1, NULL, &global, NULL, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_sq_sums, CL_TRUE, 0, sizeof(h_sq_sums), h_sq_sums, 0, NULL, NULL);

    double sq_sum = 0;
    for(int i=0; i<ARRAY_SIZE; i++) sq_sum += h_sq_sums[i];
    double variance = (sq_sum / ARRAY_SIZE) - (mean * mean);
    printf("3. Feladat: Szoras = %f\n", sqrt(variance));

    // --- 4. FELADAT: ADATFAJL ES CALLBACK ---
    unsigned char* h_file = malloc(LARGE_FILE_SIZE);
    for(size_t i=0; i<LARGE_FILE_SIZE; i++) h_file[i] = (rand() % 10 == 0) ? 0 : 1;

    int h_zeros = 0;
    cl_mem d_file = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, LARGE_FILE_SIZE, h_file, NULL);
    cl_mem d_zeros = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), &h_zeros, NULL);

    cl_kernel k_zeros = clCreateKernel(program, "count_zeros", NULL);
    clSetKernelArg(k_zeros, 0, sizeof(cl_mem), &d_file);
    clSetKernelArg(k_zeros, 1, sizeof(cl_mem), &d_zeros);

    size_t file_global = LARGE_FILE_SIZE;
    cl_event kernel_event, read_event;
    clEnqueueNDRangeKernel(queue, k_zeros, 1, NULL, &file_global, NULL, 0, NULL, &kernel_event);
    
    // Callback beallitasa a visszaolvasashoz
    clEnqueueReadBuffer(queue, d_zeros, CL_FALSE, 0, sizeof(int), &h_zeros, 1, &kernel_event, &read_event);
    clSetEventCallback(read_event, CL_COMPLETE, on_read_complete, NULL);

    clWaitForEvents(1, &read_event);
    
    cl_ulong start, end;
    clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);

    printf("4. Feladat: Zero byte-ok: %d\n", h_zeros);
    printf("Feldolgozasi ido: %f ms\n", (double)(end - start) / 1000000.0);

    return 0;
}