// 2. Feladat: Gyakorisag szamitasa
__kernel void count_frequencies(__global const int* data, __global int* counts) {
    int i = get_global_id(0);
    // atomic_inc: biztonsagos parhuzamos noveles
    atomic_inc(&counts[data[i]]);
}

// 3. Feladat: Szorashoz szukseges negyzetosszeg
__kernel void std_dev_helper(__global const float* data, __global float* sq_sums) {
    int i = get_global_id(0);
    sq_sums[i] = data[i] * data[i];
}

// 4. Feladat: Nulla byte-ok szamlalasa
__kernel void count_zeros(__global const uchar* data, __global int* zero_count) {
    int i = get_global_id(0);
    if (data[i] == 0) {
        atomic_inc(zero_count);
    }
}