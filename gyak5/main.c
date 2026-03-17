#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <math.h>

#define N 1024

const char* kernel_source = 
"__kernel void bitonic_sort_step(__global int* data, int j, int k) {    \n"
"    unsigned int i = get_global_id(0);                                \n"
"    unsigned int ixj = i ^ j;                                         \n"
"    if (ixj > i) {                                                    \n"
"        if ((i & k) == 0) {                                           \n"
"            if (data[i] > data[ixj]) {                                \n"
"                int tmp = data[i]; data[i] = data[ixj]; data[ixj] = tmp;\n"
"            }                                                         \n"
"        } else {                                                      \n"
"            if (data[i] < data[ixj]) {                                \n"
"                int tmp = data[i]; data[i] = data[ixj]; data[ixj] = tmp;\n"
"            }                                                         \n"
"        }                                                             \n"
"    }                                                                 \n"
"}                                                                     \n"
"__kernel void shell_sort_step(__global int* data, int n, int gap) {   \n"
"    int i = get_global_id(0);                                         \n"
"    if (i + gap < n) {                                                \n"
"        if (data[i] > data[i + gap]) {                                \n"
"            int tmp = data[i]; data[i] = data[i + gap]; data[i + gap] = tmp;\n"
"        }                                                             \n"
"    }                                                                 \n"
"}                                                                     \n"
"__kernel void counting_count(__global const int* data, __global int* counts, int n) {\n"
"    int i = get_global_id(0);                                         \n"
"    if (i < n) atomic_inc(&counts[data[i]]);                          \n"
"}                                                                     \n";

// Segédfüggvények
long calculate_checksum(int* arr, int n) {
    long sum = 0;
    for(int i = 0; i < n; i++) sum += arr[i];
    return sum;
}

int is_sorted(int* arr, int n) {
    for (int i = 0; i < n - 1; i++) if (arr[i] > arr[i + 1]) return 0;
    return 1;
}

// Profilozás segéd (ms-ra váltás)
double get_time_ms(cl_event event) {
    cl_ulong start, end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    return (double)(end - start) / 1000000.0;
}

int main() {
    int h_data[N];
    int n_val = N; // A FIX: változóba tesszük az N-t a & használatához
    srand(42);
    for (int i = 0; i < N; i++) h_data[i] = rand() % 1000;
    long orig_sum = calculate_checksum(h_data, N);

    cl_int err;
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    // Profilozás engedélyezése
    cl_command_queue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);

    cl_program program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    cl_mem d_data = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int)*N, h_data, &err);

    printf("--- MERESI EREDMENYEK (N=%d) ---\n", N);

    // --- 5. BITONIC SORT ---
    cl_kernel k_bitonic = clCreateKernel(program, "bitonic_sort_step", &err);
    double bitonic_time = 0;
    for (int k = 2; k <= N; k <<= 1) {
        for (int j = k >> 1; j > 0; j >>= 1) {
            cl_event ev;
            clSetKernelArg(k_bitonic, 0, sizeof(cl_mem), &d_data);
            clSetKernelArg(k_bitonic, 1, sizeof(int), &j);
            clSetKernelArg(k_bitonic, 2, sizeof(int), &k);
            size_t global = N;
            clEnqueueNDRangeKernel(queue, k_bitonic, 1, NULL, &global, NULL, 0, NULL, &ev);
            clWaitForEvents(1, &ev);
            bitonic_time += get_time_ms(ev);
            clReleaseEvent(ev);
        }
    }
    clEnqueueReadBuffer(queue, d_data, CL_TRUE, 0, sizeof(int)*N, h_data, 0, NULL, NULL);
    printf("Bitonic Sort:   %7.4f ms  [Rendezett: %s]\n", bitonic_time, is_sorted(h_data, N) ? "IGEN" : "NEM");

    // --- 6. SHELL SORT ---
    // Adatok újraosztása a méréshez
    srand(42); for (int i = 0; i < N; i++) h_data[i] = rand() % 1000;
    clEnqueueWriteBuffer(queue, d_data, CL_TRUE, 0, sizeof(int)*N, h_data, 0, NULL, NULL);
    
    cl_kernel k_shell = clCreateKernel(program, "shell_sort_step", &err);
    double shell_time = 0;
    for (int gap = N/2; gap > 0; gap /= 2) {
        cl_event ev;
        clSetKernelArg(k_shell, 0, sizeof(cl_mem), &d_data);
        clSetKernelArg(k_shell, 1, sizeof(int), &n_val); // Itt a javítás: &n_val
        clSetKernelArg(k_shell, 2, sizeof(int), &gap);
        size_t global = N;
        clEnqueueNDRangeKernel(queue, k_shell, 1, NULL, &global, NULL, 0, NULL, &ev);
        clWaitForEvents(1, &ev);
        shell_time += get_time_ms(ev);
        clReleaseEvent(ev);
    }
    clEnqueueReadBuffer(queue, d_data, CL_TRUE, 0, sizeof(int)*N, h_data, 0, NULL, NULL);
    printf("Shell Sort:     %7.4f ms  [Rendezett: %s]\n", shell_time, is_sorted(h_data, N) ? "IGEN" : "NEM");

    // --- 7. COUNTING SORT ---
    int h_counts[1000] = {0};
    cl_mem d_counts = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(h_counts), h_counts, &err);
    cl_kernel k_count = clCreateKernel(program, "counting_count", &err);
    cl_event ev_c;
    clSetKernelArg(k_count, 0, sizeof(cl_mem), &d_data);
    clSetKernelArg(k_count, 1, sizeof(cl_mem), &d_counts);
    clSetKernelArg(k_count, 2, sizeof(int), &n_val); // Itt is a javítás: &n_val
    size_t global_c = N;
    clEnqueueNDRangeKernel(queue, k_count, 1, NULL, &global_c, NULL, 0, NULL, &ev_c);
    clWaitForEvents(1, &ev_c);
    printf("Counting Sort:  %7.4f ms  (csak GPU resz)\n", get_time_ms(ev_c));

    printf("\n2. Feladat: Checksum egyezik: %s\n", (calculate_checksum(h_data, N) == orig_sum) ? "IGEN" : "NEM");

    // Takarítás (kihagyva a rövidség kedvéért, de élesben Release mindenre!)
    return 0;
}