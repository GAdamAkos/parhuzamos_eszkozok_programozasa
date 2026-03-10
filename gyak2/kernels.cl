// Mátrix szorzás: C = A * B
__kernel void matrix_mul(__global const float* A, 
                         __global const float* B, 
                         __global float* C, 
                         int width) {
    int row = get_global_id(1); // Y dimenzió
    int col = get_global_id(0); // X dimenzió
    
    float sum = 0.0f;
    for (int k = 0; k < width; k++) {
        sum += A[row * width + k] * B[k * width + col];
    }
    C[row * width + col] = sum;
}

// Mátrix transzponálás
__kernel void matrix_transpose(__global const float* input, 
                               __global float* output, 
                               int rows, 
                               int cols) {
    int r = get_global_id(1); // Eredeti sor
    int c = get_global_id(0); // Eredeti oszlop
    
    if (r < rows && c < cols) {
        // Az (r,c) elem a (c,r) helyre kerül az outputban
        output[c * rows + r] = input[r * cols + c];
    }
}