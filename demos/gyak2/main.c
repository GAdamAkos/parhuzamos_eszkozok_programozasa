#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

// 1. Feladat: Hibakódok szöveges visszaadása
const char* get_error_string(cl_int error) {
    switch(error) {
        case 0: return "CL_SUCCESS";
        case -11: return "CL_BUILD_PROGRAM_FAILURE";
        case -30: return "CL_INVALID_VALUE";
        case -34: return "CL_INVALID_CONTEXT";
        case -36: return "CL_INVALID_COMMAND_QUEUE";
        default: return "Ismeretlen hiba";
    }
}

// Segédfüggvény a .cl fájl beolvasásához
char* load_source(const char* filename) {
    FILE* fp = fopen(filename, "rb"); // "rb" - read binary, így pontosabb
    if (!fp) { printf("Nem sikerult megnyitni: %s\n", filename); exit(1); }
    
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);
    
    char* src = (char*)malloc(size + 1);
    size_t read_size = fread(src, 1, size, fp);
    src[read_size] = '\0'; // Lezárjuk a stringet a ténylegesen beolvasott méretnél
    
    fclose(fp);
    return src;
}

int main() {
    cl_int err;
    const int W = 4; // Mátrix mérete (W x W)
    size_t size = W * W * sizeof(float);

    // Adatok előkészítése
    float *h_A = (float*)malloc(size);
    float *h_B = (float*)malloc(size);
    float *h_Res = (float*)malloc(size);
    for(int i=0; i<W*W; i++) { h_A[i] = i; h_B[i] = 1.0f; }

    // 1. Platform & Device
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);

    // 2. Context & Queue (Profilozás bekapcsolva!)
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    cl_command_queue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);

    // 3. Kernel betöltése és fordítása (2. Feladat)
    char* src = load_source("kernels.cl");
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&src, NULL, &err);
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Kernel hiba:\n%s\n", log);
        return 1;
    }

    // 4. Bufferek létrehozása
    cl_mem d_A = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size, h_A, NULL);
    cl_mem d_B = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size, h_B, NULL);
    cl_mem d_C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, size, NULL, NULL);

    // 5. Mátrix szorzás futtatása profilozással (3-4. Feladat)
    cl_kernel k_mul = clCreateKernel(program, "matrix_mul", &err);
    clSetKernelArg(k_mul, 0, sizeof(cl_mem), &d_A);
    clSetKernelArg(k_mul, 1, sizeof(cl_mem), &d_B);
    clSetKernelArg(k_mul, 2, sizeof(cl_mem), &d_C);
    clSetKernelArg(k_mul, 3, sizeof(int), &W);

    size_t global[2] = {W, W};
    cl_event event;
    clEnqueueNDRangeKernel(queue, k_mul, 2, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);

    // Időmérés lekérdezése
    cl_ulong start, end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    printf("Matrix szorzas ideje: %f ms\n", (double)(end - start) / 1000000.0);

    // Eredmény visszaolvasása
    clEnqueueReadBuffer(queue, d_C, CL_TRUE, 0, size, h_Res, 0, NULL, NULL);

    printf("Eredmeny elso eleme: %.1f\n", h_Res[0]);

    // Takarítás
    free(src); free(h_A); free(h_B); free(h_Res);
    clReleaseMemObject(d_A); clReleaseMemObject(d_B); clReleaseMemObject(d_C);
    clReleaseKernel(k_mul);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}