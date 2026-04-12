__kernel void matrix_mul(__global const float* A, 
                         __global const float* B, 
                         __global float* C, 
                         int width) {
    int row = get_global_id(1);
    int col = get_global_id(0);
    
    float sum = 0.0f;
    for (int k = 0; k < width; k++) {
        sum += A[row * width + k] * B[k * width + col];
    }
    C[row * width + col] = sum;
}

__kernel void matrix_transpose(__global const float* input, 
                               __global float* output, 
                               int rows, 
                               int cols) {
    int r = get_global_id(1);
    int c = get_global_id(0);
    
    if (r < rows && c < cols) {
        output[c * rows + r] = input[r * cols + c];
    }
}