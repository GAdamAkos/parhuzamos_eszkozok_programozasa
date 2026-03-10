#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

// --- SEGÉDFÜGGVÉNYEK ---

void check_error(cl_int err, const char* msg) {
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Hiba (%d): %s\n", err, msg);
        exit(1);
    }
}

// 4. feladat: Szekvenciális ellenőrzés
void verify_vector_add(float* a, float* b, float* res, int n) {
    int ok = 1;
    for(int i = 0; i < n; i++) {
        if (a[i] + b[i] != res[i]) ok = 0;
    }
    printf("Szekvencialis ellenorzes (4. feladat): %s\n", ok ? "SIKER" : "HIBA");
}

int main() {
    cl_int err;
    int n = 8; // Teszt méret

    // 1. Környezet inicializálása
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);

    // Kernel forrás betöltése (itt most egyszerűsítve stringként)
    const char* source = 
        "__kernel void vector_add(__global const float* A, __global const float* B, __global float* C) { "
        "   int i = get_global_id(0); C[i] = A[i] + B[i]; }"
        "__kernel void fill_missing(__global float* data) { "
        "   int i = get_global_id(0); if (i > 0 && data[i] < 0.0f) data[i] = (data[i-1] + data[i+1]) / 2.0f; }"
        "__kernel void calculate_rank(__global const int* data, __global int* ranks, int n) { "
        "   int i = get_global_id(0); int c = 0; for(int j=0; j<n; j++) if(data[j]<data[i]) c++; ranks[i] = c; }"
        "__kernel void count_freq(__global const int* data, __global int* counts, __global int* is_u, int n) { "
        "   int i = get_global_id(0); int f = 0; for(int j=0; j<n; j++) if(data[j]==data[i]) f++; counts[i] = f; if(f>1) *is_u = 0; }";

    cl_program program = clCreateProgramWithSource(context, 1, &source, NULL, &err);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    // --- 4. FELADAT: VEKTOR ÖSSZEADÁS ---
    float h_A[] = {1,2,3,4,5,6,7,8}, h_B[] = {1,1,1,1,1,1,1,1}, h_C[8];
    cl_mem d_A = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*n, h_A, NULL);
    cl_mem d_B = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*n, h_B, NULL);
    cl_mem d_C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float)*n, NULL, NULL);

    cl_kernel k_add = clCreateKernel(program, "vector_add", NULL);
    clSetKernelArg(k_add, 0, sizeof(cl_mem), &d_A);
    clSetKernelArg(k_add, 1, sizeof(cl_mem), &d_B);
    clSetKernelArg(k_add, 2, sizeof(cl_mem), &d_C);

    size_t global = n;
    clEnqueueNDRangeKernel(queue, k_add, 1, NULL, &global, NULL, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_C, CL_TRUE, 0, sizeof(float)*n, h_C, 0, NULL, NULL);

    verify_vector_add(h_A, h_B, h_C, n);

    // --- 5. FELADAT: HIÁNYZÓ PÓTLÁSA ---
    float h_data[] = {10, -1, 20, -1, 30, 40, -1, 50}; // -1 jelöli a hiányzót
    cl_mem d_data = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float)*n, h_data, NULL);
    cl_kernel k_fill = clCreateKernel(program, "fill_missing", NULL);
    clSetKernelArg(k_fill, 0, sizeof(cl_mem), &d_data);
    clEnqueueNDRangeKernel(queue, k_fill, 1, NULL, &global, NULL, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_data, CL_TRUE, 0, sizeof(float)*n, h_data, 0, NULL, NULL);
    
    printf("5. feladat (Pótolt): ");
    for(int i=0; i<n; i++) printf("%.1f ", h_data[i]); printf("\n");

    // --- 6. FELADAT: RANG ---
    int h_ints[] = {15, 2, 8, 2, 20, 15, 4, 1}, h_ranks[8];
    cl_mem d_ints = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int)*n, h_ints, NULL);
    cl_mem d_ranks = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int)*n, NULL, NULL);
    cl_kernel k_rank = clCreateKernel(program, "calculate_rank", NULL);
    clSetKernelArg(k_rank, 0, sizeof(cl_mem), &d_ints);
    clSetKernelArg(k_rank, 1, sizeof(cl_mem), &d_ranks);
    clSetKernelArg(k_rank, 2, sizeof(int), &n);
    clEnqueueNDRangeKernel(queue, k_rank, 1, NULL, &global, NULL, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_ranks, CL_TRUE, 0, sizeof(int)*n, h_ranks, 0, NULL, NULL);

    printf("6. feladat (Rangok): ");
    for(int i=0; i<n; i++) printf("%d ", h_ranks[i]); printf("\n");

    // --- 7. FELADAT: GYAKORISÁG ÉS EGYEDISÉG ---
    int h_counts[8], h_unique = 1; // Alapból igaz (1)
    cl_mem d_counts = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int)*n, NULL, NULL);
    cl_mem d_uflag = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), &h_unique, NULL);
    cl_kernel k_freq = clCreateKernel(program, "count_freq", NULL);
    clSetKernelArg(k_freq, 0, sizeof(cl_mem), &d_ints);
    clSetKernelArg(k_freq, 1, sizeof(cl_mem), &d_counts);
    clSetKernelArg(k_freq, 2, sizeof(cl_mem), &d_uflag);
    clSetKernelArg(k_freq, 3, sizeof(int), &n);
    clEnqueueNDRangeKernel(queue, k_freq, 1, NULL, &global, NULL, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_counts, CL_TRUE, 0, sizeof(int)*n, h_counts, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_uflag, CL_TRUE, 0, sizeof(int), &h_unique, 0, NULL, NULL);

    printf("7. feladat (Gyakoriság): ");
    for(int i=0; i<n; i++) printf("%d ", h_counts[i]);
    printf("\nEgyedi minden elem? %s\n", h_unique ? "IGEN" : "NEM");

    // Takarítás (kihagyva a rövidség kedvéért, de élesben Release mindenre!)
    return 0;
}